:orphan:

.. _zephyr_2.4:

Zephyr 2.4.0 (Working Draft)
############################

We are pleased to announce the release of Zephyr RTOS version 2.4.0.

Major enhancements with this release include:

* Moved to using C99 integer types and deprecate Zephyr integer types.  The
  Zephyr types can be enabled by Kconfig DEPRECATED_ZEPHYR_INT_TYPES option.

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

* Moved to using C99 integer types and deprecate Zephyr integer types.  The
  Zephyr types can be enabled by Kconfig DEPRECATED_ZEPHYR_INT_TYPES option.

* The ``<sys/util.h>`` header has been promoted to a documented API with
  :ref:`experimental stability <api_lifecycle>`. See :ref:`util_api` for an API
  reference.

* The :c:func:`wdt_feed` function will now return ``-EAGAIN`` if
  issuing a feed would stall the caller.  Application code may need to
  ignore this diagnostic result or initiate another feed operation
  later.

* ``<drivers/uart.h>`` has seen its callbacks normalized.
  :c:type:`uart_callback_t` and :c:type:`uart_irq_callback_user_data_t`
  had their signature changed to add a struct device pointer as first parameter.
  :c:type:`uart_irq_callback_t` has been removed. :c:func:`uart_callback_set`,
  :c:func:`uart_irq_callback_user_data_set` and :c:func:`uart_irq_callback_set`
  user code have been modified accordingly.

* ``<drivers/dma.h>`` has seen its callback normalized. It had its signature
  changed to add a struct device pointer as first parameter. Such callback
  signature has been generalized throuh the addition of dma_callback_t.
  'callback_arg' argument has been renamed to 'user_data. All user code have
  been modified accordingly.

* ``<drivers/ipm.h>`` has seen its callback normalized.
  :c:type:`ipm_callback_t` had its signature changed to add a struct device
  pointer as first parameter. :c:func:`ipm_register_callback` user code have
  been modified accordingly. The context argument has been renamed to user_data
  and all drivers have been modified against it as well.

* The :c:func:`fs_open` function now accepts open flags that are passed as
  a third parameter.
  All custom file system front-ends require change to the implementation
  of ``open`` callback to accept the new parameter.
  To maintain original behaviour within user code, two argument invocations
  should be converted to pass a third argument ``FS_O_CREATE | FS_O_RDWR``.

* The struct device got 3 attributes renamed: ``config_info`` to ``config``,
  ``driver_api`` to ``api`` and finally ``driver_data`` to ``data``.
  This renaming was done to get rid of legacy names, for which the reasons
  do no longer apply.

* All device instances got a const qualifier. So this applies to all APIs
  manipulating ``struct device *`` (ADC, GPIO, I2C, ...). In order to avoid
  const qualifier loss on ISRs, all ISRs now take a ``const *void`` as a
  paremeter as well.

* The ``_gatt_`` and ``_GATT_`` infixes have been removed for the HRS, DIS
  and BAS APIs and the Kconfig options.

* ``<include/bluetooth/gatt.h>`` callback :c:func:`bt_gatt_attr_func_t` used by
  :c:func:`bt_gatt_foreach_attr` and :c:func:`bt_gatt_foreach_attr_type` has
  been changed to always pass the original pointer of attributes along with its
  resolved handle.

Deprecated in this release
==========================


Removed APIs in this release
============================

* Other

  * The deprecated ``MACRO_MAP`` macro has been removed from the
    :ref:`util_api`. Use ``FOR_EACH`` instead.
  * The CONFIG_NET_IF_USERSPACE_ACCESS is removed as it is no longer needed.

* Build system

  * The set of ``*_if_kconfig()`` CMake functions have been removed. Use
    ``_ifdef(CONFIG_ ...)`` instead.

Stable API changes in this release
==================================

* USB

  * HID class callbacks now takes a parameter ``const struct device*`` which
    is the HID device for which callback was called.

Kernel
******

