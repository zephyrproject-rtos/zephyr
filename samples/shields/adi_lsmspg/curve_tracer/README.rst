.. zephyr:code-sample:: adi_lsmspg_curve_tracer
   :name: ADALM-LSMSPG curve tracer
   :relevant-api: dac_interface adc_interface

   Trace the Ic-Vc curves of the NPN curve tracer circuit on the
   ADALM-LSMSPG shield.

Overview
********

This sample reproduces, on Zephyr, the NPN transistor curve tracer built
into the ADALM-LSMSPG shield (also shown in the no-OS ``adalm-lsmspg``
``curvetrace_example``). It sweeps the
2N3904's base voltage over 5 steps and, for each step, sweeps the
collector drive voltage over 50 points, measuring the resulting
collector current and voltage through the AD5592R.

The circuit, from the ADALM-LSMSPG schematic:

- AD5592R CH0 (DAC) drives the base through a 49.9 kohm resistor.
- AD5592R CH2 (DAC + ADC) drives the collector through a 49.9 ohm sense
  resistor.
- AD5592R CH1 (ADC) senses the collector voltage after the sense resistor.

Collector current is computed as the voltage drop across the sense
resistor divided by its resistance; collector voltage is read directly.
Rsense and Rbase default to the schematic values and can be changed with
:kconfig:option:`CONFIG_CURVE_TRACER_RSENSE_MOHM` and
:kconfig:option:`CONFIG_CURVE_TRACER_RBASE_OHM` (the no-OS example uses
47 ohm / 47 kohm instead).

Requirements
************

This sample needs a board with a Feather-compatible SPI connector and the
:ref:`adi_lsmspg` shield plugged in. It has been tested on
:zephyr:board:`max32666fthr`.

Building and Running
*********************

.. zephyr-app-commands::
   :zephyr-app: samples/shields/adi_lsmspg/curve_tracer
   :board: max32666fthr/max32666/cpu0
   :goals: build flash
   :compact:

Nothing runs automatically at boot. Connect to the console and type
``curvetrace`` to run one sweep:

.. code-block:: console

   uart:~$ curvetrace

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

   ===== AD5592R Curve Trace Complete =====

   Sweep took 3376 ms
   uart:~$
