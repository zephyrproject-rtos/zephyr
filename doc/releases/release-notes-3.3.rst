:orphan:

.. _zephyr_3.3:

Zephyr 3.3.0 (Working Draft)
############################

We are pleased to announce the release of Zephyr version 3.3.0.

Major enhancements with this release include:

The following sections provide detailed lists of changes by component.

Security Vulnerability Related
******************************

API Changes
***********

Changes in this release
=======================

* Newlib nano variant is no longer selected by default when
  :kconfig:option:`CONFIG_NEWLIB_LIBC` is selected.
  :kconfig:option:`CONFIG_NEWLIB_LIBC_NANO` must now be explicitly selected in
  order to use the nano variant.

* Bluetooth: Add extra options to bt_le_per_adv_sync_transfer_subscribe to
  allow disabling sync reports, and enable sync report filtering. these two
  options are mutually exclusive.

* Bluetooth: :kconfig:option:`CONFIG_BT_PER_ADV_SYNC_TRANSFER_RECEIVER`
  and :kconfig:option:`CONFIG_BT_PER_ADV_SYNC_TRANSFER_SENDER` have been
  added to enable the PAST implementation rather than
  :kconfig:option:`CONFIG_BT_CONN`.
* Flashdisk: :kconfig:option:`CONFIG_DISK_FLASH_VOLUME_NAME`,
  :kconfig:option:`CONFIG_DISK_FLASH_DEV_NAME`,
  :kconfig:option:`CONFIG_DISK_FLASH_START`,
  :kconfig:option:`CONFIG_DISK_FLASH_MAX_RW_SIZE`,
  :kconfig:option:`CONFIG_DISK_ERASE_BLOCK_SIZE`,
  :kconfig:option:`CONFIG_DISK_FLASH_ERASE_ALIGNMENT`,
  :kconfig:option:`CONFIG_DISK_VOLUME_SIZE` and
  :kconfig:option:`CONFIG_DISK_FLASH_SECTOR_SIZE` Kconfig options have been
  removed in favor of new :dtcompatible:`zephyr,flash-disk` devicetree binding.

* Regulator APIs previously located in ``<zephyr/drivers/regulator/consumer.h>``
  are now part of ``<zerphyr/drivers/regulator.h>``.

* Starting from this release ``zephyr-`` prefixed tags won't be created
  anymore. The project will continue using ``v`` tags, for example ``v3.3.0``.

* Bluetooth: Deprecate the Bluetooth logging subsystem in favor of the Zephyr
  standard logging system. To enable debugging for a particular module in the
  Bluetooth subsystem, enable `CONFIG_BT_(module name)_LOG_LEVEL_DBG` instead of
  `CONFIG_BT_DEBUG_(module name)`.

* MCUmgr img_mgmt now requires that a full sha256 hash to be used when
  uploading an image to keep track of the progress, where the sha256 hash
  is of the whole file being uploaded (different to the hash used when getting
  image states). Use of a truncated hash or non-sha256 hash will still work
  but will cause issues and failures in client software with future updates
  to Zephyr/MCUmgr such as image verification.

* MCUmgr handlers no longer need to be registered by the application code,
  handlers just need to use a define which will then call the supplied
  registration function at boot-up. If applications register this then
  those registrations should be removed to prevent registering the same
  handler multiple times.

* MCUmgr Bluetooth and UDP transports no longer need to be registered by the
  application code, these will now automatically be registered at bootup (this
  feature can be disabled for the UDP transport by setting
  :kconfig:option:`CONFIG_MCUMGR_TRANSPORT_UDP_AUTOMATIC_INIT`). If
  applications register transports then those registrations should be removed
  to prevent registering the same transport multiple times.

* MCUmgr transport Kconfigs have changed from ``select`` to ``depends on``
  which means that for applications using the Bluetooth transport,
  applications will now need to enable the following:

  * :kconfig:option:`CONFIG_BT`
  * :kconfig:option:`CONFIG_BT_PERIPHERAL`

  For CDC or serial transports:

  * :kconfig:option:`CONFIG_CONSOLE`

  For shell transport:

  * :kconfig:option:`CONFIG_SHELL`
  * :kconfig:option:`CONFIG_SHELL_BACKEND_SERIAL`

  For UDP transport:

  * :kconfig:option:`CONFIG_NETWORKING`
  * :kconfig:option:`CONFIG_NET_UDP`

* MCUmgr fs_mgmt hash/checksum function, type and variable names have been
  changed to be prefixed with ``fs_mgmt_`` to retain alignment with other
  zephyr and MCUmgr APIs.

* Python's argparse argument parser usage in Zephyr scripts has been updated
  to disable abbreviations, any future python scripts or python code updates
  must also disable allowing abbreviations by using ``allow_abbrev=False``
  when setting up ``ArgumentParser()``.

  This may cause out-of-tree scripts or commands to fail if they have relied
  upon their behaviour previously, these will need to be updated in order for
  building to work. As an example, if a script argument had ``--reset-type``
  and an out-of-tree script used this by passing ``--reset`` then it will need
  to be updated to use the full argument name, ``--reset-type``.

* Rewrote the CAN API to utilize flag bitfields instead discrete of struct
  members for indicating standard/extended CAN ID, Remote Transmission Request
  (RTR), and added support for filtering of CAN-FD format frames.

Removed APIs in this release
============================

* Removed :kconfig:option:`CONFIG_COUNTER_RTC_STM32_LSE_DRIVE*`
  This should now be configured using the ``driving_capability`` property of
  LSE clock

* Removed :kconfig:option:`CONFIG_COUNTER_RTC_STM32_LSE_BYPASS`
  This should now be configured using the new ``lse_bypass`` property of
  LSE clock

* Removed :kconfig:option:`CONFIG_COUNTER_RTC_STM32_BACKUP_DOMAIN_RESET`

* Removed deprecated tinycbor module, code that uses this module should be
  updated to use zcbor as a replacement.

* Removed deprecated GPIO flags used for setting debounce, drive strength and
  voltage level. All drivers now use vendor-specific flags as needed.

* Removed deprecated ``UTIL_LISTIFY`` helper macro.

* Removed deprecated ``pwm_pin*`` family of functions from the PWM API.

* Removed deprecated ``nvs_init`` function from the NVS filesystem API.

* Removed deprecated ``DT_CHOSEN_*_LABEL`` helper macros.

Deprecated in this release
==========================

* C++ library Kconfig options have been renamed to improve consistency. See
  below for the list of deprecated Kconfig options and their replacements:

  .. table::
     :align: center

     +----------------------------------------+------------------------------------------------+
     | Deprecated                             | Replacement                                    |
     +========================================+================================================+
     | :kconfig:option:`CONFIG_CPLUSPLUS`     | :kconfig:option:`CONFIG_CPP`                   |
     +----------------------------------------+------------------------------------------------+
     | :kconfig:option:`CONFIG_EXCEPTIONS`    | :kconfig:option:`CONFIG_CPP_EXCEPTIONS`        |
     +----------------------------------------+------------------------------------------------+
     | :kconfig:option:`CONFIG_RTTI`          | :kconfig:option:`CONFIG_CPP_RTTI`              |
     +----------------------------------------+------------------------------------------------+
     | :kconfig:option:`CONFIG_LIB_CPLUSPLUS` | :kconfig:option:`CONFIG_LIBCPP_IMPLEMENTATION` |
     +----------------------------------------+------------------------------------------------+

