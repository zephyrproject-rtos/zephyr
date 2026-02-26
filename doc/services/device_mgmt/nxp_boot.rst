.. _nxp_rom_boot_detailed:

NXP ROM Bootloader
##################

Overview
********

The NXP ROM bootloader is a firmware update solution built into the ROM of
NXP MCXW series microcontrollers. It provides secure Over-The-Air (OTA)
firmware update capabilities using NXP's Secure Binary v3.1 (SB3.1) container
format.

This bootloader serves as a backend implementation for the :ref:`dfu_boot_api`,
enabling standard Zephyr DFU operations on supported NXP devices.

Key Features
============

* **ROM-based**: No flash space required for bootloader code
* **Secure updates**: SB3.1 containers provide authentication and encryption
* **Flexible targeting**: Update destination encoded in SB3.1 file
* **Multi-core support**: Can update any core or memory region
* **External flash support**: Staging area can reside on external SPI flash
* **Hybrid mode**: Combine with MCUboot for maximum flexibility
* **Architecture flexibility**: Ability to add or remove MCUboot during updates

Architecture
************

Slot Model
==========

The NXP ROM bootloader uses a two-slot staging model:

+----------+------------------+------------------------------------------------+
| Slot     | Format           | Description                                    |
+==========+==================+================================================+
| slot0    | Plain binary     | Active running image (application code)        |
+----------+------------------+------------------------------------------------+
| slot1    | SB3.1 container  | Staging area for new firmware                  |
+----------+------------------+------------------------------------------------+

.. note::

   Unlike MCUboot's swap model, the NXP ROM bootloader uses a **staging model**
   where slot1 holds the update package (not a backup). Updates are permanent
   once applied—there is no automatic revert mechanism.

Update Flow
===========

The firmware update process follows these steps:

1. **Image Reception**: New firmware (packaged as SB3.1) is written to slot1
   via MCUmgr or other transport

2. **Pending Request**: Application programs the OTA configuration into the
   IFR0 OTACFG page, specifying the location and size of the SB3.1 file

3. **Reboot**: System resets to allow ROM bootloader to process the update

4. **ROM Processing**: On boot, ROM bootloader:

   a. Reads OTACFG configuration
   b. Locates and validates the SB3.1 container
   c. Decrypts and authenticates the payload
   d. Installs firmware to the destination encoded in SB3.1
   e. Updates status field in OTACFG
   f. Boots the new firmware

5. **Cleanup**: On first boot after update, the Zephyr DFU subsystem:

   a. Reads update status from OTACFG
   b. Clears the OTACFG page
   c. Erases slot1 to prepare for next update

SB3.1 Container Format
======================

The Secure Binary v3.1 format is NXP's secure container for firmware delivery.

.. note::

   SB3.1 files are created using NXP's Secure Provisioning SDK (SPSDK) tools.
   Refer to NXP documentation for details on creating signed/encrypted
   containers.

OTA Configuration Storage
=========================

The OTA configuration is stored in the IFR0 (Information Flash Region 0)
OTACFG page. This dedicated flash sector serves as the communication channel
between the application and the ROM bootloader.

OTACFG Structure
----------------

+--------+--------------------+------------------------------------------------+
| Offset | Field              | Description                                    |
+========+====================+================================================+
| 0x00   | update_available   | Magic value (0x746f4278) if update pending     |
+--------+--------------------+------------------------------------------------+
| 0x10   | dump_location      | 0 for internal flash, magic for external SPI   |
+--------+--------------------+------------------------------------------------+
| 0x14   | baud_rate          | SPI baud rate for external flash access        |
+--------+--------------------+------------------------------------------------+
| 0x18   | dump_address       | Physical address of SB3.1 file                 |
+--------+--------------------+------------------------------------------------+
| 0x1C   | file_size          | Size of SB3.1 file in bytes                    |
+--------+--------------------+------------------------------------------------+
| 0x20   | update_status      | Status code written by ROM after update        |
+--------+--------------------+------------------------------------------------+
| 0x30   | unlock_key         | 16-byte feature unlock key                     |
+--------+--------------------+------------------------------------------------+

Update Status Codes
-------------------

After processing an update, the ROM bootloader writes one of these status codes:

+----------------+------------+----------------------------------------------------+
| Status         | Value      | Description                                        |
+================+============+====================================================+
| Success        | 0x5ac3c35a | Update completed successfully                      |
+----------------+------------+----------------------------------------------------+
| SB3 Error      | 0x4412d283 | Failed to parse or validate SB3.1 container        |
+----------------+------------+----------------------------------------------------+
| Erase Error    | 0x2d61e1ac | Failed to erase/write update status to OTACFG page |
+----------------+------------+----------------------------------------------------+

Architecture Flexibility
========================

A powerful capability of the NXP ROM bootloader is the ability to completely
change the application architecture during an update. Since the ROM bootloader
can write to any flash region, an SB3.1 container can deliver:

