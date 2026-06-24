.. zephyr:code-sample-category:: tfm_secure_peripheral_st
   :name: TF-M Secure Peripheral for ST boards
   :show-listing:

   Isolate a peripheral and its resources with the STM32 GTZC and drive it from a
   TF-M secure partition.

Overview
********

These samples use the STM32 Global TrustZone Controller (GTZC) to assign an bus
instance and its GPIOs to the Secure state. A custom secure partition drives the
peripheral and exposes it to the Zephyr non-secure application over PSA IPC.