* MCUmgr subsystem, specifically the SMP transport API, is dropping `zephyr_`
  prefix, deprecating prefixed functions and callback type definitions with the
  prefix and replacing them with with prefix-less variants.
  The :c:struct:`zephyr_smp_transport` type, representing transport object,
  is now replaced with :c:struct:`smp_transport`, and the later one is used,
  instead of the former one, by all prefix-less functions.

  Deprecated functions and their replacements:

  .. table::
     :align: center

     +-------------------------------------+---------------------------------------+
     | Deprecated                          | Drop in replacement                   |
     +=====================================+=======================================+
     | :c:func:`zephyr_smp_transport_init` | :c:func:`smp_transport_init`          |
     +-------------------------------------+---------------------------------------+
     | :c:func:`zephyr_smp_rx_req`         | :c:func:`smp_rx_req`                  |
     +-------------------------------------+---------------------------------------+
     | :c:func:`zephyr_smp_alloc_rsp`      | :c:func:`smp_alloc_rsp`               |
     +-------------------------------------+---------------------------------------+
     | :c:func:`zephyr_smp_free_buf`       | :c:func:`smp_free_buf`                |
     +-------------------------------------+---------------------------------------+

  Deprecated callback types and their replacements:

  .. table::
     :align: center

     +---------------------------------------------+---------------------------------------+
     | Deprecated                                  | Drop in replacement                   |
     +=============================================+=======================================+
     | :c:func:`zephyr_smp_transport_out_fn`       | :c:func:`smp_transport_out_fn`        |
     +---------------------------------------------+---------------------------------------+
     | :c:func:`zephyr_smp_transport_get_mtu_fn`   | :c:func:`smp_transport_get_mtu_fn`    |
     +---------------------------------------------+---------------------------------------+
     | :c:func:`zephyr_smp_transport_ud_copy_fn`   | :c:func:`smp_transport_ud_copy_fn`    |
     +---------------------------------------------+---------------------------------------+
     | :c:func:`zephyr_smp_transport_ud_free_fn`   | :c:func:`smp_transport_ud_free_fn`    |
     +---------------------------------------------+---------------------------------------+

  NOTE: Only functions are marked as ``__deprecated``, type definitions are not.

* STM32 Ethernet Mac address Kconfig related symbols (:kconfig:option:`CONFIG_ETH_STM32_HAL_RANDOM_MAC`,
  :kconfig:option:`CONFIG_ETH_STM32_HAL_MAC4`, ...) have been deprecated in favor
  of the use of zephyr generic device tree ``local-mac-address`` and ``zephyr,random-mac-address``
  properties.

* STM32 RTC source clock should now be configured using devicetree.
  Related Kconfig :kconfig:option:`CONFIG_COUNTER_RTC_STM32_CLOCK_LSI` and
  :kconfig:option:`CONFIG_COUNTER_RTC_STM32_CLOCK_LSE` options are now
  deprecated.

* STM32 Interrupt controller Kconfig symbols such as :kconfig:option:`CONFIG_EXTI_STM32_EXTI0_IRQ_PRI`
  are removed. Related IRQ prioritues should now be configured in device tree.

* File backend for settings APIs and Kconfig options were deprecated:

  :c:func:`settings_mount_fs_backend` in favor of :c:func:`settings_mount_file_backend`

  :kconfig:option:`CONFIG_SETTINGS_FS` in favor of :kconfig:option:`CONFIG_SETTINGS_FILE`

  :kconfig:option:`CONFIG_SETTINGS_FS_DIR` in favor of creating all parent
  directories from :kconfig:option:`CONFIG_SETTINGS_FILE_PATH`

  :kconfig:option:`CONFIG_SETTINGS_FS_FILE` in favor of :kconfig:option:`CONFIG_SETTINGS_FILE_PATH`

  :kconfig:option:`CONFIG_SETTINGS_FS_MAX_LINES` in favor of :kconfig:option:`CONFIG_SETTINGS_FILE_MAX_LINES`

* PCIe APIs :c:func:`pcie_probe` and :c:func:`pcie_bdf_lookup` have been
  deprecated in favor of a centralized scan of available PCIe devices.

* POSIX API

    * Deprecated :c:macro:`PTHREAD_COND_DEFINE`, :c:macro:`PTHREAD_MUTEX_DEFINE` in favour of the
      standard :c:macro:`PTHREAD_COND_INITIALIZER` and :c:macro:`PTHREAD_MUTEX_INITIALIZER`.
    * Deprecated ``<fcntl.h>``, ``<sys/stat.h>`` header files in the minimal libc in favour of
      ``<zephyr/posix/fcntl.h>`` and ``<zephyr/posix/sys/stat.h>``.

* SPI DT :c:func:`spi_is_ready` function has been deprecated in favor of :c:func:`spi_is_ready_dt`.

* LwM2M APIs using string references as LwM2M paths has been deprecated in favor of functions
  using :c:struct:`lwm2m_path_obj` instead.

Stable API changes in this release
==================================

* MCUmgr events have been reworked to use a single, unified callback system.
  This allows better customisation of the callbacks with a lower flash size.
  Applications using the existing callback system will need to be upgraded to
  use the new API by following the :ref:`migration guide <mcumgr_cb_migration>`

* :c:func:`net_pkt_get_frag`, :c:func:`net_pkt_get_reserve_tx_data` and
  :c:func:`net_pkt_get_reserve_rx_data` functions are now requiring to specify
  the minimum fragment length to allocate, so that they work correctly also in
  case :kconfig:option:`CONFIG_NET_BUF_VARIABLE_DATA_SIZE` is enabled.
  Applications using this APIs will need to be updated to provide the expected
  fragment length.

* Marked the Controller Area Network (CAN) controller driver API as stable.

New APIs in this release
========================

Kernel
******

Architectures
*************

* ARM

* ARM

* ARM64

  * Implemented ASID support for ARM64 MMU

* RISC-V

* Xtensa

Bluetooth
*********

* Audio

  * Refactored the handling of extended and periodic advertising in the BAP
    broadcast source.
  * Implemented the Common Audio Profile initiator role.
  * Added support for Broadcast source subgroup and BIS codec configuration.
  * Renamed the CSI and VCP functionality to use the "P" postfix for profile
    instead of "S" for service.
  * Added a broadcast source metadata update function.
  * Added (un)binding of audio ISO structs to Audio Streams.
  * Added support for encrypted broadcast.
  * Added the ability to change the supported contexts in PACS.
  * Improved stream coupling for CIS as the unicast client
  * Added broadcast source metadata update function
  * Added packing to unicast group create
  * Added packing field to broadcast source
  * Renamed BASS and BASS client to BAP Scan Delegator and BPA Broadcast Assistant
  * Added support for multiple subgroups for BAP broadcast sink
  * Replaced capabilities API with PACS

* Host

  * Added a new ``BT_CONN_INTERVAL_TO_US`` utility macro.
  * Made the HCI fragmentation logic asynchronous, thus fixing a long-standing
    potential deadlock between data and control procedures.
  * Added the local advertising address to :c:func:`bt_le_ext_adv_get_info`.
  * Improved the implementation of :c:func:`bt_disable` to handle additional
    edge cases.
  * Removed all Bluetooth-specific logging macros and functionality, switching
    instead to the OS-wide ones.
  * Added a new :c:func:`bt_le_per_adv_sync_lookup_index` function.
  * Fixed missing calls to bt_le_per_adv_sync_cb.term when deleting a periodic
    advertising sync object.
  * Added local advertising address to bt_le_ext_adv_info.
  * Added the printing of function names by default when logging.
  * Changed the policy for advertising restart after disconneciton, which is now
    done only for connections in the peripheral role.
  * Added a guard to prevent bonding to the same device more than once.
  * Refactored crypto functionality from SMP into its own folder, and added the
    h8 crypto function.
  * Changed the behavior when receiving an L2CAP K-frame larger than the MPS,
    disconnecting instead of truncating it.
  * Added a new :kconfig:option:`BT_ID_ALLOW_UNAUTH_OVERWRITE` that allows
    unauthorized bond overrides with multiple identities.
  * Added suppor for the object calculate checksum feature in OTS.
  * Changed back the semantics of :kconfig:option:`BT_PRIVACY` to refer to local
    RPA address generation.
  * Modified the SMP behavior when outside a pairing procedure. The stack no
    longer sends unnecessary Pairing Failed PDUs in that state.

  * ISO: Changed ISO seq_num to 16-bit

* Mesh

  * Changed the default advertiser to be extended advertiser.
  * Made the provisioning feature set dynamic.
  * Made the maximum number of simultaneous Bluetooth connections that the mesh
    stack can use configurable via :kconfig:option:`BT_MESH_MAX_CONN`.
  * Changed the advertising duration calculation to avoid imprecise estimations.
  * Added the :kconfig:option:`BT_MESH_FRIEND_ADV_LATENCY` Kconfig option.

* Controller

  * Implemented the Read/Write Connection Accept Timeout HCI commands.
  * Implemented the Sleep Clock Accuracy Update procedure.
  * Implemented additional ISO-related HCI commands.
  * Implemented ISO-AL SDU buffering and PDU release timeout.
  * Added support for hanlding fragmented AD without chaining PDUs.
  * Added support for multiple memory pools for advertising PDUs
  * Added support for retrying the automatic peripheral connection parameter
    update.
  * Added support for deferring anchor points moves using an external hook.
  * Added a new ``LL_ASSERT_MSG`` macro for verbose assertions.
  * Added long control PDU support.
  * Added support for Broadcast ISO encryption.
  * Added support for central CIS/CIG, including ULL and Nordic LLL.
  * Added support for peripheral CIS/CIG in the Nordic LLL.
  * Added the :kconfig:option:`BT_CTLR_SLOT_RESERVATION_UPDATE` Kconfig option.
  * Integrated ISOAL for ISO broadcast.