* Initial support for virtual memory management

  * API definitions in ``include/sys/mem_manage.h``.
  * Supporting architectures will implement ``arch_mem_map()`` and enable
    ``CONFIG_MMU``.
  * The kernel is linked at its physical memory location in RAM.
  * The size of the address space is controlled via ``CONFIG_KERNEL_VM_SIZE``
    with memory mapping calls allocating virtual memory growing downward
    from the address space limit towards the system RAM mappings.
  * This infrastructure is still under heavy development.

* Device memory mapped I/O APIs

  * Namedspaced as DEVICE_MMIO and specified in a new
    ``include/sys/device_mmio.h`` header.
  * This is added to facilitate the specification and the storage location of
    device driver memory-mapped I/O regions based on system configuration.

    * Maintained entirely in ROM for most systems.
    * Maintained in RAM with hooks to memory-mapping APIs for MMU or PCI-E
      systems.

* Updates for Memory Domain APIs

  * All threads now are always a member of a memory domain. A new
    memory domain ``k_mem_domain_default`` introduced for initial threads
    like the main thread.
  * The ``k_mem_domain_destroy()`` and ``k_mem_domain_remove_thread()`` APIs
    are now deprecated and will be removed in a future release.
  * Header definitions moved to ``include/app_memory/mem_domain.h``.

* Thread stack specification improvements

  * Introduced a parallel set of ``K_KERNEL_STACK_*`` APIs for specifying
    thread stacks that will never host user threads. This will conserve memory
    as ancillary data structures (such as privilege mode elevation stacks) will
    not need to be created, and certain alignment requirements are less strict.

  * Internal interfaces to the architecture code have been simplified. All
    thread stack macros are now centrally defined, with arches declaring
    support macros to indicate the alignment of the stack pointer, the
    stack buffer base address, and the stack buffer size.

Architectures
*************

* ARC:


* ARM:

  * AARCH32

    * Added support for ARM Cortex-M1 architecture.
    * Implemented the timing API in Cortex-M architecture using the Data
      Watchpoint and Trace (DWT) unit.
    * The interrupt vector relaying feature support was extended to Cortex-M
      Mainline architecture variants.
    * Cortex-M fault handling implementation was enhanced by adding an option to
      generate and supply the full register state to the kernel fatal error
      handling mechanism.
    * Fixed Cortex-M boot sequence for single-threaded applications
      (CONFIG_MULTITHREADING=n).
    * Added thread safety to Non-Secure entry function calls in ARMv8-M
      architecture.
    * Fixed stack randomization for main thread.
    * Fixed exception vector table alignment in Cortex-M architecture
    * Increased test coverage in QEMU for ARMv6-M architecture variant.
    * Removed the implementation of arch_mem_domain_* APIs for Cortex-M

  * AARCH64

    * Re-implemented thread context-switch to use the _arch_switch() API

* POSIX:


* RISC-V:


* x86:

  * x86 MMU paging support has been overhauled to meet CONFIG_MMU requirements.

    * ``arch_mem_map()`` is implemented.
    * Restored support for 32-bit non-PAE paging. PAE use is now controlled
      via the ``CONFIG_X86_PAE`` option
    * Initial kernel page tables are now created at build time.
    * Page tables are no longer strictly identity-mapped

  * Added ``zefi`` infrastructure for packaging the 64-bit Zephyr kernel into
    an EFI application.

  * Added a GDB stub implementation that works over serial for x86 32-bit.

Boards & SoC Support
********************

* Added support for these SoC series:


* Added support for these ARM boards:

  * nRF21540 Devkit (nrf21540dk_nrf52840).


* Made these changes in other boards


* Added support for these following shields:


Drivers and Sensors
*******************

* ADC


* Audio


* Bluetooth

  * L2CAP RX MTU is now controlled by CONFIG_BT_L2CAP_RX_MTU when
    CONFIG_BT_ACL_FLOW_CONTROL is disabled, previously this was controlled
    by CONFIG_BT_RX_BUF_LEN. If CONFIG_BT_RX_BUF_LEN has been changed from its
    default value then CONFIG_BT_L2CAP_RX_MTU should be set to
    CONFIG_BT_RX_BUF_LEN - 8.

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

  * Added driver supporting the on-chip EEPROM found on NXP LPC11U6X MCUs.
  * Fixed at2x cs gpio flags extraction from DT.

