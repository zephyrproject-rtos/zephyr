.. zephyr:code-sample:: adi_lsmspg_curve_tracer
   :name: ADALM-LSMSPG curve tracer
   :relevant-api: dac_interface adc_interface

   Trace the Ic-Vc curves of the NPN and PNP curve tracer circuits on the
   ADALM-LSMSPG shield.

Overview
********

This sample reproduces, on Zephyr, the two transistor curve tracers built
into the ADALM-LSMSPG shield (also shown in the no-OS ``adalm-lsmspg``
``curvetrace_example``). For each base voltage step it sweeps the collector
drive voltage over 50 points and measures the resulting collector current and
voltage, then prints the curves as an ASCII plot.

The NPN curve tracer uses the AD5592R (SPI), from the ADALM-LSMSPG schematic:

- AD5592R CH0 (DAC) drives the 2N3904 base through a 49.9 kohm resistor.
- AD5592R CH2 (DAC + ADC) drives the collector through a 49.9 ohm sense
  resistor.
- AD5592R CH1 (ADC) senses the collector voltage after the sense resistor.

The PNP curve tracer uses the AD5593R (I2C):

- AD5593R CH3 (DAC) drives the emitter through jumper P7.
- AD5593R CH0 (DAC) drives the base through a 49.9 kohm resistor.
- AD5593R CH1 (ADC) senses the collector voltage.
- The collector returns to AD5593R CH2 (DAC + ADC) through a 49.9 ohm sense
  resistor.

Collector current is computed as the voltage drop across the sense resistor
divided by its resistance; collector voltage is read directly. The resistor
values default to the schematic values and can be changed with
:kconfig:option:`CONFIG_SAMPLE_NPN_CURVE_TRACER_RSENSE_MOHM`,
:kconfig:option:`CONFIG_SAMPLE_NPN_CURVE_TRACER_RBASE_OHM`,
:kconfig:option:`CONFIG_SAMPLE_PNP_CURVE_TRACER_RSENSE_MOHM` and
:kconfig:option:`CONFIG_SAMPLE_PNP_CURVE_TRACER_RBASE_OHM` (the no-OS example
uses 47 ohm / 47 kohm instead). The PNP emitter drive voltage is set with
:kconfig:option:`CONFIG_SAMPLE_PNP_CURVE_TRACER_EMITTER_MV`.

Requirements
************

This sample needs a board with Feather-compatible SPI and I2C connectors and
the :ref:`adi_lsmspg` shield plugged in, with jumper P7 fitted so that the
AD5593R CH3 drives the PNP emitter (the default). It has been tested on
:zephyr:board:`max32666fthr`.

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/shields/adi_lsmspg/curve_tracer
   :board: max32666fthr/max32666/cpu0
   :goals: build flash
   :compact:

Nothing runs automatically at boot. Connect to the console and type
``npn_curvetrace`` or ``pnp_curvetrace`` to run one sweep:

.. code-block:: console

   ADALM-LSMSPG NPN/PNP curve tracer
   Type 'npn_curvetrace' to run the NPN curve trace (AD5592R)
   Type 'pnp_curvetrace' to run the PNP curve trace (AD5593R)
   uart:~$ npn_curvetrace

   ========== AD5592R (SPI) NPN Curve Tracer ==========
   Vref: 2500 mV, Scale: 0.6104 mV/LSB, Rsense: 49.9 ohm, Rbase: 49900 ohm

   Starting sweep...
   Base Drive: 0.4987 V, -4.035 uA
     coll voltage: 0.0012 V  coll current: 0.0000 mA
     coll voltage: 0.0488 V  coll current: 0.0122 mA
     ...

   === AD5592R (SPI) - NPN Curve Tracer (Ic vs Vc) ===
   Y-axis: Ic (0 to 6.76 mA)
   X-axis: Vc (0 to 2.45 V)

   +------------------------------------------------------------+
   |                                                ***         |
   |         ****** ***** ***** ***** ***** ***** **            |
   ...
   +------------------------------------------------------------+
   0.0       0.49       0.98       1.47       1.96       2.45 V

   ===== AD5592R (SPI) NPN Curve Trace Complete =====

   Sweep took 3376 ms
   uart:~$ pnp_curvetrace

   ========== AD5593R (I2C) PNP Curve Tracer ==========
   Vref: 2500 mV, Scale: 0.6104 mV/LSB, Rsense: 49.9 ohm, Rbase: 49900 ohm
   Emitter Drive: 2.4988 V

   Starting sweep...
   Base Drive: 1.9995 V, -4.023 uA
     coll voltage: 2.4994 V  coll current: 0.0000 mA
     coll voltage: 2.4524 V  coll current: 0.0122 mA
     ...

   === AD5593R (I2C) - PNP Curve Tracer (Ic vs Vc) ===
   Y-axis: Ic (0 to 5.87 mA)
   X-axis: Vc (0 to 2.50 V)

   +------------------------------------------------------------+
   |            ******* ****** ***** ****** ***** ****** *******|
   |        ** *                                            **  |
   ...
   +------------------------------------------------------------+
   0.0       0.50       1.00       1.50       2.00       2.50 V

   ===== AD5593R (I2C) PNP Curve Trace Complete =====

   Sweep took 3136 ms
   uart:~$
