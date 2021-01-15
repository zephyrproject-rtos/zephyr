:orphan:

.. _zephyr_2.5:

Zephyr 2.5.0 (Working Draft)
############################

We are pleased to announce the release of Zephyr RTOS version 2.5.0.

Major enhancements with this release include:

* Introduced support for the SPARC processor architecture and the LEON
  processor implementation.

The following sections provide detailed lists of changes by component.

Security Vulnerability Related
******************************

The following CVEs are addressed by this release:

More detailed information can be found in:
https://docs.zephyrproject.org/latest/security/vulnerabilities.html

Known issues
************

You can check all currently known issues by listing them using the GitHub
interface and listing all issues with the `bug label
<https://github.com/zephyrproject-rtos/zephyr/issues?q=is%3Aissue+is%3Aopen+label%3Abug>`_.

API Changes
***********

* Removed SETTINGS_USE_BASE64 support as its been deprecated for more than
  two releases.

* The :c:func:`lwm2m_rd_client_start` function now accepts an additional
  ``flags`` parameter, which allows to configure current LwM2M client session,
  for instance enable bootstrap procedure in the curent session.

* LwM2M execute now supports arguments. The execute callback
  `lwm2m_engine_execute_cb_t` is extended with an ``args`` parameter which points
  to the CoAP payload that comprises the arguments, and an ``args_len`` parameter
  to indicate the length of the ``args`` data.

* Changed vcnl4040 dts binding default for property 'proximity-trigger'.
  Changed the default to match the HW POR state for this property.

* The :c:func:`clock_control_async_on` function will now take ``callback`` and
  ``user_data`` as arguments instead of structure which contained list node,
  callback and user data.

* The :c:func:`mqtt_keepalive_time_left` function now returns -1 if keep alive
  messages are disabled by setting ``CONFIG_MQTT_KEEPALIVE`` to 0.

* The ``CONFIG_LEGACY_TIMEOUT_API`` mode has been removed.  All kernel
  timeout usage must use the new-style k_timeout_t type and not the
  legacy/deprecated millisecond counts.

* The :c:func:`coap_pending_init` function now accepts an additional ``retries``
  parameter, allowing to specify the maximum retransmission count of the
  confirmable message.

Deprecated in this release
==========================

* Nordic nRF5340 PDK board deprecated and planned to be removed in 2.6.0.
* ARM Musca-A board and SoC support deprecated and planned to be removed in 2.6.0.

* DEVICE_INIT was deprecated in favor of utilizing DEVICE_DEFINE directly.

* DEVICE_AND_API_INIT was deprecated in favor of DEVICE_DT_INST_DEFINE and
  DEVICE_DEFINE.

Removed APIs in this release
============================

Stable API changes in this release
==================================

Kernel
******

Architectures
*************

* ARC

* ARM

  * AARCH32

    * Introduced the functionality for chain-loadable Zephyr
      fimrmware images to force the initialization of internal
      architecture state during early system boot (Cortex-M).

  * AARCH64

* POSIX

* RISC-V

* SPARC

  * Added support for the SPARC architecture, compatible with the SPARC V8
    specification and the SPARC ABI.
  * FPU is supported in both shared and unshared FP register mode.

* x86

Boards & SoC Support
********************

* Added support for these SoC series:

* Made these changes in other SoC series:

* Changes for ARC boards:

* Added support for these ARM boards:

* Added support for these SPARC boards:

  * GR716-MINI LEON3FT microcontroller development board
  * Generic LEON3 board configuration for GRLIB FPGA reference designs
  * SPARC QEMU for emulating LEON3 processors and running kernel tests

* Made these changes in other boards:

  * nRF5340 DK: Selected TF-M as the default Secure Processing Element
    (SPE) when building Zephyr for the non-secure domain.


* Added support for these following shields:

Drivers and Sensors
*******************

* ADC

* Audio

* Bluetooth

* CAN

* Clock Control

* Console

* Counter

* Crypto

* DAC

* Debug

* Display

* DMA

* EEPROM

* Entropy

* ESPI

* Ethernet

* Flash

  * CONFIG_NORDIC_QSPI_NOR_QE_BIT has been removed.  The
    quad-enable-requirements devicetree property should be used instead.

* GPIO

* Hardware Info

* I2C

* I2S

* IEEE 802.15.4

* Interrupt Controller

* IPM

* Keyboard Scan

* LED

* LED Strip

* LoRa

* Modem

* PECI

* Pinmux

* PS/2

* PWM

* Sensor

* Serial

* SPI

* Timer

* USB

  * Made USB DFU class compatible with the target configuration that does not
    have a secondary image slot.
  * Support to use USB DFU within MCUBoot with single application slot mode.

* Video

* Watchdog

* WiFi

Networking
**********

Bluetooth
*********

* Host

  * When privacy has been enabled in order to advertise towards a
    privacy-enabled peer the BT_LE_ADV_OPT_DIR_ADDR_RPA option must now
    be set, same as when privacy has been disabled.

* Mesh

  * The ``bt_mesh_cfg_srv`` structure has been deprecated in favor of a
    standalone Heartbeat API and Kconfig entries for default state values.


* BLE split software Controller

* HCI Driver

Build and Infrastructure
************************

* Improved support for additional toolchains:

* Devicetree

  * :c:macro:`DT_ENUM_IDX_OR`: new macro
  * Support for legacy devicetree macros via
    ``CONFIG_LEGACY_DEVICETREE_MACROS`` was removed. All devicetree-based code
    should be using the new devicetree API introduced in Zephyr 2.3 and
    documented in :ref:`dt-from-c`. Information on flash partitions has moved
    to :ref:`flash_map_api`.

Libraries / Subsystems
**********************

* Disk

* Management

  * MCUmgr

    * Added support for flash devices that have non-0xff erase value.
    * Added optional verification, enabled via
      :option:`CONFIG_IMG_MGMT_REJECT_DIRECT_XIP_MISMATCHED_SLOT`, of an uploaded
      Direct-XIP binary, which will reject any binary that is not able to boot
      from base address of offered upload slot.

  * updatehub

* Settings

* Random

* POSIX subsystem

* Power management

* Logging

* LVGL

  * Library has been updated to minor release v7.6.1

* Shell

* Storage

  * flash_map: Added API to get the value of an erased byte in the flash_area,
    see ``flash_area_erased_val()``.

* Tracing

* Debug

* DFU

 * boot: Reworked using MCUBoot's bootutil_public library which allow to use
   API implementation already provided by MCUboot codebase and remove
   zephyr's own implementations.

HALs
****

* HALs are now moved out of the main tree as external modules and reside in
  their own standalone repositories.

MCUBoot
*******

* bootloader

  * Added hardening against hardware level fault injection and timing attacks,
    see ``CONFIG_BOOT_FIH_PROFILE_HIGH`` and similar kconfig options.
  * Introduced Abstract crypto primitives to simplify porting.
  * Added ram-load upgrade mode (not enabled for zephy-rtos yet).
  * Renamed single-image mode to single-slot mode,
    see ``CONFIG_SINGLE_APPLICATION_SLOT``.
  * Added patch for turning off cache for Cortex M7 before chain-loading.
  * Fixed boostrapping in swap-move mode.
  * Fixed issue causing that interrupted swap-move operation might brick device
    if the primary image was padded.
  * Fixed issue causing that HW stack protection catches the chain-loaded
    application during its early initialization.
  * Added reset of Cortex SPLIM registers before boot.
  * Fixesd build issue that occurs if CONF_FILE contains multiple file paths
    instead of single file path.
  * Added watchdog feed on nRF devices. See ``CONFIG_BOOT_WATCHDOG_FEED`` option.
  * Removed the flash_area_read_is_empty() port implementation function.
  * Initialize the ARM core configuration only when selected by the user,
    see ``CONFIG_MCUBOOT_CLEANUP_ARM_CORE``.
  * Allow the final data chunk in the image to be unaligned in
    the serial-recovery protocol.
  * Kconfig: allow xip-revert only for xip-mode.
  * ext: tinycrypt: update ctr mode to stream.
  * Use minimal CBPRINTF implementation.
  * Configure logging to LOG_MINIMAL by default.
  * boot: cleanup NXP MPU configuration before boot.
  * Fix nokogiri<=1.11.0.rc4 vulnerability.
  * bootutil_public library was extracted as code which is common API for
    MCUboot and the DFU application, see ``CONFIG_MCUBOOT_BOOTUTIL_LIB``

* imgtool

  * Print image digest during verify.
  * Add possibility to set confirm flag for hex files as well.
  * Usage of --confirm implies --pad.
  * Fixed 'custom_tlvs' argument handling.
  * Add support for setting fixed ROM address into image header.


Trusted-Firmware-M
******************

* Synchronized Trusted-Firmware-M module to the upstream v1.2.0 release.

Documentation
*************

Tests and Samples
*****************

Issue Related Items
*******************

These GitHub issues were addressed since the previous 2.4.0 tagged
release:
