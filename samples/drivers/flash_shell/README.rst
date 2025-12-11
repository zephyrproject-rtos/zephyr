.. zephyr:code-sample:: flash-shell
   :name: Flash shell
   :relevant-api: flash_interface

   Explore a flash device using shell commands.

Overview
********

This is a simple shell module that allows arbitrary boards with flash
driver support to explore the flash device.

Building and Running
********************

This project can be built and executed on as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/flash_shell
   :host-os: unix
   :board: qemu_x86
   :goals: run
   :compact:

Sample Output
=============

.. code-block:: console

    uart:~$ flash page_info 0
    Page for address 0x0:
    start offset: 0x0
    size: 4096
    index: 0
    uart:~$ flash erase 0x1000
    Erase success.
    uart:~$ flash write 0x1000 0x12345678 0x9abcdef0
    Write OK.
    Verified.
    uart:~$ flash write 0x1000 0x11111111
    Write internal ERROR!
    uart:~$ flash read 0x1000 0x10
    00001000: 78 56 34 12 f0 de bc 9a  ff ff ff ff ff ff ff ff |xV4..... ........|

    uart:~$ flash write 0x101c 0xabcd1234
    Write OK.
    Verified.
    uart:~$ flash read 0x1000 0x20
    00001000: 78 56 34 12 f0 de bc 9a  ff ff ff ff ff ff ff ff |xV4..... ........|
    00001010: ff ff ff ff ff ff ff ff  ff ff ff ff 34 12 cd ab |........ ....4...|
