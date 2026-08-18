.. zephyr:code-sample:: clock-control-cdce9xx
   :name: TI cdce9xx clock control driver
   :relevant-api: clock_control_interface

   Use TI cdce9xx clock control driver to generate multiple clock signals.

Introduction
************

This sample allows controlling the TI cdce9xx clock generator capabilities.
The driver can control all outputs of the respective device.

Requirements
************
* Platform with cdce9xx device (e.g. CDCE925PERF-EVM)
* SoC configuration with I2C connection to the cdce9xx device

Configuration
*************

Example overlay for the driver, including default settings for clock outputs:

.. code-block:: devicetree

   / {
       aliases {
           clock-control-dev = &cdce925;
       };
   };

   &i2c {
       cdce925: cdce925@64 {
           compatible = "ti,cdce925";
           reg = <0x64>;
           input-clock-type = "vcxo";
           input-frequency = <27000000>;

           pll1 {
               clock-mult = <8>;
               clock-div = <2>;
               first-divider = <11>;
           };

           pll2 {
               clock-mult = <8>;
               clock-div = <2>;
               ssc = "center-0.25";
               first-divider = <10>;
           };
       };
   };

Driver Usage
************

The driver is interfaced with the :ref:`Clock Control API <clock_control_api>` functions.

Sample usage
************

This sample provides access to the driver functions with these commands:

* on <output>,
* off <output>,
* get_rate <output>,
* get_status <output>,
* set_rate <output> <rate in Hz>

Building
********

.. code-block:: none

  west build -b <board> zephyr/samples/drivers/clock_control -- -DDTC_OVERLAY_FILE=<your overlay>

Drivers prints useful debugging information to the log. With setting the debug level to DBG results of vco frequency calculation a logged.

Sample output
*************

.. code-block:: none

clock_control set_rate 2 10000000
clock_control_set_rate returned 0

[00:00:22.318,000] <dbg> cdce9xx.pll_calculate_parameter: vco_rate: 80000000, m: 27, n: 80
[00:00:22.319,000] <dbg> cdce9xx.configure_pll: configure_pll n=80 m=27 p=3 q=23 r=19
[00:00:22.323,000] <dbg> cdce9xx.dump_current_state: Input frequency: 27000000
[00:00:22.323,000] <dbg> cdce9xx.dump_current_state: Pdiv1: 0
[00:00:22.324,000] <dbg> cdce9xx.dump_current_state: PLL1 multiplexer: enabled, 80000000 Hz
[00:00:22.324,000] <dbg> cdce9xx.dump_current_state: Y2 multiplexer: Pdiv2
[00:00:22.324,000] <dbg> cdce9xx.dump_current_state: Y3 multiplexer: Pdiv3
[00:00:22.324,000] <dbg> cdce9xx.dump_current_state: Pdiv2: 8
[00:00:22.325,000] <dbg> cdce9xx.dump_current_state: Pdiv3: 0
[00:00:22.325,000] <dbg> cdce9xx.dump_current_state: PLL2 multiplexer: enabled, 108000000 Hz
[00:00:22.325,000] <dbg> cdce9xx.dump_current_state: Y4 multiplexer: Pdiv4
[00:00:22.325,000] <dbg> cdce9xx.dump_current_state: Y5 multiplexer: Pdiv5
[00:00:22.326,000] <dbg> cdce9xx.dump_current_state: Pdiv4: 10
[00:00:22.327,000] <dbg> cdce9xx.dump_current_state: Pdiv5: 0
