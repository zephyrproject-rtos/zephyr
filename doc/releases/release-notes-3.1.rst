:orphan:

.. _zephyr_3.1:

Zephyr 3.1.0 (Working Draft)
############################

The following sections provide detailed lists of changes by component.

Security Vulnerability Related
******************************

The following CVEs are addressed by this release:

More detailed information can be found in:
https://docs.zephyrproject.org/latest/security/vulnerabilities.html

* CVE-2022-1841: Under embargo until 2022-08-18
* CVE-2022-1042: Under embargo until 2022-06-19
* CVE-2022-1041: Under embargo until 2022-06-19

Known issues
************

API Changes
***********

Changes in this release
=======================

* All Zephyr public headers have been moved to ``include/zephyr``, meaning they
  need to be prefixed with ``<zephyr/...>`` when included. Because this change
  can potentially break many applications or libraries,
  :kconfig:option:`CONFIG_LEGACY_INCLUDE_PATH` is provided to allow using the
  old include path. This option is now enabled by default to allow a smooth
  transition. In order to facilitate the migration to the new include prefix, a
  script to automate the process is also provided:
  :zephyr_file:`scripts/utils/migrate_includes.py`.

* LoRaWAN: The message type parameter in :c:func:`lorawan_send` was changed
  from ``uint8_t`` to ``enum lorawan_message_type``. If ``0`` was passed for
  unconfirmed message, this has to be changed to ``LORAWAN_MSG_UNCONFIRMED``.

* Disk Subsystem: SPI mode SD cards now use the SD subsystem to communicate
  with SD cards. See :ref:`the disk access api <disk_access_api>` for an
  example of the new devicetree binding format required.

* CAN

  * Added ``const struct device`` parameter to the following CAN callback function signatures:

    * ``can_tx_callback_t``
    * ``can_rx_callback_t``
    * ``can_state_change_callback_t``

  * Allow calling the following CAN API functions from userspace:

    * :c:func:`can_set_mode()`
    * :c:func:`can_calc_timing()`
    * :c:func:`can_calc_timing_data()`
    * :c:func:`can_set_bitrate()`
    * :c:func:`can_get_max_filters()`

  * Changed :c:func:`can_set_bitrate()` to use a sample point of 75.0% for bitrates over 800 kbit/s,
    80.0% for bitrates over 500 kbit/s, and 87.5% for all other bitrates.

  * Split CAN classic and CAN-FD APIs:

    * :c:func:`can_set_timing()` split into :c:func:`can_set_timing()` and
      :c:func:`can_set_timing_data()`.
    * :c:func:`can_set_bitrate()` split into :c:func:`can_set_bitrate()` and
      :c:func:`can_set_bitrate_data()`.

  * Converted the ``enum can_mode`` into a ``can_mode_t`` bitfield and renamed the CAN mode
    definitions:

    * ``CAN_NORMAL_MODE`` renamed to :c:macro:`CAN_MODE_NORMAL`.
    * ``CAN_SILENT_MODE`` renamed to :c:macro:`CAN_MODE_LISTENONLY`.
    * ``CAN_LOOPBACK_MODE`` renamed to :c:macro:`CAN_MODE_LOOPBACK`.
    * The previous ``CAN_SILENT_LOOPBACK_MODE`` can be set using the bitmask ``(CAN_MODE_LISTENONLY |
      CAN_MODE_LOOPBACK)``.

  * STM32H7 The `CONFIG_NOCACHE_MEMORY` no longer is responsible for disabling
    data cache when defined. Now the newly introduced `CONFIG_DCACHE=n` explicitly
    does that.

  * Converted the STM32F1 pin nodes configuration names to include remap information (in
    cases other than NO_REMAP/REMAP_0)
    For instance:

    * ``i2c1_scl_pb8`` renamed to ``i2c1_scl_remap1_pb8``

Removed APIs in this release
============================

* STM32F1 Serial wire JTAG configuration (SWJ CFG) configuration choice
  was moved from Kconfig to :ref:`devicetree <dt-guide>`.
  See the :dtcompatible:`st,stm32f1-pinctrl` devicetree binding for more information.
  As a consequence, the following Kconfig symbols were removed:

  * ``CONFIG_GPIO_STM32_SWJ_ENABLE``
  * ``CONFIG_GPIO_STM32_SWJ_NONJTRST``
  * ``CONFIG_GPIO_STM32_SWJ_NOJTAG``
  * ``CONFIG_GPIO_STM32_SWJ_DISABLE``

* Removed experimental 6LoCAN protocol support.

* Removed the following deprecated CAN APIs:

  * Custom CAN error codes
  * ``can_configure()``
  * ``can_attach_workq()``
  * ``can_attach_isr()``
  * ``can_attach_msgq()``
  * ``can_detach()``
  * ``can_register_state_change_isr()``
  * ``can_write()``

Deprecated in this release
==========================

* :c:func:`nvs_init` is deprecated in favor of utilizing :c:func:`nvs_mount`.
* The TinyCBOR module has been deprecated in favor of the new zcbor CBOR
  library, included with Zephyr in this release.

* SPI

  * Deprecated the `gpio_dev`, `gpio_pin` and `gpio_dt_flags` members from
    spi_cs_control struct in favor of `gpio_dt_spec` gpio.

* PWM

  * The ``pin`` prefix has been removed from all PWM API calls. So for example,
    ``pwm_pin_set_cycles`` is now ``pwm_set_cycles``. The old API calls are
    still provided but marked as deprecated.
  * The PWM period is now always set in nanoseconds, so the ``_nsec`` and
    ``_usec`` set functions have been deprecated. Other units can be specified
    using, e.g. ``PWM_USEC()`` macros, which convert down to nanoseconds.

