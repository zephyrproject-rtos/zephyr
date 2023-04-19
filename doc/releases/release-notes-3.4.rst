:orphan:

.. _zephyr_3.4:

Zephyr 3.4.0 (Working Draft)
############################

We are pleased to announce the release of Zephyr version 3.4.0.

Major enhancements with this release include:

The following sections provide detailed lists of changes by component.

Security Vulnerability Related
******************************

API Changes
***********

Changes in this release
=======================

* Any applications using the mcuboot image manager
  (:kconfig:option:`CONFIG_MCUBOOT_IMG_MANAGER`) will now need to also select
  :kconfig:option:`CONFIG_FLASH_MAP` and :kconfig:option:`CONFIG_STREAM_FLASH`,
  this prevents a cmake dependency loop if the image manager Kconfig is enabled
  manually without also manually enabling the other options.

* Including hawkbit in an application now requires additional Kconfig options
  to be selected, previously these options would have been selected
  automatically but have changed from ``select`` options in Kconfig files to
  ``depends on``:

   +--------------------------------------------------+
   | :kconfig:option:`CONFIG_NVS`                     |
   +--------------------------------------------------+
   | :kconfig:option:`CONFIG_FLASH`                   |
   +--------------------------------------------------+
   | :kconfig:option:`CONFIG_FLASH_MAP`               |
   +--------------------------------------------------+
   | :kconfig:option:`CONFIG_STREAM_FLASH`            |
   +--------------------------------------------------+
   | :kconfig:option:`CONFIG_REBOOT`                  |
   +--------------------------------------------------+
   | :kconfig:option:`CONFIG_HWINFO`                  |
   +--------------------------------------------------+
   | :kconfig:option:`CONFIG_NET_TCP`                 |
   +--------------------------------------------------+
   | :kconfig:option:`CONFIG_NET_SOCKETS`             |
   +--------------------------------------------------+
   | :kconfig:option:`CONFIG_IMG_MANAGER`             |
   +--------------------------------------------------+
   | :kconfig:option:`CONFIG_NETWORKING`              |
   +--------------------------------------------------+
   | :kconfig:option:`CONFIG_HTTP_CLIENT`             |
   +--------------------------------------------------+
   | :kconfig:option:`CONFIG_DNS_RESOLVER`            |
   +--------------------------------------------------+
   | :kconfig:option:`CONFIG_JSON_LIBRARY`            |
   +--------------------------------------------------+
   | :kconfig:option:`CONFIG_NET_SOCKETS_POSIX_NAMES` |
   +--------------------------------------------------+
   | :kconfig:option:`CONFIG_BOOTLOADER_MCUBOOT`      |
   +--------------------------------------------------+

* Including updatehub in an application now requires additional Kconfig options
  to be selected, previously these options would have been selected
  automatically but have changed from ``select`` options in Kconfig files to
  ``depends on``:

   +--------------------------------------------------+
   | :kconfig:option:`CONFIG_FLASH`                   |
   +--------------------------------------------------+
   | :kconfig:option:`CONFIG_STREAM_FLASH`            |
   +--------------------------------------------------+
   | :kconfig:option:`CONFIG_FLASH_MAP`               |
   +--------------------------------------------------+
   | :kconfig:option:`CONFIG_REBOOT`                  |
   +--------------------------------------------------+
   | :kconfig:option:`CONFIG_MCUBOOT_IMG_MANAGER`     |
   +--------------------------------------------------+
   | :kconfig:option:`CONFIG_IMG_MANAGER`             |
   +--------------------------------------------------+
   | :kconfig:option:`CONFIG_IMG_ENABLE_IMAGE_CHECK`  |
   +--------------------------------------------------+
   | :kconfig:option:`CONFIG_BOOTLOADER_MCUBOOT`      |
   +--------------------------------------------------+
   | :kconfig:option:`CONFIG_MPU_ALLOW_FLASH_WRITE`   |
   +--------------------------------------------------+
   | :kconfig:option:`CONFIG_NETWORKING`              |
   +--------------------------------------------------+
   | :kconfig:option:`CONFIG_NET_UDP`                 |
   +--------------------------------------------------+
   | :kconfig:option:`CONFIG_NET_SOCKETS`             |
   +--------------------------------------------------+
   | :kconfig:option:`CONFIG_NET_SOCKETS_POSIX_NAMES` |
   +--------------------------------------------------+
   | :kconfig:option:`CONFIG_COAP`                    |
   +--------------------------------------------------+
   | :kconfig:option:`CONFIG_DNS_RESOLVER`            |
   +--------------------------------------------------+
   | :kconfig:option:`CONFIG_JSON_LIBRARY`            |
   +--------------------------------------------------+
   | :kconfig:option:`CONFIG_HWINFO`                  |
   +--------------------------------------------------+

