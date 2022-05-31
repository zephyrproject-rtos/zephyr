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

* Aborting an essential thread now causes a kernel panic, as the
  documentation has always promised but the kernel has never
  implemented.

* The k_timer handler can now correct itself for lost time due to very
  late-arriving interrupts.

* Defer SMP interprocessor interrupts so that they are sent only at
  schedule points and not synchronously when the scheduler state
  changes.  This prevents IPI "storms" with code that does many
  scheduler operations at once (e.g. waking up a bunch of threads).

* The timeslicing API now allows slice times to be controlled
  independently for each thread, and provides a callback to the app
  when a thread timeslice has expired.  The intent is that this will
  allow apps the tools to implement CPU resource control algorithms
  (e.g. fairness or interactivity metrics, budget tracking) that are
  out of scope for Zephyr's deterministic RTOS scheduler.

Architectures
*************

* ARM

  * AARCH32

  * AARCH64

* Xtensa

  * Optimize context switches when KERNEL_COHERENCE is enabled to
    avoid needless stack invalidations for threads that have not
    migrated between CPUs.

  * Fix bug that could return directly into a thread context from a
    nested interrupt instead of properly returning to the preempted
    ISR.

* x64_64

  * UEFI devices can now use the firmware-initialized system console
    API as a printk/logging backend, simplifying platform bringup on
    devices without known-working serial port configurations.

Bluetooth
*********

* Extended and Periodic advertising are no longer experimental
* Direction Finding is no longer experimental
* Added support for disabling Bluetooth, including a new ``bt_disable()`` API
  call

* Audio

  * Changed the implementation of PACS to indicate instead of notifying
  * Added support for the Broadcast Audio Scan Service (BASS)
  * Added support for the Hearing Access Service (HAS)
  * Added support for the Telephone Bearer Service (TBS)

* Direction Finding

  * Added sampling and switching offset configuration

* Host

  * Added new Kconfig options to select ISO Central and Peripheral role support
    separately
  * Added a new ``bt_get_appearance()`` API call
  * Implemented support for dynamic appearance, including a new
    ``bt_set_appearance()`` API call
  * Implemented support for L2CAP collision mitigation
  * Changed the scheduling of auto-initiated HCI commands so that they execute
    synchronously
  * Added a new ``bt_is_ready()`` API call to find out if Bluetooth is
    currently enabled and initialized
  * Added support for automatic MTU exchange right after a connection is
    established
  * Created a new ``auth_info_cb`` to group the security-related callbacks under
    a single struct
  * Optimized the memory usage of the Object Transfer Service
  * Added a new ``bt_hci_le_rand()`` API call to obtain a random number from the
    LE Controller
  * Added a new public API to connect EATT channels, ``bt_eatt_connect()``
  * Optimized L2CAP channels resource usage when not using dynamic channels
  * Added the ability to run the Bluetooth RX context from a workqueue, in order
    to optimize RAM usage. See ``CONFIG_BT_RECV_CONTEXT``
  * Added support for TX complete callback on EATT channels
  * Corrected the calling of the MTU callback to happen on any reconfiguration

* Mesh

  * Added support for Proxy Client
  * Added support for Provisioners over PB-GATT
  * Added a new heartbeat publication callback option

* Controller

  * Added support for the full ISO TX data path, including ISOAL
  * Added support for ISO Broadcast Channel Map Update
  * Added support for ISO Synchronized Receiver Channel Map Update
  * The new implementation of LL Control Procedures is now the default whenever
    Direction Finding is enabled
  * Added support for all missing v3 and v4 DTM commands
  * Implemented ISO-AL TX unframed fragmentation
  * Added support for back-to-back receiving of PDUs on nRF5x platforms
  * Increased the maximum number of simultaneous connections to 250

* HCI Driver

Boards & SoC Support
********************

* Added support for these SoC series:

* Removed support for these SoC series:

* Made these changes in other SoC series:

  * Added Atmel SAM UPLL clock support

* Changes for ARC boards:

* Added support for these ARM boards:

* Added support for these ARM64 boards:

* Removed support for these ARM boards:

* Removed support for these X86 boards:

* Added support for these RISC-V boards:

  * GigaDevice GD32VF103C-EVAL

* Made these changes in other boards:

  * sam4s_xplained: Add support for HWINFO
  * sam_e70_xlained: Add support for HWINFO and CAN-FD
  * sam_v71_xult: Add support for HWINFO and CAN-FD
  * gd32e103v_eval: Add prescaler to timer
  * longan_nano: Add support for TF-Card slot

* Added support for these following shields:


Drivers and Sensors
*******************

* ADC

  * Atmel SAM0: Fixed adc voltage reference

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

* HWINFO

  * Atmel SAM: Added RSTC support

