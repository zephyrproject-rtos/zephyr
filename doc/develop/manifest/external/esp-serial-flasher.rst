.. _external_module_esp_serial_flasher:

ESP Serial Flasher
##################

Introduction
************

ESP Serial Flasher is a portable C library developed and maintained by Espressif Systems that enables programming and interaction with Espressif SoCs (ESP8266, ESP32 series) from host microcontrollers running embedded operating systems. The library provides a unified API for flash operations, RAM download and execution, and device management across multiple communication interfaces including UART, USB CDC ACM, SPI, and SDIO.

This module is particularly valuable for Zephyr-based applications that need to program or interact with Espressif target devices without requiring a PC or Python runtime environment. It offers similar functionality to esptool but is optimized for resource-constrained embedded systems, making it ideal for production programming tools, bootloaders, and firmware update mechanisms.

ESP Serial Flasher supports a wide range of Espressif SoCs including ESP8266, ESP32, ESP32-S2, ESP32-S3, ESP32-C2, ESP32-C3, ESP32-H2, ESP32-C6, ESP32-C5, and ESP32-P4. The library is integrated into Zephyr as an external project module in ``subsys/esp_loader`` folder.

ESP Serial Flasher is licensed under the Apache License 2.0.

Usage with Zephyr
*****************

To pull in ESP Serial Flasher as a Zephyr module, either add it as a West project in the ``west.yaml``
file or pull it in by adding a submanifest (e.g. ``zephyr/submanifests/esf.yaml``) file
with the following content and run ``west update``:

.. code-block:: yaml

   manifest:
     projects:
       - name: esp-serial-flasher
         url: https://github.com/espressif/esp-serial-flasher
         revision: master
         path: modules/esp-serial-flasher  # adjust the path, or remove it (default is zephyrproject root)

Once the module is added, you can include the main header file in your application code:

.. code-block:: c

   #include <esp_loader.h>

The Zephyr integration provides a devicetree interface (``espressif,esp-tool``) that can be configured via device tree. The driver handles UART communication, GPIO control for boot and reset pins, and provides high-level APIs for flash operations, device connection, and target management.

For shell-based interaction, the module provides commands under the ``esf`` shell command group, allowing you to connect to ESP targets, read and write flash memory, detect flash size, erase flash, and perform register operations directly from the Zephyr shell.

Firmware images for the target can be stored inside the host SoC. You need to copy binary file(s) into the ``target-firmware/`` folder and enable the ``ESP_SERIAL_FLASHER_STATIC_RESOURCES`` config in your ``prj.conf``.
The binary files name should follow the ``<name>_<addr>.bin`` convention. For example ``boot_0x0.bin`` or, ``app_0x20000.bin``.

Reference
*********

.. target-notes::

.. _ESP Serial Flasher GitHub Repository:
   https://github.com/espressif/esp-serial-flasher

.. _ESP Serial Flasher Component Registry:
   https://components.espressif.com/components/espressif/esp-serial-flasher

.. _Zephyr Modules Documentation:
   https://docs.zephyrproject.org/latest/develop/modules.html
