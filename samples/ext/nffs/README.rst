.. _nffs_sample:

File system: NFFS
####################

Overview
*********
Set up a Newtron Flash File system (NFFS) on a Nordic Semiconductor NRF5x device.


Requirements
************
* NRF5x development kit. (Has only been tested on boards: BOARD=nrf52_pca10040 and BOARD=nrf52840_pca10056).
* A serial terminal for observing example output. 115200 baud, no parity, 8 data bits, 1 stop bit, no HW flow control.

Building and Running
********************
This sample can be found under /samples/ext/nffs in the Zephyr tree.

    $ make BOARD=nrf52840_pca10056
    or
    $ make BOARD=nrf52_pca10040

Connect one of the supported development kits to your computer via USB.
Download and run the generated /build/BOARD/zephyr.hex file with nrfjprog:

    $ nrfjprog --eraseall -f nrf52
    $ nrfjprog --program ./zephyr.hex -f nrf52 -r

Then listen for output using your favorite terminal program.
The file that is written in the NFFS file system will be opened, and a numeric value in this file
incremented each time it is written. You can see that the information is retained by power cycing the
development kit and reconnecting the serial link.


Additional information on the targets:

https://www.zephyrproject.org/doc/boards/arm/nrf52_pca10040/doc/nrf52_pca10040.html
https://www.zephyrproject.org/doc/boards/arm/nrf52840_pca10056/doc/nrf52840_pca10056.html

