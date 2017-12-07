# Arduino RFID lock

Arduino program to control an electronic lock that is released when a set of [RFID readers](https://www.sparkfun.com/products/13198) detect some pre-determined tags.

This program is prepared to run on an Arduino Uno and uses the HW serial port and two SW serial ports. Tags must be read in order due to the SW serial limitation of not being able to listen on more than one SW serial port in parallel.