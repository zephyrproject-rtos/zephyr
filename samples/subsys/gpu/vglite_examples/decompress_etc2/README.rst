.. zephyr:code-sample:: decompress_etc2
   :name: VGLite ETC2 decompression sample
   :relevant-api: display_interface

   Demonstrate VGLite GPU ETC2 texture decompression capabilities.

Overview
********

This example shows how to use the VGLite component for hardware-accelerated ETC2
texture decompression. This application draws a decompressed ETC2 (Ericsson Texture
Compression 2) texture on the screen, demonstrating VGLite's texture decompression
capabilities.

The ETC2 decompression demo showcases VGLite's ability to decompress compressed
textures in hardware, which is useful for reducing memory bandwidth and storage
requirements in graphics applications. The texture appears as a chessboard pattern
with different green shades.

.. note::
   This application is currently supported only on the RT700 board.

Requirements
************

Hardware Requirements
=====================

This sample requires a board with:

1. **VGLite GPU with ETC2 support** - The board must have a VGLite-compatible GPU with ETC2 decompression
2. **Display controller** - An integrated display or display interface
3. **Sufficient memory** - Adequate RAM for framebuffer and GPU operations
4. **Micro USB cable** - For programming and serial console
5. **Compatible display panel** - One of the supported MIPI or DBI panels

Supported Boards
================

- :zephyr:board:`mimxrt700_evk` (i.MX RT700 EVK) - **Only supported board**

.. important::
   This sample currently works only on the RT700 board due to specific ETC2
   decompression hardware requirements.

Supported Display Panels
=========================

The following display panels are supported:

**MIPI Panels:**

- RK055AHD091 MIPI panel (connect to J52 on RT700 EVK)
- RK055IQH091 MIPI panel (connect to J52 on RT700 EVK)
- RK055MHD091 MIPI panel (connect to J52 on RT700 EVK) - **Default**
- RM67162 smart MIPI DBI panel

**DBI Panels:**

- TFT Proto 5" CAPACITIVE (SSD1963) panel by Mikroelektronika
- ZC143AC72 MIPI DBI panel

**Other Panels:**

- RaspberryPi Panel (connect to J8 on RT700 EVK)

Board Settings
==============

The board must have:

- VGLite GPU hardware with ETC2 support (:dtcompatible:`nxp,vglite`)
- Display controller configured in devicetree
- ``zephyr,display`` chosen node defined
- Compatible display shield overlay

**For RT700 EVK with MIPI panels:**

- Connect MIPI panel to J52
- Default panel: RK055MHD091

**For RT700 EVK with SSD1963 panel:**

- Connect SSD1963 panel to J4
- Make sure to configure JP7 2&3 for 3.3V FLEXIO interface

**For RT700 EVK with RaspberryPi panel:**

- Connect the panel to J8
- Connect panel's 5V pin to JP43-1, GND pin to JP43-2
- Ensure R75, R76, R79, R80 are connected

Building and Running
********************

Prepare the Demo
================

1. Connect a USB cable between the host PC and the OpenSDA USB port on the target board
2. Open a serial terminal with the following settings:

   - 115200 baud rate
   - 8 data bits
   - No parity
   - One stop bit
   - No flow control

3. Build and flash the program to the target board
4. Press the reset button on your board to begin running the demo

Build Commands
==============

Example building for :zephyr:board:`mimxrt700_evk`:

.. code-block:: bash

   west build -p always -b mimxrt700_evk/mimxrt798s/cm33_cpu0 zephyr/samples/subsys/gpu/vglite_examples/decompress_etc2/ --shield rk055hdmipi4ma0 -DCONFIG_DEBUG=y -DCONFIG_DEBUG_OPTIMIZATIONS=n -DCMAKE_BUILD_TYPE=Debug -DVGLITE_OS=zephyr -DDTC_OVERLAY_FILE=$(west topdir)/zephyr/boards/nxp/mimxrt700_evk/mimxrt700_evk.overlay -DVGLITE_PLATFORM=rt700 -DEXTRA_CPPFLAGS="-DCPU_MIMXRT798SGFOA_cm33_core0"

.. code-block:: bash

   west flash

Release Build
=============

For optimized release builds, remove the debug flags:

.. code-block:: bash

   west build -p always -b mimxrt700_evk/mimxrt798s/cm33_cpu0 zephyr/samples/subsys/gpu/vglite_examples/decompress_etc2/ --shield rk055hdmipi4ma0 -DVGLITE_OS=zephyr -DDTC_OVERLAY_FILE=$(west topdir)/zephyr/boards/nxp/mimxrt700_evk/mimxrt700_evk.overlay -DVGLITE_PLATFORM=rt700 -DEXTRA_CPPFLAGS="-DCPU_MIMXRT798SGFOA_cm33_core0"

Running the Demo
================

When the example runs, you can see a decompressed ETC2 texture displayed on the
screen. The texture appears as a chessboard pattern with different green shades,
demonstrating the VGLite GPU's hardware-accelerated ETC2 texture decompression
capabilities.

The demo demonstrates:

- **ETC2 decompression** - Hardware-accelerated texture decompression
- **Memory efficiency** - Reduced memory bandwidth using compressed textures
- **High-quality output** - Lossless decompression with high visual quality
- **Chessboard pattern** - Visual demonstration with green shade variations

Configuration
*************

The sample requires specific configuration options:

- ``CONFIG_VGLITE`` - Enable VGLite GPU support
- ``CONFIG_DISPLAY`` - Enable display driver support
- ``VGLITE_OS=zephyr`` - Set VGLite OS abstraction layer to Zephyr
- ``VGLITE_PLATFORM=rt700`` - Platform-specific setting for RT700

Debug Build Options
===================

- ``CONFIG_DEBUG=y`` - Enable debug mode
- ``CONFIG_DEBUG_OPTIMIZATIONS=n`` - Disable debug optimizations
- ``CMAKE_BUILD_TYPE=Debug`` - Set CMake build type to debug

Display Shield Configuration
=============================

The default configuration uses the RK055MHD091 MIPI panel. To use different panels,
configure the appropriate shield overlay:

- ``rk055hdmipi4ma0`` - For RT700 EVK (RK055AHD091)

Board-Specific Requirements
============================

- **RT700**: Requires devicetree overlay and CPU-specific flags
- **ETC2 Support**: RT700 VGLite GPU must support ETC2 decompression

Panel Selection
===============

In Zephyr, panel selection is done through devicetree overlays and shield configurations.
The default panel is RK055MHD091 MIPI panel. To use a different panel, specify the
appropriate shield or create a custom devicetree overlay.

Serial Console Output
=====================

Connect to the serial console at 115200 baud rate to see application logs and status messages.

About ETC2 Compression
======================

ETC2 (Ericsson Texture Compression 2) is a lossy texture compression format that
provides:

- **High compression ratio** - Reduces texture memory footprint
- **Fixed decompression rate** - Predictable performance
- **Hardware support** - Fast GPU-accelerated decompression
- **Good visual quality** - Suitable for most texture types


References
**********

.. target-notes::

.. _NXP i.MX RT Series: https://www.nxp.com/products/processors-and-microcontrollers/arm-microcontrollers/i-mx-rt-crossover-mcus:IMX-RT-SERIES
.. _VGLite GitHub: https://github.com/nxp-mcuxpresso/gpu-vglite
.. _RK055HDMIPI4M Display: https://www.nxp.com/design/development-boards/i-mx-evaluation-and-development-boards/5-5-lcd-panel:RK055HDMIPI4M
.. _ETC2 Texture Compression: https://en.wikipedia.org/wiki/Ericsson_Texture_Compression