* I2C

* I2S

* Interrupt Controller

* MBOX

* MEMC

* Pin control

  * New platforms added to ``pinctrl`` state-based API:

    * Atmel SAM/SAM0

* PWM

  * Added :c:struct:`pwm_dt_spec` and associated helpers, e.g.
    :c:macro:`PWM_DT_SPEC_GET` or :c:func:`pwm_set_dt`. This addition makes it
    easier to use the PWM API when the PWM channel, period and flags are taken
    from a Devicetree PWM cell.

* Reset

  * Added reset controller driver API.

* Sensor

  * Added NCPX ADC comparator driver.
  * Enhanced the BME680 driver to support SPI.
  * Enhanced the LIS2DW12 driver to support additional filtering and interrupt
    modes.
  * Added ICM42670 6-axis accelerometer driver.
  * Enhanced the VL53L0X driver to support reprogramming its I2C address.
  * Enhanced the Microchip XEC TACH driver to support pin control and MEC172x.
  * Added ITE IT8XXX2 voltage comparator driver.
  * Fixed register definitions in the LSM6DSL driver.
  * Fixed argument passing bug in the ICM42605 driver.
  * Removed redundant DEV_NAME helpers in various drivers.
  * Enhanced the LIS2DH driver to support device power management.
  * Fixed overflow issue in sensor_value_from_double().
  * Added MAX31875 temperature sensor driver.

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
  * mcumgr will now only return `MGMT_ERR_ENOMEM` when it fails to allocate
    a memory buffer for request processing, when previously it would wrongly
    report this error when the SMP response failed to fit into a buffer;
    now when encoding of response fails `MGMT_ERR_EMSGSIZE` will be
    reported. This addresses issue :github:`44535`.

* SD Subsystem

  * Added the SD subsystem, which is used by the
    :ref:`disk access api <disk_access_api>` to interact with connected SD cards.
    This subsystem uses the :ref:`SDHC api <sdhc_api>` to interact with the SD
    host controller the SD device is connected to.

* Power management

  * Added :kconfig:option:`CONFIG_PM_DEVICE_POWER_DOMAIN_DYNAMIC`.
    This option enables support for dynamically bind devices to a Power Domain. The
    memory required to dynamically bind devices is pre-allocated at build time and
    is based on the number of devices set in
    :kconfig:option:`CONFIG_PM_DEVICE_POWER_DOMAIN_DYNAMIC_NUM`. The API introduced
    to use this feature are:

    * :c:func:`pm_device_power_domain_add()`
    * :c:func:`pm_device_power_domain_remove()`

  * The default policy was renamed from `PM_POLICY_RESIDENCY` to `PM_POLICY_DEFAULT`,
    and the `PM_POLICY_APP` to `PM_POLICY_CUSTOM`.

  * The following functions were renamed:

    * :c:func:`pm_power_state_next_get()` with :c:func:`pm_state_next_get()`
    * :c:func:`pm_power_state_force()` with :c:func:`pm_state_force()`

  * Removed the deprecated function :c:func:`pm_device_state_set()`.

  * The state constraint APIs were moved (and renamed) to the policy
    API and accounts substates.

    * :c:func:`pm_constraint_get()` with :c:func:`pm_policy_state_lock_is_active()`
    * :c:func:`pm_constraint_set()` with :c:func:`pm_policy_state_lock_get()`
    * :c:func:`pm_constraint_release()` with :c:func:`pm_policy_state_lock_put()`

  * New API to set maximum latency requirements. The `DEFAULT` policy will account
    the latency when computing the next state.

    * :c:func:`pm_policy_latency_request_add()`
    * :c:func:`pm_policy_latency_request_update()`
    * :c:func:`pm_policy_latency_request_remove()`

  * The API to set a device initial state was changed to be usable independently of
    whether :kconfig:option:`CONFIG_PM_DEVICE_RUNTIME`

    * :c:func:`pm_device_runtime_init_suspended()` with :c:func:`pm_device_init_suspended()`
    * :c:func:`pm_device_runtime_init_off()` with :c:func:`pm_device_init_off()`


HALs
****

* Atmel

  * Added dt-bindings, documentation and scripts to support state-based pin
    control (``pinctrl``) API.
  * Imported new SoCs header files:

    * SAML21
    * SAMR34
    * SAMR35

* GigaDevice

  * Fixed GD32_REMAP_MSK macro
  * Fixed gd32f403z pc3 missing pincodes

MCUboot
*******

Trusted Firmware-m
******************

* Updated to TF-M 1.6.0

Documentation
*************

Tests and Samples
*****************

Issue Related Items
*******************

These GitHub issues were addressed since the previous 3.0.0 tagged
release:
