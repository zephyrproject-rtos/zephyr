.. _qomu_usbserial:

Zephyr usbserial driver on Qomu
###############################

This sample demonstrates how to load bitstream on EOS-S3 FPGA and use the
usbserial driver.

Requirements
************
* Zephyr RTOS with printk enabled
* `QuickLogic Qomu board <https://github.com/QuickLogic-Corp/qomu-feather-dev-board>`_

Building
********

.. zephyr-app-commands::
   :zephyr-app: samples/boards/qomu
   :host-os: unix
   :board: qomu
   :goals: build
   :compact:

Flashing
********

To load example into Qomu you can use `TinyFPGA-Programmer-Application <https://github.com/QuickLogic-Corp/TinyFPGA-Programmer-Application>`_.

.. code-block:: console

   python3 /PATH/TO/BASE/DIR/TinyFPGA-Programmer-Application/tinyfpga-programmer-gui.py --mode m4 --m4app build/zephyr/zephyr.bin --reset

See `Qomu User Guide <https://github.com/QuickLogic-Corp/qomu-dev-board/blob/662f8841bdc1ed35c1539ac381182159d7cd5914/doc/Qomu_UserGuide.pdf>`_ on how to load an image to the board.

Running
*******

After connecting to the shell console you should see the following output:

.. code-block:: console

   ####################
   I am Zephyr on Qomu!
   ####################
