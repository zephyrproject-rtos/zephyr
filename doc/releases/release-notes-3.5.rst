:orphan:

.. _zephyr_3.5:

Zephyr 3.5.0 (Working Draft)
############################

We are pleased to announce the release of Zephyr version 3.5.0.

Major enhancements with this release include:

* Added native_sim (successor to native_posix)

The following sections provide detailed lists of changes by component.

Security Vulnerability Related
******************************
The following CVEs are addressed by this release:

More detailed information can be found in:
https://docs.zephyrproject.org/latest/security/vulnerabilities.html

* CVE-2023-4257: Under embargo until 2023-10-12

* CVE-2023-4258 `Zephyr project bug tracker GHSA-m34c-cp63-rwh7
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-m34c-cp63-rwh7>`_

* CVE-2023-4264 `Zephyr project bug tracker GHSA-rgx6-3w4j-gf5j
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-rgx6-3w4j-gf5j>`_

* CVE-2023-4424: Under embargo until 2023-11-01

* CVE-2023-5055: Under embargo until 2023-11-01

* CVE-2023-5139: Under embargo until 2023-10-25

* CVE-2023-5184 `Zephyr project bug tracker GHSA-8x3p-q3r5-xh9g
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-8x3p-q3r5-xh9g>`_


Kernel
******

Architectures
*************

* ARM

  * Architectural support for Arm Cortex-M has been separated from Arm
    Cortex-A and Cortex-R. This includes separate source modules to handle
    tasks like IRQ management, exception handling, thread handling and swap.
    For implementation details see :github:`60031`.

* ARM

* ARM64

* RISC-V

* Xtensa

* POSIX

  * Has been reworked to use the native simulator.
  * New boards have been added.
  * For the new boards, embedded C libraries can be used, and conflicts with the host symbols
    and libraries avoided.
  * The :ref:`POSIX OS abstraction<posix_support>` is supported in these new boards.
  * AMP targets are now supported.
  * Added support for LLVM source profiling/coverage.

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

  * Nuvoton NuMaker M46x series

* Removed support for these SoC series:

* Made these changes in other SoC series:

  * i.MX RT SOCs no longer enable CONFIG_DEVICE_CONFIGURATION_DATA by default.
    boards using external SDRAM should set CONFIG_DEVICE_CONFIGURATION_DATA
    and CONFIG_NXP_IMX_EXTERNAL_SDRAM to enabled.
  * i.MX RT SOCs no longer support CONFIG_OCRAM_NOCACHE, as this functionality
    can be achieved using devicetree memory regions

* Added support for these ARC boards:

* Added support for these ARM boards:

  * Nuvoton NuMaker Platform M467

* Added support for these ARM64 boards:

* Added support for these RISC-V boards:

* Added support for these X86 boards:

* Added support for these Xtensa boards:

* Added support for these POSIX boards:

  * :ref:`native_sim(_64) <native_sim>`
  * nrf5340bsim_nrf5340_cpu(net|app). A simulated nrf5340 SOC, which uses Babblesim for its radio
    traffic.

* Made these changes for ARC boards:

* Made these changes for ARM boards:

* Made these changes for ARM64 boards:

* Made these changes for RISC-V boards:

* Made these changes for X86 boards:

* Made these changes for Xtensa boards:

* Made these changes for POSIX boards:

  * nrf52_bsim:

    * Has been reworked to use the native simulator as its runner.
    * Multiple HW models improvements and fixes. GPIO & GPIOTE peripherals added.

* Removed support for these ARC boards:

* Removed support for these ARM boards:

* Removed support for these ARM64 boards:

* Removed support for these RISC-V boards:

* Removed support for these X86 boards:

* Removed support for these Xtensa boards:

* Made these changes in other boards:

* Added support for these following shields:

Build system and infrastructure
*******************************

* SCA (Static Code Analysis)

  * Added support for CodeChecker

* Twister now supports ``required_snippets`` in testsuite .yml files, this can
  be used to include a snippet when a test is ran (and exclude any boards from
  running that the snippet cannot be applied to).

* Interrupts

  * Added support for shared interrupts

* Added support for setting MCUboot encryption key in sysbuild which is then
  propagated to the bootloader and target images to automatically create
  encrypted updates.

* Build time priority checking: enable build time priority checking by default.
  This fails the build if the initialization sequence in the final ELF file
  does not match the devicetree hierarchy. It can be turned off by disabling
  the :kconfig:option:`COFNIG_CHECK_INIT_PRIORITIES` option.

