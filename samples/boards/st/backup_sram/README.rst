.. zephyr:code-sample:: stm32_backup_sram
   :name: Backup SRAM

   Use Backup SRAM to store a variable that persists across power cycles.

Overview
********

Multiple STM32 microcontrollers have a small backup SRAM that can be used as a
NVM when VBAT pin is supplied with a voltage source, e.g. a coin button cell.

This example shows how to define a variable on the Backup SRAM. Each time the
application runs the current value is displayed and then incremented by one. If
VBAT is preserved, the incremented value will be shown on the next power-cycle.

.. note::

    On most boards VBAT is typically connected to VDD thanks to a jumper.
    To exercise this sample with an independent VBAT source, you will need to
    remove the jumper.

Building and Running
********************

In order to run this sample, make sure to enable ``backup_sram`` node in your
board DT file.

.. zephyr-app-commands::
   :zephyr-app: samples/boards/st/backup_sram
   :board: nucleo_h743zi
   :goals: build
   :compact:

Sample Output
=============

.. code-block:: console

    Current value in backup SRAM: 11
    Next reported value should be: 12
    Keep VBAT power source and reset the board now!