* The sensor driver API clarified :c:func:`sensor_trigger_set` to state that
  the user-allocated sensor trigger will be stored by the driver as a pointer,
  rather than a copy, and passed back to the handler. This enables the handler
  to use :c:macro:`CONTAINER_OF` to retrieve a context pointer when the trigger
  is embedded in a larger struct and requires that the trigger is not allocated
  on the stack. Applications that allocate a sensor trigger on the stack need
  to be updated.

* Converted few drivers to the :ref:`input` subsystem.

  * ``gpio_keys``: moved out of ``gpio``, replaced the custom API to use input
    events instead, the :dtcompatible:`zephyr,gpio-keys` binding is unchanged
    but now requires ``zephyr,code`` to be set.
  * ``ft5336``: moved from :ref:`kscan_api` to :ref:`input`, renamed the Kconfig
    options from ``CONFIG_KSCAN_FT5336``, ``CONFIG_KSCAN_FT5336_PERIOD`` and
    ``KSCAN_FT5336_INTERRUPT`` to :kconfig:option:`CONFIG_INPUT_FT5336`,
    :kconfig:option:`CONFIG_INPUT_FT5336_PERIOD` and
    :kconfig:option:`CONFIG_INPUT_FT5336_INTERRUPT`.
  * ``kscan_sdl``: moved from :ref:`kscan_api` to :ref:`input`, renamed the Kconfig
    option from ``KSCAN_SDL`` to :kconfig:option:`CONFIG_INPUT_SDL_TOUCH` and the
    compatible from ``zephyr,sdl-kscan`` to
    :dtcompatible:`zephyr,input-sdl-touch`.
  * Touchscreen drivers converted to use the input APIs can use the
    :dtcompatible:`zephyr,kscan-input` driver to maintain Kscan compatilibity.

* The declaration of :c:func:`main` has been changed from ``void
  main(void)`` to ``int main(void)``. The main function is required to
  return the value zero. All other return values are reserved. This aligns
  Zephyr with the C and C++ language specification requirements for
  "hosted" environments, avoiding compiler warnings and errors. These
  compiler messages are generated when applications are built in "hosted"
  mode (which means without the ``-ffreestanding`` compiler flag). As the
  ``-ffreestanding`` flag is currently enabled unless the application is
  using picolibc, only applications using picolibc will be affected by this
  change at this time.

Removed APIs in this release
============================

Deprecated in this release
==========================

* Configuring applications with ``prj_<board>.conf`` files has been deprecated,
  this should be replaced by using a prj.conf with the common configuration and
  board-specific configuration in board Kconfig fragments in the ``boards``
  folder of the application.

* On nRF51 and nRF52-based boards, the behaviour of the reset reason being
  provided to :c:func:`sys_reboot` and being set in the GPREGRET register has
  been dropped. This function will now just reboot the device without changing
  the register contents. The new method for setting this register uses the boot
  mode feature of the retention subsystem, see the
  :ref:`boot mode API <boot_mode_api>` for details. To restore the deprecated
  functionality, enable
  :kconfig:option:`CONFIG_NRF_STORE_REBOOT_TYPE_GPREGRET`.

Stable API changes in this release
==================================

* Removed `bt_set_oob_data_flag` and replaced it with two new API calls:
  * :c:func:`bt_le_oob_set_sc_flag` for setting/clearing OOB flag in SC pairing
  * :c:func:`bt_le_oob_set_legacy_flag` for setting/clearing OOB flag in legacy paring

* :c:macro:`SYS_INIT` callback no longer requires a :c:struct:`device` argument.
  The new callback signature is ``int f(void)``. A utility script to
  automatically migrate existing projects can be found in
  :zephyr_file:`scripts/utils/migrate_sys_init.py`.

* Changed :c:struct:`spi_config` ``cs`` (:c:struct:`spi_cs_control`) from
  pointer to struct member. This allows using the existing SPI dt-spec macros in
  C++. SPI controller drivers doing ``NULL`` checks on the ``cs`` field to check
  if CS is GPIO-based or not, must now use :c:func:`spi_cs_is_gpio` or
  :c:func:`spi_cs_is_gpio_dt` calls.

New APIs in this release
========================

* Introduced :c:func:`flash_ex_op` function. This allows to perform extra
  operations on flash devices, defined by Zephyr Flash API or by vendor specific
  header files. Support for extra operations is enabled by
  :kconfig:option:`CONFIG_FLASH_EX_OP_ENABLED` which depends on
  :kconfig:option:`CONFIG_FLASH_HAS_EX_OP` selected by driver.

* Introduced :ref:`rtc_api` API which adds experimental support for real-time clock
  devices. These devices previously used the :ref:`counter_api` API combined with
  conversion between unix-time and broken-down time. The new API adds the mandatory
  functions :c:func:`rtc_set_time` and :c:func:`rtc_get_time`, the optional functions
  :c:func:`rtc_alarm_get_supported_fields`, :c:func:`rtc_alarm_set_time`,
  :c:func:`rtc_alarm_get_time`, :c:func:`rtc_alarm_is_pending` and
  :c:func:`rtc_alarm_set_callback` are enabled with
  :kconfig:option:`CONFIG_RTC_ALARM`, the optional function
  :c:func:`rtc_update_set_callback` is enabled with
  :kconfig:option:`CONFIG_RTC_UPDATE`, and lastly, the optional functions
  :c:func:`rtc_set_calibration` and :c:func:`rtc_get_calibration` are enabled with
  :kconfig:option:`CONFIG_RTC_CALIBRATION`.

Kernel
******

* Removed absolute symbols :c:macro:`___cpu_t_SIZEOF`,
  :c:macro:`_STRUCT_KERNEL_SIZE`, :c:macro:`K_THREAD_SIZEOF` and
  :c:macro:`_DEVICE_STRUCT_SIZEOF`

Architectures
*************

* ARC
  * Removed absolute symbols :c:macro:`___callee_saved_t_SIZEOF` and
  :c:macro:`_K_THREAD_NO_FLOAT_SIZEOF`

* ARM
  * Removed absolute symbols :c:macro:`___basic_sf_t_SIZEOF`,
  :c:macro:`_K_THREAD_NO_FLOAT_SIZEOF`, :c:macro:`___cpu_context_t_SIZEOF`
  and :c:macro:`___thread_stack_info_t_SIZEOF`

* ARM64
  * Removed absolute symbol :c:macro:`___callee_saved_t_SIZEOF`

* NIOS2
  * Removed absolute symbol :c:macro:`_K_THREAD_NO_FLOAT_SIZEOF`

* RISC-V

* SPARC
  * Removed absolute symbol :c:macro:`_K_THREAD_NO_FLOAT_SIZEOF`

* X86

* Xtensa

Bluetooth
*********

* Audio

* Direction Finding

* Host

* Mesh

  * Added experimental support for Mesh Protocol d1.1r18 specification.
  * Added experimental support for Mesh Binary Large Object Transfer Model d1.0r04_PRr00 specification.
  * Added experimental support for Mesh Device Firmware Update Model d1.0r04_PRr00 specification.

* Controller

* HCI Driver

Boards & SoC Support
********************

* Added support for these SoC series:

* Removed support for these SoC series:

* Made these changes in other SoC series:

* Added support for these ARC boards:

* Added support for these ARM boards:

  * Seeed Studio Wio Terminal

* Added support for these ARM64 boards:

* Added support for these RISC-V boards:

* Added support for these X86 boards:

* Added support for these Xtensa boards:

* Made these changes for ARC boards:

* Made these changes for ARM boards:

* Made these changes for ARM64 boards:

* Made these changes for RISC-V boards:

* Made these changes for X86 boards:

* Made these changes for Xtensa boards:

* Removed support for these ARC boards:

* Removed support for these ARM boards:

* Removed support for these ARM64 boards:

* Removed support for these RISC-V boards:

  * BeagleV Starlight JH7100

* Removed support for these X86 boards:

* Removed support for these Xtensa boards:

* Made these changes in other boards:

* Added support for these following shields:

Build system and infrastructure
*******************************

* Fixed an issue whereby older versions of the Zephyr SDK toolchain were used
  instead of the latest compatible version.

* Fixed an issue whereby building an application with sysbuild and specifying
  mcuboot's verification to be checksum only did not build a bootable image.

* Fixed an issue whereby if no prj.conf file was present then board
  configuration files would not be included by emitting a fatal error. As a
  result, prj.conf files are now mandatory in projects.

* Introduced support for extending/replacing the signing mechanism in zephyr,
  see :ref:`West extending signing <west-extending-signing>` for further
  details.

Drivers and Sensors
*******************

* ADC

 * MCUX LPADC driver now uses the channel parameter to select a software channel
   configuration buffer. Use ``zephyr,input-positive`` and
   ``zephyr,input-negative`` devicetree properties to select the hardware
   channel(s) to link a software channel configuration to.

* Battery-backed RAM

  * Added MCP7940N battery-backed RTC SRAM driver.

* CAN

* Clock control

* Counter

* Crypto

* DAC

* DFU

* Disk

* Display

* DMA

* EEPROM

  * Switched from :dtcompatible:`atmel,at24` to dedicated :dtcompatible:`zephyr,i2c-target-eeprom` for I2C EEPROM target driver.

* Entropy

* ESPI

* Ethernet