* Added a new ``initlevels`` target for printing the final device and
  :c:macro:`SYS_INIT` initialization sequence from the final ELF file.

Drivers and Sensors
*******************

* ADC

* Battery-backed RAM

* CAN

* Clock control

  * Added support for Nuvoton NuMaker M46x

* Counter

* Crypto

* DAC

* DFU

* Disk

* Display

  * Added support for ST7735S (in ST7735R driver)

* DMA

* EEPROM

* Entropy

* ESPI

* Ethernet

  * Added :kconfig:option:`CONFIG_ETH_NATIVE_POSIX_RX_TIMEOUT` to set rx timeout for native posix.

* Flash

  * Introduce npcx flash driver that supports two or more spi nor flashes via a
    single Flash Interface Unit (FIU) module and Direct Read Access (DRA) mode
    for better performance.
  * Added support for Nuvoton NuMaker M46x embedded flash

* FPGA

* Fuel Gauge

* GPIO

  * Added support for Nuvoton NuMaker M46x

* hwinfo

* I2C

* I2S

* I3C

* IEEE 802.15.4

  * A new mandatory method attr_get() was introduced into ieee802154_radio_api.
    Drivers need to implement at least
    IEEE802154_ATTR_PHY_SUPPORTED_CHANNEL_PAGES and
    IEEE802154_ATTR_PHY_SUPPORTED_CHANNEL_RANGES.
  * The hardware capabilities IEEE802154_HW_2_4_GHZ and IEEE802154_HW_SUB_GHZ
    were removed as they were not aligned with the standard and some already
    existing drivers couldn't properly express their channel page and channel
    range (notably SUN FSK and HRP UWB drivers). The capabilities were replaced
    by the standard conforming new driver attribute
    IEEE802154_ATTR_PHY_SUPPORTED_CHANNEL_PAGES that fits all in-tree drivers.
  * The method get_subg_channel_count() was removed from ieee802154_radio_api.
    This method could not properly express the channel range of existing drivers
    (notably SUN FSK drivers that implement channel pages > 0 and may not have
    zero-based channel ranges or UWB drivers that could not be represented at
    all). The method was replaced by the new driver attribute
    IEEE802154_ATTR_PHY_SUPPORTED_CHANNEL_RANGES that fits all in-tree drivers.

* Interrupt Controller

  * GIC: Architecture version selection is now based on the device tree

* Input

  * New drivers: :dtcompatible:`gpio-qdec`, :dtcompatible:`st,stmpe811`.

  * Drivers converted from Kscan to Input: :dtcompatible:`goodix,gt911`
    :dtcompatible:`xptek,xpt2046` :dtcompatible:`hynitron,cst816s`
    :dtcompatible:`microchip,cap1203`.

  * Added a Kconfig option for dumping all events to the console
    :kconfig:option:`CONFIG_INPUT_EVENT_DUMP` and new shell commands
    :kconfig:option:`CONFIG_INPUT_SHELL`.

  * Merged ``zephyr,gpio-keys`` into :dtcompatible:`gpio-keys` and added
    ``zephyr,code`` codes to all in-tree board ``gpio-keys`` nodes.

  * Renamed the callback definition macro from ``INPUT_LISTENER_CB_DEFINE`` to
    :c:macro:`INPUT_CALLBACK_DEFINE`.

* IPM

* KSCAN

* LED

* MBOX

* MEMC

* PCIE

* PECI

* Pin control

  * Added support for Nuvoton NuMaker M46x

* PWM

* Power domain

* Regulators

* Reset

  * Added support for Nuvoton NuMaker M46x

* Retained memory

  * Added support for allowing mutex support to be forcibly disabled with
    :kconfig:option:`CONFIG_RETAINED_MEM_MUTEX_FORCE_DISABLE`.

  * Fixed issue with user mode support not working.

* SDHC

* Sensor

  * Reworked the :dtcompatible:`ti,bq274xx` to add ``BQ27427`` support, fixed
    units for capacity and power channels.

* Serial

  * Added support for Nuvoton NuMaker M46x

  * NS16550: Reworked how device initialization macros.

    * CONFIG_UART_NS16550_ACCESS_IOPORT and CONFIG_UART_NS16550_SIMULT_ACCESS
      are removed. For UART using IO port access, add "io-mapped" property to
      device tree node.

