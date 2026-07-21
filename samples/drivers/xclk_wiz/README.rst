.. _xlnx_clk_wiz_sample:

Clocking Wizard sample application
######################################

Overview
********

This sample demonstrates the three Early-Access (EA) ``clock_control`` APIs
implemented by the Xilinx AXI Clocking Wizard driver
(``xlnx,clkx5-wiz-1.0``) on AMD Versal Gen 2 (Telluride / VERSAL_NET):

- **EA1** – ``clock_control_on`` / ``clock_control_off`` (clock output enable)
- **EA2** – ``clock_control_set_rate`` (dynamic reconfiguration of CLKOUT rate)
- **EA3** – ``clock_control_get_rate`` (read back the programmed output rate)

The sample targets CLKOUT0 of the first AXI Clocking Wizard instance
(``clk_wiz_0`` at ``0xb2000000``) and steps through a set of target
frequencies: 100 MHz, 200 MHz, and 300 MHz.

Hardware Requirements
*********************

- AMD Versal Gen 2 (VEK385 or equivalent) board
- Vivado bitstream with AXI Clocking Wizard IP instantiated at
   ``0xb2000000`` with a 100 MHz reference clock input
- JTAG or UART connection for serial output

Building and Running
********************

.. code-block:: bash

   west build -b versal2_apu samples/drivers/xclk_wiz
   west flash   # or load via XSDB / JTAG

Sample Output
*************

.. code-block:: none

   Clocking Wizard sample application
   Device: clock-controller@b2000000, testing CLKOUT0
   Initial CLKOUT0 rate: 100000000 Hz
   CLKOUT0 enabled (clock_control_on OK)
   Requesting CLKOUT0 = 100000000 Hz ...
   Actual  CLKOUT0 = 100000000 Hz
   Requesting CLKOUT0 = 200000000 Hz ...
   Actual  CLKOUT0 = 200000000 Hz
   Requesting CLKOUT0 = 300000000 Hz ...
   Actual  CLKOUT0 = 300000000 Hz
   CLKOUT0 disabled (clock_control_off OK)

Configuration
*************

- A ``misc_clk_0`` fixed-clock node (100 MHz reference)
- A ``clk_wiz_0`` clock-controller node at ``0xb2000000``

Adjust ``xlnx,prim-in-freq`` and ``xlnx,num-out-clks`` in the overlay to
match your specific board design.

Related Files
*************

- Driver: ``drivers/clock_control/clock_control_xclk_wiz.c``
- DTS binding: ``dts/bindings/clock/xlnx,clkx5-wiz-1.0.yaml``
- Kconfig: ``drivers/clock_control/Kconfig.xclk_wiz``
