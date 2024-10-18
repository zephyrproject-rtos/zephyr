:orphan:

.. _zephyr_4.1:

Zephyr 4.1.0 (Working Draft)
############################

We are pleased to announce the release of Zephyr version 4.1.0.

Major enhancements with this release include:

An overview of the changes required or recommended when migrating your application from Zephyr
v4.0.0 to Zephyr v4.1.0 can be found in the separate :ref:`migration guide<migration_4.1>`.

The following sections provide detailed lists of changes by component.

Security Vulnerability Related
******************************
The following CVEs are addressed by this release:

More detailed information can be found in:
https://docs.zephyrproject.org/latest/security/vulnerabilities.html

API Changes
***********

Removed APIs in this release
============================

 * The deprecated Bluetooth HCI driver API has been removed. It has been replaced by a
   :c:group:`new API<bt_hci_api>` that follows the normal Zephyr driver model.
 * The deprecated ``CAN_MAX_STD_ID`` (replaced by :c:macro:`CAN_STD_ID_MASK`) and ``CAN_MAX_EXT_ID``
   (replaced by :c:macro:`CAN_EXT_ID_MASK`) CAN API macros have been removed.
 * The deprecated ``can_get_min_bitrate()`` (replaced by :c:func:`can_get_bitrate_min`) and
   ``can_get_max_bitrate()`` (replaced by :c:func:`can_get_bitrate_max`) CAN API functions have been
   removed.
 * The deprecated ``can_calc_prescaler()`` CAN API function has been removed.

Deprecated in this release
==========================

* Deprecated the :c:func:`bt_le_set_auto_conn` API function. Application developers can achieve
  the same functionality in their application code by reconnecting to the peer when the
  :c:member:`bt_conn_cb.disconnected` callback is invoked.

* Deprecated TinyCrypt library. The reasons for this are (:github:`43712`):

  * The upstream version of this library is no longer maintained.
  * Reducing the number of cryptographic libraries in Zephyr to reduce maintenance overhead.
  * The PSA Crypto API is the recommended cryptographic library for Zephyr.

Architectures
*************

* Common

  * Introduced :kconfig:option:`CONFIG_ARCH_HAS_CUSTOM_CURRENT_IMPL`, which can be selected when
    an architecture implemented and enabled its own :c:func:`arch_current_thread` and
    :c:func:`arch_current_thread_set` functions for faster retrieval of the current CPU's thread
    pointer. When enabled, ``_current`` variable will be routed to the
    :c:func:`arch_current_thread` (:github:`80716`).

* ARC

* ARM

* ARM64

* RISC-V

  * Implements :c:func:`arch_current_thread_set` & :c:func:`arch_current_thread`, which can be enabled
    by :kconfig:option:`CONFIG_RISCV_CURRENT_VIA_GP` (:github:`80716`).

* Xtensa

* native/POSIX

  * :kconfig:option:`CONFIG_NATIVE_APPLICATION` has been deprecated.
  * For the native_sim target :kconfig:option:`CONFIG_NATIVE_SIM_NATIVE_POSIX_COMPAT` has been
    switched to ``n`` by default, and this option has been deprecated.

Kernel
******

Bluetooth
*********

* Audio

* Host

  * :kconfig:option:`CONFIG_BT_BUF_ACL_RX_COUNT` has been deprecated and
    :kconfig:option:`CONFIG_BT_BUF_ACL_RX_COUNT_EXTRA` has been added.

* HCI Drivers

* Mesh

  * Introduced a :c:member:`bt_mesh_health_cli::update` callback which is used to update the message
    published periodically.

Boards & SoC Support
********************

* Added support for these SoC series:

  * Added Raspberry Pi RP2350

* Made these changes in other SoC series:

* Added support for these boards:

   * :zephyr:board:`Raspberry Pi Pico 2 <rpi_pico2>`: ``rpi_pico2``
   * :zephyr:board:`Adafruit QT Py ESP32-S3 <adafruit_qt_py_esp32s3>`: ``adafruit_qt_py_esp32s3``

* Made these board changes:

  * All HWMv1 board name aliases which were added as deprecated in v3.7 are now removed
    (:github:`82247`).
  * ``mimxrt1050_evk`` and ``mimxrt1060_evk`` revisions for ``qspi`` and ``hyperflash`` have been
    converted into variants. ``mimxrt1060_evkb`` has been converted into revision ``B`` of
    ``mimxrt1060_evk``.
  * Enabled USB, RTC on NXP ``frdm_mcxn236``

* Added support for the following shields:

Build system and Infrastructure
*******************************

* Space-separated lists support has been removed from Twister configuration
  files. This feature was deprecated a long time ago. Projects that do still use
  them can use the :zephyr_file:`scripts/utils/twister_to_list.py` script to
  automatically migrate Twister configuration files.

* Twister

  * Test Case names for Ztest now include Ztest suite name, so the resulting identifier has
    three sections and looks like: ``<test_scenario_name>.<ztest_suite_name>.<ztest_name>``.
    These extended identifiers are used in log output, twister.json and testplan.json,
    as well as for ``--sub-test`` command line parameters (:github:`80088`).
  * The ``--no-detailed-test-id`` command line option also shortens Ztest Test Case names excluding
    its Test Scenario name prefix which is the same as the parent Test Suite id (:github:`82302`).
    Twister XML reports have full testsuite name as ``testcase.classname property`` resolving
    possible duplicate testcase elements in ``twister_report.xml`` testsuite container.

Drivers and Sensors
*******************

* ADC

* Battery

* CAN

* Charger

* Clock control

* Counter

* DAC

* Disk

* Display

  * Added flag ``frame_incomplete`` to ``display_write`` that indicates whether a write is the last
    write of the frame, allowing display drivers to implement double buffering / tearing enable
    signal handling (:github:`81250`)
  * Added ``frame_incomplete`` handling to SDL display driver (:dtcompatible:`zephyr,sdl-dc`)
    (:github:`81250`)
  * Added transparency support to SDL display driver (:dtcompatible:`zephyr,sdl-dc`) (:github:`81184`)

* Ethernet

* Flash

  * NXP MCUX FlexSPI: Add support for 4-byte addressing mode of Micron MT25Q flash family (:github:`82532`)

* FPGA

  * Extracted from :dtcompatible:`lattice,ice40-fpga` the compatible and driver for
    :dtcompatible:`lattice,ice40-fpga-bitbang`. This replaces the original ``load_mode`` property from
    the binding, which selected either the SPI or GPIO bitbang load mode.

* GNSS

* GPIO

* Hardware info

* I2C

* I2S

* I3C

* Input

* LED

  * Added a new set of devicetree based LED APIs, see :c:struct:`led_dt_spec`.
  * lp5569: added use of auto-increment functionality.
  * lp5569: implemented ``write_channels`` api.
  * lp5569: demonstrate ``led_write_channels`` in the sample.

* LED Strip

* LoRa

* Mailbox

* MDIO

* MFD

* Modem

* MIPI-DBI

* MSPI

* Pin control

  * Added new driver for Silabs Series 2 (:dtcompatible:`silabs,dbus-pinctrl`).

* PWM

* Regulators

* Reset

* RTC

* RTIO

* SDHC

* Sensors

* Serial

* SPI

* Stepper

  * Added driver for ADI TMC2209. :dtcompatible:`adi,tmc2209`.
  * Added driver for TI DRV8424. :dtcompatible:`ti,drv8424`.
  * Added :kconfig:option:`CONFIG_STEP_DIR_STEPPER` to enable common functions for step/dir steppers.

* USB

* Video

  * Changed :file:`include/zephyr/drivers/video-controls.h` to have control IDs (CIDs) matching
    those present in the Linux kernel.

* Watchdog

* Wi-Fi

Networking
**********

* ARP:

* CoAP:

* Connection manager:

* DHCPv4:

* DHCPv6:

* DNS/mDNS/LLMNR:

* gPTP/PTP:

* HTTP:

* IPSP:

* IPv4:

* IPv6:

* LwM2M:

* Misc:

* MQTT:

* Network Interface:

* OpenThread:

  * Removed the implicit enabling of the :kconfig:option:`CONFIG_NVS` Kconfig option by the :kconfig:option:`CONFIG_NET_L2_OPENTHREAD` symbol.

* PPP

* Shell:

* Sockets:

  * The deprecated :kconfig:option:`CONFIG_NET_SOCKETS_POSIX_NAMES` option has been removed.
    It was a legacy option and was used to allow user to call BSD socket API while not enabling POSIX API.
    This removal means that in order to use POSIX API socket calls, one needs to enable the
    :kconfig:option:`CONFIG_POSIX_API` option.
    If the application does not want or is not able to enable that option, then the socket API
    calls need to be prefixed by a ``zsock_`` string.

* Syslog:

* TCP:

* Websocket:

* Wi-Fi:

  * hostap: Removed the unused default Crypto module :kconfig:option:`CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO` Kconfig option.

* zperf:

USB
***

Devicetree
**********

* Added :c:macro:`DT_ANY_INST_HAS_BOOL_STATUS_OKAY`.

Kconfig
*******

Libraries / Subsystems
**********************

* Debug

* Demand Paging

* Formatted output

* Management

* Logging

* Modem modules

* Power management

* Crypto

  * The Kconfig symbol :kconfig:option:`CONFIG_MBEDTLS_PSA_STATIC_KEY_SLOTS` was
    added to allow Mbed TLS to use statically allocated buffers to store key material
    in its PSA Crypto core instead of heap-allocated ones. This can help reduce
    (or remove, if no other component makes use of it) heap memory requirements
    from the final application.

  * The Kconfig symbol :kconfig:option:`CONFIG_MBEDTLS_PSA_KEY_SLOT_COUNT` was
    added to allow selecting the number of key slots available in the Mbed TLS
    implementation of the PSA Crypto core. It defaults to 16. Since each
    slot consumes RAM memory even if unused, this value can be tweaked in order
    to minimize RAM usage.

* CMSIS-NN

* FPGA

* Random

* SD

* State Machine Framework

* Storage

  * Shell: :kconfig:option:`CONFIG_FILE_SYSTEM_SHELL_MOUNT_COMMAND` was added,
    allowing the mount subcommand to be optionally disabled. This can reduce
    flash and RAM usage since it requires the heap to be present.

* Task Watchdog

* POSIX API

* LoRa/LoRaWAN

* ZBus

HALs
****

* Nordic

* STM32

* ADI

* Espressif

MCUboot
*******

OSDP
****

Trusted Firmware-M
******************

LVGL
****

* Added ``frame_incomplete`` support to indicate whether a write is the last
  write of the frame (:github:`81250`)

Tests and Samples
*****************

* Fixed incorrect alpha values in :zephyr_file:`samples/drivers/display`. (:github:`81184`)
* Added :zephyr_file:`samples/modules/lvgl/screen_transparency`. (:github:`81184`)

Issue Related Items
*******************

Known Issues
============
