.. _interrupt_test:

Espressif interrupt test
########################

Overview
********

The nested-ISR suite verifies that the ESP32, ESP32-S2 and ESP32-S3 platforms correctly support nested interrupts, where higher-priority interrupts can preempt lower-priority ones.

Three independent hardware timers are configured.

Each timer is assigned an ISR (Interrupt Service Routine) with a priority one level higher than the previous timer.

* Timer 1: lowest priority ISR
* Timer 2: medium priority ISR
* Timer 3: highest priority ISR

Execution Flow
--------------

* Timer 1 ISR triggers first. - It simulates work (waits briefly).
* Before completing, it triggers Timer 2 interrupt.
* Timer 2 ISR preempts Timer 1 ISR (because of higher priority). - It also simulates work.
* It triggers Timer 3 interrupt.
* Timer 3 ISR preempts Timer 2 ISR (highest priority). - It performs its work and completes execution.
* After Timer 3 ISR completes, control returns to Timer 2 ISR, which then completes.
* Finally, execution resumes and completes Timer 1 ISR.

Success Criteria
----------------

Each ISR execution is verified by setting a unique "magic value". The test is successful if:

* Timer 1 ISR sets its magic value.
* Timer 2 ISR sets its magic value.
* Timer 3 ISR sets its magic value.

This confirms that ISR priorities are respected and preemption works correctly.

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
