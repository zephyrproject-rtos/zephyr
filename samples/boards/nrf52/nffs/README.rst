.. _nrf52_NFFS_sample:

nrf52 NFFS Read/Write Sample
############################

This is a simple application demonstrating how to read, write persistence data on 
flash of nRF52 SoC using NFFS file system.

Requirements
************

This sample has been tested on the Nordic nRF52840-PDK board, but would
likely also run on the nrf52_pca10040 board.

Building and Running
********************

This sample can be found under $(zephyr_base)/samples/boards/nrf52/nffs in the
Zephyr tree.

I've already included $(zephyr_base)/ext/fs/nffs directory 
in project by editing :file:'$(zephyr_base)samples/boards/nrf52/nffs/CMakeLists.txt'
which is important to build this project.

The following commands build the application.

.. code-block:: bash

   cd $(ZEPHYR_BASE)/samples/boards/nrf52/nffs
   mkdir build
   cd build
   cmake -DBOARD=nrf52840_pca10056 ..
   make
   cd zephyr
   make clean
   make all

After the build completes, the resulting binary "zephyr.hex" will be
in the current directory.  Use this file to flash the board using the
"nrfjprog" utility.

After flashing to the nrf52840_pdk board, open a serial terminal to see console messages
written by the application:

- Press Button 2 on the PDK board to create and write data in file "0.txt"
- Press Button 1 to read data from "0.txt"

For more information about the Zephyr file system, see the :ref:'file_system' documentation. 
