.. zephyr:code-sample:: stm32_mco
   :name: Master Clock Output (MCO)
   :relevant-api: pinctrl_interface

   Output an internal clock for external use by the application.

Overview
********

This sample is a minimum application to demonstrate how to output one of the internal clocks for
external use by the application.

Requirements
************

The SoC should support MCO functionality and use a pin that has the MCO alternate function.
To support another board, add a dts overlay file in boards folder.
Make sure that the output clock is enabled in dts overlay file.
Depending on the stm32 serie, several clock source and prescaler are possible for each MCOx.
The clock source is set by the DTS among the possible values for each stm32 serie.
The prescaler is set by the DTS, through the property ``prescaler = <MCOx_PRE(MCO_PRE_DIV_n)>;``

See :zephyr_file:`dts/bindings/clock/st,stm32-clock-mco.yaml`

It is required to check the Reference Manual to  configure the DTS correctly.


Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/boards/st/mco
   :board: nucleo_u5a5zj_q
   :goals: build flash

After flashing, the LSE clock will be output on the MCO pin enabled in Device Tree.
The clock can be observed using a probing device, such as a logic analyzer.
