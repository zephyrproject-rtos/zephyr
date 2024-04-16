.. _hello_world:

Hello World
###########

Overview
********

A simple sample that can be used with any :ref:`supported board <boards>` and
prints "Hello World" to the console.

Building and Running
********************

This application can be built and executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :host-os: unix
   :board: qemu_x86
   :goals: run
   :compact:

To build for another board, change "qemu_x86" above to that board's name.

Sample Output
=============

.. code-block:: console

    Hello World! x86

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.

NOTE
====

* If you want to run this code in XIP Mode, then in the prj.conf, set **CONFIG_XIP=y**, and **CONFIG_CODE_DATA_RELOCATION=y**. Now, your code will start
executing at address 0x90000000, which is your flash on vcu118 FPGA. 

* In order to run this code on XIP on ASIC, then set the FLASH_BASE_ADDRESS to 0x90000400 and in linker_secure-iot.ld change the ORIGIN of EXTFLASH to 0x90000400.