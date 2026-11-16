.. _interrupt_test:

Espressif interrupt test
########################

Overview
********

The nested-ISR suite (``esp_interrupt``) checks that an interrupt on a higher Xtensa level
preempts the handler of a lower one on ESP32, ESP32-S2 and ESP32-S3.

Three timer group counters are used. ``timer0`` keeps the shared level-1 CPU line of the timer
groups from the SoC devicetree. The overlays in ``socs/`` make ``timer1`` and ``timer2`` dedicated
consumers of ``intc`` on CPU line 21 (level 2) and CPU line 27 (level 3), lines no other node uses.

Execution Flow
--------------

* The three alarms are set 1 ms apart and every handler busy-waits 3 ms.
* The ``timer0`` alarm fires first and its handler starts waiting.
* The ``timer1`` alarm fires while that handler runs; its line is level 2, so it preempts.
* The ``timer2`` alarm fires while both run; its line is level 3, so it preempts again.
* The handlers finish in reverse order.

Success Criteria
----------------

Every handler counts the handlers already running. A handler takes its token only when the
handlers of the lower levels are still in progress and their tokens are set:

* ``timer0`` at depth 0, ``timer1`` at depth 1 with the first token set, ``timer2`` at depth 2
  with the first two tokens set.

A cycle passes when all three tokens were taken; the test runs 1000 cycles and expects no
failure. Without nesting the handlers run one after another and ``timer1`` finds depth 0.

Supported Boards
****************

- esp32_devkitc/esp32/procpu
- esp32s2_devkitc
- esp32s3_devkitc/esp32s3/procpu
- esp32c6_devkitc/esp32c6/hpcore
- esp32h2_devkitm

The nested-ISR suite needs three timer group counters and runs on ESP32, ESP32-S2 and ESP32-S3
only.

Non-IRAM pause tests (``esp_noniram`` suite)
============================================

``esp_intr_noniram_disable()`` masks every CPU line not marked ``ESP_INTR_FLAG_IRAM`` in the
devicetree, as the flash driver does while the cache is off. The suite uses the
first enabled timer group counter, whose alarm interrupt is level-triggered:

* An alarm raised while paused does not fire, the IRQ still reads as enabled, and the alarm
  fires once on ``esp_intr_noniram_enable()``.
* ``irq_disable()`` during the pause is kept after the resume.
* ``irq_enable()`` during the pause waits for the resume.

Multi-level encoding tests (``mli_encoding`` suite)
===================================================

* A shared consumer is a level-2 IRQ: the level-2 field is its interrupt matrix source and the
  level-1 field is the intmux key line.
* ``uart0`` lacks ``ESP_INTR_FLAG_IRAM`` and ``systimer0`` has it.
* A level-3 consumer carries its input index, the source of its aggregator and the key line
  (``rtc_timer`` on ``rtc_l3_intc``; on ESP32-H2 ``gpio0`` on the catch-all input of
  ``gpio0_l3_intc`` and ``ana_cmpr0`` on input 0).

Multi-level dispatch tests (``mli_dispatch`` suite)
===================================================

* A level-2 IRQ uses table slot ``CONFIG_2ND_LVL_ISR_TBL_OFFSET`` plus its source.
* The counter ISR sits in its slot, and ``irq_enable()``, ``irq_disable()`` and
  ``irq_is_enabled()`` agree.
* On ESP32-H2 the GPIO ISR sits in the catch-all slot of the ``gpio0_l3_intc`` window and the
  aggregator ISR in its level-2 slot.

Building and Running
********************

Make sure you have the target connected over USB port.

.. code-block:: console

   west build -b <board> tests/boards/espressif/interrupt
   west flash && west espressif monitor

To run the test with twister, use the following command:

.. code-block:: console

   west twister -p <board> --device-testing --device-serial=/dev/ttyUSB0 -vv --flash-before -T tests/boards/espressif/interrupt

If the external 32K crystal is connect to pins 32K_XP and 32K_XN, the test can be run with ``external_xtal`` fixture enabled:

.. code-block:: console

   west twister -p esp32s3_devkitc --device-testing --device-serial=/dev/ttyUSB0 -vv --flash-before -T tests/boards/espressif/interrupt

Sample Output
=============

.. code-block:: console

   *** Booting Zephyr OS build v4.2.0-2885-g158729997aec ***
   Running TESTSUITE esp_interrupt
   ===================================================================
   START - test_nested_isr
    PASS - test_nested_isr in 10.101 seconds
   ===================================================================
   TESTSUITE esp_interrupt succeeded
   ------ TESTSUITE SUMMARY START ------
   SUITE PASS - 100.00% [esp_interrupt]: pass = 1, fail = 0, skip = 0, total = 1 duration = 10.101 seconds
   - PASS - [esp_interrupt.test_nested_isr] duration = 10.101 seconds
   ------ TESTSUITE SUMMARY END ------
   ===================================================================
   PROJECT EXECUTION SUCCESSFUL
