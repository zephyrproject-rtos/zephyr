
NFFS: READ WRITE FILE_STATUS
###########################

Overview
********

This is a simple application demonstrating how to read, write persistence data on 

flash of nrf52 SoC using NFFS file system.


Requirements
************

This sample has been tested on the Nordic nRF52840-PDK board, but would

likely also run on the nrf52_pca10040 board.

Building and Running
********************

To build this example, we have to add "ext/fs/nffs/include" into list of

include directories which can be possible by editing $(zephyr_base)/CMakeLists.txt

After modification that list look like this ...

zephyr_include_directories(

  kernel/include

  arch/${ARCH}/include

  arch/${ARCH}/soc/${SOC_PATH}

  arch/${ARCH}/soc/${SOC_PATH}/include

  arch/${ARCH}/soc/${SOC_FAMILY}/include

  ${BOARD_DIR}

  include

  include/drivers

  ${PROJECT_BINARY_DIR}/include/generated

  ${USERINCLUDE}

  ${STDINCLUDE}

  ext/fs/nffs/include

)

##### After that

cd $(zephyr_base)/samples/boards/nrf52/nffs

mkdir build

cd build

cmake -DBOARD=nrf52840_pca10056 ..

make

cd zephyr

make clean

make all

##### After executing above commands we will find "zephyr.hex" in current directory

##### which we have to flash using nrfjprog utility.

Working
********

After flashing this "zephyr.hex" on nrf52840_pdk board, open serial terminal to see printk messages 

Press Button 2 on PDK board to create & write data in file "0.txt"

Pree Button 1 to read data on "0.txt"




