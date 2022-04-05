.. _espi_api:

eSPI
####

Overview
********

The eSPI (enhanced serial peripheral interface) is a serial bus that is
based on SPI. It also features a four-wire interface (receive, transmit, clock
and slave select) and three configurations: single IO, dual IO and quad IO.

The technical advancements include lower voltage signal levels (1.8V vs. 3.3V),
lower pin count, and the frequency is twice as fast (66MHz vs. 33MHz)
Because of its enhancements, the eSPI is used to replace the LPC
(lower pin count) interface, SPI, SMBus and sideband signals.

See `eSPI interface specification`_ for additional details.


API Reference
*************

.. doxygengroup:: espi_interface

.. _eSPI interface specification:
    https://www.intel.com/content/dam/support/us/en/documents/software/chipset-software/327432-004_espi_base_specification_rev1.0_cb.pdf
