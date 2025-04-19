# pyserial-examples

Example scripts to demonstrate serial communication between a host computer running Python and an Arduino-based microcontroller using the [Pyserial](https://pyserial.readthedocs.io/en/latest/) library.

## Communication Speed Tests

- [serial_comm.py](serial_comm.py) - module containing Python functions to communicate with an Arduino device over serial
- [comm_speed_test/comm_speed_test.ino](comm_speed_test/comm_speed_test.ino) - Arduino script to communicate with the host device
- [comm_speed_test](comm_speed_test) - Python test script to test round-trip data communication speed and accuracy

For example, with a Teensy 4.0 microcontroller (ARM Cortex-M7 at 600 MHz) and a Mac Mini running Python 3.10.16 with a serial Baud rate of 57600, I recordedthe following timings to transmit each data packet to the microcontroller and back.

1. 8 bytes : 1.2 ms
2. 256 bytes : 2.3 ms
3. 5 kb : 25.4 ms

## Robin2 Demo

The scripts in the directory [robin2_demo](robin2_demo) are adapted from original Python version 2 code published on the Arduino forum in 2014 by user Robin2:
- [Demo of PC-Arduino comms using Python](https://forum.arduino.cc/t/demo-of-pc-arduino-comms-using-python/219184) (Mar 2014)

Python scripts in this repository:
- [robin2_demo/ComArduino_py2.py](robin2_demo/ComArduino_py2.py) - updated Python 2 script
- [robin2_demo/ComArduino.py](robin2_demo/ComArduino.py) - modified for Python 3+

Also included is the original Arduino script from Robin2:
- [robin2_demo/ArduinoPC/ArduinoPC.ino](robin2_demo/ArduinoPC/ArduinoPC.ino)


See also the introductory tutorial he wrote in 2016:
 - [Serial Input Basics - updated](https://forum.arduino.cc/t/serial-input-basics-updated/382007) (Apr 2016)
