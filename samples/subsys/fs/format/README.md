# Format Sample Application

## Overview
The aim of this sample application is to show how to format different storage
devices for different file systems. There are 2 scenarios prepared for this
sample:
* littleFS on flash device
* FAT file system on RAM disk

## Building and running
To run this demo, build it for the desired board and flash it.
To build demo that will use RAM disk use `prj_ram.conf` configuration file (e.g.
by setting `CONF_FILE=prj_ram.conf`).

The demo was build for 2 boards:
1. nRF 52DK with flash
  ```
  west build -b nrf52dk_nrf52832 samples/subsys/fs/format
  west flash
  ```

2. MIMXRT1064-EVK with RAM disk
  ```
  CONF_FILE=prj_ram.conf west build -b mimxrt1064_evk samples/subsys/fs/format
  west flash
  ```

When sample runs successfully you should see following message on the screen:
```
I: LittleFS version 2.4, disk version 2.0
I: FS at flash-controller@4001e000:0x7a000 is 6 0x1000-byte blocks with 512 cycle
I: sizes: rd 16 ; pr 16 ; ca 64 ; la 32
I: Format successful
```
