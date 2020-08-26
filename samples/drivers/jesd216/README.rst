.. jesd216-sample:

JESD216 Sample
##############

Overview
********

This sample demonstrates how to use the JESD216 flash API to extract
information from a compatible serial device, and serves as utility to
generate ``jedec,spi-nor`` devicetree property values for the device.

Building and Running
********************

The application will build only for a target that has a devicetree entry
with ``jedec,spi-nor`` as a compatible, and for which the driver
supports :option:`CONFIG_FLASH_JESD216_API`.

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/jesd216
   :board: efr32mg_sltb004a
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

   MX25R8035F: SFDP v 1.6 AP ff with 2 PH
   PH0: ff00 rev 1.6: 16 DW @ 30
   Summary of BFP content:
   DTR Clocking not supported
   Addressing: 3-Byte only
   4-KiBy erase: uniform
   Support QSPI XIP
   Support 1-1-1
   Support 1-1-2: instr 3Bh, 0 mode clocks, 8 waits
   Support 1-1-4: instr 6Bh, 0 mode clocks, 8 waits
   Support 1-2-2: instr BBh, 0 mode clocks, 4 waits
   Support 1-4-4: instr EBh, 2 mode clocks, 4 waits
   Flash density: 1048576 bytes
   ET1: instr 20h for 4096 By; typ 48 ms, max 384 ms
   ET2: instr 52h for 32768 By; typ 240 ms, max 1920 ms
   ET3: instr D8h for 65536 By; typ 480 ms, max 3840 ms
   Chip erase: typ 5120 ms, max 30720 ms
   Byte program: type 32 + 1 * B us, max 192 + 6 * B us
   Page program: typ 896 us, max 5376 us
   Page size: 256 By
   Suspend: B0h ; Resume: 30h
   DPD: Enter B9h, exit ABh ; delay 40000 ns ; poll 0x3d
   size = <8388608>;
   sfdp-bfp = [
           e5 20 f1 ff  ff ff 7f 00  44 eb 08 6b  08 3b 04 bb
           ee ff ff ff  ff ff 00 ff  ff ff 00 ff  0c 20 0f 52
           10 d8 00 ff  23 72 f5 00  82 ed 04 b7  44 83 38 44
           30 b0 30 b0  f7 c4 d5 5c  00 be 29 ff  f0 d0 ff ff
           ];
   PH1: ffc2 rev 1.0: 4 DW @ 110
   sfdp-ffc2 = [
           00 36 50 16  9d f9 c0 64  fe cf ff ff  ff ff ff ff
           ];
   jedec-id = [c2 28 14];