* SPI

  * Remove npcx spi driver implemented by Flash Interface Unit (FIU) module.

* Timer

  * The TI CC13xx/26xx system clock timer compatible was changed from
    :dtcompatible:`ti,cc13xx-cc26xx-rtc` to :dtcompatible:`ti,cc13xx-cc26xx-rtc-timer`
    and the corresponding Kconfig option from :kconfig:option:`CC13X2_CC26X2_RTC_TIMER`
    to :kconfig:option:`CC13XX_CC26XX_RTC_TIMER` for improved consistency and
    extensibility. No action is required unless the internal timer was modified.

* USB

* W1

* Watchdog

* WiFi

Networking
**********

* Time and timestamps in the network subsystem, PTP and IEEE 802.15.4
  were more precisely specified and all in-tree call sites updated accordingly.
  Fields for timed TX and TX/RX timestamps have been consolidated. See
  :c:type:`net_time_t`, :c:struct:`net_ptp_time`, :c:struct:`ieee802154_config`,
  :c:struct:`ieee802154_radio_api` and :c:struct:`net_pkt` for extensive
  documentation. As this is largely an internal API, existing applications will
  most probably continue to work unchanged.

* CoAP:

  * Use 64 bit timer values for calculating transmission timeouts. This fixes potential problems for
    devices that stay on for more than 49 days when the 32 bit uptime counter might roll over and
    cause CoAP packets to not timeout at all on this event.

* LwM2M:

  * Added support for tickless mode. This removes the 500 ms timeout from the socket loop
    so the engine does not constantly wake up the CPU. This can be enabled by
    :kconfig:option:`CONFIG_LWM2M_TICKLESS`.
  * Added new :c:macro:`LWM2M_RD_CLIENT_EVENT_DEREGISTER` event.

* Wi-Fi
  * Added Passive scan support.
  * The Wi-Fi scan API updated with Wi-Fi scan parameter to allow scan mode selection.

USB
***

* USB device HID
  * Kconfig option USB_HID_PROTOCOL_CODE, deprecated in v2.6, is finally removed.

Devicetree
**********

Libraries / Subsystems
**********************

* Management

  * Introduced MCUmgr client support with handlers for img_mgmt and os_mgmt.

  * Added response checking to MCUmgr's :c:enumerator:`MGMT_EVT_OP_CMD_RECV`
    notification callback to allow applications to reject MCUmgr commands.

  * MCUmgr SMP version 2 error translation (to legacy MCUmgr error code) is now
    supported in function handlers by setting ``mg_translate_error`` of
    :c:struct:`mgmt_group` when registering a group. See
    :c:type:`smp_translate_error_fn` for function details.

  * Fixed an issue with MCUmgr img_mgmt group whereby the size of the upload in
    the initial packet was not checked.

  * Fixed an issue with MCUmgr fs_mgmt group whereby some status codes were not
    checked properly, this meant that the error returned might not be the
    correct error, but would only occur in situations where an error was
    already present.

  * Fixed an issue whereby the SMP response function did not check to see if
    the initial zcbor map was created successfully.

  * Fixes an issue with MCUmgr shell_mgmt group whereby the length of a
    received command was not properly checked.

  * Added optional mutex locking support to MCUmgr img_mgmt group, which can
    be enabled with :kconfig:option:`CONFIG_MCUMGR_GRP_IMG_MUTEX`.

  * Added MCUmgr settings management group, which allows for manipulation of
    zephyr settings from a remote device, see :ref:`mcumgr_smp_group_3` for
    details.

  * Added :kconfig:option:`CONFIG_MCUMGR_GRP_IMG_ALLOW_CONFIRM_NON_ACTIVE_IMAGE_SECONDARY`
    and :kconfig:option:`CONFIG_MCUMGR_GRP_IMG_ALLOW_CONFIRM_NON_ACTIVE_IMAGE_ANY`
    that allow to control whether MCUmgr client will be allowed to confirm
    non-active images.

  * Added :kconfig:option:`CONFIG_MCUMGR_GRP_IMG_ALLOW_ERASE_PENDING` that allows
    to erase slots pending for next boot, that are not revert slots.

  * Added ``user_data`` as an optional field to :c:struct:`mgmt_handler` when
    :kconfig:option:`CONFIG_MCUMGR_MGMT_HANDLER_USER_DATA` is enabled.

  * Added optional ``force`` parameter to os mgmt reset command, this can be checked in the
    :c:enum:`MGMT_EVT_OP_OS_MGMT_RESET` notification callback whose data structure is
    :c:struct:`os_mgmt_reset_data`.

  * Added configurable number of SMP encoding levels via
    :kconfig:option:`CONFIG_MCUMGR_SMP_CBOR_MIN_ENCODING_LEVELS`, which automatically increments
    minimum encoding levels for in-tree groups if :kconfig:option:`CONFIG_ZCBOR_CANONICAL` is
    enabled.

