.. zephyr:code-sample:: tach_xec
   :name: Microchip XEC tachometer
   :relevant-api: sensor_interface

   Measure fan speed with the Microchip XEC TACH block.

Overview
********

This sample reads :c:enumerator:`SENSOR_CHAN_RPM` once per second from every
enabled :dtcompatible:`microchip,xec-tach` node and prints the result. The nodes
are enumerated at build time, so a board overlay that enables two tachometers
gets two lines of output per second without any change to the application.

The TACH block measures how many clocks of a nominally 100 kHz reference elapse
while a configured number of tachometer edges arrive, so the driver needs to know
three things to turn that count into RPM. All come from devicetree:

* ``tach-edges`` - the number of edges the hardware counts over, one of 2, 3, 5
  or 9. This sets the width of the measurement window and therefore the
  trade-off between resolution and update rate.
* ``pulses-per-round`` - the number of tachometer periods the fan produces per
  revolution. This is a property of the fan, not of the SoC; two is by far the
  most common value for a DC brushless fan.
* ``clock-frequency`` - the frequency in Hz of the clock the hardware counts. It
  defaults to the nominal 100 kHz, which is the 48 MHz PLL divided by 480, and
  that PLL is only specified to 46.56 to 49.44 MHz when it is referenced to the
  internal silicon oscillator. The driver takes the value as exact, so a board
  that has measured the divided clock on the part can set it and cancel what is
  otherwise a flat multiplicative bias on every reading.

The sample prints all three values at start-up so that the reported RPM can be
checked against the configuration that produced it.

Requirements
************

A fan whose tachometer output is wired to a TACH input of the SoC. Without one
the TACH counter saturates, the driver reports the fan as stopped, and the
sample prints ``fan stopped``.

Overlays are provided for the following boards, all of which route GPIO050 to
the TACH0 input:

* :zephyr:board:`mec15xxevb_assy6853`
* :zephyr:board:`mec172xevb_assy6906`
* :zephyr:board:`mec_assy6941` (all MEC1653B, MEC174x and MEC175x variants)

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/tach_xec
   :board: mec_assy6941/mec1753_qsz
   :goals: build flash
   :compact:

Sample Output
*************

.. code-block:: console

   *** Booting Zephyr OS build v4.3.0 ***
   Microchip XEC tachometer sample
   tach@40006000: 5 TACH edges per measurement, 2 TACH periods per revolution,
   100000 Hz counting clock
   tach@40006000: 3245.312500 RPM
   tach@40006000: 3244.187500 RPM
   tach@40006000: 3245.312500 RPM

With no fan connected:

.. code-block:: console

   *** Booting Zephyr OS build v4.3.0 ***
   Microchip XEC tachometer sample
   tach@40006000: 5 TACH edges per measurement, 2 TACH periods per revolution,
   100000 Hz counting clock
   tach@40006000: fan stopped
   tach@40006000: fan stopped
