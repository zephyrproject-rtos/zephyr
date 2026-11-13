.. zephyr:code-sample:: nrf71-icr
   :name: nRF71 ICR fallback register reads

   Read configuration register values using the nRF71 UICR, FICR, and default fallback order.

Overview
********

This sample reads every nRF71 register supported by ``nrf71_icr_read()`` and
prints the selected value and whether it came from UICR, FICR, or the supplied
default.

The sample devicetree configures ``UICR.XTALCOMP.OFFSET`` to 1010.

Requirements
************

An nRF7120 DK using either the nRF7120 or nRF7120E application core.

Building and Running
********************

Build the application and UICR images with sysbuild, then erase and flash a
dedicated development board::

   west build --sysbuild -b nrf7120dk/nrf7120/cpuapp \
     samples/boards/nordic/nrf71_icr
   west flash --erase

The full erase is required if UICR contains a different value. Do not run this
procedure on production hardware.
