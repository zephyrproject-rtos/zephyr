.. _stm32-pm-serial-wakeup-sample:

STM32 PM Serial wakeup
######################

Overview
********

This sample is a minimum application to demonstrate serial wakeup functionality
in low power context.

.. _stm32-pm-serial-wakeup-sample-requirements:

Requirements
************

1. The board should support enabling PM. For a STM32 based target, it means that
it should support a clock source alternative to Cortex Systick that can be used
in core sleep states, as LPTIM (:dtcompatible:`st,stm32-lptim`).

2. The serial port used by the shell should be configured, using device tree, to
be a functional wakeup source:

  - Clocked by an oscillator available in Stop mode (LSE, LSI) or an oscillator capable
    that can be requested dynamically by device on activity detection (HSI on STM32WB).
  - Matching oscillator sources should be enabled
  - If LSE is selected as clock source and shell serial port is a LPUART current speed
    should be adapted (9600 bauds)
  - Port should be set as "wakeup-source"

Note: Using HSI clock is a specific

Building and Running
********************

Build and flash this sample as follows, changing ``nucleo_wb55rg`` for a board
configured to be compatible with this sample.

.. zephyr-app-commands::
   :zephyr-app: samples/boards/stm32/power_mgmt/serial_wakeup
   :board: nucleo_wb55rg
   :goals: build flash
   :compact:

After flashing, the shell is enabled and device enter sleep mode.
User is able to wake up the device by typing into the shell

PM configurations
*****************

By default, :kconfig:option:`CONFIG_PM_DEVICE` and :kconfig:option:`CONFIG_PM_DEVICE_RUNTIME`
are enabled, but user can also deactivate both or former to see each configuration in play.

Debugging
*********

:kconfig:option:`CONFIG_DEBUG` could be enabled to allow debug. Note that debug mode prevents
target to reach low power consumption.
Also note that after debug mode has been disabled, target should also be powered off in order
to get back to normal mode and reach low power consumption.

PM measurements on stm32l562e_dk using stm32l562e_dk PM shield
**************************************************************

Plug Power shield
Plug ST-Link
Set JP4 To 5V ST-Link
Set SW1 to PM_SEL_VDD
STM32Cube PowerMonitor settings to be applied:

  - Sampling Freq: max
  - Functional Mode: High

Optimal configuration for low power consumption
***********************************************

In order to reach lower power consumption numbers following parameters should be taken
into account:

  - Use a LPUART instead of a basic U(S)ART node
  - Chose LSE as clock source
  - Ensure no other oscillators are enabled (disable HSI, ...)
  - Provide "sleep" pinctrl configuration to other uart nodes.
  - Disable Debug mode

 With all these conditions matched, one can reach 10uA on stm32l562e_dk with this sample.