Boards & SoC Support
********************

* Added support for these SoC series:

  * GigaDevice GD32L23X
  * GigaDevice GD32A50X

* Removed support for these SoC series:

* Made these changes in other SoC series:

* Changes for ARC boards:

* Added support for these ARM boards:

  * Adafruit KB2040
  * GigaDevice GD32L233R-EVAL
  * GigaDevice GD32A503V-EVAL
  * Sparkfun pro micro RP2040

* Added support for these ARM64 boards:

  * i.MX93 (Cortex-A) EVK board
  * Khadas Edge-V board
  * QEMU Virt KVM

* Removed support for these ARM boards:

* Removed support for these X86 boards:

* Added support for these RISC-V boards:

  * Added LCD support for ``longan_nano`` board.

* Added support for these Xtensa boards:

* Removed support for these Xtensa boards:

* Made these changes in ARM boards:

  * The scratch partition has been removed for the following Nordic boards and
    flash used by this area re-assigned to other partitions to free up space
    and rely upon the swap-using-move algorithm in MCUboot (which does not
    suffer from the same faults or stuck image issues as swap-using-scratch
    does):
    ``nrf21540dk_nrf52840``
    ``nrf51dk_nrf51422``
    ``nrf51dongle_nrf51422``
    ``nrf52833dk_nrf52833``
    ``nrf52840dk_nrf52811``
    ``nrf52840dk_nrf52840``
    ``nrf52840dongle_nrf52840``
    ``nrf52dk_nrf52805``
    ``nrf52dk_nrf52810``
    ``nrf52dk_nrf52832``
    ``nrf5340dk_nrf5340``
    ``nrf9160dk_nrf52840``
    ``nrf9160dk_nrf9160``

    Note that MCUboot and MCUboot image updates from pre-Zephyr 3.3 might be
    incompatible with Zephyr 3.3 onwards and vice versa.

  * The default console for the ``nrf52840dongle_nrf52840`` board has been
    changed from physical UART (which is not connected to anything on the
    board) to use USB CDC instead.

* Made these changes in other boards:

  * The nrf52_bsim (natively simulated nRF52 device with BabbleSim) now models
    a nRF52833 instead of a nRF52832 device

* Added support for these following shields:

  * nPM6001 EK
  * nPM1100 EK

Build system and infrastructure
*******************************

* Code relocation

  * ``zephyr_code_relocate`` API has changed to accept a list of files to
    relocate and a location to place the files.

* Sysbuild

  * Issue with duplicate sysbuild image name causing an infinite cmake loop
    has been fixed.

  * Issue with board revision not being passed to sysbuild images has been
    fixed.

* Userspace

  * Userspace option to disable using the ``relax`` linker option has been
    added.

* Tools

  * Static code analyser (SCA) tool support has been added.

Drivers and Sensors
*******************

* ADC

* CAN

  * Added RX overflow counter statistics support (STM32 bxCAN, Renesas R-Car,
    and NXP FlexCAN).
  * Added support for TWAI on ESP32-C3.
  * Added support for multiple MCP2515 driver instances.
  * Added Kvaser PCIcan driver and support for using it under QEMU.
  * Made the fake CAN test driver generally available.
  * Added support for compiling the Native Posix Linux CAN driver against Linux
    kernel headers prior to v5.14.
  * Removed the CONFIG_CAN_HAS_RX_TIMESTAMP and CONFIG_CAN_HAS_CANFD Kconfig
    helper symbols.

* Clock control

* Counter

  * STM32 RTC based counter should now be configured using device tree.
  * Added Timer based driver for GigaDevice GD32 SoCs.

* Crypto

* DAC

* DFU

  * Remove :c:macro:`BOOT_TRAILER_IMG_STATUS_OFFS` in favor a two new functions;
    :c:func:`boot_get_area_trailer_status_offset` and :c:func:`boot_get_trailer_status_offset`

* Disk

  * STM32 SD host controller clocks are now configured via devicetree.
  * Zephyr flash disks are now configured using the :dtcompatible:`zephyr,flash-disk`
    devicetree binding
  * Flash disks can be marked as read only by setting the ``read-only`` property
    on the linked flash device partition.

* Display

* DMA

  * Adjust incorrect dma1 clock source for GD32 gd32vf103 SoC.
  * STM32 DMA variable scope cleanups
  * Intel GPDMA linked list transfer descriptors appropriately aligned to 64 byte addresses
  * Intel GPDMA fix bug in transfer configuration to initialize cfg_hi and cfg_lo
  * STM32 DMA Support for the STM32MP1 series
  * SAM XDMAC fixes to enable usage with SPI DMA transfers
  * Intel GPDMA fix to return errors on dma stop
  * Intel GPDMA disable interrupts when unneeded
  * Intel GPDMA fix for register/ip ownership
  * STM32U5 GPDMA bug fix for busy flag
  * STM32U5 Suspend and resume features added
  * Intel GPDMA Report total bytes read/written (linear link position) in dma status
  * DMA API get attribute function added, added attributes for scatter/gather blocks available
    to Intel HDA and Intel GPDMA drivers.
  * Intel GPDMA Power management functionality added
  * Intel HDA Power management functionality added
  * GD32 Slot used for peripheral selection
  * GD32 memory to memory support added
  * ESP32C3 GDMA driver added
  * Intel HDA underrun/overrun (xrun) handling and reporting added
  * Intel GPDMA underrun/overrun (xrun) handling nad reporting added
  * DMA API start/stop are defined to be repeatable callable with test cases added.
    STM32 DMA, Intel HDA, and Intel GPDMA all comply with the contract after patches.
  * NXP EDMA Unused mutex removed

* EEPROM

  * Added fake EEPROM driver for testing purposes.

* Entropy

* ESPI

* Ethernet

  * STM32: Default Mac address configuration is now uid based. Optionally, user can
    configure it to be random or provide its own address using device tree.

* Flash

  * Flash: Moved CONFIG_FLASH_FLEXSPI_XIP into the SOC level due to the flexspi clock initialization occurring in the SOC level.

  * NRF: Added CONFIG_SOC_FLASH_NRF_TIMEOUT_MULTIPLIER to allow tweaking the timeout of flash operations.

  * spi_nor: Added property mxicy,mx25r-power-mode to jedec,spi-nor binding for controlling low power/high performance mode on Macronix MX25R* Ultra Low Power flash devices.

  * spi_nor: Added check if the flash is busy during init. This used to cause
    the flash device to be unavailable until the system was restarted. The fix
    waits for the flash to become ready before continuing. In cases where a
    full flash erase was started before a restart, this might result in several
    minutes of waiting time (depending on flash size and erase speed).

  * rpi_pico: Added a flash driver for the Raspberry Pi Pico platform.

* FPGA

  * Add preliminary support for the Lattice iCE40.
  * Add Qomu board sample.

* GPIO

  * Added driver for nPM6001 PMIC GPIOs

* I2C

  * SAM0 Fixed spurious trailing data by moving stop condition from thread into ISR
  * I2C Shell command adds ability to configure bus speed through `i2c speed`
  * ITE usage of instruction local memory support
  * NPCX bus recovery on transaction timeout
  * ITE log status of registers on transfer failure
  * ESP32 enable configuring a hardware timeout to account for longer durations of clock stretching
  * ITE fix bug where an operation was done outside of the driver mutex
  * STM32 Support 10 bit addressing for target mode
  * NRFX TWIM Make transfer timeout configurable
  * STM32 Use devicetree for i2c clock source
  * DW Bug fix for clearing FIFO on initialization
  * NPCX simplify smb bank register usage
  * NXP LPI2C enable target mode
  * NXP FlexComm Adds semaphore for shared usage of bus
  * STM32 Power management support added
  * I2C Allow dumping messages in the log for all transactions, reads and writes

* I2S

* I3C

* IEEE 802.15.4

* Interrupt Controller

  * STM32: Driver configuration and initialization is now based on device tree

* IPM

* KSCAN

