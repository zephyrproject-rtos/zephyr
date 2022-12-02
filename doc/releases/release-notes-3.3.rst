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
  feature can be disabled or tweaked by adjusting:
  :kconfig:option:`CONFIG_MCUMGR_TRANSPORT_BT_AUTOMATIC_INIT`,
  :kconfig:option:`CONFIG_MCUMGR_TRANSPORT_BT_AUTOMATIC_INIT_WAIT`, and
  :kconfig:option:`CONFIG_MCUMGR_TRANSPORT_UDP_AUTOMATIC_INIT`. If applications
  register transports then those registrations should be removed to prevent
  registering the same transport multiple times. If the Bluetooth stack needs
  to be setup/initialised by another module or the application itself, then
  :kconfig:option:`CONFIG_MCUMGR_TRANSPORT_BT_AUTOMATIC_INIT` should be
  disabled and the application should call :c:func:`smp_bt_start` at startup.

Removed APIs in this release
============================

* Removed :kconfig:option:`CONFIG_COUNTER_RTC_STM32_LSE_DRIVE*`
  This should now be configured using the ``driving_capability`` property of
  LSE clock

* Removed :kconfig:option:`CONFIG_COUNTER_RTC_STM32_LSE_BYPASS`
  This should now be configured using the new ``lse_bypass`` property of
  LSE clock

* Removed :kconfig:option:`CONFIG_COUNTER_RTC_STM32_BACKUP_DOMAIN_RESET`

Deprecated in this release
==========================

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

* STM32 RTC source clock should now be configured using devicetree.
  Related Kconfig :kconfig:option:`CONFIG_COUNTER_RTC_STM32_CLOCK_LSI` and
  :kconfig:option:`CONFIG_COUNTER_RTC_STM32_CLOCK_LSE` options are now
  deprecated.

* File backend for settings APIs and Kconfig options were deprecated:

  :c:func:`settings_mount_fs_backend` in favor of :c:func:`settings_mount_file_backend`

  :kconfig:option:`CONFIG_SETTINGS_FS` in favor of :kconfig:option:`CONFIG_SETTINGS_FILE`

  :kconfig:option:`CONFIG_SETTINGS_FS_DIR` in favor of creating all parent
  directories from :kconfig:option:`CONFIG_SETTINGS_FILE_PATH`

  :kconfig:option:`CONFIG_SETTINGS_FS_FILE` in favor of :kconfig:option:`CONFIG_SETTINGS_FILE_PATH`

  :kconfig:option:`CONFIG_SETTINGS_FS_MAX_LINES` in favor of :kconfig:option:`CONFIG_SETTINGS_FILE_MAX_LINES`

* PCIe APIs :c:func:`pcie_probe` and :c:func:`pcie_bdf_lookup` have been
  deprecated in favor of a centralized scan of available PCIe devices.

* SPI DT :c:func:`spi_is_ready` function has been deprecated in favor of :c:func:`spi_is_ready_dt`.

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

New APIs in this release
========================

Kernel
******

Architectures
*************

* ARM

* ARM

* ARM64

* RISC-V

* Xtensa

Bluetooth
*********

* Audio

* Direction Finding

* Host

  * Fixed missing calls to bt_le_per_adv_sync_cb.term when deleting a periodic
    advertising sync object.

  * Added local advertising address to bt_le_ext_adv_info.

* Mesh

  * Change default advertiser to be extended advertiser.

* Controller

* HCI Driver

Boards & SoC Support
********************

* Added support for these SoC series:

* Removed support for these SoC series:

* Made these changes in other SoC series:

* Changes for ARC boards:

* Added support for these ARM boards:

* Added support for these ARM64 boards:

* Removed support for these ARM boards:

* Removed support for these X86 boards:

* Added support for these RISC-V boards:

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

* Made these changes in other boards:

* Added support for these following shields:

Build system and infrastructure
*******************************

Drivers and Sensors
*******************

* ADC

* CAN

* Clock control

* Counter

  * STM32 RTC based counter should now be configured using device tree.

* Crypto

* DAC

* DFU

* Disk

* Display

* DMA

* EEPROM

* Entropy

* ESPI

* Ethernet

  * Flash: Moved CONFIG_FLASH_FLEXSPI_XIP into the SOC level due to the flexspi clock initialization occurring in the SOC level.

  * NRF: Added CONFIG_SOC_FLASH_NRF_TIMEOUT_MULTIPLIER to allow tweaking the timeout of flash operations.

  * spi_nor: Added property mxicy,mx25r-power-mode to jedec,spi-nor binding for controlling low power/high performance mode on Macronix MX25R* Ultra Low Power flash devices.

  * spi_nor: Added check if the flash is busy during init. This used to cause
    the flash device to be unavailable until the system was restarted. The fix
    waits for the flash to become ready before continuing. In cases where a
    full flash erase was started before a restart, this might result in several
    minutes of waiting time (depending on flash size and erase speed).

* GPIO

* I2C

* I2S

* I3C

* IEEE 802.15.4

* Interrupt Controller

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

* PWM

* Power domain

* Reset

* SDHC

* Sensor

* Serial

* SPI

* Timer

* USB

* W1

* Watchdog

* WiFi

Networking
**********

IPv4 packet fragmentation support has been added, this allows large packets to
be split up before sending or reassembled during receive for packets that are
larger than the network device MTU. This is disabled by default but can be
enabled with :kconfig:option:`CONFIG_NET_IPV4_FRAGMENT`.

USB
***

Devicetree
**********

* Bindings

  * New:

    * :dtcompatible:`zephyr,flash-disk`

    * STM32 SoCs:

      * :dtcompatible: `st,stm32-lse-clock`: new ``lse-bypass`` property

Libraries / Subsystems
**********************

* File systems

  * Added new API call `fs_mkfs`.
  * Added new sample `samples/subsys/fs/format`.
  * FAT FS driver has been updated to version 0.15 w/patch1.

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

* LwM2M

  * The ``lwm2m_senml_cbor_*`` files have been regenerated using zcbor 0.6.0.

* Settings

  * Replaced all :c:func:`k_panic` invocations within settings backend
    initialization with returning / propagating error codes.

HALs
****

MCUboot
*******

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

Tests and Samples
*****************

Issue Related Items
*******************

Known Issues
============

Addressed issues
================
