:orphan:

.. _zephyr_3.3:

Zephyr 3.3.0
############

We are pleased to announce the release of Zephyr version 3.3.0.

Major enhancements with this release include:

* Introduced :ref:`Fuel Gauge <fuel_gauge_api>` subsystem for battery level
  monitoring.
* Introduced :ref:`USB-C <usbc_api>` device stack with PD (power delivery)
  support.
* Introduced :ref:`DSP (digital signal processing) <zdsp_api>` subsystem with
  CMSIS-DSP as the default backend.
* Added Picolibc support for all architectures when using Zephyr SDK.

The following sections provide detailed lists of changes by component.

Security Vulnerability Related
******************************

The following CVEs are addressed by this release:

More detailed information can be found in:
https://docs.zephyrproject.org/latest/security/vulnerabilities.html

* CVE-2023-0359: Under embargo until 2023-04-20

* CVE-2023-0779: Under embargo until 2023-04-22


API Changes
***********

* Emulator creation APIs have changed to better match
  :c:macro:`DEVICE_DT_DEFINE`. It also includes a new backend API pointer to
  allow sensors to share common APIs for more generic tests.

Changes in this release
=======================

* Newlib nano variant is no longer selected by default when
  :kconfig:option:`CONFIG_NEWLIB_LIBC` is selected.
  :kconfig:option:`CONFIG_NEWLIB_LIBC_NANO` must now be explicitly selected in
  order to use the nano variant.

* Bluetooth: Added extra options to bt_le_per_adv_sync_transfer_subscribe to
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
  are now part of ``<zephyr/drivers/regulator.h>``.

* Starting from this release ``zephyr-`` prefixed tags won't be created
  anymore. The project will continue using ``v`` tags, for example ``v3.3.0``.

