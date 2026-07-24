.. _snippet-udc-dwc2-usbotgfs-stm32f7:

UDC DWC2 OTG_FS for STM32F7 Snippet (udc-dwc2-usbotgfs-stm32f7)
###############################################################

.. code-block:: console

   west build -S udc-dwc2-usbotgfs-stm32f7 [...]

Overview
********

This snippet reconfigures the OTG_FS controller (``usbotg_fs``) of an STM32F7
SoC to use the native DWC2 USB device controller driver
(:kconfig:option:`CONFIG_UDC_DWC2`) instead of the STM32 UDC shim driver. It
also assigns the ``zephyr_udc0`` node label so that USB device samples use this
controller by default.

Supported SoCs
**************

All STM32F7 SoCs (STM32F72x, F73x, F74x, F75x, F76x, and F77x). Their OTG_FS
core is identical (6 IN and 6 OUT endpoints), so a single set of GHWCFG values
applies to the whole family.

This snippet also supports STM32F4 SoCs whose OTG_FS core has 6 IN and 6 OUT
endpoints, for example the STM32F413xG/H. Parts with a different endpoint count
use different GHWCFG values and are not covered by this variant.

Requirements
************

The target board must route the OTG_FS data lines on the fixed pins PA11 (DM)
and PA12 (DP), which is the case for all in-tree STM32F7 boards.

How to use
**********

Build a USB device sample with the snippet applied, for example:

.. code-block:: console

   west build -b stm32f723e_disco -S udc-dwc2-usbotgfs-stm32f7 \
      samples/subsys/usb/cdc_acm