Stable API changes in this release
==================================

Bluetooth
*********

* Host

  * The enum bt_l2cap_chan_state values BT_L2CAP_CONNECT and BT_L2CAP_DISCONNECT
    has been renamed to BT_L2CAP_CONNECTING and BT_L2CAP_DISCONNECTING.

  * Moved the callbacks :c:func:`pairing_complete`, :c:func:`pairing_failed` and
    :c:func:`bond_delete` from the `struct bt_auth_cb` to a newly created
    informational-only callback `struct bt_auth_info_cb`.

  * The :c:macro:bt_conn_index function now takes a `const struct bt_conn`.

  * The `struct bt_gatt_subscribe_params` :c:func:`write` callback
    function has been deprecated.  A :c:func:`subscribe` callback
    function has been added to replace it.

New APIs in this release
========================

* Util

  * Added :c:macro:`IN_RANGE` for checking if a value is in the range of two
    other values.

* SDHC API

  * Added the :ref:`SDHC api <sdhc_api>`, used to interact with SD host controllers.

* MIPI-DSI

  * Added a :ref:`MIPI-DSI api <mipi_dsi_api>`. This is an experimental API,
    some of the features/APIs will be implemented later.

* CAN

  * Added support for getting the minimum/maximum supported CAN timing parameters:

    * :c:func:`can_get_timing_min()`
    * :c:func:`can_get_timing_max()`
    * :c:func:`can_get_timing_data_min()`
    * :c:func:`can_get_timing_data_max()`

  * Added support for enabling/disabling CAN-FD mode at runtime using :c:macro:`CAN_MODE_FD`.

Kernel
******

Architectures
*************

* ARM

  * AARCH32

  * AARCH64

* Xtensa

Bluetooth
*********

* Audio

* Direction Finding

* Host

* Mesh

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

* Made these changes in other boards:

* Added support for these following shields:


Drivers and Sensors
*******************

* ADC

* CAN

  * Switched from transmitting CAN frames in FIFO/chronological order to transmitting according to
    CAN-ID priority (NXP FlexCAN, ST STM32 bxCAN, Bosch M_CAN, Microchip MCP2515).
  * Added support for ST STM32U5 to the ST STM32 FDCAN driver.
  * Renamed the base Bosch M_CAN devicetree binding compatible from ``bosch,m-can-base`` to
    :dtcompatible:`bosch,m_can-base`.
  * Added CAN controller statistics support (NXP FlexCAN, Renesas R-Car, ST STM32 bxCAN).
  * Added CAN transceiver support.
  * Added generic SocketCAN network interface and removed driver-specific implementations.

* Counter

* DAC

* Disk

* DMA

* EEPROM

* Entropy

* Ethernet

* Flash

* GPIO

* I2C

* I2S

* Interrupt Controller

* MBOX

* MEMC

* Pin control

* PWM

  * Added :c:struct:`pwm_dt_spec` and associated helpers, e.g.
    :c:macro:`PWM_DT_SPEC_GET` or :c:func:`pwm_set_dt`. This addition makes it
    easier to use the PWM API when the PWM channel, period and flags are taken
    from a Devicetree PWM cell.

* Sensor

* Serial

* SPI

* Timer

* USB

* Watchdog

Networking
**********

USB
***

Build and Infrastructure
************************

Libraries / Subsystems
**********************

* Management

  * Added support for MCUMGR Parameters command, which can be used to obtain
    MCUMGR parameters; :kconfig:option:`CONFIG_OS_MGMT_MCUMGR_PARAMS` enables
    the command.
  * Added mcumgr fs handler for getting file status which returns file size;
    controlled with :kconfig:option:`CONFIG_FS_MGMT_FILE_STATUS`
  * Added mcumgr fs handler for getting file hash/checksum, with support for
    IEEE CRC32 and SHA256, the following Kconfig options have been added to
    control the addition:

    * :kconfig:option:`CONFIG_FS_MGMT_CHECKSUM_HASH` to enable the command;
    * :kconfig:option:`CONFIG_FS_MGMT_CHECKSUM_HASH_CHUNK_SIZE` that sets size
      of buffer (stack memory) used for calculation:

      * :kconfig:option:`CONFIG_FS_MGMT_CHECKSUM_IEEE_CRC32` enables support for
        IEEE CRC32.
      * :kconfig:option:`CONFIG_FS_MGMT_HASH_SHA256` enables SHA256 hash support.
      * When hash/checksum query to mcumgr does not specify a type, then the order
        of preference (most priority) is CRC32 followed by SHA256.

  * Added mcumgr os hook to allow an application to accept or decline a reset
    request; :kconfig:option:`CONFIG_OS_MGMT_RESET_HOOK` enables the callback.
  * Added mcumgr fs hook to allow an application to accept or decline a file
    read/write request; :kconfig:option:`CONFIG_FS_MGMT_FILE_ACCESS_HOOK`
    enables the feature which then needs to be registered by the application.
  * Added supplied image header to mcumgr img upload callback parameter list
    which allows the application to inspect it to determine if it should be
    allowed or declined.
  * Made the img mgmt ``img_mgmt_vercmp`` function public to allow application-
    level comparison of image versions.

* SD Subsystem

  * Added the SD subsystem, which is used by the
    :ref:`disk access api <disk_access_api>` to interact with connected SD cards.
    This subsystem uses the :ref:`SDHC api <sdhc_api>` to interact with the SD
    host controller the SD device is connected to.

HALs
****

MCUboot
*******

Trusted Firmware-m
******************

Documentation
*************

Tests and Samples
*****************

Issue Related Items
*******************

These GitHub issues were addressed since the previous 3.0.0 tagged
release:
