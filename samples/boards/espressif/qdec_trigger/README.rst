.. zephyr:code-sample:: esp32-qdec-trigger
   :name: ESP32 qdec per-unit threshold trigger
   :relevant-api: sensor_interface

   Install per-unit threshold callbacks on the ESP32 PCNT driver.

Overview
********

Each PCNT unit has two programmable threshold watchpoints and raises
an interrupt when the counter reaches one of them. The Zephyr
:c:struct:`sensor_trigger` has no ``chan_idx`` field, so the driver
lets a caller address a specific unit through the legacy
:c:macro:`SENSOR_CHAN_PCNT_ESP32_UNIT` helper from
``<zephyr/drivers/sensor/pcnt_esp32.h>``. Unit 0 may alternatively be
addressed with :c:macro:`SENSOR_CHAN_ROTATION` or
:c:macro:`SENSOR_CHAN_ENCODER_COUNT`.

The sample sets an upper threshold on unit 0 and a lower threshold on
unit 1 and increments a per-unit counter every time the watchpoint
fires. The running hit counts are printed once per second.

This sample is ESP32-specific because it uses the
``SENSOR_CHAN_PCNT_ESP32_UNIT(n)`` helper. A portable equivalent will
only be possible once Zephyr's sensor trigger API carries a channel
index (see the follow-up RFC in the PR description).

Requirements
************

Any Espressif SoC with a PCNT peripheral. Connect two pulse sources
to the pins wired in the relevant ``boards/<board>.overlay`` file.
Without a pulse source the counters never reach the thresholds and
no callbacks fire.

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/boards/espressif/qdec_trigger
   :board: esp32s3_devkitc/esp32s3/procpu
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

   *** Booting Zephyr OS build ... ***
   unit0 upper_thresh=100: rc=0
   unit1 lower_thresh=-100: rc=0
   triggers armed: rc=0
   hits unit0=0 unit1=0
   hits unit0=1 unit1=0
   hits unit0=1 unit1=1