* Flash

  * Introduced new flash API call :c:func:`flash_ex_op` which calls
    :c:func:`ec_op` callback provided by a flash driver. This allows to perform
    extra operations on flash devices, defined by Zephyr Flash API or by vendor
    specific header files. :kconfig:option:`CONFIG_FLASH_HAS_EX_OP` should be
    selected by the driver to indicate that extra operations are supported.
    To enable extra operations user should select
    :kconfig:option:`CONFIG_FLASH_EX_OP_ENABLED`.
  * nrf_qspi_nor: Replaced custom API function ``nrf_qspi_nor_base_clock_div_force``
    with ``nrf_qspi_nor_xip_enable`` which apart from forcing the clock divider
    prevents the driver from deactivating the QSPI peripheral so that the XIP
    operation is actually possible.

* FPGA

* Fuel Gauge

* GPIO

  * Converted the ``gpio_keys`` driver to the input subsystem.

* hwinfo

* I2C

* I2S

* I3C

* IEEE 802.15.4

* Input

  * Introduced the :ref:`input` subsystem.

* Interrupt Controller

* IPM

* KSCAN

  * Added a :dtcompatible:`zephyr,kscan-input` input to kscan compatibility driver.
  * Converted the ``ft5336`` and ``kscan_sdl`` drivers to the input subsystem.

* LED

* MBOX

* MEMC

* PCIE

* PECI

* Retained memory

  * Retained memory (retained_mem) driver has been added with backends for
    Nordic nRF GPREGRET, and uninitialised RAM.

Trusted Firmware-M
******************
* Pin control

* PWM

* Power domain

* Regulators

* Reset

* SDHC

* Sensor

* Serial

* SPI

* Timer

  * Support added for stopping Nordic nRF RTC system timer, which fixes an
    issue when booting applications built in prior version of Zephyr.

* USB

* W1

* Watchdog

* WiFi

Networking
**********
* Wi-Fi

  * TWT intervals are changed from milli-seconds to micro-seconds, interval variables are also renamed.

USB
***

Devicetree
**********

Libraries / Subsystems
**********************

* File systems

  * Added :kconfig:option:`CONFIG_FS_FATFS_REENTRANT` to enable the FAT FS reentrant option.
  * With LittleFS as backend, :c:func:`fs_mount` return code was corrected to ``EFAULT`` when
    called with ``FS_MOUNT_FLAG_NO_FORMAT`` and the designated LittleFS area could not be
    mounted because it has not yet been mounted or it required reformatting.
  * The FAT FS initialization order has been updated to match LittleFS, fixing an issue where
    attempting to mount the disk in a global function caused FAT FS to fail due to not being registered beforehand.
    FAT FS is now initialized in POST_KERNEL.

* IPC

  * :c:func:`ipc_service_close_instance` now only acts on bounded endpoints.

* Management

  * Added optional input expiration to shell MCUmgr transport, this allows
    returning the shell to normal operation if a complete MCUmgr packet is not
    received in a specific duration. Can be enabled with
    :kconfig:option:`CONFIG_MCUMGR_TRANSPORT_SHELL_INPUT_TIMEOUT` and timeout
    set with
    :kconfig:option:`CONFIG_MCUMGR_TRANSPORT_SHELL_INPUT_TIMEOUT_TIME`.

  * MCUmgr fs_mgmt upload and download now caches the file handle to improve
    throughput when transferring data, the file is no longer opened and closed
    for each part of a transfer. In addition, new functionality has been added
    that will allow closing file handles of uploaded/downloaded files if they
    are idle for a period of time, the timeout is set with
    :kconfig:option:`MCUMGR_GRP_FS_FILE_AUTOMATIC_IDLE_CLOSE_TIME`. There is a
    new command that can be used to close open file handles which can be used
    after a file upload is complete to ensure that the file handle is closed
    correctly, allowing other transports or other parts of the application
    code to use it.

* Retention

  * Retention subsystem has been added which adds enhanced features over
    retained memory drivers allowing for partitioning, magic headers and
    checksum validation. See :ref:`retention API <retention_api>` for details.
    Support for the retention subsystem is experimental.

  * Boot mode retention module has been added which allows for setting/checking
    the boot mode of an application, initial support has also been added to
    MCUboot to allow for applications to use this as an entrance method for
    MCUboot serial recovery mode. See :ref:`boot mode API <boot_mode_api>` for
    details.

* RTIO

  * Added policy that every ``sqe`` will generate a ``cqe`` (previously an RTIO_SQE_TRANSACTION
    entry would only trigger a ``cqe`` on the last ``sqe`` in the transaction.

HALs
****

MCUboot
*******

* Added :kconfig:option:`CONFIG_MCUBOOT_CMAKE_WEST_SIGN_PARAMS` that allows to pass arguments to
  west sign when invoked from cmake.

Storage
*******

Trusted Firmware-M
******************

zcbor
*****

Updated from 0.6.0 to 0.7.0.
Among other things, this update brings:

* C++ improvements
* float16 support
* Improved docs
* -Wall and -Wconversion compliance

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
