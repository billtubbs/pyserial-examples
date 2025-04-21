import time
import numpy as np
import serial
from serial_comm import (
    MAX_PACKAGE_LEN,
    send_data_to_arduino, 
    receive_data_from_arduino, 
    display_debug_info, 
)

ser = serial.Serial("/dev/tty.usbmodem112977801", 57600)
print("Connected to Arduino.")

# Provide list of test times and test data
test_data = [
    (5, list(b"abcde")),
    (10, np.tile([0, 1, 2], 6).tolist()),
    (15, [250, 251, 252, 253, 254, 255]),
    (20, np.arange(0, 256, dtype="uint8")),
    (25, np.random.randint(256, size=5000, dtype='uint8')),
    (30, np.ones((0x1ff - 2, ), dtype="uint8")),  # length bytes include 255
    (35, np.ones((8189, ), dtype="uint8")),  # maximum size is 8189
    (40, np.full((8189, ), 255, dtype="uint8")),  # maximum size is 8189
    (45, np.ones((8190, ), dtype="uint8")),  # maximum size is 8189
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
        num_bytes, data_recieved = receive_data_from_arduino(ser)
        t1 = time.time()

        if num_bytes == 0:
            display_debug_info(data_recieved)
            if data_recieved.tobytes().startswith(b'Num. of data bytes exceeds buffer size'):
                msg = "Test failed"
                print(f"{int(loop_time - t_start) % 1000:03d}: {msg:s}")
                t_status_update = loop_time
                n += 1
                waiting_for_reply = False

        else:
            print(f"{int(loop_time - t_start) % 1000:03d}: Reply {n+1} received after {(t1 - t0) * 1000:.1f} ms")
            assert num_bytes == data.shape[0] + 2
            assert np.array_equal(data, data_recieved)
            t_status_update = loop_time
            n += 1
            waiting_for_reply = False

    if loop_time - t_status_update > 1:
        if waiting_for_reply:
            msg = "Waiting for reply..."
        else:
            msg = "Waiting to send..."
        print(f"{int(loop_time - t_start) % 1000:03d}: {msg:s}")
        t_status_update = loop_time

ser.close()

print("Testing complete")