* Bluetooth: Deprecated the Bluetooth logging subsystem in favor of the Zephyr
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
  application code, these will now automatically be registered at boot-up (this
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

* New :ref:`Zephyr message bus (Zbus) <zbus>` subsystem added; a message-oriented
  bus that enables one-to-one, one-to-many and many-to-many communication
  between threads.

* zTest now supports controlling test summary printouts via the
  :kconfig:option:`CONFIG_ZTEST_SUMMARY`. This Kconfig can be set to ``n`` for
  less verbose test output.

* Emulators now support a backend API pointer which allows a single class of
  devices to provide similar emulated functionality. This can be used to write
  a single test for the class of devices and testing various boards using
  different chips.

Removed APIs in this release
============================

* Removed :kconfig:option:`CONFIG_COUNTER_RTC_STM32_LSE_DRIVE*`
  This should now be configured using the ``driving_capability`` property of
  LSE clock

* Removed :kconfig:option:`CONFIG_COUNTER_RTC_STM32_LSE_BYPASS`
  This should now be configured using the new ``lse_bypass`` property of
  LSE clock

* Removed :kconfig:option:`CONFIG_COUNTER_RTC_STM32_BACKUP_DOMAIN_RESET`. Its purpose
  was to control the reset of the counter value at board reset. It is removed since
  it has too wide scope (full Backup RAM reset). Replaced by
  :kconfig:option:`CONFIG_COUNTER_RTC_STM32_SAVE_VALUE_BETWEEN_RESETS` which also
  allows to control the reset of counter value, with an opposite logic.

* Removed deprecated tinycbor module, code that uses this module should be
  updated to use zcbor as a replacement.

* Removed deprecated GPIO flags used for setting debounce, drive strength and
  voltage level. All drivers now use vendor-specific flags as needed.

* Removed deprecated ``UTIL_LISTIFY`` helper macro.

* Removed deprecated ``pwm_pin*`` family of functions from the PWM API.

* Removed deprecated ``nvs_init`` function from the NVS filesystem API.

* Removed deprecated ``DT_CHOSEN_*_LABEL`` helper macros.

* Removed deprecated property ``enable-pin-remap`` from  :dtcompatible: `st,stm32-usb`:.
  ``remap-pa11-pa12`` from :dtcompatible: `st-stm32-pinctrl`: should now be used.

Deprecated in this release
==========================

* ``xtools`` toolchain variant is now deprecated. When using a
  custom toolchain built with Crosstool-NG, the
  :ref:`cross-compile toolchain variant <other_x_compilers>` should be used instead.

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
  prefix and replacing them with prefix-less variants.
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
  are removed. Related IRQ priorities should now be configured in device tree.

* `PWM_STM32_COMPLEMENTARY` deprecated in favor of `STM32_PWM_COMPLEMENTARY`.

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

* Added an "EARLY" init level that runs immediately on entry to z_cstart()

* Refactored the internal CPU count API to allow for runtime changes

* Added support for defining application main() in C++ code

* Fixed a race condition on SMP when pending threads where a second CPU
  could attempt to run a thread before the pending thread had finished
  the context switch.

Architectures
*************

* ARC

  * Fixed & reworked interrupt management (enabling / disabling) for the SMP systems
  * Added TLS (thread-local storage) for ARC MWDT toolchain
  * Fixed & rework irq_offload implementation
  * Fixed multiple logging & cbprintf issues for ARCv3 64bit
  * Added XIP support with MWDT toolchain
  * Improved DSP support, add DSP and AGU context save / restore
  * Added XY memory support for ARC DSP targets
  * Added architectures-specific DSP tests
  * Added additional compile-time checks for unsupported configuration: ARC_FIRQ + ARC_HAS_SECURE
  * Added support for using ``__auto_type`` type for ARC MWDT toolchain
  * Added support for using ``_Generic`` and ``__fallthrough`` keywords for ARC MWDT toolchain
  * Bumped minimal required ARC MWDT version to 2022.09
  * Fixed & reworked inclusion of C/C++ headers for ARC MWDT toolchain which cased build issue with
    C++

* ARM

  * More precise 'reason' codes are now returned in the fault handler.
  * Cache functions now use proper ``sys_*`` functions.
  * Renamed default RAM region from ``SRAM`` to ``RAM``.

* ARM64

  * Implemented ASID support for ARM64 MMU

* RISC-V

  * Converted :kconfig:option:`CONFIG_MP_NUM_CPUS` to
    :kconfig:option:`CONFIG_MP_MAX_NUM_CPUS`.

  * Added support for hardware register stacking/unstacking during ISRs and
    exceptions.

  * Added support for overriding :c:func:`arch_irq_lock`,
    :c:func:`arch_irq_unlock` and :c:func:`arch_irq_unlocked`.

  * Zephyr CPU number is now decoupled from the hart ID.

  * Secondary boot code is no longer included when
    :kconfig:option:`CONFIG_MP_MAX_NUM_CPUS` equals ``1``.

  * IPIs are no longer hardcoded to :c:func:`z_sched_ipi`.

  * Implemented an on-demand context switching algorithm for thread FPU
    accesses.

  * Enabled booting from non-zero indexed RISC-V harts with
    :kconfig:option:`CONFIG_RV_BOOT_HART`.

  * Hart IDs are now mapped to Zephyr CPUs with the devicetree.

  * Added a workaround for ``MTVAL`` not updating properly on QEMU-based
    platforms.

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
  * Changed the policy for advertising restart after disconnection, which is now
    done only for connections in the peripheral role.
  * Added a guard to prevent bonding to the same device more than once.
  * Refactored crypto functionality from SMP into its own folder, and added the
    h8 crypto function.
  * Changed the behavior when receiving an L2CAP K-frame larger than the MPS,
    disconnecting instead of truncating it.
  * Added a new :kconfig:option:`BT_ID_ALLOW_UNAUTH_OVERWRITE` that allows
    unauthorized bond overrides with multiple identities.
  * Added support for the object calculate checksum feature in OTS.
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
  * Added support for handling fragmented AD without chaining PDUs.
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

  * Atmel SAMC20, SAMC21
  * Atmel SAME70Q19
  * GigaDevice GD32L23X
  * GigaDevice GD32A50X
  * NXP S32Z2/E2

* Made these changes in other SoC series:

  * STM32F1: USB Prescaler configuration is now expected to be done using
    :dtcompatible: `st,stm32f1-pll-clock`: ``usbpre``
    or :dtcompatible: `st,stm32f105-pll-clock`: ``otgfspre`` properties.
  * STM32F7/L4: Now supports configuring MCO.
  * STM32G0: Now supports FDCAN
  * STM32G4: Now supports power management (STOP0 and STOP1 low power modes).
  * STM32H7: Now supports PLL2, USB OTG HS and ULPI PHY.
  * STM32L5: Now supports RTC based :ref:`counter_api`.
  * STM32U5: Now supports :ref:`crypto_api` through AES device.
  * STM32F7/L4: Now supports configuring MCO.

* Changes for ARC boards:

  * Multiple fixes to ``mdb-hw`` and ``mdb-nsim`` west runners to improve usability
  * Added ``nsim_em11d`` board with DSP features (XY DSP with AGU and XY memory)
  * Fixed cy8c95xx I2C GPIO port init on HSDK board
  * Added SPI flash support on EM starter kit board
  * Multiple fixes for nSIM platform - configuration: adding of missing HW features or
    configurations sync
  * Improved creg_gpio platform driver - add pin_configure API
  * Added separate QEMU config ``qemu_arc_hs_xip`` for XIP testing
  * Added ``nsim_hs_sram``, ``nsim_hs_flash_xip`` nSIM platforms to verify various memory models
  * nSIM board documentation overhaul

* Added support for these ARM boards:

  * Adafruit ItsyBitsy nRF52840 Express
  * Adafruit KB2040
  * Atmel atsamc21n_xpro
  * GigaDevice GD32L233R-EVAL
  * GigaDevice GD32A503V-EVAL
  * nRF5340 Audio DK
  * Sparkfun pro micro RP2040
  * Arduino Portenta H7
  * SECO JUNO SBC-D23 (STM32F302)
  * ST Nucleo G070RB
  * ST Nucleo L4A6ZG
  * NXP X-S32Z27X-DC (DC2)

* Added support for these ARM64 boards:

  * i.MX93 (Cortex-A) EVK board
  * Khadas Edge-V board
  * QEMU Virt KVM

* Added support for these X86 boards:

  * Intel Raptor Lake CRB

* Added support for these RISC-V boards:

  * Added LCD support for ``longan_nano`` board.

* Made these changes in ARM boards:

  * sam4s_xplained: Enabled PWM
  * sam_e70_xplained: Added DMA devicetree entries for SPI
  * sam_v71_xult: Added DMA devicetree entries for SPI
  * tdk_robokit1: Added DMA devicetree entries for SPI

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
  * Forced configuration of FPU was removed from following boards:
    ``stm32373c_eval``
    ``stm32f3_disco``

  * On STM32 boards, configuration of USB, SDMMC and entropy devices that generally
    expect a 48MHz clock is now done using device tree. When available, HSI48 is enabled
    and configured as domain clock for these devices, otherwise PLL_Q output or MSI is used.
    On some boards, previous PLL SAI configuration has been changed to above options,
    since PLL SAI cannot yet be configured using device tree.

* Made these changes in other boards:

  * The nrf52_bsim (natively simulated nRF52 device with BabbleSim) now models
    a nRF52833 instead of a nRF52832 device

* Added support for these following shields:

  * Adafruit PCA9685
  * nPM6001 EK
  * nPM1100 EK
  * Semtech SX1262MB2DAS
  * Sparkfun MAX3421E

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

  * Application specific configurations of sysbuild controlled images.

* Userspace

  * Userspace option to disable using the ``relax`` linker option has been
    added.

* Tools

  * Static code analyser (SCA) tool support has been added.

Drivers and Sensors
*******************

* ADC

  * STM32: Now Supports sequencing multiple channels into a single read.
  * Fixed a problem in :c:macro:`ADC_CHANNEL_CFG_DT` that forced users to add
    artificial ``input-positive`` property in nodes related to ADC drivers that
    do not use configurable analog inputs when such drivers were used together
    with an ADC driver that uses such input configuration.
  * Added driver for TI CC13xx/CC26xx family.
  * Added driver for Infineon XMC4xxx family.
  * Added driver for ESP32 SoCs.

* Battery-backed RAM

  * STM32: Added driver to enable support for backup registers from RTC.

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

  * STM32: HSI48 can now be configured using device tree.

* Counter

  * STM32 RTC based counter domain clock (LSE/SLI) should now be configured using device tree.
  * Added Timer based driver for GigaDevice GD32 SoCs.
  * Added NXP S32 System Timer Module driver.

* DAC

  * Added support for GigaDevice GD32 SoCs.
  * Added support for Espressif ESP32 SoCs.

* DFU

  * Removed :c:macro:`BOOT_TRAILER_IMG_STATUS_OFFS` in favor a two new functions;
    :c:func:`boot_get_area_trailer_status_offset` and :c:func:`boot_get_trailer_status_offset`

* Disk

  * STM32 SD host controller clocks are now configured via devicetree.
  * Zephyr flash disks are now configured using the :dtcompatible:`zephyr,flash-disk`
    devicetree binding
  * Flash disks can be marked as read only by setting the ``read-only`` property
    on the linked flash device partition.

* DMA

  * Adjusted incorrect dma1 clock source for GD32 gd32vf103 SoC.
  * Atmel SAM: Added support to select fixed or increment address mode when using
    peripherals to memory or memory to peripheral transfers.
  * STM32 DMA variable scope cleanups
  * Intel GPDMA linked list transfer descriptors appropriately aligned to 64 byte addresses
  * Intel GPDMA fixed bug in transfer configuration to initialize cfg_hi and cfg_lo
  * STM32 DMA Support for the STM32MP1 series
  * SAM XDMAC fixes to enable usage with SPI DMA transfers
  * Intel GPDMA fixed to return errors on dma stop
  * Intel GPDMA disabled interrupts when unneeded
  * Intel GPDMA fixed for register/ip ownership
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

* Ethernet

  * STM32: Default Mac address configuration is now uid based. Optionally, user can
    configure it to be random or provide its own address using device tree.
  * STM32: Added support for STM32Cube HAL Ethernet API V2 on F4/F7/H7. By default disabled,
    it can be enabled with :kconfig:option:`CONFIG_ETH_STM32_HAL_API_V2`.
  * STM32: Added ethernet support on STM32F107 devices.
  * STM32: Now supports multicast hash filtering in the MAC. It can be enabled using
    :kconfig:option:`CONFIG_ETH_STM32_MULTICAST_FILTER`.
  * STM32: Now supports statistics logging through :kconfig:option:`CONFIG_NET_STATISTICS_ETHERNET`.
    Requires use of HAL Ethernet API V2.

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

  * STM32 OSPI: sfdp-bfp table and jedec-id can now be read from device tree and override
    the flash content if required.

  * STM32 OSPI: Now supports DMA transfer on STM32U5.

  * STM32: Flash driver was revisited to simplify reuse of driver for new series, taking
    advantage of device tree compatibles.

* FPGA

  * Added preliminary support for the Lattice iCE40.
  * Added Qomu board sample.

* GPIO

  * Atmel SAM: Added support to configure Open-Drain pins
  * Added driver for nPM6001 PMIC GPIOs
  * Added NXP S32 GPIO (SIUL2) driver

* hwinfo

  * Added hwinfo_get_device_id for ESP32-C3
  * Added reset cause for iwdg and wwdg for STM32H7 and MP1

* I2C

  * SAM0 Fixed spurious trailing data by moving stop condition from thread into ISR
  * I2C Shell command adds ability to configure bus speed through `i2c speed`
  * ITE usage of instruction local memory support
  * NPCX bus recovery on transaction timeout
  * ITE log status of registers on transfer failure
  * ESP32 enabled configuring a hardware timeout to account for longer durations of clock stretching
  * ITE fixed bug where an operation was done outside of the driver mutex
  * NRFX TWIM Made transfer timeout configurable
  * DW Bug fix for clearing FIFO on initialization
  * NPCX simplified smb bank register usage
  * NXP LPI2C enabled target mode
  * NXP FlexComm Added semaphore for shared usage of bus
  * I2C Added support for dumping messages in the log for all transactions, reads and writes
  * STM32: Slave configuration now supports 10-bit addressing.
  * STM32: Now support power management. 3 modes supported: :kconfig:option:`CONFIG_PM`,
    :kconfig:option:`CONFIG_PM_DEVICE`, :kconfig:option:`CONFIG_PM_DEVICE_RUNTIME`.
  * STM32: Domain clock can now be configured using device tree

* I3C

  * Added a new target device API :c:func:`i3c_target_tx_write` to
    explicit write to TX FIFO.

  * GETMRL and GETMWL are both optional in :c:func:`i3c_device_basic_info_get` as
    MRL and MWL are optional according to I3C specification.

  * Added a new driver to support Cadence I3C controller.

* Interrupt Controller

  * STM32: Driver configuration and initialization is now based on device tree
  * Added NXP S32 External Interrupt Controller (SIUL2) driver.

* IPM

  * ipm_stm32_ipcc: fixed an issue where interrupt mask is not cleaned correctly,
    resulting in infinite TXF interrupts.

* MBOX

  * Added NXP S32 Message Receive Unit (MRU) driver.

* PCIE

  * Support for accessing I/O BARs, which was previously removed, is back.

  * Added new API :c:func:`pcie_scan` to scan for devices.

    * This iterates through the buses and devices which are expected to
      exist. The old method was to try all possible combination of buses
      and devices to determine if there is a device there.
      :c:func:`pci_init` and :c:func:`pcie_bdf_lookup` have been updated to
      use this new API.

    * :c:func:`pcie_scan` also introduces a callback mechanism for when
      a new device has been discovered.

* Pin control

  * Common pin control properties are now defined at root level in a single
    file: :zephyr_file:`dts/bindings/pinctrl/pincfg-node.yaml`. Pin control
    bindings are expected to include it at the level they need. For example,
    drivers using the grouping representation approach need to include it at
    grandchild level, while drivers using the node approach need to include it
    at the child level. This change will only impact out-of-tree pin control
    drivers, since all in-tree drivers have been updated.
  * Added NXP S32 SIUL2 driver
  * Added Nuvoton NuMicro driver
  * Added Silabs Gecko driver
  * Added support for i.MX93 in the i.MX driver
  * Added support for GD32L23x/GD32A50x in the Gigadevice driver

* PWM

  * Atmel SAM: Added support to select pin polarity
  * Added driver for NXP PCA9685 LED controller

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

  * STM32: STM32 reset driver is now available. Devices reset line configuration should
    be done using device tree.

* SDHC

  * i.MX RT USDHC:

    - Support HS400 and HS200 mode. This mode is used with eMMC devices,
      and will enable high speed operation for those cards.
    - Support DMA operation on SOCs that do not support non-cacheable memory,
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
  * Added ESP32 and RaspberryPi Pico die temperature sensor drivers.
  * Added TDK InvenSense ICM42688 six axis IMU driver.
  * Added TDK InvenSense ICP10125 pressure and temperature sensor driver.
  * Added AMS AS5600 magnetic angle sensor driver.
  * Added AMS AS621x temperature sensor driver.
  * Added HZ-Grow R502A fingerprint sensor driver.
  * Enhanced FXOS8700, FXAS21002, and BMI270 drivers to support SPI in addition
    to I2C.
  * Enhanced ST LIS2DW12 driver to support free fall detection.
  * rpi_pico: Added die temperature sensor driver.
  * STM32 family Quadrature Decoder driver was added. Only enabled on STM32F4 for now.

* Serial

  * Atmel SAM: UART/USART: Added support to configure driver at runtime
  * STM32: DMA now supported on STM32U5 series.

  * uart_altera_jtag: added support for Nios-V UART.

  * uart_esp32: added support asynchronous operation.

  * uart_gecko: added support for pinctrl.

  * uart_mchp_xec: now supports UART on MEC15xx SoC.

  * uart_mcux_flexcomm: added support for runtime configuration.

  * uart_mcux_lpuart: added support for RS-485.

  * uart_numicro: uses pinctrl to configure UART pins.

  * uart_pl011: added support for pinctrl.

  * uart_rpi_pico: added support for runtime configuration.

  * uart_xmc4xxx: added support for interrupt so it can now be interrupt driven.
    Also added support for FIFO.

  * New UART drivers are added:

    * Cadence IP6528 UART.

    * NXP S32 LINFlexD UART.

    * OpenTitan UART.

    * QuickLogic USBserialport_S3B.

* SPI

  * Added dma support for GD32 driver.
  * Atmel SAM:

    * Added support to transfers using DMA.
    * Added support to loopback mode for testing purposes.

  * Added NXP S32 SPI driver.

* Timer

  * Corrected CPU numbering on SMP RISC-V systems using the mtime device

  * Added support for OpenTitan's privileged timer device to riscv_machine_timer

  * Refactored SYS_CLOCK_EXISTS such that it always matches the
    existence of a timer device in kconfig

  * Significant rework to nrf_rtc_timer with multiple fixes

  * Fixed prescaler correction in stm32_lptim driver and fix race with auto-reload

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
  * STM32: Now supports :c:func:`usb_dc_detach` and :c:func:`usb_dc_wakeup_request`.
  * STM32: Vbus sensing is now supported and determined based on the presence of the
    hardware detection pin(s) in the device tree. E.g: pinctrl-0 = <&usb_otg_fs_vbus_pa9 ...>;
  * RPi Pico: fixed buffer status handling, fixed infinite unhandled irq retriggers,
    fixed DATA PID toggle and control transfer handling.
  * NXP: Enabled high speed support, fixed endpoint buffer write operation.
  * nRF USBD: Removed HAL driver uninit on detach, fixed endpoints disable on
    USB stack disable.
  * Added new experimental USB device controller (UDC) API and implementation
    for nRF USBD, Kinetis USBFSOTG, and virtual controllers.
  * Added new experimental USB host controller (UDC) API and implementation
    for MAX3421E and virtual controllers.

* Watchdog

  * Added driver for nPM6001 PMIC Watchdog.
  * Added free watchdog driver for GigaDevice GD32 SoCs.
  * Added window watchdog driver for GigaDevice GD32 SoCs.
  * Added NXP S32 Software Watchdog Timer driver.

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
  * Added NXP S32 NETC Ethernet driver.

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
  * Deprecated :c:func:`openthread_set_state_changed_cb` in favour of more
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

* New experimental USB support:

  * Added new USB device stack (device_next), class implementation for CDC ACM and
    BT HCI USB transport layer.
  * Added initial support for USB host

* USB device stack (device):

  * Removed transfer cancellation on bus suspend.
  * Reworked disabling all endpoints on stack disable to allow re-enabling USB
    device stack.
  * Revised endpoint enable/disable on alternate setting.
  * Improved USB DFU support with WinUSB on Windows.
  * Added check to prevent recursive logging loop and allowed to send more than
    one byte using poll out in CDC ACM class implementation.
  * Corrected IAD and interface descriptors, removed unnecessary CDC descriptors,
    and fixed packet reception in RNDIS ethernet implementation.
  * Implemented cache synchronization after write operations in USB MSC class.


Devicetree
**********

API
===

New general-purpose macros:

- :c:macro:`DT_FOREACH_PROP_ELEM_SEP_VARGS`
- :c:macro:`DT_FOREACH_PROP_ELEM_SEP`
- :c:macro:`DT_INST_FOREACH_PROP_ELEM_SEP_VARGS`
- :c:macro:`DT_INST_FOREACH_PROP_ELEM_SEP`
- :c:macro:`DT_INST_GPARENT`
- :c:macro:`DT_NODE_MODEL_BY_IDX_OR`
- :c:macro:`DT_NODE_MODEL_BY_IDX`
- :c:macro:`DT_NODE_MODEL_HAS_IDX`
- :c:macro:`DT_NODE_MODEL_OR`

New special-purpose macros introduced for the GPIO hogs feature (see
:zephyr_file:`drivers/gpio/gpio_hogs.c`):

- :c:macro:`DT_GPIO_HOG_FLAGS_BY_IDX`
- :c:macro:`DT_GPIO_HOG_PIN_BY_IDX`
- :c:macro:`DT_NUM_GPIO_HOGS`

The following deprecated macros were removed:

- ``DT_CHOSEN_ZEPHYR_ENTROPY_LABEL``
- ``DT_CHOSEN_ZEPHYR_FLASH_CONTROLLER_LABEL``

Bindings
========

New bindings:

  - Generic or vendor-independent:

    - :dtcompatible:`usb-c-connector`
    - :dtcompatible:`usb-ulpi-phy`

  - AMS AG (ams):

    - :dtcompatible:`ams,as5600`
    - :dtcompatible:`ams,as6212`

  - Synopsys, Inc. (formerly ARC International PLC) (arc):

    - :dtcompatible:`arc,xccm`
    - :dtcompatible:`arc,yccm`

  - ARM Ltd. (arm):

    - :dtcompatible:`arm,cortex-a55`
    - :dtcompatible:`arm,ethos-u`

  - ASPEED Technology Inc. (aspeed):

    - :dtcompatible:`aspeed,ast10x0-reset`

  - Atmel Corporation (atmel):

    - :dtcompatible:`atmel,samc2x-gclk`
    - :dtcompatible:`atmel,samc2x-mclk`

  - Bosch Sensortec GmbH (bosch):

    - :dtcompatible:`bosch,bmi270`
    - :dtcompatible:`bosch,bmi270`

  - Cadence Design Systems Inc. (cdns):

    - :dtcompatible:`cdns,i3c`
    - :dtcompatible:`cdns,uart`

  - Espressif Systems (espressif):

    - :dtcompatible:`espressif,esp32-adc`
    - :dtcompatible:`espressif,esp32-dac`
    - :dtcompatible:`espressif,esp32-eth`
    - :dtcompatible:`espressif,esp32-gdma`
    - :dtcompatible:`espressif,esp32-mdio`
    - :dtcompatible:`espressif,esp32-temp`

  - GigaDevice Semiconductor (gd):

    - :dtcompatible:`gd,gd322-dma` has new helper macros to easily setup the ``dma-cells`` property.
    - :dtcompatible:`gd,gd32-dma-v1`
    - :dtcompatible:`gd,gd32-fwdgt`
    - :dtcompatible:`gd,gd32-wwdgt`

  - Hangzhou Grow Technology Co., Ltd. (hzgrow):

    - :dtcompatible:`hzgrow,r502a`

  - Infineon Technologies (infineon):

    - :dtcompatible:`infineon,xmc4xxx-adc`
    - :dtcompatible:`infineon,xmc4xxx-flash-controller`
    - :dtcompatible:`infineon,xmc4xxx-intc`
    - :dtcompatible:`infineon,xmc4xxx-nv-flash`

  - Intel Corporation (intel):

    - :dtcompatible:`intel,adsp-communication-widget`
    - :dtcompatible:`intel,adsp-dfpmcch`
    - :dtcompatible:`intel,adsp-dfpmccu`
    - :dtcompatible:`intel,adsp-mem-window`
    - :dtcompatible:`intel,adsp-sha`
    - :dtcompatible:`intel,adsp-timer`
    - :dtcompatible:`intel,hda-dai`
    - :dtcompatible:`intel,raptor-lake`

  - InvenSense Inc. (invensense):

    - :dtcompatible:`invensense,icm42688`
    - :dtcompatible:`invensense,icp10125`

  - ITE Tech. Inc. (ite):

    - :dtcompatible:`ite,it8xxx2-espi`
    - :dtcompatible:`ite,it8xxx2-gpiokscan`
    - :dtcompatible:`ite,it8xxx2-ilm`
    - :dtcompatible:`ite,it8xxx2-shi`
    - :dtcompatible:`ite,it8xxx2-usbpd`

  - Kvaser (kvaser):

    - :dtcompatible:`kvaser,pcican`

  - Lattice Semiconductor (lattice):

    - :dtcompatible:`lattice,ice40-fpga`

  - lowRISC Community Interest Company (lowrisc):

    - :dtcompatible:`lowrisc,machine-timer`
    - :dtcompatible:`lowrisc,opentitan-uart`

  - Maxim Integrated Products (maxim):

    - :dtcompatible:`maxim,max3421e_spi`

  - Microchip Technology Inc. (microchip):

    - :dtcompatible:`microchip,xec-bbled`
    - :dtcompatible:`microchip,xec-ecs`
    - :dtcompatible:`microchip,xec-espi-saf-v2`
    - :dtcompatible:`microchip,xec-qmspi-full-duplex`

  - Nordic Semiconductor (nordic):

    - :dtcompatible:`nordic,npm1100`
    - :dtcompatible:`nordic,npm6001`
    - :dtcompatible:`nordic,npm6001-gpio`
    - :dtcompatible:`nordic,npm6001-regulator`
    - :dtcompatible:`nordic,npm6001-wdt`

  - Nuvoton Technology Corporation (nuvoton):

    - :dtcompatible:`nuvoton,npcx-kscan`
    - :dtcompatible:`nuvoton,npcx-sha`
    - :dtcompatible:`nuvoton,npcx-shi`
    - :dtcompatible:`nuvoton,numicro-gpio`
    - :dtcompatible:`nuvoton,numicro-pinctrl`

  - NXP Semiconductors (nxp):

    - :dtcompatible:`nxp,css-v2`
    - :dtcompatible:`nxp,fxas21002`
    - :dtcompatible:`nxp,fxos8700`
    - :dtcompatible:`nxp,imx-flexspi-aps6408l`
    - :dtcompatible:`nxp,imx-flexspi-s27ks0641`
    - :dtcompatible:`nxp,imx-mu-rev2`
    - :dtcompatible:`nxp,imx93-pinctrl`
    - :dtcompatible:`nxp,mcux-qdec`
    - :dtcompatible:`nxp,mcux-xbar`
    - :dtcompatible:`nxp,pca9420`
    - :dtcompatible:`nxp,pca9685-pwm`
    - :dtcompatible:`nxp,pcf8574`
    - :dtcompatible:`nxp,pdcfg-power`
    - :dtcompatible:`nxp,s32-gpio`
    - :dtcompatible:`nxp,s32-linflexd`
    - :dtcompatible:`nxp,s32-mru`
    - :dtcompatible:`nxp,s32-netc-emdio`
    - :dtcompatible:`nxp,s32-netc-psi`
    - :dtcompatible:`nxp,s32-netc-vsi`
    - :dtcompatible:`nxp,s32-siul2-eirq`
    - :dtcompatible:`nxp,s32-spi`
    - :dtcompatible:`nxp,s32-swt`
    - :dtcompatible:`nxp,s32-sys-timer`
    - :dtcompatible:`nxp,s32ze-pinctrl`

  - OpenThread (openthread):

    - :dtcompatible:`openthread,config`

  - QuickLogic Corp. (quicklogic):

    - :dtcompatible:`quicklogic,usbserialport-s3b`

  - Raspberry Pi Foundation (raspberrypi):

    - :dtcompatible:`raspberrypi,pico-flash-controller`
    - :dtcompatible:`raspberrypi,pico-temp`

  - Richtek Technology Corporation (richtek):

    - :dtcompatible:`richtek,rt1718s`
    - :dtcompatible:`richtek,rt1718s-gpio-port`

  - Smart Battery System (sbs):

    - :dtcompatible:`sbs,sbs-gauge-new-api`

  - Silicon Laboratories (silabs):

    - :dtcompatible:`silabs,gecko-pinctrl`
    - :dtcompatible:`silabs,gecko-stimer`

  - Synopsys, Inc. (snps):

    - :dtcompatible:`snps,ethernet-cyclonev`

  - SparkFun Electronics (sparkfun):

    - :dtcompatible:`sparkfun,pro-micro-gpio`

  - STMicroelectronics (st):

    - :dtcompatible:`st,stm32-bbram`
    - :dtcompatible:`st,stm32-qdec`
    - :dtcompatible:`st,stm32-rcc-rctl`
    - :dtcompatible:`st,stm32wb-rf`

  - Texas Instruments (ti):

    - :dtcompatible:`ti,cc13xx-cc26xx-adc`
    - :dtcompatible:`ti,cc13xx-cc26xx-watchdog`
    - :dtcompatible:`ti,tca6424a`

  - A stand-in for a real vendor which can be used in examples and tests (vnd):

    - :dtcompatible:`vnd,emul-tester`

  - Zephyr-specific binding (zephyr):

    - :dtcompatible:`zephyr,ec-host-cmd-periph-espi`
    - :dtcompatible:`zephyr,fake-can`
    - :dtcompatible:`zephyr,fake-eeprom`
    - :dtcompatible:`zephyr,fake-regulator`
    - :dtcompatible:`zephyr,flash-disk`
    - :dtcompatible:`zephyr,gpio-emul-sdl`
    - :dtcompatible:`zephyr,gpio-keys`
    - :dtcompatible:`zephyr,ipc-icmsg-me-follower`
    - :dtcompatible:`zephyr,ipc-icmsg-me-initiator`
    - :dtcompatible:`zephyr,mmc-disk`
    - :dtcompatible:`zephyr,psa-crypto-rng`
    - :dtcompatible:`zephyr,udc-virtual`
    - :dtcompatible:`zephyr,uhc-virtual`
    - :dtcompatible:`zephyr,usb-c-vbus-adc`

Removed bindings:

  - Generic or vendor-independent:

    - ``regulator-pmic``

  - Intel Corporation (intel):

    - ``intel,adsp-lps``

  - NXP Semiconductors (nxp):

    - ``nxp,imx-flexspi-hyperram``

  - STMicroelectronics (st):

    - ``st,stm32f0-flash-controller``
    - ``st,stm32f3-flash-controller``
    - ``st,stm32l0-flash-controller``
    - ``st,stm32l1-flash-controller``
    - ``st,stm32u5-flash-controller``

Modified bindings:

  - Generic or vendor-independent:

    - All sensor devices now have a ``friendly-name`` property,
      which is a human-readable string describing the sensor.
      See :zephyr_file:`dts/bindings/sensor/sensor-device.yaml`
      for more information.

    - All DMA controller devices have had their ``dma-buf-alignment``
      properties renamed to ``dma-buf-addr-alignment``.

      Additionally, all DMA controller devices have new
      ``dma-buf-size-alignment`` and ``dma-copy-alignment`` properties.

      See :zephyr_file:`dts/bindings/dma/dma-controller.yaml` for
      more information.

    - :dtcompatible:`ns16550`:

        - new property: ``vendor-id``
        - new property: ``device-id``
        - property ``reg`` is no longer required

    - :dtcompatible:`pci-host-ecam-generic`:

        - new property: ``interrupt-map-mask``
        - new property: ``interrupt-map``
        - new property: ``bus-range``

    - :dtcompatible:`regulator-fixed`:

        - removed property: ``supply-gpios``
        - removed property: ``vin-supply``

    - :dtcompatible:`gpio-keys`:

        - new property: ``debounce-interval-ms``

  - Altera Corp. (altr):

    - :dtcompatible:`altr,jtag-uart`:

        - new property: ``write-fifo-depth``

  - ARM Ltd. (arm):

    - :dtcompatible:`arm,pl011`:

        - new property: ``pinctrl-0``
        - new property: ``pinctrl-1``
        - new property: ``pinctrl-2``
        - new property: ``pinctrl-3``
        - new property: ``pinctrl-4``
        - new property: ``pinctrl-names``

  - Atmel Corporation (atmel):

    - :dtcompatible:`atmel,sam-pwm`:

        - specifier cells for space "pwm" are now named: ['channel', 'period', 'flags'] (old value: ['channel', 'period'])
        - property ``#pwm-cells`` const value changed from 2 to 3

    - :dtcompatible:`atmel,sam-spi`:

        - new property: ``loopback``

  - Espressif Systems (espressif):

    - :dtcompatible:`espressif,esp32-twai`:

        - property ``clkout-divider`` enum value changed from [1, 2, 4, 6, 8, 10, 12, 14] to None

    - :dtcompatible:`espressif,esp32-i2c`:

        - new property: ``scl-timeout-us``

    - :dtcompatible:`espressif,esp32-spi`:

        - new property: ``dma-enabled``
        - new property: ``dma-clk``
        - new property: ``dma-host``
        - removed property: ``dma``

  - GigaDevice Semiconductor (gd):

    - :dtcompatible:`gd,gd32-dma`:

        - specifier cells for space "dma" are now named: ['channel', 'config'] (old value: ['channel'])
        - new property: ``gd,mem2mem``
        - removed property: ``resets``
        - removed property: ``reset-names``
        - property ``#dma-cells`` const value changed from 1 to 2

  - ILI Technology Corporation (ILITEK) (ilitek):

    - :dtcompatible:`ilitek,ili9341` (on spi bus):

        - property ``disctrl`` default value changed from [10, 130, 39] to [10, 130, 39, 4]

  - Infineon Technologies (infineon):

    - :dtcompatible:`infineon,xmc4xxx-uart`:

        - new property: ``fifo-start-offset``
        - new property: ``fifo-tx-size``
        - new property: ``fifo-rx-size``

  - Intel Corporation (intel):

    - :dtcompatible:`intel,adsp-power-domain`:

        - removed property: ``lps``

    - :dtcompatible:`intel,e1000`:

        - new property: ``vendor-id``
        - new property: ``device-id``
        - property ``reg`` is no longer required

    - :dtcompatible:`intel,dai-dmic`:

        - new property: ``fifo``
        - property ``shim`` type changed from array to int

  - ITE Tech. Inc. (ite):

    - :dtcompatible:`ite,it8xxx2-pinctrl-func`:

        - new property: ``pp-od-mask``
        - new property: ``pullup-mask``
        - new property: ``gpio-group``
        - property ``volt-sel-mask`` is no longer required
        - property ``func4-gcr`` is no longer required
        - property ``func3-en-mask`` is no longer required
        - property ``func3-gcr`` is no longer required
        - property ``func4-en-mask`` is no longer required
        - property ``volt-sel`` is no longer required

  - JEDEC Solid State Technology Association (jedec):

    - :dtcompatible:`jedec,spi-nor` (on spi bus):

        - new property: ``mxicy,mx25r-power-mode``

  - Microchip Technology Inc. (microchip):

    - :dtcompatible:`microchip,xec-uart`:

        - new property: ``wakerx-gpios``

    - :dtcompatible:`microchip,xec-pcr`:

        - new property: ``clk32kmon-period-min``
        - new property: ``clk32kmon-period-max``
        - new property: ``clk32kmon-duty-cycle-var-max``
        - new property: ``clk32kmon-valid-min``
        - new property: ``xtal-enable-delay-ms``
        - new property: ``pll-lock-timeout-ms``
        - new property: ``clkmon-bypass``
        - new property: ``internal-osc-disable``
        - new property: ``pinctrl-0``
        - new property: ``pinctrl-names``
        - new property: ``pinctrl-1``
        - new property: ``pinctrl-2``
        - new property: ``pinctrl-3``
        - new property: ``pinctrl-4``
        - property ``interrupts`` is no longer required

    - :dtcompatible:`microchip,xec-qmspi-ldma`:

        - new property: ``port-sel``
        - new property: ``chip-select``
        - removed property: ``port_sel``
        - removed property: ``chip_select``
        - property ``lines`` enum value changed from None to [1, 2, 4]

  - Nordic Semiconductor (nordic):

    - :dtcompatible:`nordic,nrf21540-fem`:

        - new property: ``supply-voltage-mv``

    - :dtcompatible:`nordic,qspi-nor` (on qspi bus):

        - new property: ``mxicy,mx25r-power-mode``

  - Nuvoton Technology Corporation (nuvoton):

    - :dtcompatible:`nuvoton,numicro-uart`:

        - new property: ``pinctrl-0``
        - new property: ``pinctrl-1``
        - new property: ``pinctrl-2``
        - new property: ``pinctrl-3``
        - new property: ``pinctrl-4``
        - new property: ``pinctrl-names``

    - :dtcompatible:`nuvoton,adc-cmp`:

        - new property: ``status``
        - new property: ``compatible``
        - new property: ``reg``
        - new property: ``reg-names``
        - new property: ``interrupts``
        - new property: ``interrupts-extended``
        - new property: ``interrupt-names``
        - new property: ``interrupt-parent``
        - new property: ``label``
        - new property: ``clocks``
        - new property: ``clock-names``
        - new property: ``#address-cells``
        - new property: ``#size-cells``
        - new property: ``dmas``
        - new property: ``dma-names``
        - new property: ``io-channel-names``
        - new property: ``mboxes``
        - new property: ``mbox-names``
        - new property: ``wakeup-source``
        - new property: ``power-domain``

  - NXP Semiconductors (nxp):

    - :dtcompatible:`nxp,kinetis-lpuart`:

        - new property: ``nxp,rs485-mode``
        - new property: ``nxp,rs485-de-active-low``

    - :dtcompatible:`nxp,fxas21002` (on i2c bus):

        - new property: ``reset-gpios``

    - :dtcompatible:`nxp,imx-pwm`:

        - specifier cells for space "pwm" are now named: ['channel', 'period', 'flags'] (old value: ['channel', 'period'])
        - new property: ``nxp,prescaler``
        - new property: ``nxp,reload``
        - property ``#pwm-cells`` const value changed from 2 to 3

    - :dtcompatible:`nxp,imx-usdhc`:

        - new property: ``mmc-hs200-1_8v``
        - new property: ``mmc-hs400-1_8v``

    - :dtcompatible:`nxp,lpc-sdif`:

        - new property: ``mmc-hs200-1_8v``
        - new property: ``mmc-hs400-1_8v``

  - QEMU, a generic and open source machine emulator and virtualizer (qemu):

    - :dtcompatible:`qemu,ivshmem`:

        - new property: ``vendor-id``
        - new property: ``device-id``

  - Renesas Electronics Corporation (renesas):

    - :dtcompatible:`renesas,smartbond-uart`:

        - property ``current-speed`` enum value changed from [1200, 2400, 4800, 9600, 14400, 19200, 28800, 38400, 57600, 115200, 230400, 460800, 921600, 1000000] to [4800, 9600, 14400, 19200, 28800, 38400, 57600, 115200, 230400, 500000, 921600, 1000000, 2000000]

  - Silicon Laboratories (silabs):

    - :dtcompatible:`silabs,gecko-usart`:

        - new property: ``pinctrl-0``
        - new property: ``pinctrl-1``
        - new property: ``pinctrl-2``
        - new property: ``pinctrl-3``
        - new property: ``pinctrl-4``
        - new property: ``pinctrl-names``
        - property ``location-rx`` is no longer required
        - property ``location-tx`` is no longer required
        - property ``peripheral-id`` is no longer required

    - :dtcompatible:`silabs,gecko-gpio-port`:

        - property ``peripheral-id`` is no longer required

    - :dtcompatible:`silabs,gecko-spi-usart`:

        - new property: ``pinctrl-0``
        - new property: ``pinctrl-1``
        - new property: ``pinctrl-2``
        - new property: ``pinctrl-3``
        - new property: ``pinctrl-4``
        - new property: ``pinctrl-names``
        - property ``location-clk`` is no longer required
        - property ``location-rx`` is no longer required
        - property ``location-tx`` is no longer required
        - property ``peripheral-id`` is no longer required

  - Sitronix Technology Corporation (sitronix):

    - :dtcompatible:`sitronix,st7735r` (on spi bus):

        - new property: ``rgb-is-inverted``

  - Synopsys, Inc. (snps):

    - :dtcompatible:`snps,designware-i2c`:

        - new property: ``vendor-id``
        - new property: ``device-id``
        - property ``reg`` is no longer required

  - STMicroelectronics (st):

    - :dtcompatible:`st,stm32-adc`:

        - the ``has-temp-channel``, ``has-vref-channel`` and
          ``has-vbat-channel`` properties were respectively replaced by
          ``temp-channel``, ``vref-channel`` and ``vbat-channel``

    - :dtcompatible:`st,stm32-ethernet`:

        - the built-in driver for this compatible now supports the
          ``local-mac-address`` and ``zephyr,random-mac-address`` properties
          for setting MAC addresses, and the associated Kconfig options
          (``CONFIG_ETH_STM32_HAL_RANDOM_MAC``,
          ``CONFIG_ETH_STM32_HAL_USER_STATIC_MAC``) are now deprecated

    - :dtcompatible:`st,stm32-qspi-nor` (on qspi bus):

        - new property: ``reset-cmd``
        - new property: ``reset-cmd-wait``

    - :dtcompatible:`st,stm32-uart`:

        - new property: ``resets``
        - new property: ``tx-rx-swap``
        - new property: ``reset-names``

    - :dtcompatible:`st,stm32-usart`:

        - new property: ``resets``
        - new property: ``tx-rx-swap``
        - new property: ``reset-names``

    - :dtcompatible:`st,stm32-lpuart`:

        - new property: ``resets``
        - new property: ``tx-rx-swap``
        - new property: ``reset-names``

    - :dtcompatible:`st,stm32-exti`:

        - new property: ``num-lines``
        - new property: ``line-ranges``
        - new property: ``interrupt-controller``
        - new property: ``#interrupt-cells``
        - property ``interrupts`` is now required
        - property ``interrupt-names`` is now required

    - :dtcompatible:`st,stm32-ospi`:

        - property ``clock-names`` is now required

    - :dtcompatible:`st,stm32f105-pll2-clock`:

        - new property: ``otgfspre``

    - :dtcompatible:`st,stm32f105-pll-clock`:

        - new property: ``otgfspre``

    - :dtcompatible:`st,stm32f100-pll-clock`:

        - new property: ``otgfspre``

    - :dtcompatible:`st,stm32f1-pll-clock`:

        - property ``usbpre`` type changed from int to boolean

    - :dtcompatible:`st,stm32-lse-clock`:

        - new property: ``lse-bypass``

    - :dtcompatible:`st,lis2dh12` (on i2c bus):

        - new property: ``anym-no-latch``
        - new property: ``anym-mode``

    - :dtcompatible:`st,lsm6dso` (on i2c bus):

        - new property: ``drdy-pulsed``

    - :dtcompatible:`st,lis2dh` (on i2c bus):

        - new property: ``anym-no-latch``
        - new property: ``anym-mode``

    - :dtcompatible:`st,lsm303agr-accel` (on spi bus):

        - new property: ``anym-no-latch``
        - new property: ``anym-mode``

    - :dtcompatible:`st,lis3dh` (on i2c bus):

        - new property: ``anym-no-latch``
        - new property: ``anym-mode``

    - :dtcompatible:`st,lsm6dso` (on spi bus):

        - new property: ``drdy-pulsed``

    - :dtcompatible:`st,lis2dw12` (on spi bus):

        - new property: ``odr``
        - new property: ``ff-duration``
        - new property: ``ff-threshold``

    - :dtcompatible:`st,lsm6dso32` (on spi bus):

        - new property: ``drdy-pulsed``

    - :dtcompatible:`st,lsm303dlhc-accel` (on i2c bus):

        - new property: ``anym-no-latch``
        - new property: ``anym-mode``

    - :dtcompatible:`st,lis2dh` (on spi bus):

        - new property: ``anym-no-latch``
        - new property: ``anym-mode``

    - :dtcompatible:`st,lis2dw12` (on i2c bus):

        - new property: ``odr``
        - new property: ``ff-duration``
        - new property: ``ff-threshold``

    - :dtcompatible:`st,lsm303agr-accel` (on i2c bus):

        - new property: ``anym-no-latch``
        - new property: ``anym-mode``

    - :dtcompatible:`st,lsm6dso32` (on i2c bus):

        - new property: ``drdy-pulsed``

    - :dtcompatible:`st,stm32-sdmmc`:

        - new property: ``resets``
        - new property: ``reset-names``

    - :dtcompatible:`st,stm32-ucpd`:

        - new property: ``dead-battery``
        - new property: ``pinctrl-0``
        - new property: ``pinctrl-names``
        - new property: ``pinctrl-1``
        - new property: ``pinctrl-2``
        - new property: ``pinctrl-3``
        - new property: ``pinctrl-4``

    - :dtcompatible:`st,stm32-timers`:

        - new property: ``resets``
        - new property: ``reset-names``

    - :dtcompatible:`st,stm32-lptim`:

        - new property: ``st,static-prescaler``
        - new property: ``reset-names``

    - :dtcompatible:`st,stm32-usb`:

        - removed property: ``enable-pin-remap``

  - Texas Instruments (ti):

    - :dtcompatible:`ti,ina230` (on i2c bus):

        - new property: ``current-lsb-microamps``
        - new property: ``rshunt-milliohms``
        - new property: ``alert-gpios``
        - removed property: ``irq-gpios``
        - removed property: ``current-lsb``
        - removed property: ``rshunt``

    - :dtcompatible:`ti,ina237` (on i2c bus):

        - new property: ``current-lsb-microamps``
        - new property: ``rshunt-milliohms``
        - new property: ``alert-gpios``
        - removed property: ``irq-gpios``
        - removed property: ``current-lsb``
        - removed property: ``rshunt``

  - A stand-in for a real vendor which can be used in examples and tests (vnd):

    - :dtcompatible:`vnd,pinctrl`:

        - new property: ``bias-disable``
        - new property: ``bias-high-impedance``
        - new property: ``bias-bus-hold``
        - new property: ``bias-pull-up``
        - new property: ``bias-pull-down``
        - new property: ``bias-pull-pin-default``
        - new property: ``drive-push-pull``
        - new property: ``drive-open-drain``
        - new property: ``drive-open-source``
        - new property: ``drive-strength``
        - new property: ``drive-strength-microamp``
        - new property: ``input-enable``
        - new property: ``input-disable``
        - new property: ``input-schmitt-enable``
        - new property: ``input-schmitt-disable``
        - new property: ``input-debounce``
        - new property: ``power-source``
        - new property: ``low-power-enable``
        - new property: ``low-power-disable``
        - new property: ``output-disable``
        - new property: ``output-enable``
        - new property: ``output-low``
        - new property: ``output-high``
        - new property: ``sleep-hardware-state``
        - new property: ``slew-rate``
        - new property: ``skew-delay``

  - Zephyr-specific binding (zephyr):

    - :dtcompatible:`zephyr,cdc-acm-uart` (on usb bus):

        - new property: ``tx-fifo-size``
        - new property: ``rx-fifo-size``

    - :dtcompatible:`zephyr,sdhc-spi-slot` (on spi bus):

        - bus list changed from [] to ['sd']

Other
=====

Shields

  * In order to avoid name conflicts with devices that may be defined at
    board level, it is advised, specifically for shields devicetree descriptions,
    to provide a device nodelabel in the form ``<device>_<shield>``. In-tree shields
    have been updated to follow this recommendation.

* Others

  * STM32F1 SoCs

    * Added new pinctrl definitions for STM32F1xx PWM input. In PWM capture mode
      STM32F1xx pins have to be configured as input and not as alternate.
      The new names takes the form tim1_ch1_pwm_in_pa8 for example.

    * Renamed pinctrl definitions for STM32F1xx PWM output to differentiate them
      from newly created inputs. The new names takes the form tim1_ch1_pwm_out_pa8
      instead of tim1_ch1_pwm_pa8.

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

* DSP

  * Introduced DSP (digital signal processing) subsystem with CMSIS-DSP as the
    default backend.
  * CMSIS-DSP now supports all architectures supported by Zephyr.

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

* Logging

  * Added support for logging on multiple domains.
  * :kconfig:option:`CONFIG_LOG_PRINTK` is now by default enabled which means that
    when logging is enabled then printk is by directed to the logging subsystem.
  * Added option to use custom logging header.

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

  * UpdateHub:

    * The integrity check was reworked to allow use by other libraries. Since
      then UpdateHub uses mbedTLS library as default crypto library.
    * Added a new Storage Abstraction to isolate both flash operations and
      MCUboot internals.
    * The UpdateHub User API was moved as a Zephyr public API and the userspace
      now is available. This added :c:func:`updatehub_confirm` and
      :c:func:`updatehub_reboot` functions.

* LwM2M

  * The ``lwm2m_senml_cbor_*`` files have been regenerated using zcbor 0.6.0.

* POSIX API

  * Harmonized posix type definitions across the minimal libc, newlib and picolibc.

    * Abstract ``pthread_t``, ``pthread_key_t``, ``pthread_cond_t``,
      ``pthread_mutex_t``, as ``uint32_t``.
    * Defined :c:macro:`PTHREAD_KEY_INITIALIZER`, :c:macro:`PTHREAD_COND_INITIALIZER`,
      :c:macro:`PTHREAD_MUTEX_INITIALIZER` to align with POSIX 1003.1.

  * Allowed non-prefixed standard include paths with :kconfig:option:`CONFIG_POSIX_API`.

    * I.e. ``#include <unistd.h>`` instead of ``#include <zephyr/posix/unistd.h>``.
    * Primarily to ease integration with external libraries.
    * Internal Zephyr code should continue to use prefixed header paths.

  * Enabled ``eventfd()``, ``getopt()`` by default with :kconfig:option:`CONFIG_POSIX_API`.
  * Moved / renamed header files to align with POSIX specifications.

    * E.g. move ``fcntl.h``, ``sys/stat.h`` from the minimal libc into the
      ``include/zephyr/posix`` directory. Rename ``posix_sched.h`` to ``sched.h``.
    * Move :c:macro:`O_ACCMODE`, :c:macro:`O_RDONLY`, :c:macro:`O_WRONLY`,
      :c:macro:`O_WRONLY`, to ``fcntl.h``.

  * Added :kconfig:option:`CONFIG_TIMER_CREATE_WAIT`, :kconfig:option:`CONFIG_MAX_PTHREAD_KEY_COUNT`,
    :kconfig:option:`CONFIG_MAX_PTHREAD_COND_COUNT`, :kconfig:option:`CONFIG_MAX_PTHREAD_MUTEX_COUNT`.
  * Defined :c:macro:`SEEK_SET`, :c:macro:`SEEK_CUR`, :c:macro:`SEEK_END`.

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

* Shell

  * New features:

    * SHELL_AUTOSTART configuration option. When SHELL_AUTOSTART is set to n, the shell is not
      started after boot but can be enabled later from the application code.
    * Added support for setting the help description for each entry in a dictionary.

  * Bugfix:

    * Updated to clear command buffer when leaving bypass mode to prevent undefined behaviour
      on consecutive shell operations.
    * Set RX size default to 256 if shell MCUmgr is enabled.
    * Fixed log message queue size for all backends.

  * Documentation:

    * Added information explaining commands execution.

* Utilities

  * Added the linear range API to map values in a linear range to a range index
    :zephyr_file:`include/zephyr/sys/linear_range.h`.


* Zbus

  * Added the :ref:`zbus` to Zephyr.

    * Channel-centric multi-paradigm (message-passing and publish-subscribe) communication message bus.
    * Virtual Distributed Event Dispatcher.
    * Observers can be listeners (synchronous) and subscribers (asynchronous).
    * One-to-one, one-to-many, and many-to-many communications.
    * Persistent messages distributed by shared-memory approach.
    * Delivery guarantee only for listeners.
    * Uses mutex to control channels access.
    * Added the following samples:

      * :zephyr:code-sample:`zbus-hello-world`
      * :zephyr:code-sample:`zbus-work-queue`
      * :zephyr:code-sample:`zbus-dyn-channel`
      * :zephyr:code-sample:`zbus-uart-bridge`
      * :zephyr:code-sample:`zbus-remote-mock`
      * :zephyr:code-sample:`zbus-runtime-obs-registration`
      * :zephyr:code-sample:`zbus-benchmark`

    * Added zbus channels APIs:

      * :c:func:`zbus_chan_pub`
      * :c:func:`zbus_chan_read`
      * :c:func:`zbus_chan_notify`
      * :c:func:`zbus_chan_claim`
      * :c:func:`zbus_chan_finish`
      * :c:func:`zbus_chan_name`
      * :c:func:`zbus_chan_msg`
      * :c:func:`zbus_chan_const_msg`
      * :c:func:`zbus_chan_msg_size`
      * :c:func:`zbus_chan_user_data`
      * :c:func:`zbus_chan_add_obs`
      * :c:func:`zbus_chan_rm_obs`
      * :c:func:`zbus_runtime_obs_pool`
      * :c:func:`zbus_obs_set_enable`
      * :c:func:`zbus_obs_name`
      * :c:func:`zbus_sub_wait`
      * :c:func:`zbus_iterate_over_channels`
      * :c:func:`zbus_iterate_over_observers`

    * Added the related configuration options:

      * :kconfig:option:`CONFIG_ZBUS_CHANNEL_NAME`
      * :kconfig:option:`CONFIG_ZBUS_OBSERVER_NAME`
      * :kconfig:option:`CONFIG_ZBUS_STRUCTS_ITERABLE_ACCESS`
      * :kconfig:option:`CONFIG_ZBUS_RUNTIME_OBSERVERS_POOL_SIZE`

HALs
****

* Atmel

  * sam0: Added support for SAMC20/21.
  * sam4l: Added ``US_MR_CHRL_{n}_BIT`` Register Aliases for USART Driver.

* GigaDevice

  * Added support for gd32l23x.
  * Added support for gd32a50x.

* Nordic

  * Updated nrfx to version 2.10.0.

* STM32

  * stm32cube: updated stm32h7 to cube version V1.11.0.
  * stm32cube: updated stm32l5 to cube version V1.5.0.
  * stm32cube: updated stm32wl to cube version V1.3.0.

* Espressif

  * Added Ethernet driver support
  * Added light-sleep and deep-sleep support over PM interface
  * Added ADC and DAC driver support
  * Added GDMA driver support

Storage
*******

* Flash Map API drops ``fa_device_id`` from :c:struct:`flash_area`, as it
  is no longer needed by MCUboot, and has not been populated for a long
  time now.

Trusted Firmware-M
******************

* Updated to TF-M 1.7.0 (and MbedTLS 3.2.1).
* Initial attestation service has been disabled by default due to license
  issues with the QCBOR dependency. To enable it, set the path for QCBOR via
  ``CONFIG_TFM_QCBOR_PATH`` or set the path to ``DOWNLOAD``.
* Firmware update sample removed pending update to 1.0 FWU service.
* psa_crypto sample removed pending resolution of PSA API conflicts w/MbedTLS.

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

Issue Related Items
*******************

Known Issues
============

- :github:`33747` - gptp does not work well on NXP rt series platform
- :github:`37193` - mcumgr: Probably incorrect error handling with udp backend
- :github:`37731` - Bluetooth: hci samples: Unable to allocate command buffer
- :github:`40023` - Build fails for ``native_posix`` board when using C++ <atomic> header
- :github:`42030` - can: "bosch,m-can-base": Warning "missing or empty reg/ranges property"
- :github:`43099` - CMake: ARCH roots issue
- :github:`43249` - MBEDTLS_ECP_C not build when MBEDTLS_USE_PSA_CRYPTO
- :github:`43555` - Variables not properly initialized when using data relocation with SDRAM
- :github:`43562` - Setting and/or documentation of Timer and counter use/requirements for Nordic Bluetooth driver
- :github:`44339` - Bluetooth:controller: Implement support for Advanced Scheduling in refactored LLCP
- :github:`44948` - cmsis_dsp: transform: error during building cf64.fpu and rf64.fpu for mps2_an521_remote
- :github:`45241` - (Probably) unnecessary branches in several modules
- :github:`45323` - Bluetooth: controller: llcp: Implement handling of delayed notifications in refactored LLCP
- :github:`45814` - Armclang build fails due to missing source file
- :github:`46121` - Bluetooth: Controller: hci: Wrong periodic advertising report data status
- :github:`46401` - ARM64: Relax 4K MMU mapping alignment
- :github:`46846` - lib: libc: newlib: strerror_r non-functional
- :github:`47120` - shell uart: busy wait for DTR in ISR
- :github:`47732` - Flash map does not fare well with MCU who do bank swaps
- :github:`47908` - tests/kernel/mem_protect/stack_random works unreliably and sporadically fails
- :github:`48094` - pre-commit scripts fail when there is a space in zephyr_base
- :github:`48102` - JSON parses uses recursion (breaks rule 17.2)
- :github:`48287` - malloc_prepare ASSERT happens when enabling newlib libc with demand paging
- :github:`48608` - boards: mps2_an385: Unstable system timer
- :github:`48841` - Bluetooth: df: Assert in lower link layer when requesting CTE from peer periodically with 7.5ms connection interval
- :github:`48992` - qemu_leon3: tests/posix/common/portability.posix.common fails
- :github:`49213` - logging.add.log_user test fails when compiled with GCC 12
- :github:`49390` - shell_rtt thread can starve other threads of the same priority
- :github:`49484` - CONFIG_BOOTLOADER_SRAM_SIZE should not be defined by default
- :github:`49492` - kernel.poll test fails on qemu_arc_hs6x when compiled with GCC 12
- :github:`49494` - testing.ztest.ztress test fails on qemu_cortex_r5 when compiled with GCC 12
- :github:`49614` - acrn_ehl_crb: The testcase tests/kernel/sched/schedule_api failed to run.
- :github:`49816` - ISOTP receive fails for multiple binds with same CAN ID but different extended ID
- :github:`49889` - ctf trace: unknown event id when parsing samples/tracing result on reel board
- :github:`50084` - drivers: nrf_802154: nrf_802154_trx.c - assertion fault when enabling Segger SystemView tracing
- :github:`50095` - ARC revision Kconfigs wrongly mixed with board name
- :github:`50196` - LSM6DSO interrupt handler not being called
- :github:`50501` - STM32 SPI does not work properly with async + interrupts
- :github:`50506` - nxp,mcux-usbd devicetree binding issues
- :github:`50546` - drivers: can: rcar: likely inconsistent behavior when calling can_stop() with pending transmissions
- :github:`50598` - UDP over IPSP not working on nRF52840
- :github:`50652` - RAM Loading on i.MXRT1160_evk
- :github:`50766` - Disable cross-compiling when using host toolchain
- :github:`50777` - LE Audio: Receiver start ready command shall only be sent by the receiver
- :github:`50875` - net: ip: race in access to writable net_if attributes
- :github:`50941` - sample.logger.syst.catalog.deferred_cpp fails on qemu_cortex_m0
- :github:`51024` - aarch32 excn vector not pinned in mmu causing newlib heap overlap
- :github:`51127` - UART HW DMA ( UART Communication based on HW DMA ) - Buffer Overflow test in STM32H743 Controller
- :github:`51133` - Bluetooth: audio: Sink ASE does not go to IDLE state
- :github:`51250` - ESP32-C3 pin glitches during start-up
- :github:`51317` - Confusing license references in nios2f-zephyr
- :github:`51342` - Bluetooth ISO extra ``stream_sent`` callback after ``seq_num`` 16-bit rollover
- :github:`51420` - tests: subsys: logging: log_links: logging.log_links fails
- :github:`51422` - nsim_em: tests/subsys/logging/log_link_order run failed on nsim_em
- :github:`51449` - device: device_get_binding is broken for nodes with the same name
- :github:`51604` - doc: is the documentation GDPR compliant since it uses Google Analytics without prompting the user about tracking?
- :github:`51637` - shell: bypass shell_fprintf ASSERT fail
- :github:`51728` - soc: xtensa: esp32_net: Remove binary blobs from source tree
- :github:`51774` - thread safety of adv_new in Bluetooth subsys
- :github:`51814` - ARC irq_offload doesn't honor thread switches
- :github:`51820` - Longer strings aren't logged
- :github:`51825` - west: runners: jlink: JLink.exe name collision
- :github:`51977` - newlib integration: _unlink isn't mapped to unlink
- :github:`52055` - Bluetooth: Controller: Broadcast scheduling issues
- :github:`52269` - UART documentation for uart_irq_tx_enable/disable incomplete
- :github:`52271` - west sign: imgtool: zephyr.signed.hex and zephyr.signed.bin do not have the same contents
- :github:`52362` - nrf_qspi_nor driver crash if power management is enabled
- :github:`52395` - Cannot build applications with dts having (unused) external flash partition and disabling those drivers
- :github:`52491` - Value of EVENT_OVERHEAD_START_US is set to low
- :github:`52494` - SPI NOR DPD comment is misleading/wrong
- :github:`52510` - twister: truncated handler.log reports test as "failed"
- :github:`52513` - sample.modules.chre fails on qemu_leon3
- :github:`52575` - Kconfig: excessive ``select`` usage
- :github:`52585` - PDM event handler shouldn't stop driver on allocation failure
- :github:`52589` - Add support for different SDHC high-speed modes (currently defaults to SDR25)
- :github:`52605` - esp32-usb-serial tx-complete interrupt not working in interrupt mode on esp32c3
- :github:`52623` - qemu_x86: thousands of timer interrupts per second
- :github:`52667` - nrf_rtc_timer: Booting application with zephyr < 3.0.0 from mcuboot with zephyr >= 3.0.0
- :github:`52700` - posix: getopt: implement standards-compliant reset
- :github:`52702` - drivers: wifi: esp_at: Some issues on Passive Receive mode
- :github:`52705` - RNDIS fails to enumerate on Raspberry Pi Pico
- :github:`52741` - bl5340_dvk_cpuapp has wrong button for mcuboot button
- :github:`52764` - boards: esp32c3_devkitm: unable to read memory-mapped flash memory
- :github:`52792` - ATWINC1500 : (wifi_winc1500_nm_bsp.c : nm_bsp_reset) The reset function is not logical and more
- :github:`52825` - Overflow in settime posix function
- :github:`52830` - Annoying Slirp Message console output from qemu_x86 board target
- :github:`52868` - ESP32 Wifi driver returns EIO (-5) if connecting without a sleep sometime before calling
- :github:`52869` - ESP32 Counter overflow, with no API to reset it
- :github:`52885` - modem: gsm_ppp: CONFIG_GSM_MUX: Unable to reactivate modem after executing gsm_ppp_stop()
- :github:`52886` - tests: subsys: fs: littlefs: filesystem.littlefs.default and filesystem.littlefs.custom fails
- :github:`52887` - Bluetooth: LL assert with chained adv packets
- :github:`52924` - ESP32 get the build message "IRAM0 segment data does not fit."
- :github:`52941` - Zephyr assumes ARM M7 core has a cache
- :github:`52954` - check_zephyr_package() only checks the first zephyr package rather than all the considered ones.
- :github:`52998` - tests: drivers: can: Build failure with sysroot path not quoted on Windows
- :github:`53000` - Delaying logging via CONFIG_LOG_PROCESS_THREAD_STARTUP_DELAY_MS doesn't work if another backend is disabled
- :github:`53006` - Hawbkit with b_l4s5i_iot01a - wifi_eswifi: Cannot allocate rx packet
- :github:`53008` - Invalid ISO interval configuration
- :github:`53088` - Unable to chage initialization priority of logging subsys
- :github:`53123` - Cannot run a unit test on Mac OSX with M1 Chip
- :github:`53124` - cmake: incorrect argument passing and dereference in zephyr_check_compiler_flag() and zephyr_check_compiler_flag_hardcoded()
- :github:`53137` - Bluetooth: Controller: HCI 0x45 error after 3rd AD fragment with data > 248 bytes
- :github:`53148` - Bluetooth: Controller: BT_HCI_OP_LE_BIG_TERMINATE_SYNC on syncing BIG sync returns invalid BIG handle
- :github:`53172` - SHTCx driver wrong negative temperature values
- :github:`53173` - HCI-UART: unable to preform a DFU - GATT CONN timeout
- :github:`53198` - Bluetooth: Restoring security level fails and missing some notifications
- :github:`53265` - Bluetooth: Controller: ISO interleaved broadcast not working
- :github:`53319` - USB CDC ACM UART driver's interrupt-driven API hangs when no host is connected
- :github:`53334` - Bluetooth: Peripheral disconnected with BT_HCI_ERR_LL_RESP_TIMEOUT reason and SMP timeout
- :github:`53343` - subsys: logging: use of timestamping during early boot may crash MMU-based systems
- :github:`53348` - Bluetooth: Restoring connection to peripheral issue
- :github:`53375` - net: lwm2m: write method when floating point
- :github:`53475` - The ATT_MTU value for EATT should be set as the minimum MTU of ATT Client and ATT Server
- :github:`53505` - Some device tree integers may be signed or unsigned depending on their value
- :github:`53522` - k_busy_wait function hangs on when CONFIG_SYSTEM_CLOCK_SLOPPY_IDLE is set with CONFIG_PM.
- :github:`53537` - TFM-M doesn't generate tfm_ns_signed.bin image for FOTA firmware upgrade
- :github:`53544` - Cannot see both bootloader and application RTT output
- :github:`53546` - zephyr kernel Kconfig USE_STDC_LSM6DS3TR and hal_st CMakeLists.txt lsm6ds3tr-c variable name mismatched (hyphen sign special case)
- :github:`53552` - LE Audio: Device executes receiver start ready before the CIS is connected
- :github:`53555` - ESP32-C3 Is RV32IMA, Not RV32IMC?
- :github:`53570` - SDHC SPI driver should issue CMD12 after receiving data error token
- :github:`53587` - Issue with Auto-IP and Multicast/socket connection
- :github:`53605` - tests: posix: common: portability.posix.common fails - posix_apis.test_clock_gettime_rollover
- :github:`53613` - tests: drivers: uart: uart_mix_fifo_poll: tests ``drivers.uart.uart_mix_poll_async_api_*`` fail
- :github:`53643` - Invalid warning when BLE advertising times out
- :github:`53674` - net: lwm2m: senml cbor formatter relying on implementation detail / inconsistency of lwm2m_path_to_string
- :github:`53680` - HawkBit Metadata Error
- :github:`53728` - Sensor API documentation: no mention of blocking behaviour
- :github:`53729` - Can not build for ESP32 sample program - Zephyr using CMake build
- :github:`53767` - `@kconfig` is not allowed in headline
- :github:`53780` - sysbuild with custom board compilation failed to find the board
- :github:`53790` - Flash Init fails when CONFIG_SPI_NOR_IDLE_IN_DPD=y
- :github:`53800` - Raspberry Pi Pico - ssd1306 display attempts to initialize before i2c bus is ready for communication
- :github:`53801` - k_busy_wait adds 1us delay unnecessarily
- :github:`53823` - Bluetooth init failed on nrf5340_audio_dk_nrf5340_cpuapp
- :github:`53855` - mimxrt1050_evk invalid writes to flash
- :github:`53858` - Response on the shell missing with fast queries
- :github:`53867` - kconfig: Linked code into external SEMC-controlled memory without boot header
- :github:`53871` - Bluetooth: IPSP Sample Crash on nrf52840dk_nrf52840
- :github:`53873` - Syscall parser creates syscall macro for commented/ifdefed out syscall prototype
- :github:`53917` - clang-format key incompatible with IntelliJ IDEs
- :github:`53933` - tests: lib: spsc_pbuf: lib.spsc_pbuf... hangs
- :github:`53937` - usb: stm32g0: sometimes get write error during CDC ACM enumeration when using USB hub
- :github:`53939` - USB C PD stack no callback for MSG_NOT_SUPPORTED_RECEIVED policy notify
- :github:`53964` - gpio_emul: ``gpio_*`` functions not callable within an ISR
- :github:`53980` - Bluetooth: hci: spi: race condition leading to deadlock
- :github:`53993` - platform: Raspberry Pi Pico area: USB Default config should be bus powered device for the Raspberry Pi Pico
- :github:`53996` - bt_conn_foreach() includes invalid connection while advertising
- :github:`54014` - usb: using Bluetooth HCI class in composite device leads to conflicts
- :github:`54037` - Unciast_audio_client sample application cannot work with servers with only sinks.
- :github:`54047` - Bluetooth: Host: Invalid handling of Service Changed indication if GATT Service is registered after Bluetooth initialization and before settings load
- :github:`54064` - doc: mgmt: mcumgr: img_mgmt: Documentation specifies that hash in state of images is a required field
- :github:`54076` - logging fails to build with LOG_ALWAYS_RUNTIME=y
- :github:`54085` - USB MSC Sample does not work for native_posix over USBIP
- :github:`54092` - ZCBOR code generator generates names not compatible with C++
- :github:`54101` - bluetooth: shell: Lots of checks of type (unsigned < 0) which is bogus
- :github:`54121` - Intel CAVS: tests/subsys/zbus/user_data fails
- :github:`54122` - Intel CAVS: tests/subsys/dsp/basicmath fails (timeout)
- :github:`54162` - Mass-Storage-Sample - USB HS support for the stm32f723e_disco board
- :github:`54179` - DeviceTree compile failures do not stop build
- :github:`54198` - reel board: Mesh badge demo fails to send BT Mesh message
- :github:`54199` - ENC28J60: dns resolve fails after few minutes uptime
- :github:`54200` - bq274xx incorrect conversions
- :github:`54211` - tests: kernel: timer: timer_behavior: kernel.timer.timer fails
- :github:`54226` - Code coverage collection is broken
- :github:`54240` - twister: --runtime-artifact-cleanup has no effect
- :github:`54273` - ci: Scan code workflow does not report a violation for unknown LicenseRef
- :github:`54275` - net: socket: tls: cannot send when using blocking socket
- :github:`54288` - modem: hl7800: power off draws excessive current
- :github:`54289` - Twister jobserver support eliminates parallel build for me
- :github:`54301` - esp32: Console doesn't work with power management enabled
- :github:`54317` - kernel: events: SMP race condition and one enhancement
- :github:`54330` - West build command execution takes more time or fails sometimes
- :github:`54336` - picolibc is incompatible with xcc / xcc-clang toolchains
- :github:`54364` - CANopen SYNC message is not received
- :github:`54373` - Mcuboot swap type is ``test`` when update fails
- :github:`54377` - mec172xevb: benchmark.kernel.core (and adc_api/drivers.adc) failing
- :github:`54407` - Bluetooth: Controller: ISO Central with continuous scanning asserts
- :github:`54411` - mgmt: mcumgr: Shell transport can lock shell up until device is rebooted
- :github:`54435` - mec172xevb: sample.drivers.sample.drivers.peci failing
- :github:`54439` - Missing documentation of lwm2m_rd_client_resume and lwm2m_rd_client_pause
- :github:`54444` - samples/modules/chre/sample.modules.chre should not attempt to build on toolchains w/o newlib
- :github:`54459` - hawkbit: wrong header size used while reading the version of the app
- :github:`54460` - Build system should skip ``zephyr/drivers/ethernet`` module if TAP & SLIP already provides a network driver in ``zephyr/drivers/net/slip.c``
- :github:`54498` - net: openthread: echo server do not work in userspace
- :github:`54500` - jwt: memory allocation problem after multiple jwt_sign calls
- :github:`54504` - LwM2M: Connection resume does not work after network error
- :github:`54506` - net: ieee802154_6lo: wrong fragmentation of packets with specific payload sizes
- :github:`54531` - Bluetooth: Controller: le_ext_create_connection fails with initiating_PHYs == 0x03
- :github:`54532` - Tests: Bluetooth: tester: BTP communication is not fully reliable on NRF52 board using UART
- :github:`54538` - LE Audio: BAP Unicast Client Idle/CIS disconnect race condition
- :github:`54539` - LE Audio: Unicast client should only disconnect CIS if both ASEs are not in streaming state
- :github:`54542` - Bluetooth pending tx packets assert on disable
- :github:`54554` - arch.arm.swap.tz fails to build for v2m_musca_b1_ns
- :github:`54576` - Errors during IPv4 defragmenting
- :github:`54577` - IPv6 defragmenting fails when segments do not overlap
- :github:`54581` - STM32H7 adc sequence init function unstable logic return
- :github:`54599` - net stats: many received TCP packets count as "dropped"
- :github:`54609` - driver: led: kconfig symbols mix up
- :github:`54610` - samples: kernel: metairq_dispatch: sample.kernel.metairq_dispatch hangs
- :github:`54630` - memcpy crashes with NEW_LIBC on stm32 cortex m7 with debugger attached
- :github:`54668` - shell: "log backend" command causes shell to lock up
- :github:`54670` - stm32: memcpy crashes with NEWLIBC
- :github:`54674` - modem: hl7800: DNS resolver does not start for IPv6 only
- :github:`54683` - Missing input validation in gen_driver_kconfig_dts.py
- :github:`54695` - usb mass storage on mimxrt595_evk_cm33 mount very slow
- :github:`54705` - CDC USB shell receives garbage when application starts
- :github:`54713` - LVGL Module File System Memory Leaks
- :github:`54717` - --generate-hardware-map produces TypeError: expected string or bytes-like object on Windows
- :github:`54719` - STM32 clock frequency calculation error
- :github:`54720` - QEMU bug with branch delay slots on ARC
- :github:`54726` - LittleFS test only works for specific device parameters
- :github:`54731` - USB DFU sample does not reliably upload image on RT1050
- :github:`54737` - Wrong order of member initialization for macro Z_DEVICE_INIT
- :github:`54739` - C++ Compatibility for DEVICE_DT_INST_DEFINE
- :github:`54746` - ESP32 SPI word size is not respected
- :github:`54754` - outdated version of rpi_pico hal configures USB PLL incorrectly
- :github:`54755` - small timer periods take twice as much time as they should
- :github:`54768` - nrf9160dk_nrf52840: flow control pins crossed
- :github:`54769` - Error when flashing to LPCXpresso55S06 EVK.
- :github:`54770` - Bluetooth: GATT: CCC and CF values written by privacy-disabled peer before bonding may be lost
- :github:`54773` - Bluetooth: GATT: Possible race conditions related to GATT database hash calculation after settings load
- :github:`54779` - file write gives -5 after file size reaches cache size
- :github:`54783` - stm32: NULL dereference in net_eth_carrier_on
- :github:`54785` - .data and .bss relocation to DTCM & CCM is broken with SDK 0.15+
- :github:`54798` - net: ipv4: IP packets get dropped in Zephyr when an application is receiving high rate data
- :github:`54805` - when invoke dma_stm32_disable_stream failed in interrupt callback, it will endless loop
- :github:`54813` - Bluetooth: host: Implicit sc_indicate declaration when Service Changed is disabled
- :github:`54824` - BT: Mesh: Utilizes some not initialized variables
- :github:`54826` - Clang/llvm build is broken: Error: initializer element is not a compile-time constant
- :github:`54833` - ESPXX failing in gpio tests
- :github:`54841` - Drivers: I2S: STM32: Mishandling of Master Clock output (MCK)
- :github:`54844` - RAK5010 board has wrong LIS3DH INT pin configured
- :github:`54846` - ESP32C3 SPI DMA host ID
- :github:`54855` - ESP32: Compilation errors after migrating to zephyr 3.2.0
- :github:`54856` - nRF52840 nRF52833 Bluetooth: Timeout in ``net_config_init_by_iface`` but interface is up
- :github:`54859` - LE Audio: BT_AUDIO_UNICAST_CLIENT_GROUP_STREAM_COUNT invalid descsi
- :github:`54861` - up_squared: CHRE sample output mangling fails regex verification

Addressed issues
================

* :github:`54873` - doc: Remove Google Analytics tracking code from generated documentation.
* :github:`54858` - espressif blobs does not follow zephyr requirements
* :github:`54872` - west flash --elf-file is not flashing using .elf file, but using zephyr.hex to flash
* :github:`54813` - Bluetooth: host: Implicit sc_indicate declaration when Service Changed is disabled
* :github:`54804` - Warning (simple_bus_reg): /soc/can: missing or empty reg/ranges property
* :github:`54786` - doc: Version selector should link to latest LTS version instead of 2.7.0
* :github:`54782` - nrf_rtc_timer may not properly handle a timeout that is set in specific conditions
* :github:`54770` - Bluetooth: GATT: CCC and CF values written by privacy-disabled peer before bonding may be lost
* :github:`54763` - doc: Copyright notice should be updated to 2015-2023
* :github:`54760` - net_lwm2m_engine: fcntl(F_GETFL) failed (-22) on es-wifi
* :github:`54730` - intel_adsp_ace15_mtpm: cpp.main.minimal test failing
* :github:`54718` - The rf2xx driver uses a wrong bit mask on TRAC_STATUS
* :github:`54710` - Sending NODE_RX_TYPE_CIS_ESTABLISHED messes up LLCP
* :github:`54703` - boards: thingy53: Inconsistent method of setting USB related log level
* :github:`54702` - boards: thingy53: USB remote wakeup is not correctly disabled
* :github:`54686` - RP2040: Cleanup incorrect comment and condition from the USB driver
* :github:`54685` - drivers: serial: rp2040: fix rpi pico address mapping
* :github:`54671` - Bluetooth: spurious error when using hci_rpmsg
* :github:`54666` - LE Audio: EALREADY error of ase_stream_qos() not mapped
* :github:`54659` - boards: arm: nrf52840dongle_nrf52840: Defaults to UART which is not connected (and mcuboot build fails)
* :github:`54654` - LE Audio: Kconfig typo in ``pacs.c``
* :github:`54642` - Bluetooth: Controller: Assertion on disconnecting CIS and assertion on synchronizing to first encrypted BIS
* :github:`54614` - Cannot flash b_l4s5i_iot01a samples/hello_world
* :github:`54613` - Bluetooth: Unable to enable PAST as advertiser without periodic sync support
* :github:`54605` - native_posix_64 platform broken in Twister
* :github:`54597` - SRAM2 wrong on certain stm32h7 SOC (system crashes during startup)
* :github:`54580` - samples/subsys/task_wdt fails with timeout on s32z270dc2_r52 boards
* :github:`54575` - Automatic termination with return code from a native_posix main function
* :github:`54574` - USB RNDIS Reception and Descriptor Issue(s)
* :github:`54573` - gpio_hogs test uses an incorrect GPIO spec handle
* :github:`54572` - QEMU networking breakage (Updating nrf-sdk 2.1->2.2 , implies zephyr 3.1 -> 3.2)
* :github:`54569` - MMC subsys shares sdmmc kconfigs
* :github:`54567` - Assertion in z_add_timeout() fails in drivers.uart.uart_mix_poll_async_api test
* :github:`54563` - Variable uninitialised in flash_stm32_page_layout
* :github:`54558` - LPTIM Kconfig-related build failures for nucleo_g431rb
* :github:`54557` - sample.drivers.flash.shell fails to build for adafruit_kb2040
* :github:`54556` - sample.display.lvgl.gui fails to build for stm32f429i_disc1
* :github:`54545` - boards: rpi_pico: Bad MPU settings
* :github:`54544` - Bluetooth: controller: HCI/CCO/BI-45-C setHostFeatureBit failing
* :github:`54540` - psa_crypto variants of drivers/entropy/api and crypto/rand32 tests fail to build for nrf9160dk_nrf9160_ns and nrf5340dk_nrf5340_cpuapp_ns
* :github:`54537` - logging.add.async build fails on mtpm with xcc-clang
* :github:`54534` - PSoC6/Cat1 add binary blob for Cortex-M0+ core
* :github:`54533` - tests/drivers/can/timing fails on nucleo_f746zg
* :github:`54529` - Bluetooth: shell: Missing help messages and parameters
* :github:`54528` - log switch_format, mipi_syst tests failing on intel_adsp_ace15_mtpm
* :github:`54522` - Can we embrace GNU Build IDs?
* :github:`54516` - twister: Quarantine verify works incorrectly with integration mode
* :github:`54509` - Zephyr does not configure TF-M correctly for Hard-Float
* :github:`54507` - CONFIG_PM=y results to hard fault system for STM32L083
* :github:`54499` - stm32u5 lptimer driver init must wait after interrupt reg
* :github:`54493` - samples/drivers/counter/alarm/ fails on nucleo_f746zg
* :github:`54492` - west: twister return code ignored by west
* :github:`54484` - Intel CAVS25: tests/boards/intel_adsp/ssp/ fails
* :github:`54472` - How to enable a node in main.
* :github:`54469` - nsim_sem and nsim_em7d_v22 failed in zdsp.basicmath test
* :github:`54462` - usb_dc_rpi_pico driver enables some interrupts it doesn't handle
* :github:`54461` - SAM spi bus inoperable when interrupted on fast path
* :github:`54457` - DHCPv4 starts even when interface is not operationally up
* :github:`54455` - Many tests have wrong component and are wrongly categorized
* :github:`54454` - Twister summary in some cases provides an irrelevant example
* :github:`54450` - nuvoton_pfm_m487 failed to build due to missing M48x-pinctrl.h
* :github:`54440` - tests/net/lib/lwm2m/lwm2m_registry/subsys.net.lib.lwm2m.lwm2m_registry fails to build w/toolchains that don't support newlib
* :github:`54438` - question: why lwm2m_rd_client_stop might block
* :github:`54431` - adafruit kb2040 board configuration is invalid and lack flash controller
* :github:`54428` - esp32 invalid flash dependencies
* :github:`54427` - stm32 uart driver ``LOG_`` msg crashes when entering sleep mode
* :github:`54422` - modules: openthread: multiple definition in openthread config
* :github:`54417` - Intel CAVS18: tests/subsys/dsp/basicmath fails
* :github:`54414` - stm32u5 dma driver does not support repeated start-stop
* :github:`54412` - [hci_uart] nrf52840 & BlueZ 5.55 - start / stop scanning breaks
* :github:`54410` - [BUG] TLB driver fails to unmap L2 HPSRAM region when assertions are enabled
* :github:`54409` - ETH MAC config for STM32H7X and STM32_HAL_API_V2 too late and fails
* :github:`54405` - Nominate @Vge0rge as contributor
* :github:`54401` - Uninitialized has_param struct sometimes causes BSIM "has.sh" test to fail
* :github:`54399` - Intel CAVS18: tests/subsys/zbus/user_data/user_data.channel_user_data FAILED
* :github:`54397` - Test posix_header fails on some STM32 Nucleo boards
* :github:`54395` - mgmt: mcumgr: img_grp: Upload inspect fails when using swap using scratch
* :github:`54393` - Bluetooth: Controller: Starting a second BIG causes them to overlap and have twice the interval
* :github:`54387` - soc: arm: st_stm32: Incorrect SRAM devicetree definition for the STM32L471xx
* :github:`54384` - Removal of old runner options caused downstream breakage
* :github:`54378` - Net pkt PPP dependency bug
* :github:`54374` - ARC: west runner: mdb: incorrect handling of unsupported jtag adapters
* :github:`54372` - ARC: west runner: mdb: unexpected empty argument pass to MDB executable
* :github:`54366` - tests: pin_get_config failed on it8xxx2_evb, again
* :github:`54361` - Incorrect network stats for Neighbour Discovery packets
* :github:`54360` - enable HTTPS server on Zephyr RTOS
* :github:`54356` - Bluetooth: Scanner consumption while scanning
* :github:`54351` - Tests: bluetooth: failing unittests
* :github:`54347` - zephyr/posix/fcntl.h header works differently on native_posix platform
* :github:`54344` - Bluetooth: Controller: Central ACL connections overlap Broadcast ISO BIG event
* :github:`54342` - Bluetooth: Controller: Connected ISO Central causes Peripheral to drop ISO data PDUs
* :github:`54341` - Bluetooth: Controller: Direction finding samples do not reconnect after disconnection
* :github:`54335` - tests/kernel/fatal/no-multithreading/kernel.no-mt.cpu_exception failing on qemu_cortex_m3
* :github:`54334` - Need support to define partitions for usage with mcuboot
* :github:`54332` - LwM2M engine is does not go into non-block mode anymore in native_posix target
* :github:`54327` - intel_adsp: ace: various multicore bugs, timeouts
* :github:`54321` - ARC: unusable console after west flash or west debug with mdb runners
* :github:`54318` - boards: nucleo_g474re: openocd runner is not stable enough for intensive testing
* :github:`54316` - RTT is not working correctly on STM32U5 series
* :github:`54315` - Problem seen with touch screen on RT1170 when running the LVGL sample
* :github:`54310` - Using NOCOPY code relocation generates a warning flag
* :github:`54287` - modem: hl7800: PSM hibernate draws excessive current
* :github:`54274` - newlib: Document CONFIG_NEWLIB_LIBC_NANO default change
* :github:`54258` - boards: arm: twr_ke18f: LPTMR always enabled, resulting in low system timer resolution
* :github:`54254` - tests: canbus: isotp: conformance: test fails with CONFIG_SYS_CLOCK_TICKS_PER_SEC=100
* :github:`54253` - v3.3.0-rc1: stm32: IPv6 neighbour solicitation packets are not received without CONFIG_ETH_STM32_MULTICAST_FILTER
* :github:`54247` - tests: net: tcp: net.tcp.simple fails
* :github:`54246` - Sample:subsys/ipc/rpmsg_server:The two cores cannot communicate in nRF5340
* :github:`54241` - The cy8c95xx I2C GPIO expander support was broken in #47841
* :github:`54236` - b_u585i_iot02a_ns: Can't build TFM enabled samples (TF-M 1.7.0)
* :github:`54230` - STM32H7: Kernel crash with BCM4=0
* :github:`54225` - Intel CAVS: tests/lib/c_lib/ fails
* :github:`54224` - Issues with picolibc on xtensa platforms
* :github:`54223` - Intel CAVS: tests/kernel/common/kernel.common.picolibc fails
* :github:`54214` - Display framebuffer allocation
* :github:`54210` - tests: drivers: udc drivers.udc fails
* :github:`54209` - USB C PD dead battery support
* :github:`54208` - Various RISC-V FPU context switching issues
* :github:`54205` - Regression: RiscV FPU regs not saved in multithreaded applications
* :github:`54202` - decawave_dwm1001_dev: i2c broken due to pinctrl_nrf fix
* :github:`54190` - RP2040 cannot be compiled with C11 enabled
* :github:`54173` - Bluetooth: GATT: Change awareness of bonded GATT Client is not maintained on reconnection after reboot
* :github:`54172` - Bluetooth: GATT: Written value of gatt_cf_cfg data may be dropped on power down
* :github:`54148` - qemu_x86_tiny places picolibc text outside of pinned.text
* :github:`54140` - BUS FAULT when running nmap towards echo_async sample
* :github:`54139` - Bluetooth: Audio: race hazard bt_audio_discover() callback vs unicast_client_ase_cp_discover()
* :github:`54138` - Buetooth: shell: `bt adv-data` isn't working properly
* :github:`54136` - Socket error after deregistration causes RD client state machine to re-register
* :github:`54123` - Intel CAVS 25: tests/boards/intel_adsp/ssp fails
* :github:`54117` - West flash fails on upload to Nucleo F303K8
* :github:`54104` - Bluetooth: Host: Bonding information distribution in the non-bondable mode
* :github:`54087` - tests:igmp:frdm_k64f: igmp test fails
* :github:`54086` - test:rio:frdm_k64f: rio user_space test fails with zephyr-v3.2.0-3842-g7ffc20082023
* :github:`54078` - No activity on can_tx when running the ISO-TP sample
* :github:`54072` - Bluetooth: Host: Periodic scanner does not differentiate between partial and incomplete data
* :github:`54065` - How To Change C++ Version Compilation Option For Freestanding Application?
* :github:`54053` - CI:frdm_k64f: kernel.common.stack_protection test failure
* :github:`54034` - QSPI: Unable to build the project when introduced DMA into external flash interfacing
* :github:`54017` - Modules: TF-M: Resolve QCBOR issues with TF-M 1.7.0
* :github:`54005` - esp32 Severe crash using modules with embedded PSRAM (eg esp32-wroom-32E-n8r2)
* :github:`54002` - mgmt: mcumgr: bluetooth transport: Inability to use refactored transport as a library in some circumstances
* :github:`53995` - drivers: ethernet: stm32: Enable ethernet statistics in the driver
* :github:`53994` - net: ethernet: Multicast receive packets statistics are not getting updated
* :github:`53991` - LE Audio: samples/bluetooth/broadcast_audio_sink configure error
* :github:`53989` - STM32 usb networking stack threading issue
* :github:`53967` - net: http client: HTTP timeout can lead to deadlock of global system queue
* :github:`53954` - tests: lib: ringbuffer: libraries.ring_buffer hangs
* :github:`53952` - USB C PD sink sample stops working when connected to a non-PD source
* :github:`53942` - Websocket: No close message on websocket close
* :github:`53936` - Enabling CONFIG_TRACING and CONFIG_EVENTS causes undefined reference error
* :github:`53935` - stm32 iwdt wdt_install_timeout not working properly
* :github:`53926` - Bluetooth Mesh stack question
* :github:`53916` - Multichannel PWM for STM32U575
* :github:`53913` - net: ip: igmp: IGMP doesn't get initialised because the iface->config.ip.ipv4 pointer is not initialised
* :github:`53911` - Info request: SMP Hash
* :github:`53900` - led_set_brightness() is not setting brightness after led_blink() for STM32U575
* :github:`53885` - Ethernet TCP Client Issue description with iperf/zperf
* :github:`53876` - The handle of att indication violates the spec
* :github:`53862` - Switching from USB to UART
* :github:`53859` - RFC: Board porting guide: Do not assume OS or default ports for board files
* :github:`53808` - Improve PLLI2S VCO precision
* :github:`53805` - peripheral_dis compilation reports RAM overflow for BBC microbit
* :github:`53799` - Info request: SMP hash definition
* :github:`53786` - BLE:DF: slot_plus_us is not set properly
* :github:`53782` - nrf5340dk: missing i2c bias pull up
* :github:`53781` - Allow resetting STM32 peripherals through RCC peripheral reset register
* :github:`53777` - mgmt: mcumgr: Change transport selects to depends on
* :github:`53773` - drivers: ethernet: stm32: Completion of enabling the multicast hash filter
* :github:`53756` - esp32 - Wrong value for the default cpu freq -> crash on assert
* :github:`53753` - [DOC] Mismatch of driver sample overview
* :github:`53744` - ztest: assert() functions does not always retuns
* :github:`53723` - device tree macro: GPIO_DT_SPEC_INST_GET_BY_IDX_OR does not work
* :github:`53720` - Bluetooth: Controller: Incorrect address type for PA sync established
* :github:`53715` - mec15xxevb_assy6853: broken UART console output
* :github:`53707` - fs: fcb: Add option to disable CRC for FCB entries
* :github:`53697` - Can not run the usb mass storage demo on NXP mimxrt595_evk_cm33
* :github:`53696` - sysbuild should not parse the board revision conf
* :github:`53689` - boards: nrf52840dongle_nrf52840 is missing storage partition definition
* :github:`53676` - net: lwm2m: inconsistent path string handling throughout the codebase
* :github:`53673` - samples: posix: gettimeofday does not build on native_posix
* :github:`53663` - Error while trying to flash nucleo_f446re: TARGET: stm32f4x.cpu - Not halted
* :github:`53656` - twister: samples: Bogus yaml for code_relocation sample
* :github:`53654` - SPI2 not working on STM32L412
* :github:`53652` - samples: mgmt: mcumgr: fs overlay does not work
* :github:`53642` - Display driver sample seems to mix up RGB565 and BGR565
* :github:`53636` - cdc_acm fails on lpcxpresso55s69 board
* :github:`53630` - net: ieee802154_6lo: REGRESSION: L2 MAC byte swapping results in wrong IPHC decompression
* :github:`53617` - unicast_audio_client and unicast_audio_server example assert
* :github:`53612` - file system (LFS?): failure to expand file with truncate
* :github:`53610` - k_malloc and settings
* :github:`53604` - testsuite: Broken Kconfig prevents building tests for nrf52840dk_nrf52840 platform
* :github:`53584` - STM32F1 PWM Input Capture Issue
* :github:`53579` - add_compile_definitions does not "propagate upwards" as supposed to when using west
* :github:`53568` - Kconfig search has white on white text
* :github:`53566` - Use fixup commits during code review
* :github:`53559` - mgmt: mcumgr: explore why UART interface is so slow (possible go application fault)
* :github:`53556` - Cannot add multiple out-of-the-tree secure partitions
* :github:`53549` - mgmt: mcumgr: callbacks: Callbacks events for a single group should be able to be combined
* :github:`53548` - net: ip: igmp: Mechanism to add MAC address for IGMP all systems multicast address to an ethernet multicast hash filter
* :github:`53535` - define a recommended process in zephyr for a CI framework integration
* :github:`53520` - tests: drivers: gpio: gpio_api_1pin: peripheral.gpio.1pin fails
* :github:`53513` - STM32 PWM Input Capture Issue
* :github:`53500` - ``net_tcp: context->tcp == NULL`` error messages during TCP connection
* :github:`53495` - RFC: treewide: python: argparse default configuration allows shortened command arguments
* :github:`53490` - Peridic current spike using tickless Zephyr
* :github:`53488` - Missing UUID for PBA and TMAS
* :github:`53487` - west: flash: stm32cubeprogrammer: Reset command line argument is wrong
* :github:`53474` - Sysbuild cmake enters infinite loop if 2 images are added to a build with the same name
* :github:`53470` - Unable to build using Arduino Zero
* :github:`53468` - STM32 single-wire UART not working when poll-out more than 1 char
* :github:`53466` - Nucleo F413ZH CAN bus support
* :github:`53458` - IPv4 address autoconfiguration leaks TX packets/buffers
* :github:`53455` - GATT: Deadlock while sending GATT notification from system workqueue thread
* :github:`53451` - USB: Suspending CDC ACM can lead to endpoint/transfer state mismatch
* :github:`53446` - Bluetooth: Controller: ll_setup_iso_path not working if both CIS and BIS supported
* :github:`53438` - unable to wakeup from Stop2 mode
* :github:`53437` - WaveShare xnucleo_f411re: Error: ``** Unable to reset target **``
* :github:`53433` - flash content erase in bootloader region at run time
* :github:`53430` - drivers: display: otm8009a: import 3rd-party source
* :github:`53425` - Do we have support for rtc subsecond calculation in zephyr.
* :github:`53424` - Reopen issue #49390
* :github:`53423` - [bisected] logging.log_msg_no_overflow is failing on qemu_riscv64
* :github:`53421` - cpp: defined popcount macro prevents use of std::popcount
* :github:`53419` - net: stats: DHCP packets are not counted as part of UDP counters
* :github:`53417` - CAN-FD / MCAN driver: Possible variable overflow for some MCUs
* :github:`53407` - Support of regex
* :github:`53385` - SWD not working on STM32F405
* :github:`53366` - net: ethernet: provide a way to get ethernet config
* :github:`53361` - Adding I2C devices SX1509B to Devicetree with the same address on different busses generates FATAL ERROR.
* :github:`53360` - kernel: k_msgq: add peek_more function
* :github:`53347` - WINC1500 socket recv fail
* :github:`53340` - net: lwm2m: add BinaryAppDataContainer object (19)
* :github:`53335` - Undeclared constants in devicetree_generated.h
* :github:`53326` - usb: device: usb_dfu: k_mutex is being called from isr
* :github:`53315` - Fix possible underflow in tcp flags parse.
* :github:`53306` - scripts: utils: migrate_mcumgr_kconfigs.py: Missing options
* :github:`53301` - Bluetooth: Controller: Cannot recreate CIG
* :github:`53294` - samples: mgmt: mcumgr: smp_svr: UDP file can be removed
* :github:`53293` - mgmt: mcumgr: CONFIG_MCUMGR_GRP_IMG_REJECT_DIRECT_XIP_MISMATCHED_SLOT causes a build failure
* :github:`53285` - SNTP & DATE_TIME & SERVER ADDRESS configuration and behavior
* :github:`53280` - Bluetooth: security level failure with multiple links
* :github:`53276` - doc: mgmt: mcumgr: fix "some unspecified" error
* :github:`53271` - modules: segger: KConfigs are broken
* :github:`53259` - RFC: API Change: dma: callback status
* :github:`53254` - Bluetooth: bt_conn_foreach() reports unstable conn ref before the connection is completed
* :github:`53247` - ATT timeout followed by a segmentation fault
* :github:`53242` - Controller in HCI UART RAW mode responds to Stop Discovery mgmt command with 0x0b status code
* :github:`53240` - Task Watchdog Fallback Timeout Before Installing Timeout - STM32
* :github:`53236` - Make USB VBUS sensing configurable for STM32 devices
* :github:`53231` - drivers/flash/flash_stm32l5_u5.c : unable to use full 2MB flash with TF-M activated
* :github:`53227` - mgmt: mcumgr: Possible instability with USB CDC data transfer
* :github:`53223` - LE Audio: Add interleaved packing for LE audio
* :github:`53221` - Systemview trace id overlap
* :github:`53209` - LwM2M: Replace pathstrings from the APIs
* :github:`53194` - tests: kernel: timer: starve: DTS failure stm32f3_seco_d23
* :github:`53189` - Mesh CI failure with BT_MESH_LPN_RECV_DELAY
* :github:`53175` - Select pin properties from shield overlay
* :github:`53164` - mgmt: mcumgr: NMP Timeout with smp_svr example
* :github:`53158` - SPIM transaction timeout leads to crash
* :github:`53151` - fs: FAT_FS_API MKFS test uses driver specific calls
* :github:`53147` - usb: cdc_acm: log related warning promoted to error
* :github:`53141` - For pinctrl on STM32 pin cannot be defined as push-pull with low level
* :github:`53129` - Build fails on ESP32 when enabling websocket client API
* :github:`53103` - Zephyr shell on litex : number higher than 10 are printed as repeated hex
* :github:`53101` - esp32: The startup code hangs after reboot via sys_reboot(...)
* :github:`53094` - Extend Zperf command
* :github:`53093` - ARM: Ability to query CFSR on exception
* :github:`53059` - Bluetooth: peripheral GATT notification call takes a lot of time
* :github:`53049` - Bluetooth: LL assertion fail with peripherals connect/disconnect rounds
* :github:`53048` - Bluetooth: legacy advertising doesn't resume if CONFIG_BT_EXT_ADV=y
* :github:`53046` - Bluetooth: Failed to set security level for the second connection
* :github:`53043` - Bluetooth: Peripheral misses notifications from Central after setting security level
* :github:`53033` - tests-ci : libraries: encoding: jwt test Failed
* :github:`53034` - tests-ci : os: mgmt: info_net test No Console Output(Timeout)
* :github:`53035` - tests-ci : crypto: rand32: random_ctr_drbg test No Console Output(Timeout)
* :github:`53036` - tests-ci : testing: ztest: base.verbose_1 test No Console Output(Timeout)
* :github:`53037` - tests-ci : net: mqtt_sn: client test No Console Output(Timeout)
* :github:`53038` - tests-ci : net: socket: mgmt test No Console Output(Timeout)
* :github:`53039` - tests-ci : net: socket: af_packet.ipproto_raw test No Console Output(Timeout)
* :github:`53040` - tests-ci : net: ipv4: fragment test No Console Output(Timeout)
* :github:`53019` - random: Zephyr enables TEST_RANDOM_GENERATOR when it should not
* :github:`53012` - stm32u5: timer api: lptim: k_msleep is twice long as expected
* :github:`53010` - timers: large drift if hardware timer has a low resolution
* :github:`53007` - Bluetooth: Notification callback is called with incorrect connection reference
* :github:`53002` - Incorrect hardware reset cause sets for watchdog reset on stm32h743zi
* :github:`52996` - kconfig: Use of multiple fragment files with OVERLAY_CONFIG not taking effect
* :github:`52995` - Add triage permissions for dianazig
* :github:`52983` - Bluetooth: Audio: BT_CODEC_LC3_CONFIG_DATA fails to compile with _frame_blocks_per_sdu > 1
* :github:`52981` - LOG_MODE_MINIMAL do not work with USB_CDC for nRF52840
* :github:`52975` - posix: clock: current method of capturing elapsed time leads to loss in seconds
* :github:`52970` - samples/net/wifi example does not work with ESP32
* :github:`52962` - Bluetooth non-functional on nRF5340 target
* :github:`52935` - Missing Libraries
* :github:`52931` - Filesystem Write Fails with Some SD-Cards
* :github:`52925` - Using #define LOG_LEVEL 0 does not filter out logs
* :github:`52920` - qemu_cortex_r5: tests/ztest/base
* :github:`52918` - qemu_cortex_r5` CI fails
* :github:`52914` - drivers: adc: ADC_CONFIGURABLE_INPUTS confict between 2 ADCs
* :github:`52913` - twister build fails but returns exit code of 0
* :github:`52909` - usb: usb don't work after switch from Zephyr 2.7.3 to 3.2.99 on i.MX RT1020
* :github:`52898` - mgmt: mcumgr: replace cmake functions without zephyr prefix to have zephyr prefix
* :github:`52882` - Sample applications that enable USB and error if it fails are incompatible with boards like thingy53 with auto-USB init (i.e. USB CDC for logging)
* :github:`52878` - Bluetooth: Unable use native_posix with shell demo
* :github:`52872` - Logging to USB CDC ACM limited to very low rate
* :github:`52870` - ESP32-C3 System clock resolution improvements
* :github:`52857` - Adafruit WINC1500 Wifi Shield doesn't work on nRF528XX
* :github:`52855` - Improve artifact generation for split build/test operation of twister
* :github:`52854` - twister build fails but returns exit code of 0
* :github:`52838` - Bluetooth: audio：invalid ase state transition
* :github:`52833` - Bluetooth Controller assertion on sys_reboot() with active connections (lll_preempt_calc: Actual EVENT_OVERHEAD_START_US)
* :github:`52829` - kernel/sched: Fix SMP race on pend
* :github:`52818` - samples: subsys: usb: shell: sample.usbd.shell  fails - no output from console
* :github:`52817` - tests: drivers: udc: dirvers.udc fails
* :github:`52813` - stm32h7: dsi: ltdc: clock: PLL3: clock not set up correctly or side effect
* :github:`52812` - Various problems with pipes (Not unblocking, Data Access Violation, unblocking wrong thread...)
* :github:`52805` - Code crashing due to ADC Sync operation (STM32F4)
* :github:`52803` - Kconfig: STM32F4 UF2 family ID
* :github:`52795` - Remove deprecated tinycbor module
* :github:`52794` - Possible regression for printk() output of i2c sensor data on amg88xx sample
* :github:`52788` - Re-enable LVGL support for M0 processors.
* :github:`52784` - NRF_DRIVE_S0D1 option is not always set in the nordic,nrf-twi and nordic,nrf-twim nodes, when using shield?
* :github:`52779` - Error While adding the mcuboot folder in repo.
* :github:`52776` - ite: eSPI driver: espi_it8xxx2_send_vwire() is not setting valid flag along with respective virtual wire when invoked from app code.
* :github:`52754` - tests/drivers/bbram: Refactor to use a common prj.conf
* :github:`52749` - posix: getopt: cannot use getopt() in a standard way
* :github:`52739` - Newlib defines POSIX primitives when -std=gnu
* :github:`52721` - Minimal logging does not work if printk and boot banner are disabled
* :github:`52718` - Simplify handling of fragments in ``net_buf``
* :github:`52709` - samples: subsys: nvs: sample.nvs.basic does not complete within twister default timeout
* :github:`52708` - samples: drivers: watchdog: sample.drivers.watchdog loops endlessly
* :github:`52707` - samples: subsys: task_wdt: sample.subsys.task_wdt loops endlessly
* :github:`52703` - tests: subsys: usb: device: usb.device USAGE FAULT exception
* :github:`52691` - cannot read octospi flash when partition size exceeds 4mb
* :github:`52690` - coredump: stm32l5: I-cache error on coredump backend in flash
* :github:`52675` - I2C: STM32F0 can't switch to HSI clock by default and change timing
* :github:`52673` - mcux: flexcan: forever waiting semaphore in can_send()
* :github:`52670` - tests: subsys: usb: device: usb.device hangs
* :github:`52652` - drivers: sensors: bmi08x config file
* :github:`52641` - armv7: mpu: RASR size field incorrectly initialized
* :github:`52632` - MQTT over WebSockets: After hours of running time receiving published messages is strongly delayed
* :github:`52628` - spi: NXP MCUX LPSPI driver does not correctly change baud rate once configured
* :github:`52626` - Improve LLCP unit tests
* :github:`52625` - USB device: mimxrt685_evk_cm33: premature ZLP during control IN transfer
* :github:`52614` - Empty "west build --test-item" doesn't report warnings/errors
* :github:`52602` - tests: subsys: settings: file_littlefs: system.settings.file_littlefs.raw fails
* :github:`52598` - esp32c3 Unable to do any timing faster than 1ms
* :github:`52595` - Twister: unable to run tests on real hardware
* :github:`52588` - ESP32c3 SPI driver DMA mode limited to 64 byte chunks
* :github:`52566` - Error while build esp32 samples/hello_world
* :github:`52563` - spi_transceive with DMA for STM32 does not work for devices with 16 bit words.
* :github:`52561` - fsl_flexcan missing flags
* :github:`52559` - Compiler cannot include C++ headers when both PICOLIBC and LIB_CPLUSPLUS options are set
* :github:`52556` - adc driver sample was failing with STM32 nucleo_f429zi board
* :github:`52548` - Late device driver initialization
* :github:`52539` - Broken linker script for the esp32 platform
* :github:`52534` - Missing include to src/core/lv_theme.h in src/themes/mono/lv_theme_mono.h
* :github:`52528` - Zephyr support for Nordic devices Thingy with TFLM
* :github:`52527` - net: lwm2m: wrong SenML CBOR object link encoding
* :github:`52526` - Simplify lvgl.h file by creating more header files inside module's sub-filoders (src/widgets and src/layouts)
* :github:`52518` - lib: posix: usleep() does not follow the POSIX spec
* :github:`52517` - lib: posix: sleep() does not return the number of seconds left if interrupted
* :github:`52506` - GPIO multiple gaps cause incorrect pinout check
* :github:`52493` - net: lwm2m: add 32 bits floating point support
* :github:`52486` - Losing connection with JLink on STM32H743IIK6 with Zephyr 2.7.2
* :github:`52479` - incorrect canopennode SDO CRC
* :github:`52472` - Set compiler options only for a custom/external module
* :github:`52464` - LE Audio: Unicast Client failing to create the CIG
* :github:`52462` - uart: stm32: UART clock source not initialized
* :github:`52457` - compilation error with "west build -b lpcxpresso54114_m4 samples/subsys/ipc/openamp/ "
* :github:`52455` - ARC: MWDT: minimal libc includes
* :github:`52452` - drivers: pwm: loopback test fails on frdm_k64f
* :github:`52449` - net: ip: igmp: IGMP v2 membership reports are sent to 224.0.0.2 instead of the group being reported
* :github:`52448` - esp32: subsys: settings: not working properly on esp_wrover_kit
* :github:`52443` - Concerns with maintaining separate kernel device drivers for fuel-gauge and charger ICs
* :github:`52415` - tests: kernel: timer: timer_behavior: kernel.timer.timer fails
* :github:`52412` - doc: mgmt: mcumgr: Clarify that hash in img_mgmt is a full sha256 hash and required
* :github:`52409` - STM32H7: ethernet: Device fails to receive any IP packets over ethernet after receiving UDP/IP multicast at a constant rate for some time
* :github:`52407` - tests: subsys: mgmt: mcumgr: Test needed that enables all features/Kconfigs
* :github:`52404` - mgmt: mcumgr: Callback include file has file name typo
* :github:`52401` - mgmt: mcumgr: Leftover files after directory update
* :github:`52399` - cmake error
* :github:`52393` - Controller: ACL packets NACKed after Data Length Update
* :github:`52376` - Cannot build apps with main in a .cpp file
* :github:`52366` - LE Audio: Missing released callback for streams on ACL disconnect
* :github:`52360` - samples/net/sockets/socketpair does not run as expected
* :github:`52353` - bug: sysbuild lost board reversion here
* :github:`52352` - Questions about newlib library
* :github:`52351` - Bluetooth: Controller: ISOAL ASSERT failure
* :github:`52344` - Software-based Debounced GPIO
* :github:`52339` - usb: USB device re-enabling does not work on Nordic devices (regression)
* :github:`52327` - websocket: websocket_recv_msg breaks when there is no data after header
* :github:`52324` - Bluetooth Mesh example seems broken on v3.2.99 - worked ok with v3.1.99
* :github:`52317` - drivers: wifi: eswifi: Offload sockets accessing invalid net_context, resulting in errors in FLASH_SR and blocking flash use
* :github:`52309` - SAM0 flash driver with page emulated enabled will not write data at 0 block
* :github:`52308` - pip3 is unable to build wheel for cmsis-pack-manager as part of the requirements.txt on Fedora 37.
* :github:`52307` - Linker Error using CONSOLE_GETCHAR and CONSOLE_GETLINE
* :github:`52301` - Zephyr-Hello World is not working
* :github:`52298` - CI fail multiple times due to download package from http://azure.archive.ubuntu.com/ubuntu failed.
* :github:`52296` - Regulator shell does not build due to missing atoi define
* :github:`52291` - boards: thingy53: Enable USB-CDC by default
* :github:`52284` - unable to malloc during  tests/lib/cmsis_dsp/transform.cf64
* :github:`52280` - boards: thingy53 non-secure (thingy53_nrf5340_cpuapp_ns) does not build
* :github:`52276` - stm32: ST Kit B-L4S5I-IOT01A Octospi flash support
* :github:`52267` - http client includes chunking data in response body when making a non-chunked request and server responds with chunked data
* :github:`52262` - west flash --file
* :github:`52242` - Fatal exception LoadProhibited from mcuboot when enabling newlib in wifi sample (esp32)
* :github:`52235` - counter_basic_api test fails to build on STM32 platforms
* :github:`52228` - Bluetooth: L2CAP: receive a K-frame with payload longer than MPS if Enhanced ATT enabled
* :github:`52219` - jlink runner doesn't flash bin file if a hex file is present
* :github:`52218` - ADC read locked forever with CONFIG_DEBUG_OPTIMISATION=y on STM32U5
* :github:`52216` - spi_transceive with DMA on a STM32 slave returns value incorrect
* :github:`52211` - MCUX based QDEC driver
* :github:`52196` - Calling ``bt_le_ext_adv_stop()`` can causes failures in connections when support for multiple connections are enabled
* :github:`52195` - Bluetooth: GATT: add_subscriptions not respecting encryption
* :github:`52190` - [lpcxpresso55s28] flash range starts from Secure address which is not compatible with latest Jlink(v7.82a)
* :github:`52189` - Flash file system not working on stm32h735g_disco
* :github:`52181` - ADC Channel SCAN mode for STM32U5
* :github:`52171` - Bluetooth: BR/EDR: Inappropriate l2cap channel state set/get
* :github:`52169` - Mesh provisioning static OOB incorrect zero padding
* :github:`52167` - twister: Mechanism to pass CLI args through to ztest executable
* :github:`52154` - mgmt: mcumgr: Add Kconfig to automatically register handlers and mcumgr functionality
* :github:`52139` - ppp modem doesn't send NET_EVENT_L4_CONNECTED event
* :github:`52131` - tests-ci : kernel: timer: starve test No Console Output(Timeout)
* :github:`52125` - Timer accuracy issue with STM32U5 board
* :github:`52114` - Can't make JerryScript Work with Zephyr
* :github:`52113` - Binary blobs in ``hal_telink`` submodules
* :github:`52111` - Incompatible LTO version of liblt_9518_zephyr.a
* :github:`52103` - STM32u5 dual bank flash issue
* :github:`52101` - ``bt_gatt_notify`` function does not notify data larger than 20 bytes
* :github:`52099` - mgmt: mcumgr: Rename fs_mgmt hash/checksum functions
* :github:`52095` - dfu/mcuboot: dfu/mcuboot.h used BOOT_MAX_ALIGN and BOOT_MAGIC_SZ but does not include ``bootutil/bootutil_public.h``
* :github:`52094` - STM32MP157 Debugging method use wrong GDB port when execute command ``west debug``
* :github:`52085` - can: SAM M_CAN regression
* :github:`52079` - TLS handshake failure (after client-hello) with big_http_download sample
* :github:`52073` - ESP32-C3 UART1 not available after zephyr update to v3.2.99
* :github:`52065` - west: debugserver command does not work
* :github:`52059` - Bluetooth: conn:  in multi role configuration incorrect address is set after advertising resume
* :github:`52057` - tests: kernel: timer: starve: kernel.timer.starve hangs
* :github:`52056` - Bluetooth: Missing LL data length update callback on Central and Peripheral sides
* :github:`52055` - Bluetooth: Controller: Broadcast scheduling issues
* :github:`52049` - Update mps2_an521_remote for compatibility with mps2_an521_ns
* :github:`52022` - RFC: API Change: mgmt: mcumgr: transport: Add query valid check function
* :github:`52021` - RFC: API Change: mgmt: mcumgr: Replace mgmt_ctxt struct with smp_streamer
* :github:`52009` - tests: kernel: fifo: fifo_timeout: kernel.fifo.timeout fails on nrf5340dk_nrf5340_cpuapp
* :github:`51998` - Nominate Attie Grande as zephyr Collaborator
* :github:`51997` - microTVM or zephyr bugs, No SOURCES given to Zephyr library
* :github:`51989` - stm32f303v(b-c)tx-pinctrl.dtsi, No such file or directory
* :github:`51984` - Bluetooth: Central rejects connection parameters update request from a connected peripheral
* :github:`51973` - Coding style problem, clang-format formatted code cannot pass CI.
* :github:`51951` - Zephyr Getting Started steps fail with Python v3.11
* :github:`51944` - lsm6dso sensnsor driver: to enable drdy in pulsed mode call ``lsm6dso_data_ready_mode_set()``
* :github:`51939` - mgmt: mcumgr: SMP is broken
* :github:`51931` - Failing unit test re. missing PERIPHERAL_ISO_SUPPORT KConfig selection
* :github:`51893` - LSM303dlhc sensor example not compiling for nRF52840
* :github:`51874` - Zephyr 3.1 bosch,bme280 device is in the final DTS and accessible, but DT_HAS_BOSCH_BME280_ENABLED=n
* :github:`51873` - sensor: bmp388: missing if check around i2c device ready function
* :github:`51872` - Race condition in workqueue can lead to work items being lost
* :github:`51870` - Nucleo_h743zi fails to format storage flash partition
* :github:`51855` - openocd: targeting wrong serial port / device
* :github:`51829` - qemu_x86: upgrading to q35 breaks networking samples.
* :github:`51827` - picolibc heap lock recursion mismatch
* :github:`51821` - native_posix: Cmake must be run at least twice to find ${CMAKE_STRIP}
* :github:`51815` - Bluetooth: bt_disable in loop with babblesim gatt test causes Zephyr link layer assert
* :github:`51798` - mgmt: mcumgr: image upload, then image erase, then image upload does not restart upload from start
* :github:`51797` - West espressif install not working
* :github:`51796` - LE Audio: Improve stream coupling for CIS as the unicast client
* :github:`51788` - Questionable test code in ipv6_fragment test
* :github:`51785` - drivers/clock_control: stm32: Can support configure stm32_h7 PLL2 ?
* :github:`51780` - windows-curses Python package in requirements.txt can't install if using Python 3.11
* :github:`51778` - stm32l562e-dk: Broken TF-M psa-crypto sample
* :github:`51776` - POSIX API is not portable across arches
* :github:`51761` - Bluetooth : HardFault in hci_driver on sample/bluetooth/periodic_sync using nRF52833DK
* :github:`51752` - CAN documentation points to old sample locations
* :github:`51731` - Twister has a hard dependency on ``west.log``
* :github:`51728` - soc: xtensa: esp32_net: Remove binary blobs from source tree
* :github:`51720` - USB mass sample not working for FAT FS
* :github:`51714` - Bluetooth: Application with buffer that cannot unref it in disconnect handler leads to advertising issues
* :github:`51713` - drivers: flash: spi_nor: init fails when flash is busy
* :github:`51711` - Esp32-WROVER Unable to include the header file ``esp32-pinctrl.h``
* :github:`51693` - Bluetooth: Controller: Transmits packets longer than configured max len
* :github:`51687` - tests-ci : net: socket: tcp.preempt test Failed
* :github:`51688` - tests-ci : net: socket: tcp test Failed
* :github:`51689` - tests-ci : net: socket: poll test Failed
* :github:`51690` - tests-ci : net: socket: select test Failed
* :github:`51691` - tests-ci : net: socket: tls.preempt test Failed
* :github:`51692` - tests-ci : net: socket: tls test Failed
* :github:`51676` - stm32_hal -- undefined reference to "SystemCoreClock"
* :github:`51653` - mgmt: mcumgr: bt: issue with queued packets when device is busy
* :github:`51650` - Bluetooth: Extended adv reports with legacy data should also be discardable
* :github:`51631` - bluetooth: shell: linker error
* :github:`51629` - BLE stack execution fails with CONFIG_NO_OPTIMIZATIONS=y
* :github:`51622` - ESP32 mcuboot not support chip revision 1
* :github:`51621` - APPLICATION_CONFIG_DIR, CONF_FILE do not always pick up local ``boards/*.conf``
* :github:`51620` - Add Apache Thrift Module (from GSoC 2022 Project)
* :github:`51617` - RFC: Add Apache Thrift Upstream Module (from GSoC 2022 Project)
* :github:`51611` - check_compliance.py generates file checkpath.txt which isn't in .gitingore
* :github:`51607` - DT_NODE_HAS_COMPAT does not consider parents/path
* :github:`51604` - doc: is the documentation GDPR compliant since it uses Google Analytics without prompting the user about tracking?
* :github:`51602` - Stack overflow when using mcumgr fs_mgmt
* :github:`51600` - Bluetooth assert on flash erase using mcumgr
* :github:`51594` - mgmt: mcumgr: bt: thread freezes if device disconnects
* :github:`51588` - Doc:  Broken link in the "Electronut Labs Papyr" documentation page
* :github:`51566` - broken network once lwM2M is resumed after pause
* :github:`51559` - lwm2m tests are failing
* :github:`51549` - Memory report generation breaks if app and Zephyr is located on different Windows drives
* :github:`51546` - The blinky_pwm sample does not work on raspberry pi pico
* :github:`51544` - drivers/pwm/pwm_sam.c - update period and duty cycle issue (workaround + suggestions for fix)
* :github:`51529` - frdm_k64f: tests/net/socket/tls run failed on frdm_k64f
* :github:`51528` - spurious warnings when EXTRA_CFLAGS=-save-temps=obj is passed
* :github:`51521` - subsys: bluetooth: shell: gatt.c build fails with CONFIG_DEBUG_OPTIMIZATIONS and CONFIG_BT_SHELL=y
* :github:`51520` - samples: compression lz4 fails on small-ram stm32 platforms
* :github:`51508` - openocd: can't flash STM32H7 board using STLink V3
* :github:`51506` - it8xxx2_evb: Test suite after watchdog test will display fail in the daily test
* :github:`51505` - drivers: modem: gsm: gsm_ppp_stop() does not change gsm->state
* :github:`51488` - lis2dw12 function latch is misunderstood with drdy latch
* :github:`51480` - tests-ci : drivers: watchdog test Build failure of mimxrt1064_evk
* :github:`51476` - Please add documentation or sample how to use TLS_SESSION_CACHE socket option
* :github:`51475` - Twister: Mistake timeout as skipped
* :github:`51474` - driver: stm32: usb: add detach function support
* :github:`51471` - Network protocol MQTT: When qos=1, there is a bug in the subscription and publication
* :github:`51470` - tests-ci : drivers: mipi_dsi: api test Build failure
* :github:`51469` - Intel CAVS: Failed in tests/kernel/spinlock
* :github:`51468` - mps3_an547:tests/lib/cmsis_dsp/filtering bus fault
* :github:`51464` - samples: drivers: peci: Code doesn't build for npcx7m6fb_evb board
* :github:`51458` - Only one instance of mcp2515
* :github:`51454` - Cmake error for Zephyr Sample Code in Visual Studio
* :github:`51446` - The PR #50334 breaks twister execution on various HW boards
* :github:`51437` - LoRaWan problem with uplink messages sent as a response to class C downlink
* :github:`51438` - tests-ci : net: http:  test Build failure
* :github:`51436` - tests-ci : drivers: drivers.watchdog: nxp-imxrt11xx series : watchdog build failure
* :github:`51435` - tests-ci : drivers: hwinfo: api test Failed
* :github:`51432` - Bluetooth: ISO: Remove checks for and change seq_num to uint16_t
* :github:`51424` - tests: net: socket: tls: v4 dtls sendmsg test is testing v6
* :github:`51421` - tests: net: socket: tls: net.socket.tls region ``FLASH`` overflowed
* :github:`51418` - Intel CAVS: Assertion failed in tests/subsys/logging/log_links
* :github:`51406` - Blinky not executing on Windows
* :github:`51376` - Silabs WFX200 Binary Blob
* :github:`51375` - tests/lib/devicetree/devices/libraries.devicetree.devices: build failure (bl5340_dvk_cpuapp)
* :github:`51371` - hal: nxp: ARRAY_SIZE collision
* :github:`51370` - Driver error precision LPS22HH
* :github:`51368` - tests: tests/subsys/cpp/cxx/cpp.main.picolibc: build failure
* :github:`51364` - ESP32 WIFI: when allocating system_heap to PSRAM(extern ram), wifi station can't connet to ap(indicate that ap not found)
* :github:`51360` - I2C master read failure when 10-bit addressing is used with i2c_ll_stm32_v1
* :github:`51351` - I2C: ESP32 driver does not support longer clock stretching
* :github:`51349` - Turn power domains on/off directly
* :github:`51343` - qemu_x86_tiny doesn't place libc-hooks data in z_libc_partition
* :github:`51331` - lvgl: LV_FONT_CUSTOM_DECLARE does not work as string
* :github:`51323` - kernel: tests: Evaluate "platform_allow" usage in kernel tests.
* :github:`51322` - tests: kernel: timer: timer_behavior: kernel.timer.timer fails
* :github:`51318` - x86_64: Thread Local Storage pointer not setup before first thread started
* :github:`51301` - CI: mps3_an547: test failures
* :github:`51297` - Bluetooth: Implement H8 function from cryptographic toolbox
* :github:`51294` - ztest: Broken tests in main branch due to API-breaking change ``ZTEST_FAIL_ON_ASSUME``
* :github:`51290` - samples: application_development: external_lib: does not work on windows
* :github:`51276` - CAN driver for ESP32 (TWAI) does not enable the transceiver
* :github:`51265` - net: ip: cloning of net_pkt produces dangling ll address pointers and may flip overwrite flag
* :github:`51264` - drivers: ieee802154: nrf: wrapped pkt attribute access
* :github:`51263` - drivers: ieee802154: IEEE 802.15.4 L2 does not announce (but uses) promisc mode
* :github:`51261` - drivers: ieee802154: Drivers allocate RX packets from the TX pool
* :github:`51247` - Bluetooth: RPA expired callback inconsistently called
* :github:`51235` - nominate me as zephyr contributor
* :github:`51234` - it8xxx2_evb: The testcase tests/kernel/sleep/failed to run.
* :github:`51233` - up_squared: samples/boards/up_squared/gpio_counter run failed
* :github:`51228` - Bluetooth: Privacy in scan roles not updating RPA on timeout
* :github:`51223` - Problem when using fatfs example in a out_of_tree_driver with the file ff.h
* :github:`51214` - enc28j60 appears to be unable to correctly determine network state
* :github:`51208` - Bluetooth: Host: ``bt_le_oob_get_local`` gives incorrect address
* :github:`51202` - twister: Integration errors not reported nor counted in the console output but present in the reports.
* :github:`51194` - samples/subsys/lorawan building failed
* :github:`51185` - tests/drivers/counter/counter_basic_api fails to build on mimxrt685_evk_cm33
* :github:`51177` - Change SPI configuration (bitrate) with MCUXpresso SPI driver fails
* :github:`51174` - Bluetooth: l2cap needs check rx.mps when le_recv
* :github:`51168` - ITERABLE_SECTION_ROM stores data in RAM instead of ROM
* :github:`51165` - tools/fiptool/fiptool: Permission denied
* :github:`51156` - esp32 wifi: how to use ap mode and ap+station mode?
* :github:`51153` - modem: ppp: extract access technology when MODEM_CELL_INFO is enabled
* :github:`51149` - Esp32 wifi compilation error
* :github:`51146` - Running test: drivers:  disk: disk_access with  RAM disk  fails
* :github:`51144` - PR #51017 Broke GPIO builds for LPC11u6x platforms
* :github:`51142` - TestPlan generation not picking up tests missing ``test_`` prefix
* :github:`51138` - Testing tests/lib/cmsis_dsp/ fails on some stm32 boards
* :github:`51126` - Bluetooh: host: df: wrong size of a HCI command for connectionless CTE enable in AoD mode
* :github:`51117` - tests: kernel: workq: work: kernel.work.api fails test_1cpu_drain_wait
* :github:`51108` - Ethernet: Error frames are displayed when DHCP is suspended for a long time: <inf> net_dhcpv4: Received: 192.168.1.119
* :github:`51107` - Ethernet: Error frames are often displayed: <err> eth_mcux: ENET_GetRxFrameSize return: 4001
* :github:`51105` - esp32 wifi: http transmit rate is too slow
* :github:`51102` - issue installing zepyhr when i am using west cmd
* :github:`51076` - ADC channels 8-12 not working on LPC55s6X
* :github:`51074` - logging: syst: sample failure
* :github:`51070` - ModuleNotFoundError: No module named 'elftools'
* :github:`51068` - ModuleNotFoundError: No module named 'elftools'
* :github:`51065` - building tests/subsys/jwt failed on disco_l475_iot1  with twister
* :github:`51062` - lora_recv_async receives empty buffer after multiple receptions on sx12xx
* :github:`51060` - 10-bit addressing not supported by I2C slave driver for STM32 target
* :github:`51057` - Retrieve gpios used by a device (pinctrl)
* :github:`51048` - Firmware Upgrade Issue with Mcumgr On STM32H743 controller
* :github:`51025` - mbedtls: build warnings
* :github:`51021` - openthread: build warnings
* :github:`51019` - NVS should allow overwriting existing index even if there's no room to keep the old value
* :github:`51016` - mgmt: mcumgr: Add dummy shell buffer size Kconfig entry to shell mgmt
* :github:`51015` - Build Error for ST Nucleo F103RB
* :github:`51010` - Unable to communicate with LIS2DS12 on 52840DK or custom board
* :github:`51007` - Improve process around feature freeze exceptions
* :github:`51003` - Crash when using flexcomm5 as i2c on LPC5526
* :github:`50989` - Invalid ASE State Machine Transition
* :github:`50983` - RPI Pico usb hangs up in interrupt handler for composite devices
* :github:`50976` - JSON array encoding fails on array of objects
* :github:`50974` - DHCP (IPv4) NAK not respected when in renewing state
* :github:`50973` - DHCP (IPv4) seemingly dies by trying to assign an IP of 0.0.0.0
* :github:`50970` - SAME54_xpro network driver not attached
* :github:`50953` - LE Audio: Add support for setting ISO data path for broadcast sink
* :github:`50948` - SSD1306+lvgl sample fails to display
* :github:`50947` - stm32 static IPv4 networking in smp_svr sample application does not seem to work until a ping is received
* :github:`50940` - logging.log_output_ts64 fails on qemu_arc_hs5x
* :github:`50937` - Error when building for esp32c3_devkitm
* :github:`50923` - RFC: Stable API change: Rework and improve mcumgr callback system
* :github:`50895` - ADC Voltage Reference issue with STM32U5 MCU
* :github:`50874` - Cant disable bluetooth for BLE peripheral after connection with Central
* :github:`50872` - Error while installing python dependecies
* :github:`50868` - DHCP never binds if a NAK is received during the requesting state
* :github:`50853` - STM32F7 series can't run at frequencies higher than 180MHz
* :github:`50844` - zcbor module apis which are used for mcu boot functionality are not building in cpp file against v3.1.0
* :github:`50812` - MCUmgr udp sample fails with shell - BUS FAULT
* :github:`50801` - JSON parser fails on multidimensional arrays
* :github:`50789` - west: runners: blackmagicprobe: Doesn't work on windows due to wrong path separator
* :github:`50786` - Bluetooth: Host: Extended advertising reports may block the host
* :github:`50784` - LE Audio: Missing Media Proxy checks for callbacks
* :github:`50783` - LE Audio: Reject ISO data if the stream is not in the streaming state
* :github:`50782` - LE Audio: The MPL shell module should not use opcodes
* :github:`50781` - LE Audio: ``mpl init`` causes warnings when adding objects
* :github:`50780` - LE Audio: Bidirectional handling of 2 audio streams as the unicast server when streams are configured separately not working as intended
* :github:`50778` - LE Audio: Audio shell: Unicast server cannot execute commands for the default_stream
* :github:`50776` - CAN Drivers allow sending FD frames without device being set to FD mode
* :github:`50768` - storage: DT ``fixed-partition`` with ``status = "okay"`` requires flash driver
* :github:`50746` - Stale kernel memory pool API references
* :github:`50744` - net: ipv6: Allow on creating incomplete neighbor entries and routes in case of receiving Router Advertisement
* :github:`50735` - intel_adsp_cavs18: tests/boards/intel_adsp/hda_log/boards.intel_adsp.hda_log.printk failed
* :github:`50732` - net: tests/net/ieee802154/l2/net.ieee802154.l2 failed on reel_board due to build failure
* :github:`50709` - tests: arch: arm: arm_thread_swap fails on stm32g0 or stm32l0
* :github:`50684` - After enabling  CONFIG_SPI_STM32_DMA in project config file for STM32MP157-dk2 Zephyr throwing error
* :github:`50665` - MEC15xx/MEC1501: UART and special purpose pins missing pinctrl configuration
* :github:`50658` - Bluetooth: BLE stack notifications blocks host side for too long (``drivers/bluetooth/hci/spi.c`` and ``hci_spi``)
* :github:`50656` - Wrong definition of bank size for intel memory management driver.
* :github:`50655` - STM32WB55 Bus Fault when connecting then disconnecting then connecting then disconnecting then connecting
* :github:`50620` - fifo test fails with CONFIG_CMAKE_LINKER_GENERATOR enabled on qemu_cortex_a9
* :github:`50614` - Zephyr if got the ip is "10.xxx.xxx.xxx" when join in the switchboard, then the device may can not visit the outer net, also unable to Ping.
* :github:`50603` - Upgrade to loramac-node 4.7.0 when it is released to fix async LoRa reception on SX1276
* :github:`50596` - Documentation: Broken links in the previous release documentation
* :github:`50592` - mgmt: mcumgr: Remove code/functions deprecated in zephyr 3.1 release
* :github:`50590` - openocd: Can't flash on various STM32 boards
* :github:`50587` - Regression in Link Layer Control Procedure (LLCP)
* :github:`50570` - samples/drivers/can/counter fails in twister for native_posix
* :github:`50567` - Passed test cases are reported as "Skipped" because of incomplete test log
* :github:`50565` - Fatal error after ``west flash`` for nucleo_l053r8
* :github:`50554` - Test uart async failed on Nucleo F429ZI
* :github:`50525` - Passed test cases reported as "Skipped" because test log lost
* :github:`50515` - Non-existing test cases reported as "Skipped" with reason  “No results captured, testsuite misconfiguration?” in test report
* :github:`50461` - Bluetooth: controller: LLCP: use of legacy ctrl Tx buffers
* :github:`50452` - mec172xevb_assy6906: The testcase tests/lib/cmsis_dsp/matrix failed to run.
* :github:`50446` - MCUX CAAM is disabled temporarily
* :github:`50438` - Bluetooth: Conn: Bluetooth stack becomes unusable when communicating with both centrals and peripherals
* :github:`50427` - Bluetooth: host: central connection context leak
* :github:`50426` - STM32: using SPI after STOP2 sleep causes application to hang
* :github:`50404` - Intel CAVS: tests/subsys/logging/log_immediate failed.
* :github:`50389` - Allow twister to be called directly from west
* :github:`50381` - BLE: Connection slows down massively when connecting to a second device
* :github:`50354` - ztest_new: _zassert_base : return without post processing
* :github:`50345` - Network traffic occurs before Bluetooth NET L2 (IPSP) link setup complete
* :github:`50284` - Generated linker scripts break when ZEPHYR_BASE and ZEPHYR_MODULES share structure that contains symlinks
* :github:`50256` - I2C on SAMC21 sends out stop condition incorrectly
* :github:`50193` - Impossible to connect with a peripheral with BLE and zephyr 2.7.99, BT_HCI_ERR_UNKNOWN_CONN_ID error
* :github:`50192` - nrf_qspi_nor driver might crash if power management is enabled
* :github:`50188` - Avoid using extra net buffer for L2 header
* :github:`50149` - tests: drivers: flash fails on nucleo_l152re because of wrong erase flash size
* :github:`50139` - net: ipv4: Add DSCP/ToS based QoS support
* :github:`50070` - LoRa: Support on RFM95 LoRa module combined with a nRF52 board
* :github:`50040` - shields: Settle on nodelabels naming scheme
* :github:`50028` - flash_stm32_ospi Write enable failed when building with TF-M
* :github:`49996` - tests: drivers: clock_control: nrf_lf_clock_start and nrf_onoff_and_bt fails
* :github:`49963` - Random crash on the L475 due to work->handler set to NULL
* :github:`49962` - RFC: Stable API Change:  SMP (Simple Management Protocol) transport API within MCUMgr drops ``zephyr_`` prefix in functions and type definitions and drop zst parameter from zephyr_smp_transport_out_fn
* :github:`49917` - http_client_req() sometimes hangs when peer disconnects
* :github:`49871` - zperf: Add support to stop/start download
* :github:`49870` - stm32 enables HSI48 clock with device tree
* :github:`49844` - shell: Add abort support
* :github:`49843` - net: shell: Extend ping command
* :github:`49821` - USB DFU implementation does not work with WinUSB because of missing device reset API
* :github:`49811` - DHCP cannot obtain IP, when CONFIG_NET_VLAN is enabled
* :github:`49783` - net: ipv4: packet fragmentation support
* :github:`49746` - twister: extra test results
* :github:`49740` - LE Audio: Support for application-controlled advertisement for BAP broadcast source
* :github:`49711` - tests/arch/common/timing/arch.common.timing.smp fails for CAVS15, 18
* :github:`49648` - tests/subsys/logging/log_switch_format, log_syst build failures on CAVS
* :github:`49624` - Bluetooth: Controller: Recent RAM usage increase for hci_rpmsg build
* :github:`49621` - STM32WB55 BLE Extended Advertising support
* :github:`49620` - Add picolibc documentation
* :github:`49614` - acrn_ehl_crb: The testcase tests/kernel/sched/schedule_api failed to run.
* :github:`49611` - ehl_crb: Failed to run timer testcases
* :github:`49588` - Json parser is incorrect with undefined parameter
* :github:`49584` - STM32WB55 Failed read remote feature, remote version and LE set PHY
* :github:`49530` - Bluetooth: Audio: Invalid behavior testing
* :github:`49451` - Treat carrier UP/DOWN independently to interface UP/DOWN
* :github:`49413` - TI-AM62x: Add Zephyr Support for M4 and R5 cores
* :github:`49373` - BLE scanning - BT RX thread hangs on.
* :github:`49338` - Antenna switching for Bluetooth direction finding with the nRF5340
* :github:`49313` - nRF51822 sometimes hard fault on connect
* :github:`49298` - cc3220sf: add a launchpad_connector.dtsi
* :github:`49266` - Bluetooth: Host doesn't seem to handle INCOMPLETE per adv reports
* :github:`49234` - option to configure coverage data heap size
* :github:`49228` - ti: cc13xx_cc26xx: ADC support
* :github:`49210` - BL5340 board cannot build bluetooth applications
* :github:`49208` - drivers: modem: bg9x: not supporting UDP
* :github:`49148` - Asynchronous UART API triggers Zephyr assertion on STM32WB55
* :github:`49112` - lack of support for lpsram cache
* :github:`49069` - log: cdc_acm: hard fault message does not output
* :github:`49066` - Mcumgr img_mgmt_impl_upload_inspect() can cause unaligned memory access hard fault.
* :github:`49054` - STM32H7 apps are broken in C++ mode due to HAL include craziness
* :github:`49032` - espi saf testing disabled
* :github:`49026` - Add a CI check on image file sizes (specifically around boards)
* :github:`49021` - uart async api does not provide all received data
* :github:`48954` - several NXP devicetree bindings are missing
* :github:`48953` - 'intel,sha' is missing binding and usage
* :github:`48886` - Documenting the process for treewide changes
* :github:`48857` - samples: Bluetooth: Buffer size mismatch in samples/bluetooth/hci_usb for nRF5340
* :github:`48850` - Bluetooth: LLCP: possible access to released control procedure context
* :github:`48726` - net: tests/net/ieee802154/l2/net.ieee802154.l2 failed on reel board
* :github:`48625` - GSM_PPP api keeps sending commands to muxed AT channel
* :github:`48616` - RFC: Change to clang-format coding style rules re binary operators
* :github:`48609` - drivers: gpio: expose gpio_utils.h to external GPIO drivers
* :github:`48603` - LoRa driver asynchronous receive callback clears data before the callback.
* :github:`48520` - clang-format: #include reorder due to default: SortIncludesOptions != SI_Never
* :github:`48505` - BLE stack can get stuck in connected state despite connection failure
* :github:`48473` - Setting CONFIG_GSM_MUX_INITIATOR=n results in a compile error
* :github:`48468` - GSM Mux does not transmit all queued data when uart_fifo_fill is called
* :github:`48394` - vsnprintfcb writes to ``*str`` if it is NULL
* :github:`48390` - [Intel Cavs] Boot failures on low optimization levels
* :github:`48317` - drivers: fpga: include driver for Lattice iCE40 parts
* :github:`48304` - bt_disable() does not work properly on nRF52
* :github:`48299` - SHT3XD_CMD_WRITE_TH_LOW_SET should be SHT3XD_CMD_WRITE_TH_LOW_CLEAR
* :github:`48150` - Sensor Subsystem: data types
* :github:`48148` - Sensor Subsystem: Base sensor DTS bindings
* :github:`48147` - ztest: before/after functions may run on different threads, which may cause potential issues.
* :github:`48037` - Grove LCD Sample Not Working
* :github:`48018` - ztest: static threads are not re-launched for repeated test suite execution.
* :github:`47988` - JSON parser not consistent on extra data
* :github:`47877` - ECSPI support for NXP i.MX devices
* :github:`47872` - Differentiating Samples, Tests & Demos
* :github:`47833` - Intel CAVS: cavstool.py fails to extract complete log from winstream buffer when logging is frequent
* :github:`47830` - Intel CAVS: Build failure due to #47713 PR
* :github:`47817` - samples/modules/nanopb/sample.modules.nanopb fails with protobuf > 3.19.0
* :github:`47611` - ci: workflows: compliance: Add commit title to an error msg
* :github:`47607` - Settings with FCB backend does not pass test on stm32h743
* :github:`47576` - undefined reference to ``__device_dts_ord_20`` When building with board hifive_unmatched on flash_shell samples
* :github:`47500` - twister: cmake: Failure of "--build-only -M" combined with "--test-only" for --device-testing
* :github:`47477` - qemu_leon3: tests/kernel/fpu_sharing/generic/ failed when migrating to new ztest API
* :github:`47329` - Newlib nano variant footprint reduction
* :github:`47326` - drivers: WINC1500: issues with buffer allocation when using sockets
* :github:`47324` - drivers: modem: gsm_ppp: support common gpios
* :github:`47315` - LE Audio: CAP Initiator skeleton Implementation
* :github:`47299` - LE Audio: Advertising (service) data for one or more services/roles
* :github:`47296` - LE Audio: Move board files for nRF5340 Audio development kit upstream
* :github:`47274` - mgmt/mcumgr/lib: Rework of event callback framework
* :github:`47243` - LE Audio: Add support for stream specific codec configurations for broadcast source
* :github:`47242` - LE Audio: Add subgroup support for broadcast source
* :github:`47092` - driver: nrf: uarte: new dirver breaks our implementation for uart.
* :github:`47040` - tests: drivers: gpio_basic_api and gpio_api_1pin: convert to new ztest API
* :github:`47014` - can: iso-tp: implementation test failed with twister on nucleo_g474re
* :github:`46988` - samples: net: openthread: coprocessor: RCP is missing required capabilities: tx-security tx-timing
* :github:`46986` - Logging (deferred v2) with a lot of output causes MPU fault
* :github:`46897` - tests: posix: fs: improve tests to take better advantage of new ztest features
* :github:`46844` - Timer drivers likely have off-by-one in rapidly-presented timeouts
* :github:`46824` - Prevent new uses of old ztest API
* :github:`46598` - Logging with RTT backend on STM32WB strange behavier
* :github:`46596` - STM32F74X RMII interface does not work
* :github:`46491` - Zephyr SDK 0.15.0 Checklist
* :github:`46446` - lvgl: Using sw_rotate with SSD1306 shield causes memory fault
* :github:`46351` - net: tcp: Implement fast-retransmit
* :github:`46326` - Async UART for STM32 U5 support
* :github:`46287` - Zephyr 3.2 release checklist
* :github:`46268` - Update RNDIS USB class codes for automatic driver loading by Windows
* :github:`46126` - pm_device causes assertion error in sched.c with lis2dh
* :github:`46105` - RFC: Proposal of Integrating Trusted Firmware-A
* :github:`46073` - IPSP (IPv6 over BLE) example stop working after a short time
* :github:`45921` - Runtime memory usage
* :github:`45910` - [RFC] Zbus: a message bus system
* :github:`45891` - mgmt/mcumgr/lib: Refactoring of callback subsystem in image management (DFU)
* :github:`45814` - Armclang build fails due to missing source file
* :github:`45756` - Add overlay-bt-minimal.conf for smp_svr sample application
* :github:`45697` - RING_BUF_DECLARE broken for C++
* :github:`45625` - LE Audio: Update CSIP API with new naming scheme
* :github:`45621` - LE Audio: Update VCP API with new naming scheme
* :github:`45427` - Bluetooth: Controller: LLCP: Data structure for communication between the ISR and the thread
* :github:`45222` - drivers: peci: user space handlers not building correctly
* :github:`45218` - rddrone_fmuk66: I2C configuration incorrect
* :github:`45094` - stm32: Add USB HS device support to STM32H747
* :github:`44908` - Support ESP32 ADC
* :github:`44861` - WiFi support for STM32 boards
* :github:`44410` - drivers: modem: shell: ``modem send`` doesn't honor line ending in modem cmd handler
* :github:`44399` - Zephyr RTOS support for Litex SoC with 64 bit rocket cpu.
* :github:`44377` - ISO Broadcast/Receive sample not working with coded PHY
* :github:`44324` - Compile error in byteorder.h
* :github:`44318` - boards: arm: rpi_pico: Enable CONFIG_ARM_MPU=y for raspberry pi pico board
* :github:`44281` - Bluetooth: Use hardware encryption for encryption
* :github:`44164` - Implement the equivalent of PR #44102 in LLCP
* :github:`44055` - Immediate alert client
* :github:`43998` - posix: add include/posix to search path based on Kconfig
* :github:`43986` - interrupt feature for gpio_mcp23xxx
* :github:`43836` - stm32: g0b1: RTT doesn't work properly after stop mode
* :github:`43737` - Support compiling ```native_posix`` targets on Windows using the MinGW
* :github:`43696` - mgmt/mcumgr: RFC: Standardize Kconfig option names for MCUMGR
* :github:`43655` - esp32c3: Connection fail loop
* :github:`43647` - Bluetooth: LE multirole: connection as central is not totally unreferenced on disconnection
* :github:`43604` - Checkpatch: Support in-code ignore tags
* :github:`43411` - STM32 SPI DMA issue
* :github:`43330` - usb_dc_nrfx.c starts usbd_work_queue with no name
* :github:`43308` - driver: serial: stm32: uart will lost data when use dma mode[async mode]
* :github:`43294` - LoRaWAN stack & user ChannelsMask
* :github:`43286` - Zephyr 3.1 Release Checklist
* :github:`42998` - Should board.dts enable peripherals by default?
* :github:`42910` - Bluetooth: Controller: CIS Data path setup: HCI ISO Data
* :github:`42908` - Bluetooth: Controller: CIG: LE Remove CIG
* :github:`42907` - Bluetooth: Controller: CIG: Disconnect: ACL disconnection leading to CIS Disconnection complete
* :github:`42906` - Bluetooth: Controller: CIG:  Disconnect:  Using HCI Disconnect:  Generate LL_CIS_TERMINATE_IND
* :github:`42905` - Bluetooth: Controller: CIG: LL Rejects: Remote request being rejected
* :github:`42902` - Bluetooth: Controller: CIG: Host reject: LE Reject CIS Request
* :github:`42900` - Bluetooth: Controller: CIG: LE Setup ISO Data Path
* :github:`42899` - Bluetooth: Controller: CIG: LE CIS Established Event
* :github:`42898` - Bluetooth: Controller: CIG: LE Accept CIS
* :github:`42897` - Bluetooth: Controller: CIG: LE CIS Request Event
* :github:`42896` - Bluetooth: Controller: CIG: LE Create CIS: NULL PDU scheduling
* :github:`42895` - Bluetooth: Controller: CIG: LE Create CIS: Control procedure with LL_CIS_REQ/RSP/IND PDU
* :github:`42894` - Bluetooth: Controller: CIG: LE Set CIG Parameters
* :github:`42700` - Support module.yml in zephyr repo
* :github:`42590` - mgmt/mcumgr/lib: RFC: Allow leaving out "rc" in successful respones and use "rc" only for SMP processing errors.
* :github:`42432` - i2c: unable to configure SAMD51 i2c clock frequency for standard (100 KHz) speeds
* :github:`42420` - mgmt/mcumgr/lib: Async image erase command with status check
* :github:`42374` - STM32L5: Entropy : Power Management not working due to entropy driver & stop mode
* :github:`42361` - OpenOCD flashing not working on cc1352r1_launchxl/cc26x2r1_launchxl
* :github:`41956` - Bluetooth: Controller: BIG: Synchronized receiver encryption support
* :github:`41955` - Bluetooth: Controller: BIG: Broadcaster encryption support
* :github:`41830` - CONF_FILE, OVERLAY_CONFIG parsing expands ``${ZEPHYR_<whatever>_MODULE_DIR}``
* :github:`41823` - Bluetooth: Controller: llcp: Remote request are dropped due to lack of free proc_ctx
* :github:`41822` - BLE IPSP sample cannot handle large ICMPv6 Echo Request
* :github:`41784` - virtio device driver
* :github:`41771` - tests: drivers: adc: Test doesn't build for mec172xevb_assy6906
* :github:`41765` - assert.h should not include non libc headers
* :github:`41694` - undefined reference to ``_open``
* :github:`41622` - Infinite mutual recursion when SMP and ATOMIC_OPERATIONS_C are set
* :github:`41606` - stm32u5: Re-implement VCO input and EPOD configuration
* :github:`41581` - STM32 subghzspi fails pinctrl setup
* :github:`41380` - stm32h7: Ethernet: Migrate driver to the new eth HAL api
* :github:`41213` - LE Audio: Update GA services to use the multi-instance macro
* :github:`41212` - LE Audio: Store of bonded data
* :github:`41209` - LE Audio: MCS support for multiple instances
* :github:`41073` - twister: no way to specify arguments for the binary zephyr.exe
* :github:`40982` - Build system: West: Add a warning when used repository does not match manifest
* :github:`40972` - Power management support for MEC172x
* :github:`40944` - BUILTIN_STACK_CHECKER and MPU_STACK_GUARD with a thread using the FPU will fault the bulltin stack checker
* :github:`40928` - mgmt/mcumgr/lib: Check image consistency after writing last chunk
* :github:`40924` - mgmt/mcumgr/lib: Do not re-upload image, by default, to the secondary slot
* :github:`40868` - Add a pre and post initialization among CONFIG_APPLICATION_INIT_PRIORITY
* :github:`40850` - Add Zephyr logging support to mgmt/mcumgr/lib
* :github:`40833` - driver: i2c: TCA9546a: Have compilation fails when driver init priority missmatch
* :github:`40642` - Why does CMake wrongly believe the rimage target is changing?
* :github:`40582` - how the zephyr supportting with running cadence hifi4 lx7,reset_vectorXEA2.s ?
* :github:`40561` - BLE notification and indication callback data are difficult to pass to other threads...
* :github:`40560` - Callbacks lack context information...
* :github:`39740` - Road from pinmux to pinctrl
* :github:`39712` - bq274xx sensor - Fails to compile when CONFIG_PM_DEVICE enabled
* :github:`39598` - use of __noinit with ecc memory hangs system
* :github:`39520` - Add support for the BlueNRG-LP SoC
* :github:`39431` - arduino_nano_33_ble_sense: Add More Devices to the Device Tree
* :github:`39331` - ti: cc13xx_cc26xx: watchdog timer driver
* :github:`39234` - Add support for the Sensririon SCD30 CO2 sensor
* :github:`39194` - Process: investigate GitHub code review replacements
* :github:`39037` - CivetWeb samples fail to build with CONFIG_NEWLIB_LIBC
* :github:`39025` - Bluetooth: Periodic Advertising, Filter Accept List, Resolving list related variable name abbreviations
* :github:`38947` - Issue with SMP commands sent over the UART
* :github:`38880` - ARC: ARCv2: qemu_arc_em / qemu_arc_hs don't work with XIP disabled
* :github:`38668` - ESP32S I2S
* :github:`38570` - Process: binary blobs in Zephyr
* :github:`38450` - Python script for checking PR errors
* :github:`38346` - twister command line parameter clean up and optimizate twister documents
* :github:`38291` - Make Zephyr modules compatible with PlatformIO libdeps
* :github:`38251` - cmake: DTC_OVERLAY_FILE flags cancel board <board>.overlay files
* :github:`38041` - Logging-related tests fails on qemu_arc_hs6x
* :github:`37855` - STM32 - kconfigs to determine if peripheral is available
* :github:`37346` - STM32WL LoRa increased the current in "suspend_to_idle" state
* :github:`37056` - Clarify device power states
* :github:`36953` - <err> lorawan: MlmeConfirm failed : Tx timeout
* :github:`36951` - twister: report information about tests instability
* :github:`36882` - MCUMGR: fs upload fail for first time file upload
* :github:`36724` - The road to a stable Controller Area Network driver API
* :github:`36601` - Add input support to audio_codec
* :github:`36553` - LoRaWAN Sample: ``join accept`` but "Join failed"
* :github:`36544` - RFC: API Change: Bluetooth: Read Multiple
* :github:`36343` - Bluetooth: Mesh: Modularizing the proxy feature
* :github:`36301` - soc: cypress: Port Zephyr to Cypress CYW43907
* :github:`36297` - Move BSS section to the end of image
* :github:`35986` - POSIX: multiple definition of posix_types
* :github:`35812` - ESP32 Factory app partition is not bootable
* :github:`35316` - log_panic() hangs kernel
* :github:`35238` - ieee802.15.4 support for stm32wb55
* :github:`35237` - build: Enhance twister to follow all module.yml in module list
* :github:`35177` - example-application: Add example library & tests
* :github:`34949` - console Bluetooth LE backend
* :github:`34597` - Mismatch between ``ot ping`` and ``net ping``
* :github:`34536` - Simple event-driven framework
* :github:`34324` - RTT is not working on STM32
* :github:`34269` - LOG_MODE_MINIMAL BUILD error
* :github:`34049` - Nordic nrf9160 switching between drivers and peripherals
* :github:`33876` - Lora sender sample build error for esp32
* :github:`33704` - BLE Shell Scan application filters
* :github:`32875` - Benchmarking Zephyr vs. RIOT-OS
* :github:`32756` - Enable mcumgr shell management to send responses to UART other than assigned to shell
* :github:`32733` - RS-485 support
* :github:`32339` - reimplement tests/kernel/timer/timer_api
* :github:`32288` - Enhance the ADC functionality on the STM32 devices to all available ADC channels
* :github:`32213` - Universal error code type
* :github:`31959` - BLE firmware update STM32WB stuck in loop waiting for CPU2
* :github:`31298` - tests/kernel/gen_isr_table failed on hsdk and nsim_hs_smp sometimes
* :github:`30391` - Unit Testing in Zephyr
* :github:`30348` - XIP can't be enabled with ARC MWDT toolchain
* :github:`30212` - Disk rewrites same flash page multiple times.
* :github:`30159` - Clean code related to dts fixup files
* :github:`30042` - usbd: support more than one configuration descriptors
* :github:`30023` - Device model: add debug helpers for when device_get_binding() fails
* :github:`29986` - Add support for a single node having multiple bus types
* :github:`29832` - Redundant error check of function  uart_irq_update() in ``tests/drivers/uart/uart_basic_api/src/test_uart_fifo.c``
* :github:`29495` - SD card slow write SPI/Fatfs/stm32
* :github:`29160` - arm: Always include arch/arm/include
* :github:`29136` - usb: add USB device stack shell support
* :github:`29135` - usb: allow the instances of a USB class to be enabled and disabled at runtime
* :github:`29134` - usb: allow more extensive settings of the device descriptor
* :github:`29133` - usb: USB device stack should store and validate the device state
* :github:`29132` - usb: USB device stack should track and check the state of control transfers
* :github:`29087` - Moving (some) boards to their own repo/module
* :github:`28998` - net: if: extend list of admin/operational states of network interface
* :github:`28872` - Support ESP32 as Bluetooth controller
* :github:`28864` - sanitycheck: Make sanitycheck test specifiacation compatible
* :github:`28617` - enable CONFIG_TEST for all samples
* :github:`27819` - Memory Management for MMU-based devices for LTS2
* :github:`27258` - Ring buffer does not allow to partially put/get data
* :github:`26796` - Interrupts on Cortex-M do not work with CONFIG_MULTITHREADING=n
* :github:`26392` - e1000 ethernet driver needs to be converted to DTS
* :github:`26109` - devicetree: overloaded DT_REG_ADDR() and DT_REG_SIZE() for PCI devices
* :github:`25917` - Bluetooth: Deadlock with TX of ACL data and HCI commands (command blocked by data)
* :github:`25417` - net: socket: socketpair: check for ISR context
* :github:`25407` - No tests/samples covering socket read()/write() calls
* :github:`25055` - Redundant flash shell commands
* :github:`24653` - device_pm: clarify and document usage
* :github:`23887` - drivers: modem: question: Should modem stack include headers to put into zephyr/include?
* :github:`23165` - macOS setup fails to build for lack of "elftools" Python package
* :github:`23161` - I2C and sensor deinitialization
* :github:`23072` - #ifdef __cplusplus missing in tracking_cpu_stats.h
* :github:`22049` - Bluetooth: IRK handling issue when using multiple local identities
* :github:`21995` - Bluetooth: controller: split: Porting of connection event length
* :github:`21724` - dts: edtlib: handle child-binding or child-child-binding as 'normal' binding with compatible
* :github:`21446` - samples: add SPI slave
* :github:`21239` - devicetree: Generation of the child-bindigs items as a common static initializer
* :github:`20707` - Define GATT service at run-time
* :github:`20262` - dt-binding for timers
* :github:`19713` - usb: investigate if Network Buffer can be used in USB device stack and USB drivers
* :github:`19496` - insufficient test case coverage for log subsystem
* :github:`19356` - LwM2M sample reorganization: split out LwM2M source into object-based .c files
* :github:`19259` - doc: two-column tricks for HTML breaks PDF
* :github:`19243` - Support SDHC & samples/subsys/fs on FRDM-K64F
* :github:`19152` - MK22F51212 MPU defines missing
* :github:`18892` - POSIX subsys: transition to #include_next for header consistency
* :github:`17171` - Insufficient code coverage for lib/os/fdtable.c
* :github:`16961` - Modules: add a SHA1 check to avoid updating module in the past
* :github:`16942` - Missing test case coverage for include/misc/byteorder.h functions
* :github:`16851` - west flash error on zephyr v1.14.99
* :github:`16444` - drivers/flash/flash_simulator: Support for required read alignment
* :github:`16088` - Verify POSIX PSE51 API requirements
* :github:`15453` - Kconfig should enforce that at most one Console driver is enabled at a time
* :github:`15181` - ztest issues
* :github:`14753` - nrf52840_pca10056: Leading spurious 0x00 byte in UART output
* :github:`14577` - Address latency/performance in nRF51 timer ISR
* :github:`13170` - Porting guide for advanced board (multi CPU SoC)
* :github:`12504` - STM32: add USB_OTG_HS example
* :github:`12401` - Target Capabilities / Board Directory Layout Capabilities
* :github:`12367` - Power management strategy of Zephyr can't work well on nRF52 boards.
* :github:`11594` - Cleanup GNUisms to make the code standards compliant
* :github:`9045` - A resource-saving programming model
* :github:`6198` - unit test: Add unit test example which appends source files to SOURCES list
* :github:`5697` - Driver API review/cleanup/rework
* :github:`3849` - Reduce the overall memory usage of the LwM2M library
* :github:`2837` - Ability to use hardware-based block ciphers
* :github:`2647` - Better cache APIs needed.
