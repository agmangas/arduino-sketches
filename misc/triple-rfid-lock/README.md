# Triple Arduino RFID Lock

Arduino program to control an electronic lock that is released when three RFID sensors based on the [ID-12LA](https://www.sparkfun.com/products/11827) module detect some pre-determined tags.

This program is designed to run on an Arduino Uno and uses the HW serial port and two SW serial ports.

> Tags must be read in a predefined order due to the SW serial limitation of not being able to listen on more than one SW serial port in parallel.

## Configuring the Tag IDs

You should update the `acceptedTags` array with the tag IDs that you wish to use to release the lock. The first item corresponds to the first RFID sensor (HW port), the second item to the second RFID sensor (SW port #1), and so on.