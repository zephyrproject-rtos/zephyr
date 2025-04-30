MAX44009 Ambient Light Sensor Sample
====================================

Overview
--------

This sample demonstrates how to obtain ambient light measurements (in lux)
from a **MAX44009** digital light sensor over I²C and print them to the console
in a loop.

Requirements
------------

* MAX44009 sensor connected to a supported board’s I²C bus
* A Zephyr-supported board with I²C and console support
  (e.g., ``qemu_cortex_m3`` for build verification only)

Building and Running
--------------------

.. code-block:: console

   # From the root of the Zephyr repository
   west build -b qemu_cortex_m3 samples/sensor/max44009
   west build -t run

Expected console output:

.. code-block:: console

   lux: 123.45
   lux: 122.97

For real hardware, adapt the board and devicetree overlay according to the
sensor’s I²C address (default: ``0x4A``).

Configuration
-------------

The MAX44009 driver is enabled via the Kconfig symbol
:kconfig:`CONFIG_MAX44009`, set in
:zephyr_file:`samples/sensor/max44009/prj.conf`.

References
----------

* `MAX44009 Datasheet (Analog Devices) <https://www.analog.com/media/en/technical-documentation/data-sheets/MAX44009.pdf>`__