* LED

* MBOX

* MEMC

* PCIE

* PECI

* Pin control

  * Common pin control properties are now defined at root level in a single
    file: :zephyr_file:`dts/bindings/pinctrl/pincfg-node.yaml`. Pin control
    bindings are expected to include it at the level they need. For example,
    drivers using the grouping representation approach need to include it at
    grandchild level, while drivers using the node approach need to include it
    at the child level. This change will only impact out-of-tree pin control
    drivers, sinc all in-tree drivers have been updated.
  * Added NXP S32 SIUL2 driver
  * Added Nuvoton NuMicro driver
  * Added Silabs Gecko driver
  * Added support for i.MX93 in the i.MX driver
  * Added support for GD32L23x/GD32A50x in the Gigadevice driver

* PWM

* Power domain

* Regulators

  * Completed an API overhaul so that devices like PMICs can be supported. The
    API now offers a clear and concise API that allows to perform the following
    operations:

      - Enable/disable regulator output (reference counted)
      - List supported voltages
      - Get/set operating voltage
      - Get/set maximum current
      - Get/set operating mode
      - Obtain errors, e.g. overcurrent.

    The devicetree part maintains compatibility with Linux bindings, for example,
    the following properties are well supported:

      - ``regulator-boot-on``
      - ``regulator-always-on``
      - ``regulator-min-microvolt``
      - ``regulator-max-microvolt``
      - ``regulator-min-microamp``
      - ``regulator-max-microamp``
      - ``regulator-allowed-modes``
      - ``regulator-initial-mode``

    A common driver class layer takes care of the common functionality so that
    driver implementations are kept simple. For example, allowed voltage ranges
    are verified before calling into the driver.

    An experimental parent API to configure DVS (Dynamic Voltage Scaling) has
    also been introduced.

  * Refactored NXP PCA9420 driver to align with the new API.
  * Added support for nPM6001 PMIC (LDO and BUCK converters).
  * Added support for nPM1100 PMIC (allows to dynamically change its mode).
  * Added a new test that allows to verify regulator output voltage using the
    ADC API.
  * Added a new test that checks API behavior provided we have a well-behaved
    driver.

* Reset

* SDHC

  * i.MX RT USDHC:

    - Support HS400 and HS200 mode. This mode is used with eMMC devices,
      and will enable high speed operation for those cards.
    - Support DMA operation on SOCs that do not support noncacheable memory,
      such as the RT595. DMA will enable higher performance SD modes,
      such as HS400 and SDR104, to reliably transfer data using the
      SD host controller

* Sensor

  * Refactored all drivers to use :c:macro:`SENSOR_DEVICE_DT_INST_DEFINE` to
    enable a new sensor info iterable section and shell command. See
    :kconfig:option:`CONFIG_SENSOR_INFO`.
  * Refactored all sensor devicetree bindings to inherit new base sensor device
    properties in :zephyr_file:`dts/bindings/sensor/sensor-device.yaml`.
  * Added sensor attribute support to the shell.
  * Added MCUX and STM32 quadrature encoder drivers.
  * Added ESP32 and RaspberryPi Pico die temperature sensor drivers.
  * Added TDK InvenSense ICM42688 six axis IMU driver.
  * Added TDK InvenSense ICP10125 pressure and temperature sensor driver.
  * Added AMS AS5600 magnetic angle sensor driver.
  * Added AMS AS621x temperature sensor driver.
  * Added HZ-Grow R502A fingerprint sensor driver.
  * Enhanced FXOS8700, FXAS21002, and BMI270 drivers to support SPI in addition
    to I2C.
  * Enhanced ST LIS2DW12 driver to support freefall detection.
  * rpi_pico: Added die temperature sensor driver.

* Serial

* SPI

  * Added dma support for GD32 driver.

* Timer

* USB

  * STM32F1: Clock bus configuration is not done automatically by driver anymore.
    It is user's responsibility to configure the proper bus prescaler using clock_control
    device tree node to achieve a 48MHz bus clock. Note that, in most cases, core clock
    is 72MHz and default prescaler configuration is set to achieve 48MHz USB bus clock.
    Prescaler only needs to be configured manually when core clock is already 48MHz.

  * STM32 (non F1): Clock bus configuration is now expected to be done in device tree
    using ``clocks`` node property. When a dedicated HSI 48MHz clock is available on target,
    is it configured by default as the USB bus clock, but user has the ability to select
    another 48MHz clock source. When no HSI48 is available, a specific 48MHz bus clock
    source should be configured by user.

* W1

* Watchdog

  * Added driver for nPM6001 PMIC Watchdog.
  * Added free watchdog driver for GigaDevice GD32 SoCs.
  * Added window watchdog driver for GigaDevice GD32 SoCs.

* WiFi

Networking
**********

* CoAP:

  * Implemented insertion of a CoAP option at arbitrary position.

* Ethernet:

  * Fixed AF_PACKET/SOCK_RAW/IPPROTO_RAW sockets on top of Ethernet L2.
  * Added support for setting Ethernet MAC address with net shell.
  * Added check for return values of the driver start/stop routines when
    bringing Ethernet interface up.
  * Added ``unknown_protocol`` statistic for packets with unrecognized protocol
    field, instead of using ``error`` for this purpose.

* HTTP:

  * Reworked HTTP headers: moved methods to a separate header, added status
    response codes header and grouped HTTP headers in a subdirectory.
  * Used :c:func:`zsock_poll` for HTTP timeout instead of a delayed work.

* ICMPv4:

  * Added support to autogenerate Echo Request payload.

* ICMPv6:

  * Added support to autogenerate Echo Request payload.
  * Fixed stats counting for ND packets.

* IEEE802154:

  * Improved short address support.
  * Improved IEEE802154 context thread safety.
  * Decoupled IEEE802154 parameters from :c:struct:`net_pkt` into
    :c:struct:`net_pkt_cb_ieee802154`.
  * Multiple other minor fixes/improvements.

* IPv4:

  * IPv4 packet fragmentation support has been added, this allows large packets
    to be split up before sending or reassembled during receive for packets that
    are larger than the network device MTU. This is disabled by default but can
    be enabled with :kconfig:option:`CONFIG_NET_IPV4_FRAGMENT`.
  * Added support for setting/reading DSCP/ECN fields.
  * Fixed packet leak in IPv4 address auto-configuration procedure.
  * Added support for configuring IPv4 addresses with ``net ipv4`` shell
    command.
  * Zephyr now adds IGMP all systems 224.0.0.1 address to all IPv4 network
    interfaces by default.

* IPv6:

  * Made it possible to add route to router's link local address.
  * Added support for setting/reading DSCP/ECN fields.
  * Improved test coverage for IPv6 fragmentation.
  * Added support for configuring IPv6 addresses with ``net ipv6`` shell
    command.
  * Added support for configuring IPv6 routes with ``net route`` shell
    command.

* LwM2M:

  * Renamed ``LWM2M_RD_CLIENT_EVENT_REG_UPDATE_FAILURE`` to
    :c:macro:`LWM2M_RD_CLIENT_EVENT_REG_TIMEOUT`. This event is now used in case
    of registration timeout.
  * Added new LwM2M APIs for historical data storage for LwM2M resource.
  * Updated LwM2M APIs to use ``const`` pointers when possible.
  * Added shell command to lock/unlock LwM2M registry.
  * Added shell command to enable historical data cache for a resource.
  * Switched to use ``zsock_*`` functions internally.
  * Added uCIFI LPWAN (ID 3412) object implementation.
  * Added BinaryAppDataContainer (ID 19) object implementation.
  * Deprecated :kconfig:option:`CONFIG_LWM2M_RD_CLIENT_SUPPORT`, as it's now
    considered as an integral part of the LwM2M library.
  * Added support for SenML Object Link data type.
  * Fixed a bug causing incorrect ordering of the observation paths.
  * Deprecated string based LwM2M APIs. LwM2M APIs now use
    :c:struct:`lwm2m_obj_path` to represent object/resource paths.
  * Refactored ``lwm2m_client`` sample by splitting specific functionalities
    into separate modules.
  * Multiple other minor fixes within the LwM2M library.

