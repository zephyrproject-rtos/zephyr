.. zephyr:code-sample:: usb-host-uac2
   :name: USB Host UAC2 Audio
   :relevant-api: usb_host_core_api

   Play and record audio from a USB audio device.

Overview
********

This sample demonstrates how to use the USB Host UAC2 (USB Audio Class 2.0) driver
to play and record audio with a USB audio device connected to a Zephyr device acting
as a USB host.

Upon connection, the USB audio device is detected and configured automatically.
The sample plays a 1000 Hz sine wave tone and can simultaneously record audio data.

Requirements
************

This sample uses the USB host stack and requires the USB host controller driver.

A USB audio device supporting UAC2 (USB Audio Class 2.0) specification is required.
The device can be:

- USB headset (playback and capture)
- USB microphone (capture only)
- USB speakers (playback only)
- USB audio interface/sound card

Building and Running
********************

The sample can be built and flashed as follows:

.. tabs::

   .. group-tab:: NXP RW612

      .. zephyr-app-commands::
         :zephyr-app: samples/subsys/usb/host_uac2
         :board: rd_rw612_bga/rw612
         :goals: build flash
         :compact:

   .. group-tab:: NXP i.MX RT700

      .. zephyr-app-commands::
         :zephyr-app: samples/subsys/usb/host_uac2
         :board: mimxrt700_evk/mimxrt798s/cm33_cpu0
         :goals: build flash
         :compact:

The device is expected to detect the USB audio device automatically when connected.

Sample Output
=============

When a USB audio device is connected, you should see:

.. code-block:: console

   *** Booting Zephyr OS build v4.4.0-2682-gf1190db3cdea ***
   <inf> main: ===========================================
   <inf> main: USB Audio Class 2.0 Host Sample Application
   <inf> main: ===========================================
   <inf> main: USB host audio device is ready
   <inf> main: Waiting for USB audio device to connect...
   <inf> main: Please connect a USB Audio Class 2.0 device
   <inf> usbh_dev: New device with address 1 state 2
   <inf> usbh_dev: Configuration 1 bNumInterfaces 4
   <inf> usbh_uac2: UAC2 device connected
   <inf> usbh_dev: Set Interfaces 0, alternate 0 -> 0
   <inf> usbh_dev: Set Interfaces 1, alternate 0 -> 0
   <inf> usbh_dev: Set Interfaces 2, alternate 0 -> 0
   <inf> usbh_uac2: Device configured successfully
   <inf> usbh_uac2: UAC2 device (addr=1) initialization completed
   <inf> main: Audio device connected!

Viewing Device Information
==========================

The sample displays comprehensive device information:

.. code-block:: console

   <inf> main: === Audio Device Information ===
   <inf> main: Device Capabilities:
   <inf> main:   Type: Full-Duplex (Capture + Playback)
   <inf> main: Capture Format:
   <inf> main:   Sample Rate: 48000 Hz
   <inf> main:   Channels: 2
   <inf> main:   Bit Resolution: 16 bits
   <inf> main:   Sample Rate Range (Capture):
   <inf> main:     48000 Hz (fixed)
   <inf> main:   Volume control not supported (Capture)
   <inf> main: Playback Format:
   <inf> main:   Sample Rate: 44100 Hz
   <inf> main:   Channels: 2
   <inf> main:   Bit Resolution: 16 bits
   <inf> main:   Sample Rate Range (Playback):
   <inf> main:     5 supported rate(s):
   <inf> main:       [0] 44100 Hz
   <inf> main:       [1] 48000 Hz
   <inf> main:       [2] 96000 Hz
   <inf> main:       [3] 192000 Hz
   <inf> main:       [4] 384000 Hz
   <inf> main:   Volume control not supported (Playback)

Testing Sample Rate Support
============================

The sample tests which sample rates are actually supported:

.. code-block:: console

   <inf> main: === Testing Sample Rate Support ===
   <inf> main: Playback Sample Rates:
   <inf> main:   8000 Hz: Not supported
   <inf> main:   16000 Hz: Not supported
   <inf> main:   44100 Hz: Supported
   <inf> main:   48000 Hz: Supported
   <inf> main:   96000 Hz: Supported
   <inf> main: Capture Sample Rates:
   <inf> main:   8000 Hz: Not supported
   <inf> main:   16000 Hz: Not supported
   <inf> main:   44100 Hz: Not supported
   <inf> main:   48000 Hz: Supported
   <inf> main:   96000 Hz: Not supported

Configuring Audio Format
=========================

The sample configures the audio format to 48000 Hz:

.. code-block:: console

   <inf> main: === Configuring Audio Format ===
   <inf> usbh_uac2: Playback sample rate set to 48000 Hz
   <inf> main:   Playback sample rate: 48000 Hz
   <inf> main: Playback volume control not supported, skipping
   <inf> usbh_uac2: Playback mute set to 0
   <inf> main: Playback audio unmuted
   <inf> usbh_uac2: Capture sample rate set to 48000 Hz
   <inf> main:   Capture sample rate: 48000 Hz
   <inf> main: Capture volume control not supported, skipping
   <inf> usbh_uac2: Capture mute set to 0
   <inf> main: Capture audio unmuted

Audio Playback and Capture
===========================

The sample starts playing a 1000 Hz sine wave and recording audio:

.. code-block:: console

   <inf> main: === Starting Audio Playback ===
   <inf> usbh_dev: Set Interfaces 2, alternate 0 -> 1
   <inf> main: Playback started - playing 1000 Hz sine wave continuously
   <inf> main: === Starting Audio Capture ===
   <inf> usbh_dev: Set Interfaces 1, alternate 0 -> 1
   <inf> main: Capture started - recording audio data

Session Monitoring
==================

