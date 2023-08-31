.. _hello_world_xip:

Hello World in XiP
##################

Overview
********

A simple sample that can be used with any :ref:`supported board <boards>`
which has an external NOR octo- or quad- flash and
prints "Hello World from external flash" to the console.


Building and Running
********************

This application can be built with 
west build -p always -b b_u585i_iot02a samples/boards/stm32/hello_world_xip/ --sysbuild -- -DSB_CONFIG_BOOTLOADER_MCUBOOT=y

and executed on b_u585i_iot02a as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/stm32/hello_world_xip
   :host-os: unix
   :board: b_u585i_iot02a
   :goals: run
   :compact:

To build for another board, change "b_u585i_iot02a" above to that board's name.

Sample Output
=============

.. code-block:: console

    Hello World! from external flash b_u585i_iot02a


Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.
