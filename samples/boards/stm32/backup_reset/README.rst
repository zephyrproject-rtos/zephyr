.. _stm32_backup_reset:

STM32 Backup Reset
##################

Overview
********

Multiple STM32 microcontrollers have a small backup SRAM that can be used as a
NVM when VBAT pin is supplied with a voltage source, e.g. a coin button cell.
This example shows how to define a variable on the Backup SRAM. Each time the
application runs, the current value is displayed and then incremented by one.
After power On, the stored value is supposed to start from 0, whatever the VBAT.


.. note::

    On stm32F2x or stm32F446 or stm32F7x or stm32F405-based
    (STM32F405/415, STM32F407/417, STM32F427/437, STM32F429/439)
    the BackUp SRAM is not resetted when the VBAT is off
    but when the protection level changes from 1 to 0.
    With this sample, it is resetted on power On.


Building and Running
********************

In order to run this sample, make sure to enable ``backup_sram`` node in your
board DT file.

.. zephyr-app-commands::
   :zephyr-app: samples/boards/stm32/backup_reset
   :board: nucleo_f429zi
   :goals: build
   :compact:

Sample Output
=============

.. code-block:: console

    Current value in backup SRAM: 11
    Next reported value should be: 12
    Reset or Power off/on the board now!
