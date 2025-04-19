import time
import numpy as np
import serial
from serial_comm import (
    send_data_to_arduino, 
    receive_data_from_arduino, 
    display_debug_info, 
)

ser = serial.Serial("/dev/tty.usbmodem112977801", 57600)
print("Connected to Arduino...")

test_data = [
    (5, list(b"abcde")),
    (10, np.tile([32, 33, 34], 6).tolist()),
    (15, np.arange(0, 252, dtype="uint8")),  # TODO: Fails when this is 253 bytes or longer
    (20, np.tile(np.arange(0, 10), 8)),
]

num_loops = len(test_data)
n = 0
waiting_for_reply = False
t_start = time.time()
t_status_update = t_start
while n < num_loops:
    loop_time = time.time()

    if ser.in_waiting == 0 and waiting_for_reply is False:
        send_time, data = test_data[n]
        if loop_time - t_start > send_time:
            data = np.array(data, dtype=np.uint8)
            t0 = time.time()
            send_data_to_arduino(ser, data)
            waiting_for_reply = True
            print(f"{int(loop_time - t_start) % 1000:03d}: Test data {n+1} sent. ")
            t_status_update = loop_time

    if ser.in_waiting > 0:
        data_received = receive_data_from_arduino(ser)
        t1 = time.time()

        if data_received[0] == 0:
            display_debug_info(data_received[1])
        else:
            print(f"{int(loop_time - t_start) % 1000:03d}: Reply {n+1} received after {(t1 - t0) * 1000:.1f} ms")
            t_status_update = loop_time
            assert data_received[0] == data.shape[0]
            assert np.array_equal(data, data_received[1][3:-1])
            n += 1
            waiting_for_reply = False

    if loop_time - t_status_update > 1:
        if waiting_for_reply:
            msg = "Waiting for reply..."
        else:
            msg = "Listening..."
        print(f"{int(loop_time - t_start) % 1000:03d}: {msg:s}")
        t_status_update = loop_time

ser.close()

print("Test complete")