* Misc:

  * Updated various networking test suites to use the new ztest API.
  * Added redirect support for ``big_http_download`` sample and updated the
    server URL for TLS variant.
  * Fixed memory leak in ``net udp`` shell command.
  * Fixed cloning of LL address for :c:struct:`net_pkt`.
  * Added support for QoS and payload size setting in ``net ping`` shell
    command.
  * Added support for aborting ``net ping`` shell command.
  * Introduced carrier and dormant management on network interfaces. Separated
    interface administrative state from operational state.
  * Improved DHCPv4 behavior with multiple DHCPv4 servers in the network.
  * Fixed net_mgmt event size calculation.
  * Added :kconfig:option:`CONFIG_NET_LOOPBACK_MTU` option to configure loopback
    interface MTU.
  * Reimplemented the IP/UDP/TCP checksum calculation to speed up the
    processing.
  * Removed :kconfig:option:`CONFIG_NET_CONFIG_SETTINGS` use from test cases to
    improve test execution on real platforms.
  * Added MQTT-SN library and sample.
  * Fixed variable buffer length configuration
    (:kconfig:option:`CONFIG_NET_BUF_VARIABLE_DATA_SIZE`).
  * Fixed IGMPv2 membership report destination address.
  * Added mutex protection for the connection list handling.
  * Separated user data pointer from FIFO reserved space in
    :c:struct:`net_context`.
  * Added input validation for ``net pkt`` shell command.

* OpenThread:

  * Implemented PSA support for ECDSA API.
  * Fixed :c:func:`otPlatRadioSetMacKey` when asserts are disabled.
  * Depracated :c:func:`openthread_set_state_changed_cb` in favour of more
    generic :c:func:`openthread_state_changed_cb_register`.
  * Implemented diagnostic GPIO commands.

* SNTP:

  * Switched to use ``zsock_*`` functions internally.
  * Fixed the library operation with IPv4 disabled.

* Sockets:

  * Fixed a possible memory leak on failed TLS socket creation.

* TCP:

  * Extended the default TCP out-of-order receive queue timeout to 2 seconds.
  * Reimplemented TCP ref counting, to prevent situation, where TCP connection
    context could be released prematurely.

* Websockets:

  * Reimplemented websocket receive routine to fix several issues.
  * Implemented proper websocket close procedure.
  * Fixed a bug where websocket would overwrite the mutex used by underlying TCP
    socket.

* Wi-Fi:

  * Added support for power save configuration.
  * Added support for regulatory domain configuration.
  * Added support for power save timeout configuration.

* zperf

  * Added option to set QoS for zperf.
  * Fixed out of order/lost packets statistics.
  * Defined a public API for the library to allow throughput measurement without shell enabled.
  * Added an option for asynchronous upload.

USB
***

Devicetree
**********

* Bindings

  * New:

    * :dtcompatible:`zephyr,flash-disk`

    * STM32 SoCs:

      * :dtcompatible: `st,stm32-lse-clock`: new ``lse-bypass`` property
      * :dtcompatible: `st,stm32-ethernet`: now allows ``local-mac-address`` and
         ``zephyr,random-mac-address`` properties.

    * GD32 SoCs:

      * :dtcompatible: `gd,gd322-dma`: Provide some helper macro to easily setup `dma-cells`.

Libraries / Subsystems
**********************

* C Library

  * Newlib nano variant is no longer selected by default when
    :kconfig:option:`CONFIG_NEWLIB_LIBC` is selected.
    :kconfig:option:`CONFIG_NEWLIB_LIBC_NANO` must now be explicitly selected
    in order to use the nano variant.
  * Picolibc now supports all architectures supported by Zephyr.
  * Added C11 ``aligned_alloc`` support to the minimal libc.

* C++ Library

  * C++ support in Zephyr is no longer considered a "subsystem" because it
    mainly consists of the C++ ABI runtime library and the C++ standard
    library, which are "libraries" that are dissimilar to the existing Zephyr
    subsystems. C++ support components are now located in ``lib/cpp`` as
    "C++ library."
  * C++ ABI runtime library components such as global constructor/destructor
    and initialiser handlers, that were previously located under
    ``subsys/cpp``, have been moved to ``lib/cpp/abi`` in order to provide a
    clear separation between the C++ ABI runtime library and the C++ standard
    library.
  * C++ minimal library components have been moved to ``lib/cpp/minimal``.
  * C++ tests have been moved to ``tests/lib/cpp``.
  * C++ samples have been moved to ``samples/cpp``.
  * :kconfig:option:`CONFIG_CPLUSPLUS` has been renamed to
    :kconfig:option:`CONFIG_CPP`.
  * :kconfig:option:`CONFIG_EXCEPTIONS` has been renamed to
    :kconfig:option:`CONFIG_CPP_EXCEPTIONS`.
  * :kconfig:option:`CONFIG_RTTI` has been renamed to
    :kconfig:option:`CONFIG_CPP_RTTI`.
  * :kconfig:option:`CONFIG_LIB_CPLUSPLUS` is deprecated. A toolchain-specific
    C++ standard library Kconfig option from
    :kconfig:option:`CONFIG_LIBCPP_IMPLEMENTATION` should be selected instead.
  * Zephyr subsystems and modules that require the features from the full C++
    standard library (e.g. Standard Template Library) can now select
    :kconfig:option:`CONFIG_REQUIRES_FULL_LIBC`, which automatically selects
    a compatible C++ standard library.
  * Introduced :kconfig:option:`CONFIG_CPP_MAIN` to support defining ``main()``
    function in a C++ source file. Enabling this option makes the Zephyr kernel
    invoke ``int main(void)``, which is required by the ISO C++ standards, as
    opposed to the Zephyr default ``void main(void)``.
  * Added no-throwing implementation of new operator to the C++ minimal
    library.
  * Added support for new operator with alignment request (C++17) to the C++
    minimal library.
  * Added GNU C++ standard library support with Picolibc when using a suitably
    configured toolchain (e.g. the upcoming Zephyr SDK 0.16.0 release).

* Cache

  * Introduced new Cache API
  * ``CONFIG_HAS_ARCH_CACHE`` has been renamed to
    :kconfig:option:`CONFIG_ARCH_CACHE`
  * ``CONFIG_HAS_EXTERNAL_CACHE`` has been renamed to
    :kconfig:option:`CONFIG_EXTERNAL_CACHE`

* File systems

  * Added new API call `fs_mkfs`.
  * Added new sample `samples/subsys/fs/format`.
  * FAT FS driver has been updated to version 0.15 w/patch1.
  * Added the option to disable CRC checking in :ref:`fcb_api` by enabling the
    Kconfig option :kconfig:option:`CONFIG_FCB_ALLOW_FIXED_ENDMARKER`
    and setting the `FCB_FLAGS_CRC_DISABLED` flag in the :c:struct:`fcb` struct.

* IPC

  * Added :c:func:`ipc_rpmsg_deinit`, :c:func:`ipc_service_close_instance` and
    :c:func:`ipc_static_vrings_deinit`  functions
  * Added deregister API support for icmsg backend
  * Added a multi-endpoint feature to icmsg backend
  * Added no-copy features to icmsg backend

* ISO-TP

  * Rewrote the ISO-TP API to not reuse definitions from the CAN controller API.

