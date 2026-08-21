.. _snippet-uhc-dwc2-usbotghs-stm32f7:

UHC DWC2 OTG_HS for STM32F7 Snippets
####################################

.. code-block:: console

   west build -S uhc-dwc2-usbotghs-stm32f7-usbphyc [...]

Overview
********

This snippet configures the OTG_HS controller (``usbotg_hs``) of an STM32F7
SoC to use the native DWC2 USB host controller driver
(:kconfig:option:`CONFIG_UHC_DWC2`). It also assigns the ``zephyr_uhc0`` node
label so that USB host samples use this controller by default.

Supported SoCs
**************

The generic overlay wires the OTG_HS controller to the embedded USB high-speed
PHY (USBPHYC, ``&usbphyc``). Only STM32F7 SoCs that integrate this PHY are
supported: STM32F723, F730, and F733.

STM32F74x, F75x, F76x, and F77x SoCs expose OTG_HS only through an external
ULPI PHY (there is no ``usbphyc`` node in their device tree) and are not covered
by this snippet.

Requirements
************

The target board must be able to supply VBUS on the OTG_HS port. VBUS switching
is handled by the per-board overlay.

How to use
**********

Build a USB host sample with the snippet applied, for example:

.. code-block:: console

   west build -b stm32f723e_disco -S uhc-dwc2-usbotghs-stm32f7-usbphyc \
      samples/subsys/usb/shell
