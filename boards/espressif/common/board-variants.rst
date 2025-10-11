:orphan:

.. espressif-board-variants

Board variants using Snippets
=============================

ESP32 boards can be assembled with different modules using multiple combinations of SPI flash sizes, PSRAM sizes and PSRAM modes.
The snippets under ``snippets/espressif`` provide a modular way to apply these variations at build time without duplicating board definitions.

The following snippet-based variants are supported:

===============  ========================
Snippet name     Description
===============  ========================
*Flash memory size*
-----------------------------------------
``flash-4M``     Board with 4MB of flash
``flash-8M``     Board with 8MB of flash
``flash-16M``    Board with 16MB of flash
``flash-32M``    Board with 32MB of flash
---------------  ------------------------
*PSRAM memory size*
-----------------------------------------
``psram-2M``     Board with 2MB of PSRAM
``psram-4M``     Board with 4MB of PSRAM
``psram-8M``     Board with 8MB of PSRAM
---------------  ------------------------
*PSRAM utilization*
-----------------------------------------
``psram-reloc``  Relocate flash to PSRAM
``psram-wifi``   Wi-Fi buffers in PSRAM
===============  ========================

To apply a board variant, use the ``-S`` flag with west build:

.. zephyr-app-commands::
   :tool: west
   :zephyr-app: samples/hello_world
   :board: <board>
   :goals: build
   :snippets: flash-32M,psram-4M
   :compact:

**Note:** These snippets are applicable to boards with compatible hardware support for the selected flash/PSRAM configuration.
