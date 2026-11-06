.. zephyr:code-sample:: usb-dwc3-host
   :name: DWC3 xHCI host (build smoke)
   :relevant-api: uhc_interface

   Build-only check that the Versal DWC3 xHCI UHC driver links for ``versal_apu``.

Overview
********

This sample does not require USB sticks at build time. Runtime enumeration and
MSC I/O are demonstrated by :zephyr:code-sample:`usb-host-msc`.

Building
********

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/usb/dwc3_host
   :board: versal_apu
   :goals: build
   :extra-args: -DDTC_OVERLAY_FILE=boards/versal_apu.overlay
   :compact:
