"""Python module to facilitate data communication with a connected device
via serial USB connection.

Bill Tubbs
6 Apr 2025

"""

import serial
import time
import numpy as np
import numba as nb
from itertools import chain


START_MARKER = 254
END_MARKER = 255
SPECIAL_BYTE = 253
MAX_MSG_LEN = 8192


def send_data_to_arduino(ser, data):
    global START_MARKER, END_MARKER
    length_encoded = encode_data(data.shape[0].to_bytes(length=2, byteorder='big'))
    data_encoded = encode_data(data)
    ser.write(chain.from_iterable([
        [START_MARKER],
        length_encoded,
        data_encoded,
        [END_MARKER]
    ]))


def receive_data_from_arduino(ser):
    global START_MARKER, END_MARKER
    # read data until the start character is found
    bytes_seq = ser.read_until(bytes([START_MARKER]), size=MAX_MSG_LEN)
    assert len(bytes_seq) < MAX_MSG_LEN, "No start marker found"
    # read data until the end marker is found
    bytes_seq = bytes([START_MARKER]) + ser.read_until(bytes([END_MARKER]), size=MAX_MSG_LEN)
    assert len(bytes_seq) < MAX_MSG_LEN, f"No end marker found after {MAX_MSG_LEN} bytes"
    # decode and convert to numpy array
    bytes_seq = decode_bytes(bytes_seq)
    n_bytes = int.from_bytes(bytes_seq[1:3], byteorder='big')
    return (n_bytes, bytes_seq)


@nb.njit()
def encode_data(data : nb.uint8[:]) -> nb.uint8[:]:
    # TODO: Could this be converted to return bytes?
    global SPECIAL_BYTE
    data_out : nb.uint8[:] = []
    for x in data:
        if x >= SPECIAL_BYTE:
            data_out.append(SPECIAL_BYTE)
            data_out.append(x - SPECIAL_BYTE)
        else:
            data_out.append(x)
    return np.array(data_out, dtype='uint8')


@nb.njit()
def decode_bytes(bytes_seq : nb.uint8[:]) -> nb.uint8[:]:
    # TODO: Could this be converted to accept bytes?
    global SPECIAL_BYTE
    data_out : nb.uint8[:] = []
    n = 0
    while n < len(bytes_seq):
        x = bytes_seq[n]
        if x == SPECIAL_BYTE:
            n += 1
            x = SPECIAL_BYTE + bytes_seq[n]
        data_out.append(x)
        n += 1
    return np.array(data_out, dtype='uint8')


def display_data(data):
    print(f"NUM BYTES SENT: {data[1]:d}")
    print(f"    DATA RECVD: {data[2:-1].tolist()!r}")


def display_debug_info(debugStr):
    print(f"DEBUG MSG-> {bytes(debugStr[3:-1])!r}")


def wait_for_arduino(ser):
    """Wait until the Arduino sends 'Arduino name: ...' - allows time for Arduino
    reset. It also ensures that any bytes left over from a previous message are
    discarded.
    """
    global END_MARKER
    while True:
        while ser.in_waiting == 0:
            pass
        # wait until an end marker is received from the Arduino to make sure it is ready to proceed
        msg = []
        while True:  # gets the initial debugMessage
            x = ser.read()
            msg.append(x)
            if ord(x) == END_MARKER:
                break
        msg = b''.join(msg)
        display_debug_info(msg)
        print()
        if msg.find(b"Arduino name:") != -1:
            arduino_name = msg[msg.index[':']+1:].strip()
            break
    return arduino_name


# Call these functions to make sure they are jit compiled
data = [0, 64, 65, 253, 254, 255]
encoded_data = encode_data(data)
decoded_data = decode_bytes(encoded_data)
assert np.array_equal(decoded_data, data)


#======================================
# THE DEMO PROGRAM STARTS HERE
#======================================

def main():

    for _ in range(10):
        try:
            # NOTE the user must ensure that the next line refers to the correct comm port
            ser = serial.Serial("/dev/tty.usbmodem112977801", 57600)
            break
        except serial.serialutil.SerialException:
            print("Failed to open serial connection. Waiting to try again...")
            time.sleep(1)
    else:
        raise Exception("Gave up trying to open serial connection.")

    print("Waiting for Arduino...")
    arduino_name = wait_for_arduino(ser)

    print("Arduino is ready")

    test_data = [
        list(b"abcde"),
        list(b"zxcv1234"),
        [ord(b"a"), 16, 32, 0, 203],
        [ord(b"b"), 16, 32, 253, 255, 254, 253, 0],
        list(b"fghijk")
    ]

    num_loops = len(test_data)
    n = 0
    waiting_for_reply = False

    while n < num_loops:
        print(f"LOOP {n:d}")

        if ser.in_waiting == 0 and waiting_for_reply is False:
            data = np.array(test_data[n], dtype=np.uint8)
            send_data_to_arduino(ser, data)
            print("=====sent from PC==========")
            print(f"LOOP NUM {n:d}")
            print(f"DATA SENT: {data.tolist()!r}")
            print("===========================")
            waiting_for_reply = True

        if ser.in_waiting > 0:
            data_received = receive_data_from_arduino(ser)

            if data_received[0] == 0:
                display_debug_info(data_received[1])

            if data_received[0] > 0:
                display_data(data_received[1])
                assert np.array_equal(data, data_received[1][2:-1])
                print("Reply Received")
                n += 1
                waiting_for_reply = False

            print("\n===========\n")

            time.sleep(0.3)

    ser.close()


if __name__ == "__main__":
    main()
