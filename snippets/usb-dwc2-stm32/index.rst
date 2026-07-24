.. _usb-dwc2-stm32-snippets:

USB DWC2 STM32 snippets
#######################

These snippets reconfigure the STM32 OTG_FS/OTG_HS controllers to use the native
DWC2 USB controller driver (:kconfig:option:`CONFIG_UDC_DWC2`) (or
:kconfig:option:`CONFIG_UHC_DWC2`) instead of the STM32 UDC shim driver. Pick
the variant that matches the controller core of the target SoC.

.. toctree::
   :maxdepth: 1
   :glob:

   **/*
