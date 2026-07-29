.. _dfu:

Device Firmware Upgrade
#######################

Overview
********

The Device Firmware Upgrade subsystem provides the necessary frameworks to
upgrade the image of a Zephyr-based application at run time. It currently
consists of the following modules:

* :zephyr_file:`subsys/dfu/boot/`: Interface code to bootloaders
* :zephyr_file:`subsys/dfu/img_util/`: Image management code

The DFU subsystem deals with image management, but not with the transport
or management protocols themselves required to send the image to the target
device. For information on these protocols and frameworks please refer to the
:ref:`device_mgmt` section.

.. _dfu_boot_api:

DFU Boot Abstraction API
========================

The DFU boot abstraction API provides a bootloader-agnostic interface for
image management operations. This abstraction layer isolates bootloader-specific
code from higher-level subsystems like MCUmgr's img_mgmt, enabling support for
multiple bootloaders through a common API.

The API provides the following functionality:

* Reading image information (version, flags, hash) from flash slots
* Validating image headers
* Managing boot slot selection (pending, confirm operations)
* Querying swap type status
* Flash area and slot management utilities

Currently, MCUboot is the supported backend implementation, but the abstraction
allows for other bootloader backends to be added in the future.

API Reference
-------------

.. doxygengroup:: dfu_boot_api

.. _flash_img_api:

Flash Image
===========

The flash image API as part of the Device Firmware Upgrade (DFU) subsystem
provides an abstraction on top of Flash Stream to simplify writing firmware
image chunks to flash.

The flash image module has been refactored to isolate MCUboot-specific code,
allowing the core flash image utilities to be potentially reused with other
bootloaders. MCUboot-specific functionality (such as trailer scrambling and
image start offset handling) is conditionally compiled when
:kconfig:option:`CONFIG_BOOTLOADER_MCUBOOT` is enabled.

API Reference
-------------

.. doxygengroup:: flash_img_api

.. _mcuboot_api:

MCUBoot API
===========

The MCUboot API is provided to get version information and boot status of
application images. It allows to select application image and boot type
for the next boot.

.. note::

   For new applications, consider using the :ref:`dfu_boot_api` which provides
   a bootloader-agnostic interface. The MCUboot API remains available for
   backward compatibility and for MCUboot-specific functionality not covered
   by the abstraction layer.

API Reference
-------------

.. doxygengroup:: mcuboot_api

Bootloaders
***********

.. _mcuboot:

MCUboot
=======

Zephyr is directly compatible with the open source, cross-RTOS
`MCUboot boot loader`_. It interfaces with MCUboot and is aware of the image
format required by it, so that Device Firmware Upgrade is available when MCUboot
is the boot loader used with Zephyr. The source code itself is hosted in the
`MCUboot GitHub Project`_ page.

MCUboot serves as the reference backend implementation for the
:ref:`dfu_boot_api`. When :kconfig:option:`CONFIG_BOOTLOADER_MCUBOOT` is enabled,
the DFU boot abstraction functions are implemented using MCUboot's bootutil
library.

In order to use MCUboot with Zephyr you need to take the following into account:

1. You will need to define the flash partitions required by MCUboot; see
   :ref:`flash_map_api` for details.
2. You will have to specify your flash partition as the chosen code partition

.. code-block:: devicetree

   / {
      chosen {
         zephyr,code-partition = &slot0_partition;
      };
   };

3. Your application's :file:`.conf` file needs to enable the
   :kconfig:option:`CONFIG_BOOTLOADER_MCUBOOT` Kconfig option in order for Zephyr to
   be built in an MCUboot-compatible manner
4. You need to build and flash MCUboot itself on your device
5. You might need to take precautions to avoid mass erasing the flash and also
   to flash the Zephyr application image at the correct offset (right after the
   bootloader)

More detailed information regarding the use of MCUboot with Zephyr  can be found
in the `MCUboot with Zephyr`_ documentation page on the MCUboot website.

Adding New Bootloader Backends
==============================

To add support for a new bootloader, implement the functions declared in
:zephyr_file:`include/zephyr/dfu/dfu_boot.h`. The key functions to implement are:

* ``dfu_boot_read_img_info()`` - Read image metadata from a slot
* ``dfu_boot_validate_header()`` - Validate an image header
* ``dfu_boot_get_swap_type()`` - Get the swap type for an image
* ``dfu_boot_set_pending()`` - Set a slot as pending for next boot
* ``dfu_boot_confirm()`` - Confirm the current image

Common utility functions like ``dfu_boot_get_flash_area_id()``,
``dfu_boot_get_erased_val()``, ``dfu_boot_read()``, and ``dfu_boot_erase_slot()``
are provided in :zephyr_file:`subsys/dfu/boot/dfu_boot_utils.c` and can be
reused by different bootloader backends.

.. _MCUboot boot loader: https://mcuboot.com/
.. _MCUboot with Zephyr: https://docs.mcuboot.com/readme-zephyr
.. _MCUboot GitHub Project: https://github.com/runtimeco/mcuboot