* Management

  * MCUmgr functionality deprecated in 3.1 has been removed:
    CONFIG_FS_MGMT_UL_CHUNK_SIZE, CONFIG_IMG_MGMT_UL_CHUNK_SIZE,
    CONFIG_OS_MGMT_ECHO_LENGTH
  * MCUmgr fs_mgmt issue with erasing a file prior to writing the first block
    of data has been worked around by only truncating/deleting the file data
    if the file exists. This can help work around an issue whereby logging is
    enabled and the command is sent on the same UART as the logging system, in
    which a filesystem error was emitted.
  * A MCUmgr bug when using the smp_svr sample with Bluetooth transport that
    could have caused a stack overflow has been fixed.
  * A MCUmgr issue with Bluetooth transport that could cause a deadlock of the
    mcumgr thread if the remote device disconnected before the output message
    was sent has been fixed.
  * A MCUmgr img_mgmt bug whereby the state of an image upload could persist
    when it was no longer valid (e.g. after an image erase command) has been
    fixed.
  * MCUmgr fs_mgmt command has been added that allows querying/listing the
    supported hash/checksum types.
  * MCUmgr Bluetooth transport will now clear unprocessed commands sent if a
    remote device disconnects instead of processing them.
  * A new MCUmgr transport function pointer has been added which needs
    registering in ``smp_transport_init`` for removing invalid packets for
    connection-orientated transports. If this is unimplemented, the function
    pointer can be set to NULL.
  * MCUmgr command handler definitions have changed, the ``mgmt_ctxt`` struct
    has been replaced with the ``smp_streamer`` struct, the zcbor objects need
    to replace ``cnbe`` object access with ``writer`` and ``cnbd`` object
    access with ``reader`` to successfully build.
  * MCUmgr callback system has been reworked with a unified singular interface
    which supports status passing to the handler (:ref:`mcumgr_callbacks`).
  * MCUmgr subsystem directory structure has been flattened and contents of the
    lib subdirectory has been redistributed into following directories:

    .. table::
       :align: center

       +----------------+-------------------------------------------+
       | Subdirectory   | MCUmgr area                               |
       +================+===========================================+
       | mgmt           | MCUmgr management functions, group        |
       |                | registration, and so on;                  |
       +----------------+-------------------------------------------+
       | smp            | Simple Management Protocol processing;    |
       +----------------+-------------------------------------------+
       | transport      | Transport support and transport API;      |
       +----------------+-------------------------------------------+
       | grp            | Command groups, formerly lib/cmd;         |
       |                | each group, which has Zephyr built in     |
       |                | support has its own directory here;       |
       +----------------+-------------------------------------------+
       | util           | Utilities used by various subareas of     |
       |                | MCUmgr.                                   |
       +----------------+-------------------------------------------+

    Public API interfaces for above areas are now exported through zephyr_interface,
    and headers for them reside in ``zephyr/mgmt/mcumgr/<mcumgr_subarea>/``.
    For example to access mgmt API include ``<zephyr/mgmt/mcumgr/mgmt/mgmt.h>``.

    Private headers for above areas can be accessed, when required, using paths:
    ``mgmt/mcumgr/mgmt/<mcumgr_subarea>/``.
  * MCUmgr os_mgmt info command has been added that allows querying details on
    the kernel and application, allowing application-level extensibility
    see :ref:`mcumgr_os_application_info` for details.

  * MCUMgr :kconfig:option:`CONFIG_APP_LINK_WITH_MCUMGR` has been removed as
    it has not been doing anything.

  * MCUmgr Kconfig option names have been standardised. Script
    :zephyr_file:`scripts/utils/migrate_mcumgr_kconfigs.py` has been provided
    to make transition to new Kconfig options easier.
    Below table provides information on old names and new equivalents:

    .. table::
       :align: center

       +------------------------------------------------+-------------------------------------------------------+
       | Old Kconfig option name                        | New Kconfig option name                               |
       +================================================+=======================================================+
       | MCUMGR_SMP_WORKQUEUE_STACK_SIZE                | MCUMGR_TRANSPORT_WORKQUEUE_STACK_SIZE                 |
       +------------------------------------------------+-------------------------------------------------------+
       | MCUMGR_SMP_WORKQUEUE_THREAD_PRIO               | MCUMGR_TRANSPORT_WORKQUEUE_THREAD_PRIO                |
       +------------------------------------------------+-------------------------------------------------------+
       | MGMT_MAX_MAIN_MAP_ENTRIES                      | MCUMGR_SMP_CBOR_MAX_MAIN_MAP_ENTRIES                  |
       +------------------------------------------------+-------------------------------------------------------+
       | MGMT_MIN_DECODING_LEVELS                       | MCUMGR_SMP_CBOR_MIN_DECODING_LEVELS                   |
       +------------------------------------------------+-------------------------------------------------------+
       | MGMT_MIN_DECODING_LEVEL_1                      | MCUMGR_SMP_CBOR_MIN_DECODING_LEVEL_1                  |
       +------------------------------------------------+-------------------------------------------------------+
       | MGMT_MIN_DECODING_LEVEL_2                      | MCUMGR_SMP_CBOR_MIN_DECODING_LEVEL_2                  |
       +------------------------------------------------+-------------------------------------------------------+
       | MGMT_MIN_DECODING_LEVEL_3                      | MCUMGR_SMP_CBOR_MIN_DECODING_LEVEL_3                  |
       +------------------------------------------------+-------------------------------------------------------+
       | MGMT_MIN_DECODING_LEVEL_4                      | MCUMGR_SMP_CBOR_MIN_DECODING_LEVEL_4                  |
       +------------------------------------------------+-------------------------------------------------------+
       | MGMT_MIN_DECODING_LEVEL_5                      | MCUMGR_SMP_CBOR_MIN_DECODING_LEVEL_5                  |
       +------------------------------------------------+-------------------------------------------------------+
       | MGMT_MAX_DECODING_LEVELS                       | MCUMGR_SMP_CBOR_MAX_DECODING_LEVELS                   |
       +------------------------------------------------+-------------------------------------------------------+
       | MCUMGR_CMD_FS_MGMT                             | MCUMGR_GRP_FS                                         |
       +------------------------------------------------+-------------------------------------------------------+
       | FS_MGMT_MAX_FILE_SIZE_64KB                     | MCUMGR_GRP_FS_MAX_FILE_SIZE_64KB                      |
       +------------------------------------------------+-------------------------------------------------------+
       | FS_MGMT_MAX_FILE_SIZE_4GB                      | MCUMGR_GRP_FS_MAX_FILE_SIZE_4GB                       |
       +------------------------------------------------+-------------------------------------------------------+
       | FS_MGMT_MAX_OFFSET_LEN                         | MCUMGR_GRP_FS_MAX_OFFSET_LEN                          |
       +------------------------------------------------+-------------------------------------------------------+
       | FS_MGMT_DL_CHUNK_SIZE_LIMIT                    | MCUMGR_GRP_FS_DL_CHUNK_SIZE_LIMIT                     |
       +------------------------------------------------+-------------------------------------------------------+
       | FS_MGMT_DL_CHUNK_SIZE                          | MCUMGR_GRP_FS_DL_CHUNK_SIZE                           |
       +------------------------------------------------+-------------------------------------------------------+
       | FS_MGMT_FILE_STATUS                            | MCUMGR_GRP_FS_FILE_STATUS                             |
       +------------------------------------------------+-------------------------------------------------------+
       | FS_MGMT_CHECKSUM_HASH                          | MCUMGR_GRP_FS_CHECKSUM_HASH                           |
       +------------------------------------------------+-------------------------------------------------------+
       | FS_MGMT_CHECKSUM_HASH_CHUNK_SIZE               | MCUMGR_GRP_FS_CHECKSUM_HASH_CHUNK_SIZE                |
       +------------------------------------------------+-------------------------------------------------------+
       | FS_MGMT_CHECKSUM_IEEE_CRC32                    | MCUMGR_GRP_FS_CHECKSUM_IEEE_CRC32                     |
       +------------------------------------------------+-------------------------------------------------------+
       | FS_MGMT_HASH_SHA256                            | MCUMGR_GRP_FS_HASH_SHA256                             |
       +------------------------------------------------+-------------------------------------------------------+
       | FS_MGMT_FILE_ACCESS_HOOK                       | MCUMGR_GRP_FS_FILE_ACCESS_HOOK                        |
       +------------------------------------------------+-------------------------------------------------------+
       | FS_MGMT_PATH_SIZE                              | MCUMGR_GRP_FS_PATH_LEN                                |
       +------------------------------------------------+-------------------------------------------------------+
       | MCUMGR_CMD_IMG_MGMT                            | MCUMGR_GRP_IMG                                        |
       +------------------------------------------------+-------------------------------------------------------+
       | IMG_MGMT_USE_HEAP_FOR_FLASH_IMG_CONTEXT        | MCUMGR_GRP_IMG_USE_HEAP_FOR_FLASH_IMG_CONTEXT         |
       +------------------------------------------------+-------------------------------------------------------+
       | IMG_MGMT_UPDATABLE_IMAGE_NUMBER                | MCUMGR_GRP_IMG_UPDATABLE_IMAGE_NUMBER                 |
       +------------------------------------------------+-------------------------------------------------------+
       | IMG_MGMT_VERBOSE_ERR                           | MCUMGR_GRP_IMG_VERBOSE_ERR                            |
       +------------------------------------------------+-------------------------------------------------------+
       | IMG_MGMT_DUMMY_HDR                             | MCUMGR_GRP_IMG_DUMMY_HDR                              |
       +------------------------------------------------+-------------------------------------------------------+
       | IMG_MGMT_DIRECT_IMAGE_UPLOAD                   | MCUMGR_GRP_IMG_DIRECT_UPLOAD                          |
       +------------------------------------------------+-------------------------------------------------------+
       | IMG_MGMT_REJECT_DIRECT_XIP_MISMATCHED_SLOT     | MCUMGR_GRP_IMG_REJECT_DIRECT_XIP_MISMATCHED_SLOT      |
       +------------------------------------------------+-------------------------------------------------------+
       | IMG_MGMT_FRUGAL_LIST                           | MCUMGR_GRP_IMG_FRUGAL_LIST                            |
       +------------------------------------------------+-------------------------------------------------------+
       | MCUMGR_CMD_OS_MGMT                             | MCUMGR_GRP_OS                                         |
       +------------------------------------------------+-------------------------------------------------------+
       | MCUMGR_GRP_OS_OS_RESET_HOOK                    | MCUMGR_GRP_OS_RESET_HOOK                              |
       +------------------------------------------------+-------------------------------------------------------+
       | OS_MGMT_RESET_MS                               | MCUMGR_GRP_OS_RESET_MS                                |
       +------------------------------------------------+-------------------------------------------------------+
       | OS_MGMT_TASKSTAT                               | MCUMGR_GRP_OS_TASKSTAT                                |
       +------------------------------------------------+-------------------------------------------------------+
       | OS_MGMT_TASKSTAT_ONLY_SUPPORTED_STATS          | MCUMGR_GRP_OS_TASKSTAT_ONLY_SUPPORTED_STATS           |
       +------------------------------------------------+-------------------------------------------------------+
       | OS_MGMT_TASKSTAT_MAX_NUM_THREADS               | MCUMGR_GRP_OS_TASKSTAT_MAX_NUM_THREADS                |
       +------------------------------------------------+-------------------------------------------------------+
       | OS_MGMT_TASKSTAT_THREAD_NAME_LEN               | MCUMGR_GRP_OS_TASKSTAT_THREAD_NAME_LEN                |
       +------------------------------------------------+-------------------------------------------------------+
       | OS_MGMT_TASKSTAT_SIGNED_PRIORITY               | MCUMGR_GRP_OS_TASKSTAT_SIGNED_PRIORITY                |
       +------------------------------------------------+-------------------------------------------------------+
       | OS_MGMT_TASKSTAT_STACK_INFO                    | MCUMGR_GRP_OS_TASKSTAT_STACK_INFO                     |
       +------------------------------------------------+-------------------------------------------------------+
       | OS_MGMT_ECHO                                   | MCUMGR_GRP_OS_ECHO                                    |
       +------------------------------------------------+-------------------------------------------------------+
       | OS_MGMT_MCUMGR_PARAMS                          | MCUMGR_GRP_OS_MCUMGR_PARAMS                           |
       +------------------------------------------------+-------------------------------------------------------+
       | MCUMGR_CMD_SHELL_MGMT                          | MCUMGR_GRP_SHELL                                      |
       +------------------------------------------------+-------------------------------------------------------+
       | MCUMGR_CMD_SHELL_MGMT_LEGACY_RC_RETURN_CODE    | MCUMGR_GRP_SHELL_LEGACY_RC_RETURN_CODE                |
       +------------------------------------------------+-------------------------------------------------------+
       | MCUMGR_CMD_STAT_MGMT                           | MCUMGR_GRP_STAT                                       |
       +------------------------------------------------+-------------------------------------------------------+
       | STAT_MGMT_MAX_NAME_LEN                         | MCUMGR_GRP_STAT_MAX_NAME_LEN                          |
       +------------------------------------------------+-------------------------------------------------------+
       | MCUMGR_GRP_ZEPHYR_BASIC                        | MCUMGR_GRP_ZBASIC                                     |
       +------------------------------------------------+-------------------------------------------------------+
       | MCUMGR_GRP_BASIC_CMD_STORAGE_ERASE             | MCUMGR_GRP_ZBASIC_STORAGE_ERASE                       |
       +------------------------------------------------+-------------------------------------------------------+
       | MGMT_VERBOSE_ERR_RESPONSE                      | MCUMGR_SMP_VERBOSE_ERR_RESPONSE                       |
       +------------------------------------------------+-------------------------------------------------------+
       | MCUMGR_SMP_REASSEMBLY                          | MCUMGR_TRANSPORT_REASSEMBLY                           |
       +------------------------------------------------+-------------------------------------------------------+
       | MCUMGR_BUF_COUNT                               | MCUMGR_TRANSPORT_NETBUF_COUNT                         |
       +------------------------------------------------+-------------------------------------------------------+
       | MCUMGR_BUF_SIZE                                | MCUMGR_TRANSPORT_NETBUF_SIZE                          |
       +------------------------------------------------+-------------------------------------------------------+
       | MCUMGR_BUF_USER_DATA_SIZE                      | MCUMGR_TRANSPORT_NETBUF_USER_DATA_SIZE                |
       +------------------------------------------------+-------------------------------------------------------+
       | MCUMGR_SMP_BT                                  | MCUMGR_TRANSPORT_BT                                   |
       +------------------------------------------------+-------------------------------------------------------+
       | MCUMGR_SMP_REASSEMBLY_BT                       | MCUMGR_TRANSPORT_BT_REASSEMBLY                        |
       +------------------------------------------------+-------------------------------------------------------+
       | MCUMGR_SMP_REASSEMBLY_UNIT_TESTS               | MCUMGR_TRANSPORT_REASSEMBLY_UNIT_TESTS                |
       +------------------------------------------------+-------------------------------------------------------+
       | MCUMGR_SMP_BT_AUTHEN                           | MCUMGR_TRANSPORT_BT_AUTHEN                            |
       +------------------------------------------------+-------------------------------------------------------+
       | MCUMGR_SMP_BT_CONN_PARAM_CONTROL               | MCUMGR_TRANSPORT_BT_CONN_PARAM_CONTROL                |
       +------------------------------------------------+-------------------------------------------------------+
       | MCUMGR_SMP_BT_CONN_PARAM_CONTROL_MIN_INT       | MCUMGR_TRANSPORT_BT_CONN_PARAM_CONTROL_MIN_INT        |
       +------------------------------------------------+-------------------------------------------------------+
       | MCUMGR_SMP_BT_CONN_PARAM_CONTROL_MAX_INT       | MCUMGR_TRANSPORT_BT_CONN_PARAM_CONTROL_MAX_INT        |
       +------------------------------------------------+-------------------------------------------------------+
       | MCUMGR_SMP_BT_CONN_PARAM_CONTROL_LATENCY       | MCUMGR_TRANSPORT_BT_CONN_PARAM_CONTROL_LATENCY        |
       +------------------------------------------------+-------------------------------------------------------+
       | MCUMGR_SMP_BT_CONN_PARAM_CONTROL_TIMEOUT       | MCUMGR_TRANSPORT_BT_CONN_PARAM_CONTROL_TIMEOUT        |
       +------------------------------------------------+-------------------------------------------------------+
       | MCUMGR_SMP_BT_CONN_PARAM_CONTROL_RESTORE_TIME  | MCUMGR_TRANSPORT_BT_CONN_PARAM_CONTROL_RESTORE_TIME   |
       +------------------------------------------------+-------------------------------------------------------+
       | MCUMGR_SMP_BT_CONN_PARAM_CONTROL_RETRY_TIME    | MCUMGR_TRANSPORT_BT_CONN_PARAM_CONTROL_RETRY_TIME     |
       +------------------------------------------------+-------------------------------------------------------+
       | MCUMGR_SMP_DUMMY                               | MCUMGR_TRANSPORT_DUMMY                                |
       +------------------------------------------------+-------------------------------------------------------+
       | MCUMGR_SMP_DUMMY_RX_BUF_SIZE                   | MCUMGR_TRANSPORT_DUMMY_RX_BUF_SIZE                    |
       +------------------------------------------------+-------------------------------------------------------+
       | MCUMGR_SMP_SHELL                               | MCUMGR_TRANSPORT_SHELL                                |
       +------------------------------------------------+-------------------------------------------------------+
       | MCUMGR_SMP_SHELL_MTU                           | MCUMGR_TRANSPORT_SHELL_MTU                            |
       +------------------------------------------------+-------------------------------------------------------+
       | MCUMGR_SMP_SHELL_RX_BUF_COUNT                  | MCUMGR_TRANSPORT_SHELL_RX_BUF_COUNT                   |
       +------------------------------------------------+-------------------------------------------------------+
       | MCUMGR_SMP_UART                                | MCUMGR_TRANSPORT_UART                                 |
       +------------------------------------------------+-------------------------------------------------------+
       | MCUMGR_SMP_UART_ASYNC                          | MCUMGR_TRANSPORT_UART_ASYNC                           |
       +------------------------------------------------+-------------------------------------------------------+
       | MCUMGR_SMP_UART_ASYNC_BUFS                     | MCUMGR_TRANSPORT_UART_ASYNC_BUFS                      |
       +------------------------------------------------+-------------------------------------------------------+
       | MCUMGR_SMP_UART_ASYNC_BUF_SIZE                 | MCUMGR_TRANSPORT_UART_ASYNC_BUF_SIZE                  |
       +------------------------------------------------+-------------------------------------------------------+
       | MCUMGR_SMP_UART_MTU                            | MCUMGR_TRANSPORT_UART_MTU                             |
       +------------------------------------------------+-------------------------------------------------------+
       | MCUMGR_SMP_UDP                                 | MCUMGR_TRANSPORT_UDP                                  |
       +------------------------------------------------+-------------------------------------------------------+
       | MCUMGR_SMP_UDP_IPV4                            | MCUMGR_TRANSPORT_UDP_IPV4                             |
       +------------------------------------------------+-------------------------------------------------------+
       | MCUMGR_SMP_UDP_IPV6                            | MCUMGR_TRANSPORT_UDP_IPV6                             |
       +------------------------------------------------+-------------------------------------------------------+
       | MCUMGR_SMP_UDP_PORT                            | MCUMGR_TRANSPORT_UDP_PORT                             |
       +------------------------------------------------+-------------------------------------------------------+
       | MCUMGR_SMP_UDP_STACK_SIZE                      | MCUMGR_TRANSPORT_UDP_STACK_SIZE                       |
       +------------------------------------------------+-------------------------------------------------------+
       | MCUMGR_SMP_UDP_THREAD_PRIO                     | MCUMGR_TRANSPORT_UDP_THREAD_PRIO                      |
       +------------------------------------------------+-------------------------------------------------------+
       | MCUMGR_SMP_UDP_MTU                             | MCUMGR_TRANSPORT_UDP_MTU                              |
       +------------------------------------------------+-------------------------------------------------------+

  * MCUmgr responses where ``rc`` (result code) is 0 (no error) will no longer
    be present in responses and in cases where there is only an ``rc`` result,
    the resultant response will now be an empty CBOR map. The old behaviour can
    be restored by enabling
    :kconfig:option:`CONFIG_MCUMGR_SMP_LEGACY_RC_BEHAVIOUR`.

  * MCUmgr now has log outputting on most errors from the included fs, img,
    os, shell, stat and zephyr_basic group commands. The level of logging can be
    controlled by adjusting: :kconfig:option:`CONFIG_MCUMGR_GRP_FS_LOG_LEVEL`,
    :kconfig:option:`CONFIG_MCUMGR_GRP_IMG_LOG_LEVEL`,
    :kconfig:option:`CONFIG_MCUMGR_GRP_OS_LOG_LEVEL`,
    :kconfig:option:`CONFIG_MCUMGR_GRP_SHELL_LOG_LEVEL`,
    :kconfig:option:`CONFIG_MCUMGR_GRP_STAT_LOG_LEVEL` and
    :kconfig:option:`CONFIG_MCUMGR_GRP_ZBASIC_LOG_LEVEL`.

  * MCUmgr img_mgmt has a new field which is sent in the final packet (if
    :kconfig:option:`CONFIG_IMG_ENABLE_IMAGE_CHECK` is enabled) named ``match``
    which is a boolean and is true if the uploaded data matches the supplied
    hash, or false otherwise.

  * MCUmgr img_mgmt will now skip receiving data if the provided hash already
    matches the hash of the data present (if
    :kconfig:option:`CONFIG_IMG_ENABLE_IMAGE_CHECK` is enabled) and finish the
    upload operation request instantly.

  * MCUmgr img_mgmt structs are now packed, which fixes a fault issue on
    processors that do not support unaligned memory access.

  * If MCUmgr is used with the shell transport and ``printk()`` functionality
    is used, there can be an issue whereby the ``printk()`` calls output during
    a MCUmgr frame receive, this has been fixed by default in zephyr by routing
    ``printk()`` calls to the logging system, For user applications,
    :kconfig:option:`CONFIG_LOG_PRINTK` should be enabled to include this fix.

  * A bug when MCUmgr shell transport is used (issue was observed over USB CDC
    but could also occur with UART) whereby the default shell receive ring
    buffer is insufficient has been fixed by making the default size 256 bytes
    instead of 64 when the shell MCUmgr transport is selected.

