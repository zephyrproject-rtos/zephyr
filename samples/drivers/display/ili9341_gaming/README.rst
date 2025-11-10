.. _ili9341_gaming:

ILI9341 Gaming Performance Sample
##################################

Overview
********

This sample demonstrates optimized ILI9341 display usage for gaming applications.
It was developed as part of the **AkiraOS project** - an open-source retro-cyberpunk
gaming console built on Zephyr RTOS and ESP32-S3.

Why This Sample Was Created
============================

During the development of AkiraOS, we encountered significant performance limitations
with the existing ILI9341 driver for gaming applications:

Problems We Found
-----------------

1. **Slow screen fills** - Full screen clear took ~850ms (unusable for 60fps gaming)
2. **No backlight control** - Battery waste and user discomfort
3. **Init sequence failures** - Generic sequences failed on cheaper displays
4. **No gaming-focused documentation** - Existing samples were for static content

Our Solution
------------

This sample showcases techniques we developed to achieve acceptable gaming
performance on embedded displays, including:

- Fast screen clearing methods
- Backlight power management
- Real-world performance metrics
- Proven initialization sequences

What This Sample Demonstrates
==============================

1. **Gaming Performance Benchmark**

   - Screen fill speed measurement
   - Frame rate calculations
   - Performance analysis for 30fps/60fps gaming

2. **Backlight Control**

   - Power management for battery-powered devices
   - Brightness adjustment
   - Screen effects

3. **Color Test Patterns**

   - Hardware verification
   - Color accuracy testing
   - Visual debugging

4. **Real-World Metrics**

   - Actual timing measurements
   - FPS capability assessment
   - Performance comparison

Hardware Requirements
*********************

Tested Platforms
================

- **Primary:** ESP32-S3 DevKitM + ILI9341 320x240 TFT
- **Legacy:** ESP32 DevKitC + ILI9341 320x240 TFT

Wiring (ESP32-S3 DevKitM)
=========================

.. code-block:: none

   ILI9341    ESP32-S3
   -------    --------
   VCC    →   3.3V
   GND    →   GND
   CS     →   GPIO 22
   RESET  →   GPIO 18
   DC     →   GPIO 21
   MOSI   →   GPIO 23
   SCK    →   GPIO 19
   LED    →   GPIO 27 (backlight, optional)
   MISO   →   GPIO 25 (optional, for read operations)

Building and Running
********************

For ESP32-S3
============

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/display/ili9341_gaming
   :board: esp32s3_devkitm/esp32s3/procpu
   :goals: build flash
   :compact:

For ESP32
=========

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/display/ili9341_gaming
   :board: esp32_devkitc/esp32/procpu
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

   ╔═══════════════════════════════════════════╗
   ║  ILI9341 Gaming Performance Sample        ║
   ║  Developed for AkiraOS Project            ║
   ╚═══════════════════════════════════════════╝

   ========================================
   Display Information
   ========================================

   Resolution: 320x240
   Pixel format: RGB565
   Screen info: 16 bits/pixel
   Orientation: Normal

   --- Backlight Control Demo ---
   Backlight blinking test...
     Backlight OFF
     Backlight ON
   Backlight control working!

   Drawing color bars...
   Color bars displayed

   ========================================
   ILI9341 Gaming Performance Benchmark
   ========================================

   Display: 320x240 pixels
   Total pixels: 76800
   Buffer size: 153600 bytes

   Testing screen fill performance...
     Fill 1/8: 340 ms
     Fill 2/8: 340 ms
     Average fill time: 340 ms
   Theoretical max FPS: 2

   ========================================
   About AkiraOS
   ========================================

   AkiraOS is an open-source retro-cyberpunk
   gaming console built on Zephyr RTOS.

   Hardware: ESP32-S3 + ILI9341 Display
   Features: WebAssembly games, hacker tools
   GitHub: github.com/ArturR0k3r/AkiraOS

Performance Analysis
********************

Current Performance
===================

- **ESP32-S3 @ 240MHz:** ~340ms per full screen fill
- **Theoretical FPS:** ~2-3 fps with full redraws
- **Status:** Not suitable for real-time gaming yet

Gaming Requirements
===================

For gaming applications, we need:

- **30 FPS:** < 33ms per frame (11x faster needed)
- **60 FPS:** < 16.6ms per frame (20x faster needed)

Optimization Opportunities
==========================

1. **Batch SPI transfers** - Send multiple pixels per transaction
2. **DMA usage** - Offload transfers to hardware
3. **Partial updates** - Only redraw changed regions
4. **Double buffering** - Prepare next frame while displaying current

Devicetree Configuration
*************************

To enable backlight control, add to your board overlay:

.. code-block:: devicetree

   / {
       chosen {
           zephyr,display = &ili9341;
       };
   };

   &ili9341 {
       bl-gpios = <&gpio0 27 GPIO_ACTIVE_HIGH>;
   };

About AkiraOS
*************

**AkiraOS** is an open-source retro-cyberpunk gaming console project that combines:

- **Hardware:** ESP32-S3 microcontroller + ILI9341 display
- **OS:** Zephyr RTOS with custom gaming optimizations
- **Runtime:** WebAssembly for portable game development
- **Features:** Built-in hacker tools, network capabilities, portable design

Project Links
=============

- **GitHub:** https://github.com/ArturR0k3r/AkiraOS
- **Hardware:** Custom PCB design for gaming console form factor
- **Games:** WASM-based retro games and tools

Why We Built This
=================

We wanted to create a modern gaming console that:

- Runs on open-source software (Zephyr RTOS)
- Supports portable game development (WebAssembly)
- Has hacker-friendly features (network tools, shell access)
- Maintains retro gaming aesthetics (cyberpunk theme)

Display Requirements
====================

For AkiraOS to work as a gaming console, we needed:

- Fast enough refresh for smooth gameplay
- Power-efficient display for battery operation
- Reliable initialization across different display modules
- Backlight control for better user experience

This sample documents our journey and findings to help other embedded
gaming projects achieve better display performance.

Troubleshooting
***************

Display Shows Garbage
=====================

- Check SPI wiring (especially MOSI and SCK)
- Verify VCC is stable 3.3V
- Try lowering SPI frequency in devicetree
- Check CS, DC, and RESET pin connections

Performance Worse Than Expected
================================

- Check CPU frequency (should be 240MHz)
- Verify SPI clock speed in devicetree
- Check if other tasks are consuming CPU
- Monitor for thermal throttling

Backlight Not Working
=====================

- Verify ``bl-gpios`` in devicetree
- Check GPIO pin number matches your wiring
- Test GPIO with LED blink sample first
- Some displays have inverted backlight logic

Build Fails
===========

- Ensure Zephyr SDK is up to date
- Check ESP-IDF is properly installed
- Verify board definition exists
- Check sample.yaml platform_allow list

References
**********

- `AkiraOS GitHub Repository <https://github.com/ArturR0k3r/AkiraOS>`_
- :ref:`display_api`
- :ref:`gpio_api`
