.. _samples_flash_shell:

Flash Shell Sample
##################

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

    uart:~$ flash_sample page_count
    Flash device contains 1024 pages.
    uart:~$ flash_sample page_erase 1023
    Erasing page 1023 (start offset 0xffc00, size 0x400)
    uart:~$ flash_sample page_write 1023 8 17 19 28 39
    Reading back written bytes:
    11 13 1c 27
    uart:~$ flash_sample page_write 1023 4 77 9 1 2
    Reading back written bytes:
    4d 09 01 02
    uart:~$ flash_sample page_read 1023 4 12
    4d 09 01 02 | 11 13 1c 27
    ff ff ff ff
    uart:~$ flash_sample page_read 1023 0 16
    ff ff ff ff | 4d 09 01 02
    11 13 1c 27 | ff ff ff ff
