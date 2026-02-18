ES7210 Custom Driver Sample
###########################

Overview
********

This sample demonstrates basic register-level operations for the
``everest,es7210`` custom codec driver:

- WHOAMI register read
- I2S interface register programming
- per-channel ADC gain programming
- I2S RX capture with per-block peak/RMS output

Validated Default Profile
*************************

The driver now defaults to a profile validated on M5Stack CoreS3:

- ``SDP_INTERFACE1 (0x11) = 0x80`` (32-bit I2S slot)
- ``SDP_INTERFACE2 (0x12) = 0x00`` (non-TDM)

You can override these defaults in devicetree with:

- ``sdp-interface1``
- ``sdp-interface2``

Build and Run
*************

Example for M5Stack CoreS3 PROCPU:

.. code-block:: console

   west build -b m5stack_cores3/esp32s3/procpu  zephyr/samples/drivers/audio/es7210_custom
   west flash
