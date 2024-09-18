.. zephyr:code-sample:: fs-format
   :name: Format filesystem
   :relevant-api: file_system_api

   Format different storage devices for different file systems.

Overview
***********

This sample shows how to format different storage
devices for different file systems. There are 2 scenarios prepared for this
sample:

* littleFS on flash device
* FAT file system on RAM disk

Building and running
********************

To run this sample, build it for the desired board and scenario and flash it.

The Flash scenario is supported on the nrf52dk/nrf52832 board.
The RAM disk scenario is supported on the mimxrt1064_evk board.
To build the RAM disk sample, the configuration `prj_ram.conf` needs to be used by setting `CONF_FILE=prj_ram.conf`.

The Flash sample for the nrf 52DK board can be built as follow:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/fs/format
   :board: nrf52dk/nrf52832
   :goals: build flash
   :compact:

The RAM disk sample for the MIMXRT1064-EVK board can be built as follow:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/fs/format
   :board: mimxrt1064_evk
   :conf: "prj_ram.conf"
   :goals: build flash
   :compact:

Sample Output
=============

When the sample runs successfully you should see following message on the screen:
.. code-block:: console

  I: LittleFS version 2.4, disk version 2.0
  I: FS at flash-controller@4001e000:0x7a000 is 6 0x1000-byte blocks with 512 cycle
  I: sizes: rd 16 ; pr 16 ; ca 64 ; la 32
  I: Format successful
