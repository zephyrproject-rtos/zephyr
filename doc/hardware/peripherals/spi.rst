.. _spi_api:

Serial Peripheral Interface (SPI) Bus
#####################################

Overview
********

Terminology
===========

The Zephyr SPI API uses the inclusive terminology selected by
:ref:`coding_guideline_inclusive_language`, following the `OSHWA resolution
to redefine SPI signal names`_:

* The device driving the clock is the *controller*, devices it addresses are
  *peripherals* (see :c:macro:`SPI_OP_MODE_CONTROLLER` and
  :c:macro:`SPI_OP_MODE_PERIPHERAL`).
* Data signals are named from each device's own perspective: *SDO* (Serial
  Data Out) and *SDI* (Serial Data In), with *CS* (Chip Select) for the
  select line.

The former master/slave and MOSI/MISO names remain available as compatibility
aliases. They are deprecated since Zephyr v4.5 and will be removed in Zephyr
v5.0.

.. _OSHWA resolution to redefine SPI signal names:
   https://oshwa.org/resources/a-resolution-to-redefine-spi-signal-names/

API Reference
*************

.. doxygengroup:: spi_interface
