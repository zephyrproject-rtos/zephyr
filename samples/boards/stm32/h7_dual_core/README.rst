.. _stm32_h7_dual_core:

STM32 HSEM IPM driver sample
############################

Overview
********
Blinky led triggered by mailbox new message.

Building and Running
********************

Build for stm32h747i_disco_m7:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/stm32/h7_dual_core
   :board: stm32h747i_disco_m7
   :goals: build

Build for stm32h747i_disco_m4:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/stm32/h7_dual_core
   :board: stm32h747i_disco_m4
   :goals: build

Sample Output
=============

.. code-block:: console

    STM32 h7_dual_core application