* Entropy


* ESPI


* Ethernet

  * Added VLAN support to Intel e1000 driver.
  * Added Ethernet support to stm32h7 based boards.
  * Added support for setting fixed configuration and read from device tree
    for ENET ETH interface and PHY in mcux driver.
  * Added support for device that do not use SMI for PHY setup in mcux driver.
  * Added support for multiport gPTP in native_posix driver. This allows gPTP
    bridging testing.
  * Fixed MAC registers in enc28j60 driver to the latest Microchip reference manual.

* Flash

  * The driver selected by ``CONFIG_SPI_FLASH_W25QXXDV`` has been
    removed as it is unmaintained and all its functionality is available
    through ``CONFIG_SPI_NOR``.  Out of tree uses should convert to the
    supported driver using the ``jedec,spi-nor`` compatible.


* GPIO


* Hardware Info


* I2C

  * Introduced new driver for NXP LPC11U6x SoCs.  See
    :option:`CONFIG_I2C_LPC11U6X`.

  * Introduced new driver for emulated I2C devices, where I2C operations
    are forwarded to a module that emulates responses from hardware.
    This enables testing without hardware and allows unusual conditions
    to be synthesized to test driver behavior.  See
    :option:`CONFIG_I2C_EMUL`.


* I2S


* IEEE 802.15.4

  * Allow user to disable auto-start of IEEE 802.15.4 network interface.
    By default the IEEE 802.15.4 network interface is automatically started.
  * Added support for setting TX power in rf2xx driver.
  * Added Nordic 802.15.4 multiprotocol support, see :option:`CONFIG_NRF_802154_MULTIPROTOCOL_SUPPORT`.
  * Added Kconfig :option:`CONFIG_IEEE802154_VENDOR_OUI_ENABLE` option for defining OUI.

* Interrupt Controller


* IPM


* Keyboard Scan


* LED


* LED Strip


* LoRa


* Modem

  * Added option to query the IMSI and ICCID from the SIM.
  * Added support for offloaded Sierra Wireless HL7800 modem.

* PECI


* Pinmux


* PS/2


* PWM


* Sensor

  * Added support for wsen-itds Accel Sensor.


* Serial


* SPI

  * The SPI driver subsystem has been updated to use the flags specified
    in the cs-gpios devicetree properties rather than the
    SPI_CS_ACTIVE_LOW/HIGH configuration options.  Devicetree files that
    specify 0 for this field will probably need to be updated to specify
    GPIO_ACTIVE_LOW.  SPI_CS_ACTIVE_LOW/HIGH are still used for chip
    selects that are not specified by a cs-gpios property.


* Timer


* USB

  * The usb_enable() function, which, for some samples, was invoked
    automatically on system boot up, now needs to be explicitly called
    by the application in order to enable the USB subsystem. If your
    application relies on any of the following Kconfig options, then
    it shall also enable the USB subsystem:

    * :option:`CONFIG_OPENTHREAD_NCP_SPINEL_ON_UART_ACM`
    * :option:`CONFIG_USB_DEVICE_NETWORK_ECM`
    * :option:`CONFIG_USB_DEVICE_NETWORK_EEM`
    * :option:`CONFIG_USB_DEVICE_NETWORK_RNDIS`
    * :option:`CONFIG_TRACING_BACKEND_USB`
    * :option:`CONFIG_USB_UART_CONSOLE`

* Video


* Watchdog


* WiFi

  * Added IPv6 support to Simplelink driver.
  * Added DNS offloading support to eswifi driver.
  * Fixed esp driver offload protocol parsing.
  * Fixed esp driver GPIO reset control logic.
  * Fixed eswifi driver offloading packet parsing.


Networking
**********

* The new TCP stack is enabled by default. The legacy TCP stack is not yet
  removed and can be used if needed.
* The network interface is made a kernel object. This allows better access
  control handling when usermode is enabled.
