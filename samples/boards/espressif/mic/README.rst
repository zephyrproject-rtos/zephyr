.. zephyr:code-sample:: esp32s3_box3_audio_demo
   :name: ESP32-S3-BOX-3 Audio Demo
   :relevant-api: audio_interface i2s_interface gpio_interface i2c_interface fs_interface

   Comprehensive audio demonstration for ESP32-S3-BOX-3 featuring ES8311 speaker and ES7210 microphone with LittleFS recording.

Overview
********

This sample demonstrates the complete audio capabilities of the ESP32-S3-BOX-3 board:

* **ES8311 Audio Codec** - High-quality speaker output with volume control
* **ES7210 4-Channel ADC** - Professional microphone input with programmable gain
* **I2S Audio Interface** - Full-duplex audio streaming at 16kHz, 16-bit stereo
* **LittleFS Storage** - Audio recording to flash memory with 5MB partition
* **Shell Interface** - Interactive commands for file management and audio extraction
* **GPIO Control** - Power amplifier and mute control

The application performs a complete audio system test:

1. **Audio System Initialization**
   - Mount LittleFS filesystem with 5MB storage partition
   - Initialize ES8311 codec for speaker output
   - Initialize ES7210 ADC for microphone input
   - Configure I2S for full-duplex operation at 16kHz

2. **Speaker Testing**
   - Play silence (system test)
   - Generate and play 1kHz sine wave
   - Generate and play musical melody (A4-C5-E5 progression)
   - Optional: Play Canon in D (if audio data included)

3. **Microphone Recording**
   - Record 5 seconds of microphone input
   - Save as raw 16-bit stereo PCM to ``/lfs/audio_000.raw``
   - Display recording statistics (bytes recorded, peak amplitude)

4. **Interactive Shell**
   - File system commands (``fs ls /lfs``)
   - Binary file extraction (``bincat <filename>``)
   - Compatible with ``ultra_fast_extract.py`` for audio retrieval

Hardware Features
*****************

**Audio Codecs:**
- ES8311: Low-power stereo audio codec with integrated headphone amplifier
- ES7210: 4-channel ADC with built-in microphone bias and programmable gain

**Audio Specifications:**
- Sample Rate: 16kHz (optimized for voice recording)
- Bit Depth: 16-bit
- Channels: 2 (stereo)
- I2S Format: Standard I2S with ESP32 as master

**Storage:**
- LittleFS filesystem on 5MB flash partition
- Supports multiple audio recordings
- Wear leveling and power-loss protection

Building and Running
********************

This application can be built and executed on the ESP32-S3-BOX-3 board:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/espressif/mic
   :board: esp32s3_box3/esp32s3/procpu
   :goals: build flash
   :compact:

The application includes a device tree overlay (``esp32s3_box3_procpu.overlay``) that:
- Increases storage partition from 192KB to 5MB
- Optimizes partition layout for audio recording applications

Configuration
*************

Key configuration options in ``prj.conf``:

.. code-block:: kconfig

   # Audio subsystem
   CONFIG_AUDIO=y
   CONFIG_I2S=y
   CONFIG_ES8311=y
   CONFIG_ES7210=y
   
   # File system for recording
   CONFIG_FILE_SYSTEM=y
   CONFIG_FILE_SYSTEM_LITTLEFS=y
   
   # Shell for interactive commands
   CONFIG_SHELL=y

Shell Commands
**************

The application provides interactive shell commands:

.. code-block:: console

   uart:~$ fs ls /lfs
        187392 audio_000.raw
   
   uart:~$ bincat /lfs/audio_000.raw
   BINSTART
   <hex data lines>
   BINEND

Audio File Extraction
*********************

Use the included ``ultra_fast_extract.py`` script to extract recorded audio:

.. code-block:: bash

   # Extract audio file from device
   python3 ultra_fast_extract.py 


Sample Output
*************

.. code-block:: console

   [00:00:00.078,000] <inf> esp32s3_box3_demo: === ESP32-S3-BOX-3 Audio Demo (ES8311 + ES7210) ===
   [00:00:00.079,000] <inf> esp32s3_box3_demo: LittleFS mounted at /lfs
   [00:00:00.079,000] <inf> esp32s3_box3_demo: Storage: 5120 KB total, 5120 KB free
   [00:00:00.079,000] <inf> esp32s3_box3_demo: PA enabled (GPIO46 HIGH)
   [00:00:00.079,000] <inf> esp32s3_box3_demo: Audio unmuted
   [00:00:00.346,000] <inf> esp32s3_box3_demo: ES8311 ready
   [00:00:00.360,000] <inf> esp32s3_box3_demo: ES7210 ready
   [00:00:00.361,000] <inf> esp32s3_box3_demo: I2S configured (TX + RX, 16000 Hz, 16-bit stereo)
   [00:00:00.361,000] <inf> esp32s3_box3_demo: Audio system ready — ES8311 speaker + ES7210 microphone
   [00:00:03.466,000] <inf> esp32s3_box3_demo: Playing 1 kHz sine wave...
   [00:00:07.572,000] <inf> esp32s3_box3_demo: Playing melody (A4-C5-E5)...
   [00:01:15.986,000] <inf> esp32s3_box3_demo: Recording 5000 ms to /lfs/audio_000.raw ...
   [00:01:20.007,000] <inf> esp32s3_box3_demo: I2S TX+RX started — recording...
   [00:01:21.043,000] <inf> esp32s3_box3_demo: Recorded 65536 bytes, peak 1300
   [00:01:22.067,000] <inf> esp32s3_box3_demo: Recorded 131072 bytes, peak 2120
   [00:01:22.958,000] <inf> esp32s3_box3_demo: Recording done: 183 blocks, 187392 bytes, peak 2120
   [00:01:22.958,000] <inf> esp32s3_box3_demo: Saved: /lfs/audio_000.raw (187392 bytes)
   [00:01:22.958,000] <inf> esp32s3_box3_demo: Demo completed successfully. Application will now idle.
   [00:01:22.958,000] <inf> esp32s3_box3_demo: Available shell commands:
   [00:01:22.958,000] <inf> esp32s3_box3_demo:   fs ls /lfs          - List recorded files
   [00:01:22.958,000] <inf> esp32s3_box3_demo:   bincat <filename>   - Extract file as hex for ultra_fast_extract.py

