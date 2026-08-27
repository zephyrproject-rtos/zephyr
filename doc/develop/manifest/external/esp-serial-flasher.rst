.. _external_module_esp_serial_flasher:

ESP Serial Flasher
##################

Introduction
************

ESP Serial Flasher is a portable C library developed and maintained by Espressif Systems that
enables programming and interacting with Espressif SoCs (ESP8266, ESP32 series) from
host microcontrollers running embedded operating systems.
The library provides a unified API for flash operations, RAM download and execution, and device
management across multiple communication interfaces including UART, USB CDC ACM, SPI, and SDIO.

This module is particularly valuable for Zephyr-based applications that need to program or interact
with Espressif target devices without requiring a PC or Python runtime environment.
It offers similar functionality to esptool but is optimized for resource-constrained embedded systems,
making it ideal for production programming tools, bootloaders, and firmware update mechanisms.

ESP Serial Flasher supports a wide range of Espressif SoCs. For the list of supported
targets and more information, see `ESP Serial Flasher GitHub`_.

ESP Serial Flasher is licensed under the Apache License 2.0.

Usage with Zephyr
*****************

To pull in ESP Serial Flasher as a Zephyr module, either add it as a West project
in the ``west.yml`` file or pull it in by adding a submanifest
(e.g. ``zephyr/submanifests/esp-serial-flasher.yaml``) with the following content and run
``west update``:

.. code-block:: yaml

   manifest:
     projects:
       - name: esp-serial-flasher
         url: https://github.com/espressif/esp-serial-flasher
         revision: master
         path: modules/esp-serial-flasher # adjust path or leave blank for west workspace root directory

Once the module is added, you can include the main header file in your application code:

.. code-block:: c

   #include <esp_loader.h>

The Zephyr integration provides a devicetree binding, ``espressif,esp-loader``, which must be
defined in your application's devicetree at least once. Select the active instance with the
``zephyr,esp-loader`` chosen property. The driver handles UART communication, GPIO control for
boot and reset pins, and provides high-level APIs for flash operations, device connection, and
target management.

Firmware images for the target can be embedded in the host application.
Place the binaries in a ``target-firmware/`` folder and describe flash addresses in
``target-firmware/images.csv``: one line per image as ``<filename>;<offset>``, with the offset in
hex and **no** ``0x`` prefix (for example ``app.bin;10000``).

After including ``bin_images_sections.cmake`` (from the Zephyr example), ``create_resources()`` in
your application's ``CMakeLists.txt`` converts these files into C arrays. Enable the driver with
``CONFIG_ESP_SERIAL_FLASHER=y`` in ``prj.conf``.
See ``examples/zephyr_example`` in the `ESP Serial Flasher GitHub`_ repository for ``images.csv``,
CMake, and devicetree setup.

For shell-based interaction, build ``examples/zephyr_example`` with
``CONFIG_ESP_SERIAL_FLASHER_SHELL=y``.
The sample provides the ``esf`` shell command group to connect to ESP targets, read and write flash
memory, detect flash size, erase flash, and read and write registers from the Zephyr shell.

Reference
*********

- `ESP Serial Flasher Component Registry`_
- `Zephyr Modules Documentation`_

.. target-notes::

.. _ESP Serial Flasher GitHub:
   https://github.com/espressif/esp-serial-flasher

.. _ESP Serial Flasher Component Registry:
   https://components.espressif.com/components/espressif/esp-serial-flasher

.. _Zephyr Modules Documentation:
   https://docs.zephyrproject.org/latest/develop/modules.html