* **MCUboot + Application**: Add MCUboot to a system that previously ran without it
* **Application only**: Remove MCUboot from a system to reclaim flash space
* **Complete system image**: Replace the entire flash contents

This enables seamless transitions between:

* **ROM-only mode** ↔ **Hybrid mode** (with MCUboot)

.. note::

   When transitioning architectures, the SB3.1 container should include all
   necessary components to ensure the system boots correctly. For example,
   when adding MCUboot, the container should include both MCUboot and the
   application image (in MCUboot format) to guarantee a bootable system.

Hybrid Mode
***********

Overview
========

The hybrid mode (:kconfig:option:`CONFIG_NXPBOOT_HYBRID_MCUBOOT`) enables
simultaneous use of both MCUboot and the NXP ROM bootloader. This powerful
combination provides:

* **MCUboot** manages the Zephyr application image with full swap/revert support
* **NXP ROM bootloader** handles updates to components MCUboot cannot access

How It Works
============

In hybrid mode, the DFU subsystem automatically detects the image format
in slot1 by reading the magic number:

+------------------+------------+------------------------------------------+
| Magic            | Format     | Update Path                              |
+==================+============+==========================================+
| 0x96f3b83d       | MCUboot    | Standard MCUboot swap mechanism          |
+------------------+------------+------------------------------------------+
| 0x33766273       | SB3.1      | NXP ROM bootloader OTA mechanism         |
+------------------+------------+------------------------------------------+

Important Considerations
========================

**Image Confirmation Before SB3 Updates**

In hybrid mode, when MCUboot is managing the Zephyr application, the running
image **must be confirmed** before uploading an SB3.1 container to slot1.

This is critical because:

* If the active MCUboot image is in "test" mode (not yet confirmed), the
  previous image still resides in slot1 as a rollback target
* Uploading an SB3.1 container to slot1 would overwrite this rollback image
* If the system reboots before confirmation, MCUboot would attempt to revert
  to a now-corrupted slot1, resulting in a bricked device

.. warning::

   Never upload an SB3.1 container while the active MCUboot image is
   unconfirmed. This would destroy the rollback image and potentially
   brick the device.

When using MCUmgr for image management, this is handled automatically:
the ``img_mgmt`` module considers slot1 "in use" when the active image is
unconfirmed and will reject new uploads until confirmation occurs.

If MCUmgr is not used, the application must explicitly check the image
state before accepting SB3.1 uploads:

.. code-block:: c

   #include <zephyr/dfu/dfu_boot.h>

   bool can_accept_sb3_upload(void)
   {
       int swap_type = dfu_boot_get_swap_type(0);

       /* Only accept SB3 uploads when no MCUboot swap is pending */
       if (swap_type == DFU_BOOT_SWAP_TYPE_REVERT) {
           /* Active image not confirmed - slot1 contains rollback image */
           return false;
       }

       return true;
   }

Typical Scenarios
*****************

Scenario 1: NXP ROM Bootloader Only (No Hybrid Mode)
====================================================

This scenario applies when:

* **Swap/revert mechanism is not required**: The application can tolerate
  permanent updates without rollback capability
* **Flash space optimization is critical**: Removing MCUboot frees up
  valuable flash space for the application
* **External flash is not available or desired**: Using only internal flash
  simplifies the hardware design

In this configuration:

* The application runs directly from slot0 without MCUboot
* Updates are delivered as SB3.1 containers to slot1
* The ROM bootloader installs updates permanently
* No swap partition or MCUboot code is needed

.. code-block:: text

   ┌─────────────────────────────────────────────────────────┐
   │                    Flash Layout                         │
   ├─────────────────────────────────────────────────────────┤
   │  slot0: Zephyr Application (plain binary)               │
   ├─────────────────────────────────────────────────────────┤
   │  slot1: Staging area (SB3.1 containers)                 │
   ├─────────────────────────────────────────────────────────┤
   │  storage: Application data                              │
   └─────────────────────────────────────────────────────────┘

**Configuration:**

.. code-block:: kconfig

   CONFIG_NXPBOOT_IMG_MANAGER=y
   # CONFIG_BOOTLOADER_MCUBOOT is not set

Scenario 2: Hybrid Mode (MCUboot + NXP ROM Bootloader)
======================================================

This scenario applies when:

* **Swap/revert mechanism is required**: Critical applications need the
  ability to rollback to a known-good image
* **Multi-core updates are needed**: Other processor cores require firmware
  updates that MCUboot cannot perform
* **MCUboot updates are needed**: The bootloader itself needs to be field-upgradeable

A concrete example is the **NXP MCXW71**, a dual-core SoC where:

* **Core 0** runs Zephyr with MCUboot managing application updates
* **Core 1 (NBU)** runs a proprietary BLE controller / 802.15.4 firmware

The NBU firmware resides in a flash region that is
**not accessible from Core 0** at runtime. The only way to update the NBU
firmware is through the NXP ROM bootloader, which runs before any application
code and has full system access.

