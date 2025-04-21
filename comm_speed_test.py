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
print("Connected to Arduino...")

test_data = [
    (5, list(b"abcde")),
    (10, np.tile([32, 33, 34], 6).tolist()),
    (15, [250, 251, 252, 253, 254, 255]),
    (20, np.arange(0, 256, dtype="uint8")),
    (25, np.random.randint(256, size=5000, dtype='uint8')),
    (30, np.ones((8186, ), dtype="uint8")),  # maximum size is 8186
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
        num_bytes, data_received = receive_data_from_arduino(ser)
        t1 = time.time()

        if num_bytes == 0:
            display_debug_info(data_received)
            if data_received.tobytes().startswith(b'getSerialData failed: data length exceeds'):
                msg = "Test failed"
                print(f"{int(loop_time - t_start) % 1000:03d}: {msg:s}")
                n += 1
                waiting_for_reply = False

        else:
            print(f"{int(loop_time - t_start) % 1000:03d}: Reply {n+1} received after {(t1 - t0) * 1000:.1f} ms")
            t_status_update = loop_time
            #breakpoint()
            assert num_bytes == data.shape[0] + 2
            assert np.array_equal(data, data_received)
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