* The kernel stacks are used in network related threads to save memory when
  usermode is enabled.
* Network statistics collection can be enabled in key points of the network
  stack. This can be used to get information where time is spent in RX or TX.
* The BSD socket sendmsg() can now be used with AF_PACKET type sockets.
* Added support for enabling OpenThread reference device.
* Added support for enabling MQTT persistent sessions.
* Added "net tcp recv" command to net shell to enable TCP RX in manual testing.
* Added ObjLnk resource type support to LWM2M.
* Added userspace support to MQTT publisher, echo-server and echo-client
  sample applications.
* Added support to rejecting received and unsupported PPP options.
* Added support for select() when using socket offloading.
* Added support for IPv6 multicast packet routing.
* Added support to SOCK_DGRAM type sockets for AF_PACKET family.
* Added support for using TLS sockets when using socket offloading.
* Added additonal checks in IPv6 to ensure that multicasts are only passed to the
  upper layer if the originating interface actually joined the destination
  multicast group.
* Allow user to specify TCP port number in HTTP request.
* Allow application to initialize the network config library instead of network
  stack calling initialization at startup. This enables better control of
  network resources but requires application to call net_config_init_app()
  manually.
* Allow using wildcards in CoAP resource path description.
* Allow user to specify used network interface in net-shell ping command.
* Allow user to select a custom mbedtls library in OpenThread.
* Removed dependency to :option:`CONFIG_NET_SOCKETS_POSIX_NAMES` from offloaded
  WiFi device drivers.
* Print more gPTP status information in gptp net shell.
* Fixed the network traffic class statistics collection.
* Fixed WiFi shell when doing a scan.
* Fixed IPv6 routes when nexthop is link local address of the connected peer.
* Fixed IPv6 Router Solicitation message handling.
* Fixed BSD socket lib and set errno to EBADF if socket descriptor is invalid.
* Fixed received DNS packet parsing.
* Fixed DNS resolving by ignoring incoming queries while we are resolving a name.
* Fixed CoAP zero length option parsing.
* Fixed gPTP port numbering to start from 1.
* Fixed gPTP BMCA priority vector calculation.
* Fixed multiple interface bound socket recv() for AF_PACKET sockets.
* Fixed PPP Term-Req and Term-Ack packet length when sending them.
* Fixed PPP ipv6cp and ipcp Configure-Rej handling.
* Fixed PPP option parsing and negotiation handling.
* Fixed PPP ipcp option handling when the protocol goes down.
* Fixed PPP ipv6cp and ipcp network address removal when connection goes down.
* Added support to rejecting received and unsupported PPP options.
* Added initial support for PAP authentication in PPP.
* Fixed a race PPP when ppp_fsm_open() was called in CLOSED state.
* Fixed LWM2M FOTA socket closing.
* Fixed LWM2M block transfer retransmissions.
* Fixed LWM2M opaque data transfer in block mode.
* Fixed LWM2M Security and Server object instance matching.
* Fixed LWM2M updating lifetime on Register Update event.
* Fixed MQTT double CONNACK event notification on server reject.


Bluetooth
*********

* Host:


* BLE split software Controller:

* HCI Driver:

  * bt_hci_evt_is_prio() removed, use bt_hci_evt_get_flags() instead when
    CONFIG_BT_RECV_IS_RX_THREAD is defined and call bt_recv and bt_recv_prio
    when their flag is set, otherwise always call bt_recv().

Build and Infrastructure
************************

* Devicetree

* Support for multiple SOC and ARCH roots.
  The :ref:`SOC_ROOT <application>` and ``ARCH_ROOT`` variables used to specify
  support files for out of tree SoCs and architectures now accept multiple
  paths, separated by semicolons. As a result, the ``SOC_DIR`` Kconfig variable
  is no longer supported.

  Uses like ``source $(SOC_DIR)/<path>`` must be changed to
  ``rsource <relative>/<path>`` or similar.

* BOARD, SOC, DTS, and ARCH roots can now be specified in each module's
  :file:`zephyr/module.yml` file; see :ref:`modules_build_settings`.

