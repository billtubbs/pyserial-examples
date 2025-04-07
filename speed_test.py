import time
import numpy as np
import serial
from serial_comm import (
    wait_for_arduino, 
    send_data_to_arduino, 
    receive_data_from_arduino, 
    display_debug_info, 
    display_data
)


ser = serial.Serial("/dev/tty.usbmodem112977801", 57600)

print("Waiting for Arduino...")
wait_for_arduino(ser)

print("Arduino is ready")

test_data = [
    list(b"abcde"),
    np.tile([32, 33, 34], 6).tolist()
]

num_loops = len(test_data)
n = 0
waiting_for_reply = False

while n < num_loops:

    if ser.in_waiting == 0 and waiting_for_reply is False:
        data = np.array(test_data[n], dtype=np.uint8)

        t0 = time.time()
        send_data_to_arduino(ser, data)
        waiting_for_reply = True

    if ser.in_waiting > 0:
        data_received = receive_data_from_arduino(ser)
        t1 = time.time()

        if data_received[0] == 0:
            display_debug_info(data_received[1])

        if data_received[0] > 0:
            print(f"Reply {n} received after {(t1 - t0) * 1000:.1f} ms")
            # TODO: Second test fails
    #         (Pdb) data
    #         array([32, 33, 34, 32, 33, 34, 32, 33, 34, 32, 33, 34, 32, 33, 34, 32, 33,
    #    34], dtype=uint8)
    #         (Pdb) data_received[1][2:-1]
    #         array([32, 33, 34, 32, 33, 34, 32, 33, 34, 32, 33, 34, 32, 33, 34, 32, 32,
    #    33], dtype=uint8)
            breakpoint()
            assert np.array_equal(data, data_received[1][2:-1])
            n += 1
            waiting_for_reply = False

ser.close()

print("Test complete")