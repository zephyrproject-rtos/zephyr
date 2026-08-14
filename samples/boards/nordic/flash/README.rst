.. zephyr:code-sample:: nordic-flash
   :name: Nordic flash write during Bluetooth LE activity
   :relevant-api: flash_interface bt_gap bluetooth

   Write to flash while the Bluetooth LE radio is active.

Overview
********

This sample exercises on-chip flash writes on Nordic nRF51, nRF52, and nRF54
series parts while the Bluetooth LE radio is active. It starts a connectable
advertiser and then performs a series of :c:func:`flash_area_write` calls to
the ``storage_partition``, logging the return code and duration of each write.

In parallel, the sample runs active scanning and prints every received
advertising report, increasing radio contention during the flash writes.
Scanning is enabled by :kconfig:option:`CONFIG_BT_OBSERVER` (on by default) and
can be turned off at build time to build the sample without the scanner. Both
advertising and scanning are stopped once the write loop completes.

The flash driver serializes flash writes with the radio using the flash
synchronization ticker. If the flash write slot is too short, a
``flash_area_write`` can time out with ``-ETIMEDOUT`` after the flash timeout
elapses, which breaks Bluetooth bond persistence and any settings write
performed while the radio is active.

To build without the scanner, disable the option at build time, for example::

   west build -b nrf54l15dk/nrf54l15/cpuapp \
     samples/boards/nordic/flash -- -DCONFIG_BT_OBSERVER=n

Requirements
************

A Nordic nRF51, nRF52, or nRF54 series board with an on-chip flash controller,
a ``storage_partition`` fixed partition, and a Bluetooth LE Controller, for
example:

- :zephyr:board:`nrf51dk`
- :zephyr:board:`nrf52840dk`
- :zephyr:board:`nrf54l15dk`

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nordic/flash
   :board: nrf54l15dk/nrf54l15/cpuapp
   :goals: build flash
   :compact:

After flashing, the board advertises as ``Zephyr Flash sample`` and writes to
flash while advertising. Example output on success::

   *** Booting Zephyr OS build ... ***
   Bluetooth LE advertising, writing to flash while radio is active
   iter 0 err=0 (2 ms)
   iter 1 err=0 (2 ms)
   ...
   Successfully completed flash write while radio was active
