:orphan:

.. espressif-board-variants

Board variants using Snippets
=============================

ESP32 boards can be assembled with different modules using multiple combinations of SPI flash sizes, PSRAM sizes and PSRAM modes.
The snippets under ``snippets/espressif`` provide a modular way to apply these variations at build time without duplicating board definitions.

The following snippet-based variants are supported:

==========================  =========================
Snippet name                Description
==========================  =========================
*Flash memory size*
-----------------------------------------------------
``espressif-flash-4M``      Board with 4MB of flash
``espressif-flash-8M``      Board with 8MB of flash
``espressif-flash-16M``     Board with 16MB of flash
``espressif-flash-32M``     Board with 32MB of flash
``espressif-flash-64M``     Board with 64MB of flash
``espressif-flash-128M``    Board with 128MB of flash

*PSRAM memory size*
-----------------------------------------------------
``espressif-psram-2M``      Board with 2MB of PSRAM
``espressif-psram-4M``      Board with 4MB of PSRAM
``espressif-psram-8M``      Board with 8MB of PSRAM

*PSRAM utilization*
-----------------------------------------------------
``espressif-psram-reloc``   Relocate flash to PSRAM
``espressif-psram-wifi``    Wi-Fi buffers in PSRAM
==========================  =========================

To apply a board variant, use the ``-S`` flag with west build:

.. zephyr-app-commands::
   :tool: west
   :zephyr-app: samples/hello_world
   :board: <board>
   :goals: build
   :snippets: espressif-flash-32M,espressif-psram-4M
   :compact:

.. note::

   These snippets are only applicable to boards with compatible hardware support for the selected flash/PSRAM configuration.

   - If no FLASH snippet is used, the board default flash size will be used.
   - If no PSRAM snippet is used, the board default psram size will be used.