* LwM2M

  * The ``lwm2m_senml_cbor_*`` files have been regenerated using zcbor 0.6.0.

* POSIX API

  * Harmonize posix type definitions across the minimal libc, newlib and picolibc.

    * Abstract ``pthread_t``, ``pthread_key_t``, ``pthread_cond_t``,
      ``pthread_mutex_t``, as ``uint32_t``.
    * Define :c:macro:`PTHREAD_KEY_INITIALIZER`, :c:macro:`PTHREAD_COND_INITIALIZER`,
      :c:macro:`PTHREAD_MUTEX_INITIALIZER` to align with POSIX 1003.1.

  * Allow non-prefixed standard include paths with :kconfig:option:`CONFIG_POSIX_API`.

    * I.e. ``#include <unistd.h>`` instead of ``#include <zephyr/posix/unistd.h>``.
    * Primarily to ease integration with external libraries.
    * Internal Zephyr code should continue to use prefixed header paths.

  * Enable ``eventfd()``, ``getopt()`` by default with :kconfig:option:`CONFIG_POSIX_API`.
  * Move / rename header files to align with POSIX specifications.

    * E.g. move ``fcntl.h``, ``sys/stat.h`` from the minimal libc into the
      ``include/zephyr/posix`` directory. Rename ``posix_sched.h`` to ``sched.h``.
    * Move :c:macro:`O_ACCMODE`, :c:macro:`O_RDONLY`, :c:macro:`O_WRONLY`,
      :c:macro:`O_WRONLY`, to ``fcntl.h``.

  * Added :kconfig:option:`CONFIG_TIMER_CREATE_WAIT`, :kconfig:option:`CONFIG_MAX_PTHREAD_KEY_COUNT`,
    :kconfig:option:`CONFIG_MAX_PTHREAD_COND_COUNT`, :kconfig:option:`CONFIG_MAX_PTHREAD_MUTEX_COUNT`.
  * Define :c:macro:`SEEK_SET`, :c:macro:`SEEK_CUR`, :c:macro:`SEEK_END`.

