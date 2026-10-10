.. zephyr:code-sample:: admt4000
   :name: ADMT4000 True Power-On Multiturn Sensor
   :relevant-api: sensor_interface

   Get angle, turn count, temperature, cosine, sine, and radius data from an ADMT4000 sensor.

Description
***********

This sample application periodically (1 Hz) reads sensor data from the
ADMT4000 True Power-On Multiturn Sensor over SPI and prints the results
to the console.

The following measurements are displayed each cycle:

- **Angle** — absolute angle in degrees
- **Turns** — multiturn count (quarter-turn resolution)
- **Temperature** — die temperature in degrees Celsius
- **Cosine** — normalized cosine component, in the range [-1, 1)
- **Sine** — normalized sine component, in the range [-1, 1)
- **Radius** — AMR signal amplitude (magnitude of the sine/cosine vector), in mV/V

On startup, the sample also reads and displays the current conversion mode
and angle filter settings.

References
**********

- ADMT4000: https://www.analog.com/ADMT4000

Wiring
******

This sample uses the ADMT4000 sensor controlled via the SPI interface.
Connect Supply: **VDD**, **GND** and Interface: **SCLK**, **MOSI**,
**MISO**, **CS**.

The provided ``adi_sdp_k1`` overlay targets the EVAL-ADMT4000ARD1Z Arduino
shield, which plugs directly into the board's Arduino header, so no manual
wiring is required on that board.

Magnetic (coil) reset
*********************

The ADMT4000 GMR turn counter can be magnetically reset by pulsing the coil
drive line (``COIL_RS``). The driver exposes this through the
``SENSOR_ATTR_ADMT4000_COIL_RESET`` attribute, which pulses the pin defined by
the ``coil-rs-gpios`` devicetree property and then restarts the acquisition
sequence.

On the EVAL-ADMT4000ARD1Z, the coil driver is powered by two additional
evaluation-board control lines that are **not** part of the ADMT4000 device
itself: ``V_EN`` (supply enable) and ``SHDN_N`` (shutdown, active low). The
sample's ``adi_sdp_k1`` overlay brings these up at boot via GPIO hogs
(``V_EN`` high, ``SHDN_N`` released) so the coil is powered. On custom
hardware without these controls, simply omit the hogs; the coil reset still
toggles ``COIL_RS`` and restarts acquisition.

Building and Running
********************

This project outputs sensor data to the console. It requires an ADMT4000
sensor connected via SPI. A board-specific devicetree overlay is needed
to define the SPI bus and chip select configuration.

The sample ships with an overlay for the following board (using the
EVAL-ADMT4000ARD1Z Arduino shield):

- ``adi_sdp_k1``

By default the sample uses the asynchronous (RTIO) read path. Build and flash
it for the Analog Devices SDP-K1:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/admt4000
   :board: adi_sdp_k1
   :goals: build flash

To run on a different board, add a matching overlay under ``boards/`` that
describes the ADMT4000 SPI connection and (optionally) the ``coil-rs-gpios``
line.

Read modes
==========

The sample supports two sensor read paths, selected at build time:

- **Asynchronous (RTIO)** — the default (``prj.conf``, with
  ``CONFIG_SENSOR_ASYNC_API=y``). Uses :c:func:`sensor_read` and the sensor
  decoder API.
- **Polling (synchronous)** — selected with ``prj_polling.conf`` (async
  disabled). Uses :c:func:`sensor_sample_fetch` and
  :c:func:`sensor_channel_get`.

To build the polling variant, point ``CONF_FILE`` at ``prj_polling.conf``:

.. zephyr-app-commands::
   :zephyr-app: samples/sensor/admt4000
   :board: adi_sdp_k1
   :gen-args: -DCONF_FILE=prj_polling.conf
   :goals: build flash

Both paths read and print the same channels; the banner line reports which
mode is active (``Mode: RTIO (async)`` or ``Mode: Polling (sync)``).

Sample Output
=============

.. code-block:: console

   *** Booting Zephyr OS ***
   ADMT4000 sample application
   Device: admt4000@0
   Mode: RTIO (async)

   Conversion mode: continuous
   Angle filter: disabled
   Magnetic (coil) reset: done

   Angle: 45.000000 deg  Turns: 3.00  Temp: 25.000000 C  Cos: 0.707092  Sin: 0.707092  Radius: 1.234500 mV/V
   Angle: 45.000000 deg  Turns: 3.00  Temp: 25.000000 C  Cos: 0.707092  Sin: 0.707092  Radius: 1.234500 mV/V
   Angle: 46.000000 deg  Turns: 3.00  Temp: 25.061000 C  Cos: 0.705627  Sin: 0.708557  Radius: 1.234700 mV/V

   <repeats endlessly>
