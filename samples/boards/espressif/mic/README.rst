.. zephyr:code-sample:: esp32s3_box3_audio_recording
   :name: ESP32-S3-BOX-3 Audio Recording Demo
   :relevant-api: audio_interface i2s_interface gpio_interface i2c_interface fs_interface

   Comprehensive audio demonstration for ESP32-S3-BOX-3 featuring ES8311 speaker and ES7210 microphone with LittleFS recording.

Overview
********

This sample demonstrates the complete audio capabilities of the ESP32-S3-BOX-3 board,
showcasing both audio playback and recording functionality. The application integrates
multiple audio codecs, file system storage, and interactive shell commands for a
comprehensive audio development platform.

**Key Features:**

* **ES8311 Audio Codec** - High-quality speaker output with 91% volume control
* **ES7210 4-Channel ADC** - Professional microphone input with programmable gain  
* **I2S Audio Interface** - Full-duplex audio streaming (48kHz playback, 16kHz recording)
* **LittleFS Storage** - Audio recording to flash memory with 5MB partition
* **Shell Interface** - Interactive commands for file management and audio extraction
* **GPIO Control** - Power amplifier and mute control
* **PCM Audio Playback** - Test audio file playback at 48kHz, 16-bit stereo

The application performs a complete audio system test:

1. **Audio System Initialization**
   - Mount LittleFS filesystem with 5MB storage partition
   - Initialize ES8311 codec for speaker output at 91% volume
   - Initialize ES7210 ADC for microphone input  
   - Configure I2S for full-duplex operation (48kHz TX, 16kHz RX)

2. **Audio Playback**
   - Play test.mp3 audio file converted to 48kHz PCM format
   - Demonstrate proper audio buffer management and streaming
   - Show playback progress indicators

3. **Microphone Recording**
   - Record 5 seconds of microphone input at 16kHz, 16-bit stereo
   - Save as raw PCM to ``/lfs/audio_000.raw``
   - Display recording statistics (bytes recorded, peak amplitude)

4. **Interactive Shell**
   - File system commands (``fs ls /lfs``)
   - Binary file extraction (``bincat <filename>``)
   - Compatible with ``ultra_fast_extract.py`` for audio retrieval

Requirements
************

* ESP32-S3-BOX-3 board with built-in ES8311 and ES7210 codecs
* Microphone for audio input (built-in or external)
* Speaker or headphones for audio output
* USB cable for programming and power

Wiring
******

The ESP32-S3-BOX-3 has built-in audio components that are pre-wired:

.. list-table::
   :header-rows: 1

   * - Component
     - ESP32S3 Pin
     - Description
   * - ES8311/ES7210 SDA
     - GPIO41
     - I2C data for codec control
   * - ES8311/ES7210 SCL
     - GPIO40
     - I2C clock for codec control
   * - I2S BCLK
     - GPIO17
     - Bit clock for audio data
   * - I2S LRCK
     - GPIO47
     - Left/Right clock (frame sync)
   * - I2S DOUT
     - GPIO15
     - Audio data output (to ES8311)
   * - I2S DIN
     - GPIO16
     - Audio data input (from ES7210)
   * - PA Control
     - GPIO46
     - Power amplifier enable
   * - Mute Control
     - GPIO1
     - Audio mute control

Building and Running
********************

Build and flash the sample for ESP32S3 Box3:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/espressif/mic
   :board: esp32s3_box3/esp32s3/procpu
   :goals: build flash monitor

The application includes a device tree overlay (``esp32s3_box3_procpu.overlay``) that:
- Increases storage partition from 192KB to 5MB
- Optimizes partition layout for audio recording applications

Sample Output
*************

The sample will initialize both audio codecs and perform audio playback and recording:

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
   [00:00:00.361,000] <inf> esp32s3_box3_demo: Playing test.mp3...
   [00:00:00.361,000] <inf> esp32s3_box3_demo: 📊 Total samples: 534528, Sample rate: 48000Hz, Duration: ~5568ms
   [00:00:00.361,000] <inf> esp32s3_box3_demo: 🎵 Test audio playback started!
   [00:00:01.200,000] <inf> esp32s3_box3_demo: 🎶 Playback progress: 10%
   [00:00:02.040,000] <inf> esp32s3_box3_demo: 🎶 Playback progress: 20%
   [00:00:05.986,000] <inf> esp32s3_box3_demo: 🎵 Test audio playback completed!
   [00:00:06.986,000] <inf> esp32s3_box3_demo: Now recording microphone — speak into the mic!
   [00:00:07.986,000] <inf> esp32s3_box3_demo: Recording 5000 ms to /lfs/audio_000.raw ...
   [00:00:08.007,000] <inf> esp32s3_box3_demo: I2S TX+RX started — recording...
   [00:00:09.043,000] <inf> esp32s3_box3_demo: Recorded 65536 bytes, peak 1300
   [00:00:10.067,000] <inf> esp32s3_box3_demo: Recorded 131072 bytes, peak 2120
   [00:00:12.958,000] <inf> esp32s3_box3_demo: Recording done: 183 blocks, 187392 bytes, peak 2120
   [00:00:12.958,000] <inf> esp32s3_box3_demo: Saved: /lfs/audio_000.raw (187392 bytes)

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
   
   # Memory management for audio buffers
   CONFIG_HEAP_MEM_POOL_SIZE=131072

Audio File Support
******************

The sample includes a test audio file (test.mp3) that has been converted to 48kHz, 16-bit stereo PCM format for direct I2S playback. The audio data is embedded as a C array in the firmware.

**Audio Specifications:**
- Sample Rate: 48kHz (playback) / 16kHz (recording)
- Bit Depth: 16-bit
- Channels: Stereo (2 channels)
- Format: PCM (uncompressed)
- Duration: ~5.6 seconds

**Adding Custom Audio:**

To replace the test audio with your own content:

1. Convert your audio file to raw PCM format:
   
   .. code-block:: bash
   
      ffmpeg -i your_audio.mp3 -f s16le -ar 48000 -ac 2 audio.raw

2. Convert the raw PCM to C array format using a conversion script
3. Replace the contents of ``test_audio.c`` and ``test_audio.h``
4. Rebuild the project

The sample automatically detects the presence of ``test_audio.c`` and enables audio playback.

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
   python3 ultra_fast_extract.py /dev/ttyUSB0 audio_000.raw

The extracted file will be in 16-bit stereo PCM format at 16kHz sample rate,
suitable for playback with audio tools like Audacity or conversion to other formats.

Troubleshooting
***************

Common issues and solutions:

* **No audio playback**: Check PA control GPIO and ensure power amplifier is enabled
* **No audio recording**: Check microphone connection and ES7210 initialization
* **Distorted audio**: Verify I2S timing configuration and reduce volume if clipping occurs
* **File system errors**: Verify LittleFS partition size and mounting
* **I2S errors**: Ensure proper pin configuration and clock timing
* **Shell not responding**: Check UART connection and baud rate (115200)
* **Memory allocation errors**: Increase heap size if audio buffers fail to allocate

References
**********

* `ES8311 Datasheet <https://www.everest-semi.com/pdf/ES8311%20PB.pdf>`_
* `ES7210 Datasheet <https://www.everest-semi.com/pdf/ES7210%20PB.pdf>`_
* :ref:`audio_api`
* :ref:`i2s_api`
* :ref:`file_system_api`

