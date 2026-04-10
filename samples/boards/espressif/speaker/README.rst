.. zephyr:code-sample:: esp32s3_box3_speaker
   :name: ESP32-S3-BOX-3 Audio Speaker Demo
   :relevant-api: audio_interface i2s_interface gpio_interface

   Demonstrate audio playback using ES8311 codec and I2S interface on ESP32-S3-BOX-3.

Overview
********

This sample demonstrates high-quality audio playback capabilities of the ESP32-S3-BOX-3 board
using the ES8311 audio codec and I2S interface. The sample showcases professional audio
features including power amplifier control, volume management, and PCM audio playback.

The application performs a comprehensive audio system test:

1. **Audio System Initialization**
   - Configure ES8311 codec for 48kHz, 16-bit stereo output
   - Initialize I2S interface as master with proper timing
   - Enable power amplifier and configure mute control
   - Set optimal volume level (85%) for clear audio

2. **Audio Playback**
   - Play test.mp3 audio file converted to 48kHz PCM format
   - Demonstrate proper audio buffer management and streaming
   - Show progress indicators during playback

The sample demonstrates proper audio buffer management, I2S configuration,
and codec control using Zephyr's audio subsystem with real audio content.

Requirements
************

* ESP32-S3-BOX-3 board with built-in ES8311 audio codec
* Speaker or headphones connected to audio output
* USB cable for programming and power

Wiring
******

The ESP32-S3-BOX-3 has built-in audio components that are pre-wired:

.. list-table::
   :header-rows: 1

   * - Component
     - ESP32S3 Pin
     - Description
   * - ES8311 SDA
     - GPIO41
     - I2C data for codec control
   * - ES8311 SCL
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
     - Audio data output to codec
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
   :zephyr-app: samples/boards/espressif/speaker
   :board: esp32s3_box3/esp32s3/procpu
   :goals: build flash monitor

Sample Output
*************

The sample will initialize the audio system and play the test audio:

.. code-block:: console

   ========================================
     ESP32-S3-BOX-3 Audio Demo
     Board: esp32s3_box3
   ========================================
   [00:00:00.076,000] <inf> esp32s3_box3_demo: Initializing audio system...
   [00:00:00.076,000] <inf> esp32s3_box3_demo: ✓ PA control enabled
   [00:00:00.076,000] <inf> esp32s3_box3_demo: ✓ Audio unmuted
   [00:00:00.076,000] <inf> esp32s3_box3_demo: ✓ ES8311 device ready
   [00:00:00.341,000] <inf> esp32s3_box3_demo: ✓ ES8311 codec initialized (48kHz, 16-bit, stereo, I2S)
   [00:00:00.341,000] <inf> esp32s3_box3_demo: ✓ Volume set to 85% (217/255)
   [00:00:00.342,000] <inf> esp32s3_box3_demo: ✓ I2S device ready
   [00:00:00.342,000] <inf> esp32s3_box3_demo: ✓ I2S configured (48kHz, 16-bit, stereo, MCLK=12.288MHz)
   [00:00:00.342,000] <inf> esp32s3_box3_demo: 🎵 Audio system initialized successfully!
   [00:00:00.342,000] <inf> esp32s3_box3_demo: 🔊 Ready for music playback at 48kHz, 85% volume
   [00:00:00.342,000] <inf> esp32s3_box3_demo: ▶ Playing test.mp3...
   [00:00:00.342,000] <inf> esp32s3_box3_demo: 🎵 Starting test.mp3 playback...
   [00:00:00.342,000] <inf> esp32s3_box3_demo: 📊 Total samples: 253440, Block size: 1024 samples
   [00:00:00.342,000] <inf> esp32s3_box3_demo: 📊 Sample rate: 48000Hz, Duration: ~2640ms
   [00:00:00.342,000] <inf> esp32s3_box3_demo: 🎵 Test audio playback started!
   [00:00:00.597,000] <inf> esp32s3_box3_demo: 🎶 Playback progress: 100%
   [00:00:01.108,000] <inf> esp32s3_box3_demo: 🎵 Test audio playback completed!
   [00:00:01.108,000] <inf> esp32s3_box3_demo: ✓ Test audio playback completed
   [00:00:01.108,000] <inf> esp32s3_box3_demo: 🎉 Audio demo completed

Configuration
*************

The audio system can be configured via device tree and Kconfig options:

**Key Kconfig Options:**

.. code-block:: kconfig

   # Audio subsystem
   CONFIG_AUDIO=y
   CONFIG_I2S=y
   CONFIG_ES8311=y
   
   # GPIO for power control
   CONFIG_GPIO=y
   
   # Memory management for audio buffers
   CONFIG_HEAP_MEM_POOL_SIZE=32768

**Device Tree Configuration:**

.. code-block:: devicetree

   &i2c0 {
       es8311: es8311@18 {
           compatible = "everest,es8311";
           reg = <0x18>;
           pa-gpios = <&gpio1 14 GPIO_ACTIVE_HIGH>;
           status = "okay";
       };
   };

   &i2s0 {
       status = "okay";
       pinctrl-0 = <&i2s0_default>;
       pinctrl-names = "default";
   };

Audio File Support
******************

The sample includes a test audio file (test.mp3) that has been converted to 48kHz, 16-bit stereo PCM format for direct I2S playback. The audio data is embedded as a C array in the firmware.

**Audio Specifications:**
- Sample Rate: 48kHz
- Bit Depth: 16-bit
- Channels: Stereo (2 channels)
- Format: PCM (uncompressed)
- Duration: ~2.6 seconds

**Adding Custom Audio:**

To replace the test audio with your own content:

1. Convert your audio file to raw PCM format:
   
   .. code-block:: bash
   
      ffmpeg -i your_audio.mp3 -f s16le -ar 48000 -ac 2 audio.raw

2. Convert the raw PCM to C array format using a conversion script
3. Replace the contents of ``test_audio.c`` and ``test_audio.h``
4. Rebuild the project

The sample automatically detects the presence of ``test_audio.c`` and enables audio playback.

Troubleshooting
***************

Common issues and solutions:

* **No audio output**: Check PA control GPIO and ensure power amplifier is enabled
* **Distorted audio**: Verify I2S timing configuration and reduce volume if clipping occurs
* **Static or noise**: Ensure audio data is in correct PCM format (48kHz, 16-bit, stereo)
* **I2S configuration errors**: Ensure proper pin assignments and clock frequencies
* **Codec initialization failure**: Check I2C communication with ES8311
* **Memory allocation errors**: Increase heap size if audio buffers fail to allocate

References
**********

* `ES8311 Datasheet <https://www.everest-semi.com/pdf/ES8311%20PB.pdf>`_
* :ref:`audio_api`
* :ref:`i2s_api`