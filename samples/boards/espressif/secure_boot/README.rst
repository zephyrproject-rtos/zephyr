.. zephyr:code-sample:: espressif_secure_boot
   :name: Espressif Secure Boot v2

   Build MCUboot with Espressif Secure Boot v2 on every in-tree Espressif SoC.

Overview
********

This sample enables Espressif Secure Boot v2 through sysbuild. The ROM
authenticates the signed MCUboot image. The application is still signed with
imgtool and is not Secure Boot v2 signed.

The Twister scenario is build-only. It does not flash or boot a device. The
PEM files under :file:`keys/` are test keys.

The MCUboot image in this sample turns logging, the console, and GPIO off and
uses a single application slot. That keeps the bootloader body inside the ROM
Secure Boot buffer. The post-build check still fails if the body grows past
the limit for that SoC.

``boards/espressif/common/building-flashing.rst`` covers key generation,
the ROM bootloader size limit, and enabling Secure Boot together with
hardware flash encryption.

Building
********

.. zephyr-app-commands::
   :zephyr-app: samples/boards/espressif/secure_boot
   :board: esp32c3_devkitc/esp32c3
   :goals: build
   :west-args: --sysbuild
   :gen-args: -DSB_CONFIG_SOC_FAMILY_ESPRESSIF_SECURE_BOOT=y
