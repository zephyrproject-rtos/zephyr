.. zephyr:code-sample:: clock-control-litex
   :name: LiteX clock control driver
   :relevant-api: clock_control_interface

   Use LiteX clock control driver to generate multiple clock signals.

Introduction
************

This sample is providing an overview of LiteX clock control driver capabilities.
The driver uses Mixed Mode Clock Manager (MMCM) module to generate up to 7 clocks with defined phase, frequency and duty cycle.

Requirements
************
* LiteX-capable FPGA platform with MMCM modules (for example Digilent Arty A7 development board)
* SoC configuration with VexRiscv soft CPU and Xilinx 7-series MMCM interface in LiteX (S7MMCM module)
* Optional: clock output signals redirected to output pins for testing

Configuration
*************
Basic configuration of the driver, including default settings for clock outputs, is held in Device Tree clock control nodes.

.. literalinclude:: ../../../dts/riscv/riscv32-litex-vexriscv.dtsi
   :language: dts
   :start-at: clk0: clock-controller@0 {
   :end-at: };
   :dedent:

.. literalinclude:: ../../../dts/riscv/riscv32-litex-vexriscv.dtsi
   :language: dts
   :start-at: clk1: clock-controller@1 {
   :end-at: };
   :dedent:

.. literalinclude:: ../../../dts/riscv/riscv32-litex-vexriscv.dtsi
   :language: dts
   :start-at: clock0: clock@e0004800 {
   :end-at: };
   :dedent:

This configuration defines 2 clock outputs: ``clk0`` and ``clk1`` with default frequency set to 100MHz, 0 degrees phase offset and 50% duty cycle. Special care should be taken when defining values for FPGA-specific configuration (parameters from ``litex,divclk-divide-min`` to ``litex,vco-margin``).

**Important note:**  ``reg`` properties in ``clk0`` and ``clk1`` nodes reference the clock output number (``clkout_nr``)

Driver Usage
************

The driver is interfaced with the :ref:`Clock Control API <clock_control_api>` function ``clock_control_on()`` and a LiteX driver specific structure (:c:struct:`litex_clk_setup`).

| To change clock parameter it is needed to cast a pointer to structure :c:struct:`litex_clk_setup` onto :c:type:`clock_control_subsys_t` and use it with :c:func:`clock_control_on()`.
| This code will try to set on ``clk0`` frequency 50MHz, 90 degrees of phase offset and 75% duty cycle.

.. code-block:: c

	struct device *dev;
	int ret;
	struct litex_clk_setup setup = {
		.clkout_nr = 0,
		.rate = 50000000,
		.duty = 75,
		.phase = 90
	};
	dev = DEVICE_DT_GET(MMCM);
	clock_control_subsys_t sub_system = (clock_control_subsys_t)&setup;
	if ((ret = clock_control_on(dev, sub_system)) != 0) {
		LOG_ERR("Set CLKOUT%d param error!", setup.clkout_nr);
		return ret;
	}

Clock output status (frequency, duty and phase offset) can be acquired with function ``clock_control_get_status()`` and clock output frequency only can be queried with ``clock_control_get_rate()``

In both getter functions, basic usage is similar to ``clock_control_on()``. Structure ``litex_clk_setup`` is used to set ``clkout_nr`` of clock output from which data is to be acquired.

Sample usage
************

This example provides a simple way of checking various clock output settings. User can pick one of 4 possible scenarios:

* Frequency range,
* Duty cycle range,
* Phase range,
* Setting frequency, duty and phase at once, then check clock status and rate,

Scenarios are selected by defining ``LITEX_CLK_TEST`` as one of:

* ``LITEX_TEST_FREQUENCY``
* ``LITEX_TEST_DUTY``
* ``LITEX_TEST_PHASE``
* ``LITEX_TEST_SINGLE``

Code is performed on 2 clock outputs with ``clkout_nr`` defined in ``LITEX_CLK_TEST_CLK1`` and ``LITEX_CLK_TEST_CLK2``. Tests are controlled by separate defines for each scenario.

Building
********

.. code-block:: none

  west build -b litex_vexriscv zephyr/samples/drivers/clock_control

Drivers prints a lot of useful debugging information to the log. It is highly recommended to enable logging and synchronous processing of log messages and set log level to ``Info``.

Sample output
*************

.. code-block:: none

  [00:00:00.200,000] <inf> CLK_CTRL_LITEX: CLKOUT0: set rate: 100000000 HZ
  [00:00:00.240,000] <inf> CLK_CTRL_LITEX: CLKOUT1: updated rate: 100000000 to 100000000 HZ
  [00:00:00.280,000] <inf> CLK_CTRL_LITEX: CLKOUT0: set duty: 50%
  [00:00:00.320,000] <inf> CLK_CTRL_LITEX: CLKOUT0: set phase: 0 deg
  [00:00:00.360,000] <inf> CLK_CTRL_LITEX: CLKOUT1: set rate: 100000000 HZ
  [00:00:00.400,000] <inf> CLK_CTRL_LITEX: CLKOUT1: set duty: 50%
  [00:00:00.440,000] <inf> CLK_CTRL_LITEX: CLKOUT1: set phase: 0 deg
  [00:00:00.440,000] <inf> CLK_CTRL_LITEX: LiteX Clock Control driver initialized
  *** Booting Zephyr OS build zephyr-v2.2.0-2810-g1ca5dda196c3  ***
  Clock Control Example! riscv32
  device name: clock0
  clock control device is 0x40013460, name is clock0
  Clock test
  Single test
  [00:00:00.510,000] <inf> CLK_CTRL_LITEX: CLKOUT0: set rate: 15000000 HZ
  [00:00:00.550,000] <inf> CLK_CTRL_LITEX: CLKOUT0: set phase: 90 deg
  [00:00:00.590,000] <inf> CLK_CTRL_LITEX: CLKOUT0: set duty: 25%
  [00:00:00.630,000] <inf> CLK_CTRL_LITEX: CLKOUT1: set rate: 15000000 HZ
  [00:00:00.670,000] <inf> CLK_CTRL_LITEX: CLKOUT1: set duty: 75%
  Getters test
  CLKOUT0: get_status: rate:15000000 phase:90 duty:25
  CLKOUT0: get_rate:15000000
  CLKOUT1: get_status: rate:15000000 phase:0 duty:75
  CLKOUT1: get_rate:15000000
  Clock test done returning: 0

References
**********

- :zephyr:board:`litex_vexriscv`
