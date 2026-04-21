.. zephyr:code-sample:: qdec_multi
   :name: Multi-unit quadrature decoder
   :relevant-api: sensor_interface

   Read several quadrature encoders from a single sensor device using
   the Read-and-Decode API.

Overview
********

Some quadrature decoder peripherals expose more than one counting unit
behind a single hardware block (for example the ESP32 PCNT has 4-8
units per block). The legacy :c:func:`sensor_channel_get` API can only
return one channel of a given type per device, so multi-unit access
must go through the Read-and-Decode API: :c:func:`sensor_read` fills
an encoded buffer with every unit's value, and the driver's
:c:struct:`sensor_decoder_api` returns the per-unit reading by matching
:c:member:`sensor_chan_spec.chan_idx` against the DT ``reg`` of each
``unit@N`` child.

The sample expects the board overlay to:

* provide a ``qdec0`` alias pointing to the multi-unit decoder node, and
* declare every unit as a ``unit@N`` child with ``reg = <N>``.

It then enumerates every child of ``qdec0`` and prints the rotation
reading of each one every second. The reading is returned as a q31
angle in degrees when the unit's DT node sets ``counts-per-revolution``,
or as a raw count otherwise.

Requirements
************

Any Zephyr-supported quadrature decoder driver that exposes multiple
units as DT children and implements a :c:struct:`sensor_decoder_api`.
At the time of writing the ESP32 PCNT driver is the only upstream
driver that follows this pattern. Overlays for ESP32, ESP32-S2,
ESP32-S3, ESP32-C5, ESP32-C6 and ESP32-H2 are provided under
``boards/``.

Connect two rotary encoders (or any source of quadrature pulses) to
the pins listed in the relevant ``boards/<board>.overlay`` file, or
replace the overlay with your own wiring.

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/qdec_multi
   :board: esp32s3_devkitc/esp32s3/procpu
   :goals: build flash
   :compact:

Sample Output
=============

With nothing connected to the decoder input pins the counters sit at
zero:

.. code-block:: console

   *** Booting Zephyr OS build ... ***
   qdec multi-unit sample: 2 units
   unit 0: q31=0 shift=9
   unit 1: q31=0 shift=9
   ...

When rotating an encoder on unit 0 its q31 value grows (divide by
``2^(31 - shift)`` to get degrees):

.. code-block:: console

   unit 0: q31=377487360 shift=9
   unit 1: q31=0 shift=9