* SD Subsystem

  * Added support for eMMC protocol in Zephyr.

    - Speed modes up to HS400 are supported using 1.8v operation.
    - Additional protocol tests have been added to verify eMMC functionality.
    - Disk subsystem tests have been updated to function with eMMC.

  * Card and host combinations that cannot utilize UHS (ultra high speed) mode
    will now use 4 bit bus width when possible. This will greatly improve
    performance for these systems.

* Settings

  * Replaced all :c:func:`k_panic` invocations within settings backend
    initialization with returning / propagating error codes.

* Utilities

  * Added the linear range API to map values in a linear range to a range index
    :zephyr_file:`include/zephyr/sys/linear_range.h`.

HALs
****

* GigaDevice

  * Added support for gd32l23x.
  * Added support for gd32a50x.

MCUboot
*******

Storage
*******

* Flash Map API drops ``fa_device_id`` from :c:struct:`flash_area`, as it
  is no longer needed by MCUboot, and has not been populated for a long
  time now.

Trusted Firmware-M
******************

zcbor
*****

Upgraded zcbor to 0.6.0. Among other things, this brings in a few convenient
changes for Zephyr:

* In the zcbor codebase, the ``ARRAY_SIZE`` macro has been renamed to
  ``ZCBOR_ARRAY_SIZE`` to not collide with Zephyr's :c:macro:`ARRAY_SIZE` macro.
* The zcbor codebase now better supports being used in C++ code.

The entire release notes can be found at
https://github.com/zephyrproject-rtos/zcbor/blob/0.6.0/RELEASE_NOTES.md

Documentation
*************

* Upgraded to Doxygen 1.9.6.
* It is now possible to link to Kconfig search results.

Tests and Samples
*****************

Issue Related Items
*******************

Known Issues
============

Addressed issues
================