.. code-block:: text

   ┌─────────────────────────────────────────────────────────┐
   │                   Core 0 Flash                          │
   ├─────────────────────────────────────────────────────────┤
   │  MCUboot                                                │
   ├─────────────────────────────────────────────────────────┤
   │  slot0: Zephyr App (managed by MCUboot)                 │
   ├─────────────────────────────────────────────────────────┤
   │  slot1: MCUboot images OR SB3.1 containers              │
   ├─────────────────────────────────────────────────────────┤
   │                   Core 1 Flash (NBU)                    │
   ├─────────────────────────────────────────────────────────┤
   │  NBU Firmware (updated via ROM bootloader / SB3.1)      │
   └─────────────────────────────────────────────────────────┘

**Configuration:**

.. code-block:: kconfig

   CONFIG_BOOTLOADER_MCUBOOT=y
   CONFIG_NXPBOOT_IMG_MANAGER=y
   # CONFIG_NXPBOOT_HYBRID_MCUBOOT is auto-selected

**Update paths in hybrid mode:**

1. **Zephyr application update**: Upload MCUboot-format image → MCUboot
   performs swap with rollback support

2. **NBU firmware update**: Upload SB3.1 container targeting NBU flash →
   ROM bootloader installs on next boot

3. **MCUboot update**: Upload SB3.1 container targeting MCUboot region →
   ROM bootloader updates MCUboot

Configuration
*************

Devicetree Setup
================

IFR0 Region
-----------

Define the IFR0 flash region with the OTA configuration sector:

.. code-block:: devicetree

   ifr0: flash@1000000 {
       compatible = "nxp,mcxw-ifr";
       reg = <0x1000000 DT_SIZE_K(32)>;
       erase-block-size = <8192>;
       write-block-size = <16>;
       ota-cfg-sector = <3>;
   };

Flash Partitions
----------------

Define slot0 and slot1 partitions. Note that slot1 should be larger than
slot0 to accommodate SB3.1 container overhead:

.. code-block:: devicetree

   &flash {
       partitions {
           compatible = "fixed-partitions";
           #address-cells = <1>;
           #size-cells = <1>;

           /* Active application image */
           slot0_partition: partition@0 {
               reg = <0x0 DT_SIZE_K(488)>;
           };

           /* Staging area for SB3.1 containers */
           slot1_partition: partition@7a000 {
               reg = <0x7a000 DT_SIZE_K(496)>;
           };

           /* Application storage */
           storage_partition: partition@f6000 {
               reg = <0xf6000 DT_SIZE_K(32)>;
           };
       };
   };

Chosen Node
-----------

Specify the active partition:

.. code-block:: devicetree

   / {
       chosen {
           zephyr,code-partition = &slot0_partition;
       };
   };

External Flash (Optional)
-------------------------

For systems with external SPI flash, slot1 can be placed there:

.. code-block:: devicetree

   &mx25r6435f {
       status = "okay";

       partitions {
           compatible = "fixed-partitions";
           #address-cells = <1>;
           #size-cells = <1>;

           slot1_partition: partition@0 {
               reg = <0x0 DT_SIZE_M(1)>;
           };
       };
   };

The DFU subsystem automatically detects external flash placement and
configures the OTACFG accordingly.

Kconfig Options
===============

Basic NXP ROM Boot
------------------

.. code-block:: kconfig

   # Use NXP ROM bootloader (standalone mode)
   CONFIG_NXPBOOT_IMG_MANAGER=y

Hybrid Mode
-----------

.. code-block:: kconfig

   # Use both MCUboot and NXP ROM bootloader
   CONFIG_BOOTLOADER_MCUBOOT=y
   CONFIG_NXPBOOT_IMG_MANAGER=y
   # CONFIG_NXPBOOT_HYBRID_MCUBOOT is auto-selected

Troubleshooting
***************

Update Fails with SB3 Error
===========================

* Verify the SB3.1 file is correctly signed with the device's keys
* Ensure the slot1 partition is aligned to the flash sector size
* Check that the SB3.1 version is 3.1 (major=3, minor=1)
* Ensure the file is not corrupted during transfer

No Update Occurs After Reboot
=============================

* Verify OTACFG was programmed correctly (check logs)
* Ensure the unlock key matches device configuration
* Check that slot1 contains a valid SB3.1 file

Hybrid Mode: Wrong Update Path Selected
========================================

* Verify the image magic number is correct
* Check that CONFIG_NXPBOOT_HYBRID_MCUBOOT is enabled
* Ensure MCUboot is properly configured and running

References
**********

* :ref:`dfu_boot_api` - DFU Boot Abstraction API
* :ref:`mcuboot` - MCUboot documentation
* `NXP SPSDK Documentation`_ - Secure Provisioning SDK for creating SB3.1 files

.. _NXP SPSDK Documentation: https://spsdk.readthedocs.io/
