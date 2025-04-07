"""Python module to facilitate data communication with a connected device
via serial USB connection.

Bill Tubbs
6 Apr 2025

"""

import serial
import time
import numpy as np
from numba import jit, njit, byte, int64, uint8


START_MARKER = 254
END_MARKER = 255
SPECIAL_BYTE = 253


def send_data_to_arduino(ser, data):
    global START_MARKER, END_MARKER
    length = data.shape[0].to_bytes(length=1, byteorder='big')
    data_encoded = encode_data(data)
    bytes_seq = b''.join([
        bytes([START_MARKER]), 
        length, 
        data_encoded.tobytes(), 
        bytes([END_MARKER])
    ])
    ser.write(bytes_seq)


def receive_data_from_arduino(ser):
    global START_MARKER, END_MARKER
    # read data until the start character is found
    ser.read_until(bytes([START_MARKER]), size=255)
    # read data until the end marker is found
    bytes_seq = bytes([START_MARKER]) + ser.read_until(bytes([END_MARKER]), size=255)
    # decode and convert to numpy array
    returnData = [bytes_seq[1], decode_bytes(bytes_seq)]
    return returnData


@njit()
def encode_data(data : uint8[:]):
    global SPECIAL_BYTE
    data_out : byte[:] = []
    for x in data:
        if x >= SPECIAL_BYTE:
            data_out.append(SPECIAL_BYTE)
            data_out.append(x - SPECIAL_BYTE)
        else:
            data_out.append(x)
    return np.array(data_out, dtype='uint8')


@njit()
def decode_bytes(bytes_seq : byte[:]):
    global SPECIAL_BYTE
    data_out : uint8[:] = []
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
    print(f"DEBUG MSG-> {debugStr[2:-1]!r}")


def wait_for_arduino(ser):
    """Wait until the Arduino sends 'Arduino Ready' - allows time for Arduino
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
        if msg.find(b"Arduino Ready") != -1:
            break


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
    wait_for_arduino(ser)

    print("Arduino is ready")

    test_data = [
        list(b"abcde"),
        list(b"zxcv1234"),
        [ord(b"a"), 16, 32, 0, 203],
        [ord(b"b"), 16, 32, 253, 255, 254, 253, 0],
        list(b"fghijk")
    ]

    numLoops = len(test_data)
    n = 0
    waitingForReply = False

    while n < numLoops:
        print(f"LOOP {n:d}")

        if ser.in_waiting == 0 and waitingForReply is False:
            data = np.array(test_data[n], dtype=np.uint8)
            send_data_to_arduino(ser, data)
            print("=====sent from PC==========")
            print(f"LOOP NUM {n:d}")
            print(f"DATA SENT: {data.tolist()!r}")
            print("===========================")
            waitingForReply = True

        if ser.in_waiting > 0:
            dataRecvd = receive_data_from_arduino(ser)

            if dataRecvd[0] == 0:
                display_debug_info(dataRecvd[1])

            if dataRecvd[0] > 0:
                display_data(dataRecvd[1])
                assert np.array_equal(data, dataRecvd[1][2:-1])
                print("Reply Received")
                n += 1
                waitingForReply = False

            print("\n===========\n")

            time.sleep(0.3)

    ser.close


if __name__ == "__main__":
    main()
