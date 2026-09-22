.. zephyr:code-sample:: adi-pm
   :name: ADI Power Modes

   Exercise power modes for a given SoC

Overview
********

The ADI PM sample exercises the power modes of a given SoC for demonstration
purposes and power measurements.

The ``prj.conf`` shows the required Kconfig for enabling power management and
the devicetree overlay file shows how to configure the RAM retention per memory
bank.

Requirements
************

The board should support power management and have a counter device that can
be used as a system timer companion.

Building and Running
********************

Build and flash ADI power modes as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/adi/power_mgmt/power_modes
   :board: max32657evkit/max32657
   :goals: build flash
   :compact:

After flashing, the application repeatedly cycles through all power modes
defined in the devicetree by sleeping at least the minimum residency time
required for each mode.

Tips:

- When testing power modes or enabling PM on your device, it's a good idea
  to enable :kconfig:option:`CONFIG_BOOT_DELAY` so your debugger doesn't get
  locked out.