* File systems

  * Added support for ext2 file system.
  * Added support of mounting littlefs on the block device from the shell/fs.
  * Added alignment parameter to FS_LITTLEFS_DECLARE_CUSTOM_CONFIG macro, it can speed up read/write
    operation for SDMMC devices in case when we align buffers on CONFIG_SDHC_BUFFER_ALIGNMENT,
    because we can avoid extra copy of data from card bffer to read/prog buffer.

* Retention

  * Added the :ref:`blinfo_api` subsystem.

  * Added support for allowing mutex support to be forcibly disabled with
    :kconfig:option:`CONFIG_RETENTION_MUTEX_FORCE_DISABLE`.

* Binary descriptors

  * Added the :ref:`binary_descriptors` (``bindesc``) subsystem.

HALs
****

* Nuvoton

  * Added Nuvoton NuMaker M46x

MCUboot
*******

  * Added :kconfig:option:`CONFIG_MCUBOOT_BOOTLOADER_NO_DOWNGRADE`
    that allows to inform application that the on-board MCUboot has been configured
    with downgrade  prevention enabled. This option is automatically selected for
    DirectXIP mode and is available for both swap modes.

  * Added :kconfig:option:`CONFIG_MCUBOOT_BOOTLOADER_MODE_OVERWRITE_ONLY`
    that allows to inform application that the on-board MCUboot will overwrite
    the primary slot with secondary slot contents, without saving the original
    image in primary slot.

  * Fixed issue with serial recovery not showing image details for decrypted images.

  * Fixed issue with serial recovery in single slot mode wrongly iterating over 2 image slots.

  * Fixed an issue with boot_serial repeats not being processed when output was sent, this would
    lead to a divergence of commands whereby later commands being sent would have the previous
    command output sent instead.

  * Fixed an issue with the boot_serial zcbor setup encoder function wrongly including the buffer
    address in the size which caused serial recovery to fail on some platforms.

  * Fixed wrongly building in optimize for debug mode by default, this saves a significant amount
    of flash space.

  * Fixed issue with serial recovery use of MBEDTLS having undefined operations which led to usage
    faults when the secondary slot image was encrypted.

  * Added error output when flash device fails to open and asserts are disabled, which will now
    panic the bootloader.

  * Added currently running slot ID and maximum application size to shared data function
    definition.

  * Added P384 and SHA384 support to imgtool.

  * Added optional serial recovery image state and image set state commands.

  * Added ``dumpinfo`` command for signed image parsing in imgtool.

  * Added ``getpubhash`` command to dump the sha256 hash of the public key in imgtool.

  * Added support for ``getpub`` to print the output to a file in imgtool.

  * Added support for dumping the raw versions of the public keys in imgtool.

  * Added support for sharing boot information with application via retention subsystem.

  * Added support for serial recovery to read and handle encrypted seondary slot partitions.

  * Removed ECDSA P224 support.

  * Removed custom image list boot serial extension support.

  * Reworked boot serial extensions so that they can be used by modules or from user repositories
    by switching to iterable sections.

  * Reworked image encryption support for Zephyr, static dummy key files are no longer in the code,
    a pem file must be supplied to extract the private and public keys. The Kconfig menu has
    changed to only show a single option for enabling encryption and selecting the key file.

  * Reworked the ECDSA256 TLV curve agnostic and renamed it to ``ECDSA_SIG``.

  * CDDL auto-generated function code has been replaced with zcbor function calls, this now allows
    the parameters to be supplied in any order.

  * The MCUboot version in this release is version ``2.0.0+0-rc1``.

Storage
*******

Trusted Firmware-M
******************

Trusted Firmware-A
******************

* Updated to TF-A 2.9.0.

zcbor
*****

Documentation
*************

Tests and Samples
*****************

* Created common sample for file systems (`fs_sample`). It originates from sample for FAT
  (`fat_fs`) and supports both FAT and ext2 file systems.

Known Issues
************
