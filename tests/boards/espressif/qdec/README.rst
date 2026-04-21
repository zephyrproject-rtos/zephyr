.. _esp32_qdec_test:

Espressif QDEC self-loopback test
#################################

Overview
********

This test exercises the ESP32 PCNT (pulse counter / quadrature decoder)
driver end-to-end on real hardware without requiring any external
wiring. Each PCNT signal input is configured in the board overlay to
read a pad that the test also drives as a software output (configured
as ``GPIO_INPUT | GPIO_OUTPUT_INACTIVE`` so the input buffer stays
live). The ESP32 GPIO matrix feeds the software-driven pad level back
into PCNT, so the test can synthesise every edge it needs from
``gpio_pin_set_dt`` calls.

The suite covers:

* baseline multi-unit read through the Read and Decode API;
* per-unit ``SENSOR_ATTR_OFFSET`` via the
  ``SENSOR_CHAN_PCNT_ESP32_UNIT(n)`` helper and attr get roundtrip;
* the private ``SENSOR_ATTR_PCNT_ESP32_RESET`` attribute actually
  clearing the hardware counter;
* legacy :c:func:`sensor_channel_get` backward compatibility;
* rejection of unsupported channels;
* four units counting concurrently at independent rates;
* upper and lower threshold triggers fired per unit;
* :c:macro:`SENSOR_CHAN_ROTATION` degrees conversion with
  ``counts-per-revolution``;
* ``high-limit`` watchpoint auto-resetting the counter and firing a
  trigger;
* ``zero-cross-event`` enable not breaking normal counting;
* quadrature decoding with CH0+CH1 configured for opposite direction.

Supported Boards
****************

* ``esp32_devkitc/esp32/procpu``
* ``esp32s2_saola``
* ``esp32s3_devkitc/esp32s3/procpu``
* ``esp32c5_devkitc/esp32c5/hpcore``
* ``esp32c6_devkitc/esp32c6/hpcore``
* ``esp32h2_devkitm/esp32h2``

Building and Running
********************

Make sure the target is connected over USB.

.. zephyr-app-commands::
   :zephyr-app: tests/boards/espressif/qdec
   :board: esp32s3_devkitc/esp32s3/procpu
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

   *** Booting Zephyr OS build ... ***
   Running TESTSUITE qdec_selfloop
    PASS - test_01_baseline in 0.001 seconds
    PASS - test_02_per_unit_offset in 0.001 seconds
    PASS - test_03_attr_get_roundtrip in 0.001 seconds
    PASS - test_04_reset_clears_counter in 0.031 seconds
    PASS - test_05_legacy_channel_get in 0.001 seconds
    PASS - test_06_unsupported_channel in 0.001 seconds
    PASS - test_07_independent_counts in 0.240 seconds
    PASS - test_08_upper_thresh_trigger in 0.072 seconds
    PASS - test_10_lower_thresh in 0.047 seconds
    PASS - test_11_four_units_concurrent in 0.060 seconds
    PASS - test_12_rotation_degrees in 0.401 seconds
    PASS - test_13_high_limit_auto_reset in 0.162 seconds
    PASS - test_14_zero_cross_unit_counts in 0.048 seconds
    PASS - test_15_quadrature_direction in 0.102 seconds
   TESTSUITE qdec_selfloop succeeded
   ------ TESTSUITE SUMMARY START ------
   SUITE PASS - 100.00% [qdec_selfloop]: pass = 14, fail = 0, skip = 0, total = 14
   ------ TESTSUITE SUMMARY END ------
   PROJECT EXECUTION SUCCESSFUL