The sample logs session status every 10 seconds:

.. code-block:: console

   <inf> main: Playback: 10 seconds elapsed
   <inf> main: Capture: 10 seconds elapsed
   <inf> main: Playback: 20 seconds elapsed
   <inf> main: Capture: 20 seconds elapsed
   <inf> main: Playback: 30 seconds elapsed
   <inf> main: Capture: 30 seconds elapsed

Device Types
============

The sample automatically detects the device type:

**Full-Duplex Device (Headset)**

.. code-block:: console

   <inf> main:   Type: Full-Duplex (Capture + Playback)

**Playback Only (Speakers)**

.. code-block:: console

   <inf> main:   Type: Playback Only (Headphones/Speakers)

**Capture Only (Microphone)**

.. code-block:: console

   <inf> main:   Type: Capture Only (Microphone/Line-In)

Configuration Options
*********************

The sample can be configured through ``prj.conf``:

Audio Configuration
===================

- :kconfig:option:`CONFIG_USBH_UAC2_CLASS` - Enable UAC2 host support
- :kconfig:option:`CONFIG_USBH_UAC2_INSTANCES_COUNT` - Number of UAC2 devices (default: 1)
- :kconfig:option:`CONFIG_USBH_UAC2_STREAM_IN_CONCURRENT_TRANSFERS` - Concurrent IN transfers (default: 4)
- :kconfig:option:`CONFIG_USBH_UAC2_STREAM_OUT_CONCURRENT_TRANSFERS` - Concurrent OUT transfers (default: 4)

Memory Configuration
====================

- :kconfig:option:`CONFIG_HEAP_MEM_POOL_SIZE` - General heap size (default: 8192)
- :kconfig:option:`CONFIG_MAIN_STACK_SIZE` - Main thread stack size (default: 4096)
- :kconfig:option:`CONFIG_USBH_USB_DEVICE_HEAP` - USB device heap size (default: 4096)

USB Transfer Configuration
===========================

- :kconfig:option:`CONFIG_UHC_XFER_COUNT` - Max transfer requests (default: 16)
- :kconfig:option:`CONFIG_UHC_BUF_COUNT` - Transfer buffer count (default: 16)
- :kconfig:option:`CONFIG_UHC_BUF_POOL_SIZE` - Transfer buffer pool size (default: 4096)

Logging Configuration
=====================

- :kconfig:option:`CONFIG_LOG` - Enable logging
- :kconfig:option:`CONFIG_LOG_BUFFER_SIZE` - Log buffer size (default: 4096)
- :kconfig:option:`CONFIG_USBH_UAC2_LOG_LEVEL_INF` - UAC2 driver log level

Troubleshooting
***************

Device Not Detected
===================

If the USB audio device is not detected:

1. Verify the device is UAC2 compatible (not UAC1)
2. Check USB cable and connection
3. Try a different USB port
4. Check device power requirements

.. code-block:: console

   <err> usbh_dev: Device enumeration failed

Audio Playback Issues
=====================

If you don't hear audio:

1. Check device volume (if supported)
2. Verify mute is off
3. Check sample rate compatibility
4. Try a different audio device

.. code-block:: console

   <err> usbh_uac2: Failed to start audio OUT stream

Audio Capture Issues
====================

If audio recording doesn't work:

1. Check microphone permissions
2. Verify capture is supported
3. Check sample rate settings
4. Verify input source selection

.. code-block:: console

   <err> usbh_uac2: Failed to start audio IN stream

Memory Allocation Failures
===========================

If you see memory allocation errors:

- :kconfig:option:`CONFIG_HEAP_MEM_POOL_SIZE` - Increase to 16384 or higher
- :kconfig:option:`CONFIG_USBH_USB_DEVICE_HEAP` - Increase to 8192 or higher
- :kconfig:option:`CONFIG_UHC_BUF_POOL_SIZE` - Increase to 8192 or higher

.. code-block:: console

   <err> usbh_uac2: Failed to allocate buffer

Audio Dropouts or Glitches
===========================

If audio has dropouts or glitches:

- :kconfig:option:`CONFIG_USBH_UAC2_STREAM_OUT_CONCURRENT_TRANSFERS` - Increase to 8
- :kconfig:option:`CONFIG_USBH_UAC2_STREAM_IN_CONCURRENT_TRANSFERS` - Increase to 8
- :kconfig:option:`CONFIG_UHC_BUF_COUNT` - Increase to 32
- :kconfig:option:`CONFIG_UHC_BUF_POOL_SIZE` - Increase to 8192

Performance Issues
==================

If audio performance is poor:

- Reduce logging level: :kconfig:option:`CONFIG_USBH_UAC2_LOG_LEVEL_WRN`
- Enable optimizations: :kconfig:option:`CONFIG_SPEED_OPTIMIZATIONS`
- Disable debug: :kconfig:option:`CONFIG_NO_OPTIMIZATIONS` = n

Sample Rate Not Supported
==========================

If the desired sample rate is not supported:

1. Check device capabilities in the log
2. Use a supported rate from the range
3. Try 48000 Hz (most common)

.. code-block:: console

   <inf> main:   Sample Rate Range (Playback):
   <inf> main:     5 supported rate(s):
   <inf> main:       [0] 44100 Hz
   <inf> main:       [1] 48000 Hz

Volume Control Not Available
=============================

Some USB audio devices don't support volume control via USB:

.. code-block:: console

   <inf> main:   Volume control not supported (Playback)

In this case, use the device's physical volume controls or system mixer.

Known Limitations
*****************

- Only UAC2 devices are supported (UAC1 devices will not work)
- Maximum 8 concurrent UAC2 devices (configurable)
- Only Format Type I is supported (most USB headsets/speakers/microphones)