Libraries / Subsystems
**********************

* Disk


* Management

  * MCUmgr:

    * Moved mcumgr into its own directory.
    * UDP port switched to using kernel stack.
    * smp: added missing socket close in error path.

  * Added support for Open Supervised Device Protocol (OSDP), see :option:`CONFIG_OSDP`.

  * updatehub:

    * Moved updatehub from lib to subsys/mgmt directory.
    * Fixed out-of-bounds access and add flash_img_init return value check.
    * Fixed getaddrinfo resource leak.


* Settings:

  * If a setting read is attempted from a channel that doesn't support reading return an error rather than faulting.
  * disallow modifying the content of a static subtree name.


* Random


* POSIX subsystem:


* Power management:

* Logging:

  * Fixed immediate logging with multiple backends.
  * Switched logging thread to use kernel stack.
  * Allow users to disable all shell backends at one using :option:`CONFIG_SHELL_LOG_BACKEND`.
  * Added Spinel protocol logging backend.
  * Fixed timestamp calculation when using NEWLIB

* LVGL

  * Library has been updated to the new major release v7.0.2.

  * It is important to note that v7 introduces multiple API changes and new
    configuration settings, so applications developed on v6 or previous versions
    will likely require some porting work. Refer to `LVGL 7 Release notes
    <https://github.com/lvgl/lvgl/releases/tag/v7.0.0>`_ for more information.

  * LVGL Kconfig option names have been aligned with LVGL. All LVGL
    configuration options ``LV_[A-Z0-9_]`` have a matching Zephyr Kconfig
    option named as ``CONFIG_LVGL_[A-Z0-9_]``.

  * LVGL Kconfig constants have been aligned with upstream suggested defaults.
    If your application relies on any of the following Kconfig defaults consider
    checking if the new values are good or they need to be adjusted:

    * :option:`CONFIG_LVGL_HOR_RES_MAX`
    * :option:`CONFIG_LVGL_VER_RES_MAX`
    * :option:`CONFIG_LVGL_DPI`
    * :option:`CONFIG_LVGL_DISP_DEF_REFR_PERIOD`
    * :option:`CONFIG_LVGL_INDEV_DEF_READ_PERIOD`
    * :option:`CONFIG_LVGL_INDEV_DEF_DRAG_THROW`
    * :option:`CONFIG_LVGL_TXT_LINE_BREAK_LONG_LEN`
    * :option:`CONFIG_LVGL_CHART_AXIS_TICK_LABEL_MAX_LEN`

  * Note that ROM usage is significantly higher on v7 for minimal
    configurations. This is in part due to new features such as the new drawing
    system. LVGL maintainers are currently investigating ways for reducing the
    library footprint when some options are not enabled, so you should wait for
    future releases if higher ROM usage is a concern for your application.


* Shell:

  * Switched to use kernel stacks.
  * Fixed select command.
  * Fixed prompting dynamic commands.


* Tracing:
  * Tracing backed API now checks if init function exists prio to calling it.

* Shell:
  * Change behavior when more than ``CONFIG_SHELL_ARGC_MAX`` arguments
  are passed.  Before 2.3 extra arguments were joined to the last  argument.
  In 2.3 extra arguments caused a fault.  Now the shell will report that the
  command cannot be processed.

HALs
****

* HALs are now moved out of the main tree as external modules and reside in
  their own standalone repositories.

Documentation
*************


Tests and Samples
*****************
  * nvs: Do full chip erase when flashing.
  * nrf: onoff_level_lighting_vnd_app: Fixed build with mcumgr.
  * drivers: flash_shell: new commands write_unaligned and write_pattern.
  * bluetooth: hci_spi: Fixed cmd_hdr and acl_hdr usage.
  * Removed zephyr nfc sample.
  * drivers: Fixed uninitialized spi_cfg in spi_fujitsu_fram sample.
  * Updated configuration for extended advertising in Bluetooth hci_uart and hci_rpmsg examples.

Issue Related Items
*******************

These GitHub issues were addressed since the previous 2.3.0 tagged
release:
