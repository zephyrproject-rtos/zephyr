:orphan:

.. _zephyr_4.0:

Zephyr 4.0.0 (Working Draft)
############################

We are pleased to announce the release of Zephyr version 4.0.0.

Major enhancements with this release include:

An overview of the changes required or recommended when migrating your application from Zephyr
v3.7.0 to Zephyr v4.0.0 can be found in the separate :ref:`migration guide<migration_4.0>`.

The following sections provide detailed lists of changes by component.

Security Vulnerability Related
******************************
The following CVEs are addressed by this release:

More detailed information can be found in:
https://docs.zephyrproject.org/latest/security/vulnerabilities.html

API Changes
***********

* Removed deprecated arch-level CMSIS header files
  ``include/zephyr/arch/arm/cortex_a_r/cmsis.h`` and
  ``include/zephyr/arch/arm/cortex_m/cmsis.h``. ``cmsis_core.h`` needs to be
  included now.

* Removed deprecated ``ceiling_fraction`` macro. :c:macro:`DIV_ROUND_UP` needs
  to be used now.

* Deprecated ``EARLY``, ``APPLICATION`` and ``SMP`` init levels can no longer be
  used for devices.

Removed APIs in this release
============================

* Macro ``K_THREAD_STACK_MEMBER``, deprecated since v3.5.0, has been removed.
  Use :c:macro:`K_KERNEL_STACK_MEMBER` instead.
* ``CBPRINTF_PACKAGE_COPY_*`` macros, deprecated since Zephyr 3.5.0, have been removed.

Deprecated in this release
==========================

* Deprecated the :c:func:`net_buf_put` and :c:func:`net_buf_get` API functions in favor of
  :c:func:`k_fifo_put` and :c:func:`k_fifo_get`.

Architectures
*************

* ARC

* ARM

* ARM64

  * Added initial support for :c:func:`arch_stack_walk` that supports unwinding via esf only

* RISC-V

  * The stack traces upon fatal exception now prints the address of stack pointer (sp) or frame
    pointer (fp) depending on the build configuration.

  * When :kconfig:option:`CONFIG_EXTRA_EXCEPTION_INFO` is enabled, the exception stack frame (arch_esf)
    has an additional field ``csf`` that points to the callee-saved-registers upon an fatal error,
    which can be accessed in :c:func:`k_sys_fatal_error_handler` by ``esf->csf``.

    * For SoCs that select `RISCV_SOC_HAS_ISR_STACKING`, the `SOC_ISR_STACKING_ESF_DECLARE` has to
      include the `csf` member, otherwise the build would fail.

* Xtensa

* x86

  * Added initial support for :c:func:`arch_stack_walk` that supports unwinding via esf only

Kernel
******

Bluetooth
*********

* Audio

  * :c:func:`bt_tbs_client_register_cb` now supports multiple listeners and may now return an error.

* Host

  * Added API :c:func:`bt_gatt_get_uatt_mtu` to get current Unenhanced ATT MTU of a given
    connection (experimental).

* HCI Drivers

Boards & SoC Support
********************

* Added support for these SoC series:

* Made these changes in other SoC series:

  * NXP S32Z270: Added support for the new silicon cut version 2.0. Note that the previous
    versions (1.0 and 1.1) are no longer supported.

* Added support for these boards:

* Made these board changes:

  * :ref:`native_posix<native_posix>` has been deprecated in favour of
    :ref:`native_sim<native_sim>`.
  * Support for Google Kukui EC board (``google_kukui``) has been dropped.

* Added support for the following shields:

Build system and Infrastructure
*******************************

* Added support for .elf files to the west flash command for jlink, pyocd and linkserver runners.

Documentation
*************

 * Added two new build commands, ``make html-live`` and ``make html-live-fast``, that automatically locally
   host the generated documentation. They also automatically rebuild and rehost the documentation when changes
   to the input ``.rst`` files are detected on the filesystem.

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

* Ethernet

  * LiteX: Renamed the ``compatible`` from ``litex,eth0`` to :dtcompatible:`litex,liteeth`.

* Flash

* GNSS

* GPIO

* Hardware info

* I2C

* I2S

* I3C

* Input

* LED

  * lp5569: added ``charge-pump-mode`` property to configure the charge pump of the lp5569.

  * lp5569: added ``enable-gpios`` property to describe the EN/PWM GPIO of the lp5569.

  * LED code samples have been consolidated under the :zephyr_file:`samples/drivers/led` directory.

* LED Strip

  * Updated ws2812 GPIO driver to support dynamic bus timings

* LoRa

* Mailbox

* MDIO

* MFD

* Modem

  * Added support for the U-Blox LARA-R6 modem.
  * Added support for setting the modem's UART baudrate during init.

* MIPI-DBI

* MSPI

* Pin control

* PWM

* Regulators

* Reset

* RTC

* RTIO

* SDHC

* Sensors

  * The existing driver for the Microchip MCP9808 temperature sensor transformed and renamed
    to support all JEDEC JC 42.4 compatible temperature sensors. It now uses the
    :dtcompatible:`jedec,jc-42.4-temp` compatible string instead to the ``microchip,mcp9808``
    string.

* Serial

  * LiteX: Renamed the ``compatible`` from ``litex,uart0`` to :dtcompatible:`litex,uart`.

* SPI

* USB

* Video

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
  * Location object: optional resources altitude, radius, and speed can now be
  used optionally as per the location object's specification. Users of these
  resources will now need to provide a read buffer.

  * lwm2m_senml_cbor: Regenerated generated code files using zcbor 0.9.0

* Misc:

* MQTT:

* Network Interface:

* OpenThread

* PPP

* Shell:

* Sockets:

* Syslog:

* TCP:

* Websocket:

* Wi-Fi:

* zperf:

USB
***

Devicetree
**********

Kconfig
*******

Libraries / Subsystems
**********************

* Debug

* Demand Paging

* Formatted output

* Management

  * MCUmgr

    * Added support for :ref:`mcumgr_smp_group_10`, which allows for listing information on
      supported groups.
    * Fixed formatting of milliseconds in :c:enum:`OS_MGMT_ID_DATETIME_STR` by adding
      leading zeros.

* Logging

* Modem modules

* Power management

* Crypto

  * Mbed TLS was updated to version 3.6.1. The release notes can be found at:
    https://github.com/Mbed-TLS/mbedtls/releases/tag/mbedtls-3.6.1

* CMSIS-NN

* FPGA

* Random

* SD

* Shell:

  * Reorganized the ``kernel threads`` and ``kernel stacks`` shell command under the
    L1 ``kernel thread`` shell command as ``kernel thread list`` & ``kernel thread stacks``
  * Added multiple shell command to configure the CPU mask affinity / pinning a thread in
    runtime, do ``kernel thread -h`` for more info.

* State Machine Framework

* Storage

  * LittleFS: The module has been updated with changes committed upstream
    from version 2.8.1, the last module update, up to and including
    the released version 2.9.3.

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

zcbor
*****

* Updated the zcbor library to version 0.9.0.
  Full release notes at https://github.com/NordicSemiconductor/zcbor/blob/0.9.0/RELEASE_NOTES.md
  Migration guide at https://github.com/NordicSemiconductor/zcbor/blob/0.9.0/MIGRATION_GUIDE.md
  Highlights:

    * Many code generation bugfixes

    * You can now decide at run-time whether the decoder should enforce canonical encoding.

    * Allow --file-header to accept a path to a file with header contents

Tests and Samples
*****************

Issue Related Items
*******************

Known Issues
============
