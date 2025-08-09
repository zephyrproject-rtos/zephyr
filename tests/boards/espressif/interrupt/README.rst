.. _interrupt_test:

Espressif's Xtensa interrupt test
#################################

Overview
********

TODO


Supported Boards
****************
- esp32s3_devkitm/esp32s3/procpu

Building and Running
********************

Make sure you have the target connected over USB port.

.. code-block:: console

   west build -b <board> tests/boards/espressif/interrupt
   west flash && west espressif monitor

To run the test with twister, use the following command:

.. code-block:: console

   west twister -p <board> --device-testing --device-serial=/dev/ttyUSB0 -vv --flash-before -T tests/boards/espressif/interrupt

If the external 32K crystal is connect to pins 32K_XP and 32K_XN, the test can be run with ``external_xtal`` fixture enabled:

.. code-block:: console

	west twister -p esp32s3_devkitm --device-testing --device-serial=/dev/ttyUSB0 -vv --flash-before -T tests/boards/espressif/interrupt

Sample Output
=============
