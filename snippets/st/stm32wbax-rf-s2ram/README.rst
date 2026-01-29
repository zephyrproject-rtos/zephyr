.. _snippet-st-stm32wbax-rf-s2ram:

STM32WBAX RF-compatible PM-with-S2RAM configuration (stm32wbax-rf-s2ram)
########################################################################

Overview
********

This snippet enables Power Management with ``suspend-to-ram`` support (mapped to
STM32 ``Standby`` hardware low-power mode) on boards equipped with an STMicroelectronics
STM32WBA series SoC. The configuration applied is suitable for RF applications.

Tested successfully on :zephyr:board:`nucleo_wba55cg` and :zephyr:board:`nucleo_wba65ri`.

.. code-block:: console

   west build -S stm32wbax-rf-s2ram [...]

Requirements
************

This snippet is only valid for boards with an STM32WBA series SoC.
