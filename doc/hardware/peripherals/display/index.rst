.. _display_api:

Display
#######

The Display subsystem in Zephyr provides a unified way to interact with a wide range of display
devices. The Display API is transport-agnostic: it describes what you want the display to do,
without exposing how the data moves over the wire.

MIPI Display Bus Interface (DBI)
********************************

The **MIPI DBI** specification defines several parallel and serial buses for connecting a host to a
display controller. In Zephyr, DBI support provides bus-level primitives for command writes, reads,
pixel transfers, reset, and related operations that a driver uses internally to implement the
generic API.

Applications do not use DBI functions directly. Instead, they call the generic Display API (e.g., to
write pixels), and the display driver handles the DBI protocol under the hood.

MIPI-DBI defines 3 interface types:

* Type A: Motorola 6800 parallel bus
* Type B: Intel 8080 parallel bus
* Type C: SPI Type serial bit bus with 3 options:

  #. 9 write clocks per byte, final bit is command/data selection bit
  #. Same as above, but 16 write clocks per byte
  #. 8 write clocks per byte. Command/data selected via GPIO pin

Currently, the API does not support Type C controllers with 16 write clocks (option 2).

MIPI Display Serial Interface (DSI)
***********************************

The **MIPI DSI** standard is a high-speed differential serial bus designed for modern color TFT
panels. Zephyr's DSI support provides the primitives needed by drivers to implement the generic
Display API on top of a DSI link.

As with DBI, applications never call DSI functions directly. They remain portable by using the
generic Display API, while the driver handles the DSI transactions internally.

API Reference
*************

Generic Display Interface
=========================

.. doxygengroup:: display_interface

.. _mipi_dbi_api:

MIPI Display Bus Interface (DBI)
================================

.. doxygengroup:: mipi_dbi_interface

.. _mipi_dsi_api:

MIPI Display Serial Interface (DSI)
===================================

.. doxygengroup:: mipi_dsi_interface

Grove LCD Display
=================

.. doxygengroup:: grove_display

BBC micro:bit Display
=====================

.. doxygengroup:: mb_display

Monochrome Character Framebuffer
================================

.. doxygengroup:: monochrome_character_framebuffer
