.. _snippet-hw-test:

Hardware Test Snippet (hw-test)
###############################

Overview
********

This snippet enables the hardware a board makes available for testing its
drivers: the controllers to use, their pin configuration, the signals wired to
them, and what a test can sample, loop back or transfer. Boards provide it in
:file:`snippets/hw-test/` under their own directory; the snippet itself carries
no settings.

A board describes everything in one place, so the parts for the different
subsystems must not conflict: a pin, a chip select or a DMA channel is claimed
by one peripheral only, and every test requesting the snippet gets the whole
description. Tests take what they need from it, by enabled devicetree nodes,
by the compatible of a test node, or by a label the board assigns.

Kconfig options of a subsystem are not part of the snippet. A test enables the
subsystem it exercises; the driver of an enabled controller follows from the
devicetree.

Tests using it
**************

- :zephyr_file:`tests/drivers/adc/adc_api`: every channel node of an enabled
  ADC controller, and the DMA it is wired to for the DMA scenarios.
- :zephyr_file:`tests/drivers/spi/spi_loopback`: the ``test-spi-loopback-slow``
  and ``test-spi-loopback-fast`` nodes on the SPI bus with the loopback wire.
- :zephyr_file:`tests/drivers/dma/loop_transfer`: the DMA controller labelled
  ``tst_dma0``.
