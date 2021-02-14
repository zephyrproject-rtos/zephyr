:orphan:

.. _zephyr_2.5:

Zephyr 2.5.0
#############

We are pleased to announce the release of Zephyr RTOS version 2.5.0.

Major enhancements with this release include:

* Introduced support for the SPARC processor architecture and the LEON
  processor implementation.
* Added Thread Local Storage (TLS) support
* Added support for per thread runtime statistics
* Added support for building with LLVM on X86
* Added new synchronization mechanisms using Condition Variables
* Add support for demand paging, initial support on X86.

The following sections provide detailed lists of changes by component.

Security Vulnerability Related
******************************

The following CVEs are addressed by this release:

* CVE-2021-3323: Under embargo until 2021-04-14
* CVE-2021-3321: Under embargo until 2021-04-14
* CVE-2021-3320: Under embargo until 2021-04-14

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

* The ``CONFIG_BT_CTLR_CODED_PHY`` is now disabled by default for builds
  combining both Bluetooth host and controller.

* The :c:func:`coap_packet_append_payload` function will now take a pointer to a
  constant buffer as the ``payload`` argument instead of a pointer to a writable
  buffer.

* The :c:func:`coap_packet_init` function will now take a pointer to a constant
  buffer as the ``token`` argument instead of a pointer to a writable buffer.

* A new :ref:`regulator_api` API has been added to support controlling power
  sources.  Regulators can also be associated with devicetree nodes, allowing
  drivers to ensure the device they access has been powered up.  For simple
  GPIO-only regulators a devicetree property ``supply-gpios`` is defined as a
  standard way to identify the control signal in nodes that support power
  control.

* :c:type:`fs_tile_t` objects must now be initialized by calling
  :c:func:`fs_file_t_init` before their first use.

* :c:type:`fs_dir_t` objects must now be initialized by calling
  :c:func:`fs_dir_t_init` before their first use.

Deprecated in this release
==========================

* Nordic nRF5340 PDK board deprecated and planned to be removed in 2.6.0.
* ARM Musca-A board and SoC support deprecated and planned to be removed in 2.6.0.

* DEVICE_INIT was deprecated in favor of utilizing DEVICE_DEFINE directly.

* DEVICE_AND_API_INIT was deprecated in favor of DEVICE_DT_INST_DEFINE and
  DEVICE_DEFINE.

* Bluetooth

  * Deprecated the :c:func:`bt_set_id_addr` function, use :c:func:`bt_id_create`
    before calling :c:func:`bt_enable` instead. When ``CONFIG_PRIVACY`` is
    enabled a valid IRK has to be supplied by the application for this case.

Removed APIs in this release
============================

* Bluetooth

  * The deprecated BT_LE_SCAN_FILTER_DUPLICATE define has been removed,
    use BT_LE_SCAN_OPT_FILTER_DUPLICATE instead.
  * The deprecated BT_LE_SCAN_FILTER_WHITELIST define has been removed,
    use BT_LE_SCAN_OPT_FILTER_WHITELIST instead.
  * The deprecated bt_le_scan_param::filter_dup argument has been removed,
    use bt_le_scan_param::options instead.
  * The deprecated bt_conn_create_le() function has been removed,
    use bt_conn_le_create() instead.
  * The deprecated bt_conn_create_auto_le() function has been removed,
    use bt_conn_le_create_auto() instead.
  * The deprecated bt_conn_create_slave_le() function has been removed,
    use bt_le_adv_start() instead with bt_le_adv_param::peer set to the remote
    peers address.
  * The deprecated BT_LE_ADV_* macros have been removed,
    use the BT_GAP_ADV_* enums instead.
  * The deprecated bt_conn_security function has been removed,
    use bt_conn_set_security instead.
  * The deprecated BT_SECURITY_* defines NONE, LOW, MEDIUM, HIGH, FIPS have been
    removed, use the L0, L1, L2, L3, L4 defines instead.
  * The deprecated BT_HCI_ERR_AUTHENTICATION_FAIL define has been removed,
    use BT_HCI_ERR_AUTH_FAIL instead.

* Kernel

  * The deprecated k_mem_pool API has been removed entirely (for the
    past release it was backed by a k_heap, but maintained a
    compatible API).  Now all instantiated heaps must be
    sys_heap/k_heaps.  Note that the new-style heap is a general
    purpose allocator and does not make the same promises about block
    alignment/splitting.  Applications with such requirements should
    look at porting their logic, or perhaps at the k_mem_slab utility.

Stable API changes in this release
==================================

Kernel
******

* Added support for per thread runtime statistics
* Added new synchronization mechanisms using Condition Variables
* Thread Local Storage (TLS)

  * Introduced thread local storage support for the following architectures:

    * ARC
    * Arm Cortex-M
    * Arm Cortex-R
    * AArch64
    * RISC-V
    * Sparc
    * x86 and x86_64
    * Xtensa

  * This allows variables declared with ``__thread`` keyword to be allocated
    on a per-thread basis, and every thread has its own copy of these
    variables.
  * Enable via :option:`CONFIG_THREAD_LOCAL_STORAGE`.
  * ``errno`` can be stored inside TLS if :option:`CONFIG_ERRNO_IN_TLS`
    is enabled (together with :option:`CONFIG_ERRNO`). This allow user
    threads to access the value of ``errno`` without making a system call.

* Memory Management

  * Added page frame management for physical memory to keep track of
    the status of each page frame.
  * Added :c:func:`k_mem_map` which allows applications to increase
    the data space available via anonymous memory mappings.
  * Added :c:func:`k_mem_free_get` which returns the amount of
    physical anonymous memory remaining.
  * Paging structure must now be pre-allocated so that there is no need
    to do memory allocations when mapping memory. Because of this,
    :c:func:`arch_mem_map` may no longer fail.

* Demand Paging

  * Introduced the framework for demand paging and infrastructure for
    custom eviction algorithms and implementation of backing stores.
  * Currently the whole kernel is pinned and remaining physical memory
    can be used for paging.

Architectures
*************

* ARC

  * Fixed execution on ARC HS with one interrupt bank and fast interrupts (FIRQ)
    enabled
  * Hardened SMP support
  * Improved mdb west runner to support simulation on SMP nSIM-based
    configurations
  * Improved mdb west runner to support nSIM-based configurations execution
    on real HW (FPGA-based)
  * Added documentation page with Zephyr support status on ARC processor
  * Added coverage support for nSIM-based configurations
  * Switched to upstream OpenOCD for ARC
  * Various minor fixes/improvements for ARC MWDT toolchain infrastructure

* ARM

  * AARCH32

    * Introduced the functionality for chain-loadable Zephyr
      firmware images to force the initialization of internal
      architecture state during early system boot (Cortex-M).
    * Changed the default Floating Point Services mode to
      Shared FP registers mode.
    * Enhanced Cortex-M Shared FP register mode by implementing
      dynamic lazy FP register stacking in threads.
    * Added preliminary support for Cortex-R7 variant.
    * Fixed inline assembly code in Cortex-M system calls.
    * Enhanced and fixed Cortex-M TCS support.
    * Enabled interrupts before switching to main in single-thread
      Cortex-M builds (CONFIG_MULTITHREADING=n).
    * Fixed vector table relocation in non-XIP Cortex-M builds.
    * Fixed exception exit routine for fatal error exceptions in
      Cortex-R.
    * Fixed interrupt nesting in ARMv7-R architecture.


  * AARCH64

    * Fixed registers printing on error and beautified crash dump output
    * Removed CONFIG_SWITCH_TO_EL1 symbol. By default the execution now drops
      to EL1 at boot
    * Deprecated booting from EL2
    * Improved assembly code and errors catching in EL3 and EL1 during the
      start routine
    * Enabled support for EL0 in the page tables
    * Fixed vector table alignment
    * Introduced support to boot Zephyr in NS mode
    * Fixed alignment fault in z_bss_zero
    * Added PSCI driver
    * Added ability to generate image header
    * Improved MMU code and driver

* RISC-V

  * Added support for PMP (Physical Memory Protection).
    Integrate PMP in Zephyr allow to support userspace (with shared
    memory) and stack guard features.

* SPARC

  * Added support for the SPARC architecture, compatible with the SPARC V8
    specification and the SPARC ABI.
  * FPU is supported in both shared and unshared FP register mode.

* x86

  * Enabled soft float support for Zephyr SDK
  * ``CONFIG_X86_MMU_PAGE_POOL_PAGES`` is removed as paging structure
    must now be pre-allocated.
  * Mapping of physical memory has changed:

    * This allows a smaller virtual address space thus requiring a smaller
      paging structure.
    * Only the kernel image is mapped when :option:`CONFIG_ACPI` is not enabled.
    * When :option:`CONFIG_ACPI` is enabled, the previous behavior to map
      all physical memory is retained as platforms with ACPI are usually not
      memory constrained and can accommodate bigger paging structure.

  * Page fault handler has been extended to support demand paging.

Boards & SoC Support
********************

* Added support for these SoC series:

  * Cypress PSoC-63
  * Intel Elkhart Lake

* Made these changes in other SoC series:

* Changes for ARC boards:

  * Added icount support for ARC QEMU boards
  * Added MWDT compiler options for HSDK board
  * Added missing taps into JTAG chain for the dual-core configuration of the
    HSDK board

* Added support for these ARM boards:

  * Cypress CY8CKIT_062_BLE board

* Added support for these x86 boards:

  * Elkhart Lake CRB board
  * ACRN configuration on Elkhart Lake CRB board
  * Slim Bootloader configuration on Elkhart Lake CRB board

* Added support for these SPARC boards:

  * GR716-MINI LEON3FT microcontroller development board
  * Generic LEON3 board configuration for GRLIB FPGA reference designs
  * SPARC QEMU for emulating LEON3 processors and running kernel tests

* Added support for these NXP boards:

  * LPCXpresso55S28
  * MIMXRT1024-EVK

* Added support for these STM32 boards and SoCs :

  * Cortex-M Trace Reference Board V1.2 (SEGGER TRB STM32F407)
  * MikroE Clicker 2 for STM32
  * STM32F103RCT6 Mini
  * ST Nucleo F303K8
  * ST Nucleo F410RB
  * ST Nucleo H723ZG
  * ST Nucleo L011K4
  * ST Nucleo L031K6
  * ST Nucleo L433RC-P
  * ST STM32L562E-DK Discovery
  * STM32F105xx and STM32F103xG SoC variants
  * STM32G070xx SoC variants
  * STM32G474xB/C SoC variants
  * STM32L071xx SoC variants
  * STM32L151xC and STM32L152xC SoC variants

* Made these global changes in STM32 boards and SoC series:

  * Pin control configuration is now done through devicetree and existing
    macros to configure pins in pinmux.c files are tagged as deprecated.
    The new pin settings are provided thanks to .dtsi files distributed in
    hal_stm32 module.
  * Generic LL headers, also distributed in hal_stm32 module, are now available
    to abstract series references in drivers.
  * Hardware stack protection is now default on all boards with enabled MPU
    (SRAM > 64K ), excluding F0/G0/L0 series.
  * West flash STM32CubeProgrammer runner was added as a new option for STM32
    boards flashing (to be installed separately).

* Made these changes in other boards:

  * CY8CKIT_062_WIFI_BT_M0: was renamed to CY8CKIT_062_WIFI_BT.
  * CY8CKIT_062_WIFI_BT_M4: was moved into CY8CKIT_062_WIFI_BT.
  * CY8CKIT_062_WIFI_BT: Now M0+/M4 are at same common board.
  * nRF5340 DK: Selected TF-M as the default Secure Processing Element
    (SPE) when building Zephyr for the non-secure domain.
  * SAM4E_XPRO: Added support to SAM-BA ROM bootloader.
  * SAM4S_XPLAINED: Added support to SAM-BA ROM bootloader.
  * Extended LPCXpresso55S69 to support dual-core.
  * Enhanced MIMXRT1064-EVK to support QSPI flash storage and LittleFS.
  * Updated MIMXRT685-EVK to increase the core clock frequency.
  * Updated NXP i.MX RT, Kinetis, and LPC boards to enable hardware stack
    protection by default.
  * Fixed Segger RTT and SystemView support on NXP i.MX RT boards.
  * Demand paging is turned on by default for ``qemu_x86_tiny``.
  * Updated zefi.py to use cross-compiler while building Zephyr.
  * Enabled code coverage report for ``qemu_x86_64``.
  * Removed support for legacy APIC timer driver.
  * Added common memory linker for x86 SoCs.
  * Enabled configuration to reserve the first megabyte in x86 SoCs.

* Added support for these following shields:

  * Inventek es-WIFI shield
  * Sharp memory display generic shield

Drivers and Sensors
*******************

* ADC

  * Added support for ADC on STM32G0 Series.
  * Introduced the ``adc_sequence_options::user_data`` field.

* CAN

  * We reworked the configuration API.
    A user can now specify the timing manually (define prop segment,
    phase segment1, phase segment2, and prescaler) or use a newly introduced
    algorithm to calculate optimal timing values from a bitrate and sample point.
    The bitrate and sample point can be specified in the devicetree too.
    It is possible to change the timing values at runtime now.

  * We reworked the zcan_frame struct due to undefined behavior.
    The std_id (11-bit) and ext_id (29-bit) are merged to a single id
    field (29-bit). The union of both IDs was removed.

  * We made the CANbus API CAN-FD compatible.
    The zcan_frame data-field can have a size of >8 bytes now.
    A flag was introduced to mark a zcan_frame as CAN-FD frame.
    A flag was introduced that enables a bitrate switch in CAN-FD frames.
    The configuration API supports an additional timing parameter for the CAN-FD
    data-phase.

  * drivers are converted to use the new DEVICE_DT_* macros.

* Clock Control

  * Added NXP LPC driver.

* DAC

  * STM32: Enabled support for G0 and H7 series.
  * Added TI DACx3608 driver.

* DMA

  * kmalloc was removed from STM32 DMAMUX driver initialization.

* EEPROM

  * Marked the EEPROM API as stable.
  * Added support for AT24Cxx devices.

* Ethernet

  * Added support for Distributed Switch Architecture (DSA) devices.
    Currently only ip_k66f board supports DSA.
  * Added support for w5500 Ethernet controller.
  * Reworked the NXP MCUX driver to use DT_INST_FOREACH.

* Flash

  * CONFIG_NORDIC_QSPI_NOR_QE_BIT has been removed.  The
    quad-enable-requirements devicetree property should be used instead.
  * MPU_ALLOW_FLASH_WRITE is now default on STM32 boards when MPU is enabled.
  * Add driver for STM32H7 and STM32L1 SoC series.
  * Add QSPI NOR Flash controller support for STM32 family.
  * Added NXP LPC legacy flash driver.
  * Added NXP FlexSPI flash driver for i.MX RT SoCs.
  * Added support for nRF53 Series SoCs in the nRF QSPI NOR flash driver
    (nrf_qspi_nor).

* GPIO

  * Added Cypress PSoC-6 driver.
  * Added Atmel SAM4L driver.

* Hardware Info

  * Added Cypress PSoC-6 driver.

* I2C

  * Added driver support for lmx6x, it8xxx2, and npcx7 plaforms.
  * Added Atmel SAM4L TWIM driver.
  * Added I2C slave support in the microchip i2c driver.
  * Reversed 2.4 decision to downgrade I2C eeprom slave driver to a
    test.  It's a driver again.

* I2S

* IEEE 802.15.4

  * nRF:

    * Added IEEE 802.15.4 support for nRF5340.
    * Added support for failed rx notification.

  * cc13xx/cc26xx:

    * Added multi-protocol radio support.
    * Added sub-ghz support.
    * Added raw mode support.

* Interrupt Controller

  * Added Cypress PSoC-6 Cortex-M0+ interrupt multiplexer driver.

* memc

  * Added FMC/SDRAM memory controller for STM32 family

* Modem

  * Improved RX with HW flow control in modem interface API.
  * Improved reading from interface in command handler.
  * Fixed race condition when waiting on cmd reply.
  * Added support for Quectel bg95 modem.
  * Constified modem command structures to reduce RAM usage.

  * hl7800:

    * Fixed buffer handling issues.
    * Fixed setting DNS address.
    * Fixed file open in fw update.
    * Fixed cases where socket would not close.

  * sara-r4:

    * Added sanity timeout for @ prompt.
    * Fixed redundant wait after sendto.
    * Improved offload_sendmsg() support.
    * Added Kconfig to configure RSSI work.
    * Added direct CMD to catch @ when sending data.
    * Sanitize send_socket_data() semaphore handling.

  * bg96:

    * Fixed UDP packet management.

  * GSM:

    * Added start/stop API support so that application can turn off
      the GSM/PPP modem if needed to save power.
    * Avoid wrapping each byte in muxing headers in PPP.
    * Added support to remove PPP IPv4 ipcp address on network down.

* PECI

* Pinmux

  * STM32 pinmux driver has been reworked to allow pin configuration using
    devicetree definitions. The previous C macros are now deprecated.

* PWM

  * Added support for generating PWM signal based on RTC in the pwm_nrf5_sw
    driver.
  * Added optional API for capturing the PWM pulse width and period.
  * Added PWM capture driver for the NXP Kinetis Pulse Width Timer (PWT).
  * Removed the DesignWare and PCA9685 controller drivers.

* Sensor

  * Fixed current conversion to milliamps in the MAX17055 driver.
  * Added multi-instance support to the FXOS8700, IIS2DLPC, and IIS2ICLX
    drivers.
  * Added Invensense ICM42605 driver.
  * Added NXP MCUX ACMP driver.
  * Fixed gyro units in the FXAS21002 driver.
  * Fixed pressure and temperature registers in the DPS310 driver.
  * Added I2C support to the BMI160 driver.
  * Added IIS2ICLX driver.
  * Aligned ST sensor drivers to stmemsc HAL i/f v1.03.
  * Fixed temperature units in the IIS2MDC driver.
  * Added emulator for Bosch BMI160 accelerometer.
  * Added device power management support to the LIS2MDL driver.

* Serial

  * Added ASYNC API support on STM32 family.

* SPI

  * Enhanced NXP MCUX Flexcomm driver to support DMA.

* Timer

* USB

  * Reworked nrfx driver to use mem_slab for event elements and
    and static memory for OUT endpoints.
  * Fixed ZLP handling for nrfx driver.
  * Added support for USB Device mode on STM32F105xx parts.

* Video

* Watchdog

  * Added NXP i.MX RT driver.

* WiFi

  * eswifi:

    * Added uart bus interface. This enables all Inventek modules with
      IWIN AT Commands firmware.

  * esp:

    * Fixed thread-safety access on esp_socket operations.
    * Fixed scheduling each RX packet on separate work thread.
    * Fixed initializing socket work structures only once.
    * Reworked +IPD and +CIPRECVDATA handling.
    * Stopped locking scheduler when sending data.
    * Added DHCP/Static IP Support.
    * Added support using DNS servers.
    * Enhanced CWMODE support.
    * Added support for configuring hostname.
    * Added support for power-gpios to enable ESP module.
    * Added support 32-bit length in +IPD.
    * Added support for reconfiguring UART baudrate after initial communication.
    * Improved packet allocation failure handling by closing stream sockets.

Networking
**********

* CoAP:

  * Fixed discovery response formatting according to RFC6690.
  * Randomized initial ACK timeout.
  * Reworked pending retransmission logic.
  * Fixed long options encoding.

* DHCPv4:

  * Added start/bound/stop network management events for DHCPv4.
  * Fixed timeout scheduling with multiple network interfaces.
  * Fixed timeout on entry to bound state.
  * Fixed invalid timeout on send failure.
  * Fixed bounds checking in timeout.
  * Fixed endian issue.
  * Added randomization to message interval.
  * Limited message interval to a maximum of 64 seconds.

* DNS:

  * Added resolving literal IP addresses even when DNS is disabled.
  * Added support for DNS Service Discovery (dns-sd).
  * Fixed getaddrinfo() to respect socket type hints.

* HTTP:

  * Added chunked encoding body support to HTTP client API.

* IPv6:

  * Tweaked IPv6 DAD and RS timeout handling.
  * Fixed multiple endian issues.
  * Fixed unaligned access to IPv6 address.

* LwM2M:

  * Added dimension discovery support.
  * Implemented bootstrap discovery.
  * Fixed message find based on pending/reply.
  * Reworked bootstrap DELETE operation.
  * Added path generation macro.
  * Added a way to notify the application on network error.
  * Added a callback to notify socket errors to applications.
  * Send Registration Update on lifetime changes.
  * Fixed PULL FW update in case of URI parse errors.
  * Fixed separate response handling.
  * Start notify sequence numbers on 0.
  * Enhanced packing of TLV integers more efficiently.
  * Improved token generation.
  * Fixed the bootstrap to be optional.

* Misc:

  * Allow user to select pre-emptive or co-operative RX/TX threads.
  * Refactored RX and TX thread priorities.
  * Only start the network logging backend if the autostarting is enabled.
  * Added support for simultaneous UDP/TCP and raw sockets in applications.
  * Enabled solicit node multicast group registration for Bluetooth IPSP
    connections.
  * Added net_buf_remove API to manipulate data at the end of network buffers.
  * Added checks to syslog-net that ensure immediate logging mode is not set as
    the network logging is not compatible with it.
  * Implemented SO_RCVTIMEO socket receive timeout option.
  * Added support to update unique hostname on link address changes.
  * Added locking to IPv6, CAN and packet socket bind calls.
  * Added network management events monitor support.

* MQTT:

  * Reset client state before notifying application with MQTT_EVT_DISCONNECT event.

* OpenThread:

  * Added support for RCP (Radio Co-Processor) mode.
  * Made radio workqueue stack size configurable.
  * Added joining thread multicast addresses which are added to Zephyr.
  * Added SRP Kconfig options.
  * Enabled CSL and TREL config options.
  * Added option to enable software CSMA backoff.
  * Added support to configure platform info.
  * Added Kconfigs to change values in Zephyr.
  * Removed unused defines from platform configuration.

* Samples:

  * Added TagoIO IoT Cloud HTTP post sample.
  * Fixed the return code in MQTT Docker tests.
  * Added support to allow DHCPv4 or manually set addresses in zperf sample.
  * Use IPv4 instead of IPv6 in coap-server to support Docker based testing.
  * Added connection manager support to dumb_http_server_mt sample.
  * Added support for large file in dumb_http_server_mt sample.
  * Added support for running the gptp sample X seconds to support Docker based testing.
  * Added Docker based testing to http_client sample.
  * Refractored code structure and reduced RAM usage of civetweb sample.
  * Added suspend/resume shell commands to gsm_modem sample.
  * Added Docker based testing support to network logging sample.

* TCP:

  * The new TCP stack is enabled by default. Legacy TCP stack is deprecated but
    still available and scheduled for removal in next 2.6 release.
  * Added support to queue received out-of-order TCP data.
  * Added connection termination if the TCP handshake is not finalized.
  * Enhanced received TCP RST packet handling.
  * Fixed TCP connection from Windows 10.

* TLS:

  * Use Maximum Fragment Length (MFL) extension by default.
  * Added ALPN extension option to TLS.
  * Fixed TLS context leak on socket allocation failure.

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

USB
***

* USB synchronous transfer

  * Fixed possible deadlock in usb_transfer_sync().
  * Check added to prevent starting new transfer if an other transfer is
    already ongoing on same endpoint.

* USB DFU class

  * Made USB DFU class compatible with the target configuration that does not
    have a secondary image slot.
  * Support to use USB DFU within MCUBoot with single application slot mode.
  * Separate PID for DFU mode added to avoid problems caused by the host OS
    caching the remaining descriptors when switching to DFU mode.
  * Added timer for appDETACH state and revised descriptor handling to
    to meet specification requirements.

* USB HID class

  * Reworked transfer handling after suspend and resume events.

* Samples

  * Reworked disk and FS configuration in MSC sample. MSC sample can be
    built with none or one of two supported file systems, LittleFS or FATFS.
    Disk subsystem can be flash or RAM based.

Build and Infrastructure
************************

* Improved support for additional toolchains:

* Devicetree

  * Support for legacy devicetree macros via
    ``CONFIG_LEGACY_DEVICETREE_MACROS`` was removed. All devicetree-based code
    should be using the new devicetree API introduced in Zephyr 2.3 and
    documented in :ref:`dt-from-c`. Information on flash partitions has moved
    to :ref:`flash_map_api`.
  * It is now possible to resolve at build time the device pointer associated
    with a device that is defined in devicetree, via ``DEVICE_DT_GET``.  See
    :ref:`dt-get-device`.
  * Enhanced support for enumerated property values via new macros:

    - :c:macro:`DT_ENUM_IDX_OR`
    - :c:macro:`DT_ENUM_TOKEN`
    - :c:macro:`DT_ENUM_UPPER_TOKEN`

  * New hardware specific macros:

    - :c:macro:`DT_GPIO_CTLR_BY_IDX`
    - :c:macro:`DT_GPIO_CTLR`
    - :c:macro:`DT_MTD_FROM_FIXED_PARTITION`

  * Miscellaneous new node-related macros:

    - :c:macro:`DT_GPARENT`
    - :c:macro:`DT_INVALID_NODE`
    - :c:macro:`DT_NODE_PATH`
    - :c:macro:`DT_SAME_NODE`

  * Property access macro changes:

    - :c:macro:`DT_PROP_BY_PHANDLE_IDX_OR`: new macro
    - :c:macro:`DT_PROP_HAS_IDX` now expands to a literal 0 or 1, not an
      expression that evaluates to 0 or 1

  * Dependencies between nodes are now exposed via new macros:

    - :c:macro:`DT_DEP_ORD`, :c:macro:`DT_INST_DEP_ORD`
    - :c:macro:`DT_REQUIRES_DEP_ORDS`, :c:macro:`DT_INST_REQUIRES_DEP_ORDS`
    - :c:macro:`DT_SUPPORTS_DEP_ORDS`, :c:macro:`DT_INST_SUPPORTS_DEP_ORDS`

* West

  * Improve bossac runner. It supports now native ROM bootloader for Atmel
    MCUs and extended SAM-BA bootloader like Arduino and Adafruit UF2. The
    devices supported depend on bossac version inside Zephyr SDK or in users
    path. The recommended Zephyr SDK version is 0.12.0 or newer.

Libraries / Subsystems
**********************

* File systems

  * API

    * Added c:func:`fs_file_t_init` function for initialization of
      c:type:`fs_file_t` objects.

    * Added c:func:`fs_dir_t_init` function for initialization of
      c:type:`fs_dir_t` objects.

  * :option:`CONFIG_FS_LITTLEFS_FC_MEM_POOL` has been deprecated and
    should be replaced by :option:`CONFIG_FS_LITTLEFS_FC_HEAP_SIZE`.

* Management

  * MCUmgr

    * Added support for flash devices that have non-0xff erase value.
    * Added optional verification, enabled via
      :option:`CONFIG_IMG_MGMT_REJECT_DIRECT_XIP_MISMATCHED_SLOT`, of an uploaded
      Direct-XIP binary, which will reject any binary that is not able to boot
      from base address of offered upload slot.

  * updatehub

    * Added support to Network Manager and interface overlays at UpdateHub
      sample. Ethernet is the default interface configuration and overlays
      can be used to change default configuration
    * Added WIFI overlay
    * Added MODEM overlay
    * Added IEEE 802.15.4 overlay [experimental]
    * Added BLE IPSP overlay as [experimental]
    * Added OpenThread overlay as [experimental].

* Settings

* Random

* POSIX subsystem

* Power management

  * Use a consistent naming convention using **pm_** namespace.
  * Overhaul power states. New states :c:enum:`pm_state` are more
    meaningful and ACPI alike.
  * Move residency information and supported power states to devicetree
    and remove related Kconfig options.
  * New power state changes notification API :c:struct:`pm_notifier`
  * Cleanup build options.

* LVGL

  * Library has been updated to minor release v7.6.1

* Storage

  * flash_map: Added API to get the value of an erased byte in the flash_area,
    see ``flash_area_erased_val()``.

* DFU

 * boot: Reworked using MCUBoot's bootutil_public library which allow to use
   API implementation already provided by MCUboot codebase and remove
   zephyr's own implementations.

* Crypto

  * mbedTLS updated to 2.16.9

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
  * Fixed verification with protected TLVs.


Trusted-Firmware-M
******************

* Synchronized Trusted-Firmware-M module to the upstream v1.2.0 release.

Documentation
*************

Tests and Samples
*****************

  * A sample was added to demonstrate how to use the ADC driver API.
  * Sanitycheck script was renamed to twister

Issue Related Items
*******************

These GitHub issues were addressed since the previous 2.4.0 tagged
release:

* :github:`32221` - Sporadic kernel panics on stm32g4 flash erase/writes
* :github:`32203` - Cannot set static address when using hci_usb or hci_uart on nRF5340 attached to Linux Host
* :github:`32181` - samples: tests: Tests from samples/boards/nrf/nrfx fail
* :github:`32179` - samples: tests: Tests from samples/subsys/usb/audio fail
* :github:`32112` - intel_adsp_cavs15: a part of testcases run failed with same error
* :github:`31819` - intel_adsp_cavs15: signing not correct thus download firmware failed
* :github:`31675` - [Coverity CID :216790] Division or modulo by zero in tests/drivers/can/timing/src/main.c
* :github:`31607` - Bluetooth: host: bt_conn_auth_cb callbacks are not called when pairing to BLE 4.1 central in BT_SECURITY_L4 mode.
* :github:`28685` - Bluetooth: Characteristic unsubscribe under indication load results in ATT timeout
* :github:`26495` - Make k_poll work with KERNEL_COHERENCE
* :github:`21033` - Read out heap space used and unallocated
* :github:`19655` - Milestones toward generalized representation of timeouts
* :github:`12028` - Enable 16550 UART driver on x86_64
* :github:`32206` - CMSIS-DSP support seems broken on link
* :github:`32194` - Source files missing specification of SPDX-License-Identifier in comments
* :github:`32167` - Bluetooth: controller: conformance testcase failures
* :github:`32153` - Use of deprecated macro's in dma_iproc_pax_v1, and dma_iproc_pax_v2
* :github:`32152` - DEVICE_AND_API_INIT and DEVICE_INIT deprecation marking is not working
* :github:`32151` - Use of deprecated macro's in icm42605
* :github:`32143` - AArch64 idle loop corrupts IRQ state with CONFIG_TRACING
* :github:`32142` - dtc: Unrecognized check name "unique_unit_address_if_enabled"
* :github:`32136` - z_unpend1_no_timeout non-atomic
* :github:`32095` - guiconfig search fails
* :github:`32078` - build error with llvm: samples/subsys/fs/littlefs
* :github:`32070` - How to manage power consumption when working with peripheral_hr sample on NRF52832
* :github:`32067` - Bluetooth: Mesh: Devkey and addr not stored correctly
* :github:`32064` - Minimal libc malloc() is unprotected
* :github:`32059` - Getting Started - Windows - Toolchain not found
* :github:`32048` - doc: power management: Remove references to previous PM states terminology
* :github:`32046` - LMP90xxx ADC driver fails to initialise more than one instance
* :github:`32045` - boards: Inaccurate values for ram/flash in nrf5340dk_nrf5340_cpuapp.yaml
* :github:`32040` - BT_AUDIO_UNICAST selection rejected in nightly tests
* :github:`32033` - Bluetooth mesh : LPN doesn't receive messages from Friend
* :github:`32030` - dma: stm32: remove dump stream info in irq
* :github:`32015` - Thread local storage is broken when adding more thread variables
* :github:`32014` - Is there a sample that uses SAADC (analog to digital converter)?
* :github:`32007` - Wrong clock value at USART1 in STM32F2 dtsi file
* :github:`32005` - stm32: async uart tests fail
* :github:`32002` - Cannot build encrypted images on Zephyr
* :github:`31996` - tests/bluetooth/init/bluetooth.init.test_ctlr_peripheral_iso fails to build on a few platforms
* :github:`31994` - drivers: flash: stm32h7: fix int/long int warnings
* :github:`31989` - nrfx_uarte serial driver does not go to low power mode after setting off state
* :github:`31976` - dma: loop_transfer issue on nucleo_wb55rg
* :github:`31973` - Stm32 uart async driver changes offset after callback
* :github:`31952` - Linking fails with latest master on ARM64 platform
* :github:`31948` - tests: drivers: spi: spi_loopback: became skipped whereas it used to be run
* :github:`31947` - Cleanup devicetree warnings generated by dtc
* :github:`31946` - arm,arm-timer dts compatible should be arm,armv8-timer
* :github:`31944` - flashing not working with openocd runner
* :github:`31938` - Invalid SPDX license identifier used in file
* :github:`31937` - sample.bluetooth.peripheral_hr_rv32m1_vega_ri5cy does not build
* :github:`31930` - uart_nrfx_uarte: `CONFIG_UART_ASYNC_API` with `CONFIG_PM_DEVICE` breaks
* :github:`31928` - usb loopback not work on nrf52840
* :github:`31924` - IVSHMEM with ACRN not working
* :github:`31921` - west flash not working with pyocd
* :github:`31920` - BME280: Use of deprecated `CONFIG_DEVICE_POWER_MANAGEMENT`
* :github:`31911` - Bluetooth: Mesh: Network buffer overflow on too long proxy messages
* :github:`31907` - settings: Unhandled error in NVS backend
* :github:`31905` - Question : Friend & Low power node with nRF52840
* :github:`31876` - west signing seems to be broken on windows
* :github:`31867` - samples/scheduler/metairq_dispatc failed on iotdk boards
* :github:`31858` - xtensa crt1.S hard coding
* :github:`31853` - Devicetree API - Getting GPIO details from pin
* :github:`31847` - BT ISO channel. error value set, but not returned.
* :github:`31836` - Correct values of _msg_len arg in BT_MESH_MODEL_PUB_DEFINE macro
* :github:`31835` - Type conflict (uint32_t) vs. (uint32_t:7) leads to overflow (276 vs. 20)
* :github:`31822` - tests: drivers: timer: Test drivers.timer.nrf_rtc_timer.stress fails on nrf52 platforms
* :github:`31817` - mec15xxevb_assy6853: tests/boards/mec15xxevb_assy6853/i2c_api/ failed
* :github:`31807` - USB DFU Broken for STM32L4
* :github:`31800` - west build; west build --board=qemu_x86   fails with "unknown BOARD"
* :github:`31797` - need 2.5 release notes on switch to k_heap from mem_pool
* :github:`31791` - samples: hello-world: extra slash in path
* :github:`31789` - samples/scheduler/metairq_dispatch: Regression after 30916 (sched: timeout: Do not miss slice timeouts)
* :github:`31782` - adc: test and sample failed on STM32
* :github:`31778` - Calling k_sem_give causes MPU Fault on nRF52833
* :github:`31769` - Twister:  AttributeError: 'NoneType' object has no attribute 'serial_pty'
* :github:`31767` - twister: rename variable p
* :github:`31749` - fs: fs_opendir can corrupt fs_dir_t object given via zdp parameter
* :github:`31741` - tests:subsys_canbus_isotp: mimxrt1060 meet recv timeout
* :github:`31735` - intel_adsp_cavs15: use twister to run kernel testcases has no output
* :github:`31733` - Unable to build socket can with frdm_k64f
* :github:`31729` - test: build fatal related testcase failed on qemu_cortex_m0 and run failed on qemu_nios2
* :github:`31727` - system_off fails to go into soft_off (deep sleep) state on cc1352r1_launchxl
* :github:`31726` - RISC-V MIV SoC clock rate is specified 100x too slow
* :github:`31721` - tests: nrf: posix: portability.posix.common.tls.newlib fails on nrf9160dk_nrf9160
* :github:`31704` - tests/bluetooth/init/bluetooth.init.test_ctlr_tiny Fails to build on nrf52dk_nrf52832
* :github:`31696` - UP² Celeron version (not the Atom one) has no console
* :github:`31693` - Bluetooth: controller: Compilation error when Encryption support is disabled
* :github:`31684` - intel_adsp_cavs15: Cannot download firmware of kernel testcases
* :github:`31681` - [Coverity CID :216796] Uninitialized scalar variable in tests/subsys/power/power_mgmt/src/main.c
* :github:`31680` - [Coverity CID :216795] Unchecked return value in tests/kernel/msgq/msgq_api/src/test_msgq_contexts.c
* :github:`31679` - [Coverity CID :216794] Pointless string comparison in tests/lib/devicetree/api/src/main.c
* :github:`31678` - [Coverity CID :216793] Division or modulo by zero in tests/ztest/error_hook/src/main.c
* :github:`31677` - [Coverity CID :216792] Out-of-bounds access in tests/net/lib/dns_addremove/src/main.c
* :github:`31676` - [Coverity CID :216791] Side effect in assertion in tests/lib/p4workq/src/main.c
* :github:`31674` - [Coverity CID :216788] Explicit null dereferenced in tests/ztest/error_hook/src/main.c
* :github:`31673` - [Coverity CID :216787] Wrong sizeof argument in tests/kernel/mem_heap/mheap_api_concept/src/test_mheap_api.c
* :github:`31672` - [Coverity CID :216786] Side effect in assertion in tests/kernel/threads/thread_apis/src/test_threads_cancel_abort.c
* :github:`31671` - [Coverity CID :216785] Side effect in assertion in tests/lib/p4workq/src/main.c
* :github:`31670` - [Coverity CID :216783] Side effect in assertion in tests/lib/p4workq/src/main.c
* :github:`31669` - [Coverity CID :215715] Unchecked return value in tests/subsys/fs/littlefs/src/testfs_mount_flags.c
* :github:`31668` - [Coverity CID :215714] Unchecked return value in tests/subsys/fs/fs_api/src/test_fs_mount_flags.c
* :github:`31667` - [Coverity CID :215395] Out-of-bounds access in tests/net/lib/dns_sd/src/main.c
* :github:`31666` - [Coverity CID :215394] Out-of-bounds access in tests/net/lib/dns_sd/src/main.c
* :github:`31665` - [Coverity CID :215393] Argument cannot be negative in tests/net/lib/dns_sd/src/main.c
* :github:`31664` - [Coverity CID :215390] Argument cannot be negative in tests/net/lib/dns_sd/src/main.c
* :github:`31663` - [Coverity CID :215389] Out-of-bounds access in tests/net/lib/dns_sd/src/main.c
* :github:`31662` - [Coverity CID :215388] Argument cannot be negative in tests/net/lib/dns_sd/src/main.c
* :github:`31661` - [Coverity CID :215387] Out-of-bounds access in tests/net/lib/dns_sd/src/main.c
* :github:`31660` - [Coverity CID :215385] Out-of-bounds access in tests/net/lib/dns_sd/src/main.c
* :github:`31659` - [Coverity CID :215384] Out-of-bounds access in tests/net/lib/dns_sd/src/main.c
* :github:`31658` - [Coverity CID :215383] Argument cannot be negative in tests/net/lib/dns_sd/src/main.c
* :github:`31657` - [Coverity CID :215382] Operands don't affect result in tests/net/lib/dns_sd/src/main.c
* :github:`31656` - [Coverity CID :215380] Out-of-bounds access in tests/net/lib/dns_sd/src/main.c
* :github:`31655` - [Coverity CID :215378] Argument cannot be negative in tests/net/lib/dns_sd/src/main.c
* :github:`31654` - [Coverity CID :215377] Out-of-bounds access in tests/net/lib/dns_sd/src/main.c
* :github:`31653` - [Coverity CID :215375] Out-of-bounds access in tests/net/lib/dns_sd/src/main.c
* :github:`31652` - [Coverity CID :215374] Out-of-bounds access in tests/net/lib/dns_sd/src/main.c
* :github:`31651` - [Coverity CID :215371] Out-of-bounds access in tests/net/lib/dns_sd/src/main.c
* :github:`31650` - [Coverity CID :215370] Argument cannot be negative in tests/net/lib/dns_sd/src/main.c
* :github:`31649` - [Coverity CID :215369] Out-of-bounds access in tests/net/lib/dns_sd/src/main.c
* :github:`31648` - [Coverity CID :216800] Operands don't affect result in lib/os/heap.c
* :github:`31647` - [Coverity CID :216789] Wrong sizeof argument in include/kernel.h
* :github:`31646` - [Coverity CID :215712] Assignment of overlapping memory in lib/os/cbprintf_complete.c
* :github:`31645` - [Coverity CID :215711] Wrong sizeof argument in include/kernel.h
* :github:`31644` - [Coverity CID :216798] Unused value in subsys/net/lib/sockets/socketpair.c
* :github:`31643` - [Coverity CID :215372] Logically dead code in subsys/net/lib/sockets/sockets_tls.c
* :github:`31642` - [Coverity CID :216784] Uninitialized scalar variable in drivers/can/can_common.c
* :github:`31640` - mcuboot build is broken
* :github:`31631` - x86: ehl_crb_sbl: Booting fails with Slim Bootloader
* :github:`31630` - Incorrect configuration override option for west flash
* :github:`31629` - mcumgr-cli image upload is failing on shell channel after MCUBOOT_BOOTUTIL library was introduced
* :github:`31627` - tests/subsys/power/power_mgmt/subsys.power.device_pm  fails to build on nrf5340dk_nrf5340_cpunet &  nrf5340pdk_nrf5340_cpunet
* :github:`31616` - test: ipc: Test from samples/subsys/ipc/rpmsg_service fails on nrf5340dk_nrf5340_cpuapp
* :github:`31614` - drivers: clock_control: Kconfig.stm32xxx PLL div range for each serie
* :github:`31613` - Undefined reference errors when using External Library with k_msgq_* calls
* :github:`31609` - CoAP discovery response does not follow CoRE link format specification
* :github:`31599` - 64 bit race on timer counter in cavs_timer
* :github:`31584` - Twister: json reports generation takes too much time
* :github:`31582` - STM32F746ZG: No pwm signal output when running /tests/drivers/pwm/pwm_api
* :github:`31579` - sam_e70_xplained: running tests/subsys/logging/log_core failed
* :github:`31573` - Wrong log settings in can_stm32 driver
* :github:`31569` - lora: sx126x: interrupt pin permanently enabled
* :github:`31567` - lora: SX126x  modems consume excess power until used for first time
* :github:`31566` - up_squared: Couldn't get testcase log from console for all testcases.
* :github:`31562` - unexpected sign-extension in Kconfig linker symbols on 64-bit platforms
* :github:`31560` - Fix incorrect usage of default in dts bindings
* :github:`31555` - tests:drivers_can_api: mimxrt1060 can api test meet assert failure
* :github:`31551` - lorawan: setting datarate does not allow sending larger packets
* :github:`31549` - tests/kernel/lifo/lifo_usage/kernel.lifo.usage fails on m2gl025_miv
* :github:`31546` - DTS device dependency is shifting memory addresses between builds
* :github:`31543` - Documentation: Spelling
* :github:`31531` - STM32 can driver don't set prescaler
* :github:`31528` - introduction of demand paging support causing qemu failures on x86_64, qemu_x86_64_nokpti
* :github:`31524` - littlefs: Too small heap for file cache.
* :github:`31517` - UP² broken (git bisect findings inside)
* :github:`31511` - AArch32 exception exit routine behaves incorrectly on fatal exceptions
* :github:`31510` - Some drivers return invalid z_timer_cycle_get_32() value
* :github:`31508` - up_squared:  tests/kernel/sched/deadline/ failed.
* :github:`31505` - qemu_cortex_m0: Cmake build failure
* :github:`31504` - qemu_cortex_m0: Cmake build failure
* :github:`31502` - it8xxx2_evb should not define TICKLESS_CAPABLE
* :github:`31488` - build failure w/twister and SDK 0.12.1 related to
* :github:`31486` - make htmldocs-fast not working in development workspace
* :github:`31485` - west flash --runner=jlink  should raise error when CONFIG_BUILD_OUTPUT_BIN=n
* :github:`31472` - tests: kernel: poll: timeout with FPU enabled
* :github:`31467` - samples: bluetooth: peripheral_hids: Pairing fails on the nucleo_wb55rg board.
* :github:`31444` - Error in include/net/socket_select.h
* :github:`31439` - nrf5340dk_nrf5340_cpunet configuring incomplete
* :github:`31436` - compliance script broken
* :github:`31433` - samples/bluetooth/hci_pwr_ctrl stack overflow on nRF52DK_nRF52832
* :github:`31419` - tests/ztest/error_hook failed on ARC boards
* :github:`31414` - samples/net/mqtt_publisher link error: undefined reference to ``z_impl_sys_rand32_get``
* :github:`31400` - Extending ``zephyr,code-partition`` with ``zephyr,code-header-size``
* :github:`31386` - sam_e70b_xplained: running tests/drivers/watchdog/wdt_basic_api/ timeout for v1.14-branch
* :github:`31385` - ARC version of sys_read32 only reads uint16_t on Zephyr v2.4
* :github:`31379` - Update CAN-API Documentation
* :github:`31370` - Question about serial communication using virtual COM
* :github:`31362` - kconfiglib.py _save_old() may rename /dev/null -- replacing /dev/null with a file
* :github:`31358` -  ``west build`` might destroy your repository, as it is defaulting doing pristine.
* :github:`31344` - iotdk: running tests/ztest/error_hook/ failed
* :github:`31343` -  sam_e70_xplained: running tests/net/socket/af_packet/ failed
* :github:`31342` - sam_e70_xplained: running tests/net/ptp/clock/ failed
* :github:`31340` - sam_e70_xplained: running tests/subsys/logging/log_core/ failed
* :github:`31339` - nsim_em: running tests/ztest/error_hook/ failed
* :github:`31338` - mimxrt1050_evk: running tests/kernel/fpu_sharing/float_disable/ failed
* :github:`31333` - adding a periodic k_timer causes k_msleep to never return in tests/kernel/context
* :github:`31330` - Getting started guide outdated: Step 4 - Install a toolchain
* :github:`31327` - ci compliance failures due to intel_adsp_cavs25 sample
* :github:`31316` - Issue in UDP management for BG96
* :github:`31308` - Cannot set static address when using hci_usb or hci_uart on nRF5340 attached to Linux Host
* :github:`31301` - intel_adsp_cavs15: run kernel testcases failed.
* :github:`31289` - Problems building grub2 bootloader for Zephyr
* :github:`31285` - LOG resulting in incorrect output
* :github:`31282` - Kernel: Poll: Code Suspected Logic Problem
* :github:`31272` - CANOpen Sample compilation fails
* :github:`31262` - tests/kernel/threads/tls/kernel.threads.tls.userspace failing
* :github:`31259` - uart.h: Clarification required on uart_irq_tx_ready uart_irq_rx_ready
* :github:`31258` - watch dog (WWDT) timeout calculation for STM32 handles biggest timeout and rollover wrong
* :github:`31235` - Cortex-M: vector table relocation is incorrect with XIP=n
* :github:`31234` - twister: Add choice for tests sorting into subsets
* :github:`31226` - tests/drivers/dma/loop_transfer does not use ztest
* :github:`31219` - newlib printk float formatting not working
* :github:`31207` - Non-existent event in asynchronous UART API
* :github:`31206` - coap.c : encoding of options with lengths larger than 268 is not proper
* :github:`31203` - fatal error: setjmp.h: No such file or directory
* :github:`31194` - twister: using unsupported fixture without defined harness causes an infinite loop during on-target test execution
* :github:`31168` - Wrong linker option syntax for printf and scanf with float support
* :github:`31158` - Ethernet (ENC424J600) with dumb_http_server_mt demo does not work
* :github:`31153` - twister build of samples/audio/sof/sample.audio.sof fails on most platforms
* :github:`31145` - Litex-vexriscv address misaligned with dumb_http_server example
* :github:`31143` - samples: audio: sof: compilation issue, include file not found.
* :github:`31137` - Seems like the rule ".99 tag to signify major work started, minor+1 started " not used anymore ?
* :github:`31134` - LittleFS: Error Resizing the External QSPI NOR Flash in nRF52840dk
* :github:`31114` - Bluetooth: Which coding (S2 vs S8) is used during advertising on Coded PHY?
* :github:`31100` - Recvfrom not returning -1 if UDP and len is too small for packet.
* :github:`31091` - usb: usb_transfer_sync deadlocks on disconnect/cancel transfer
* :github:`31086` - bluetooth: Resume peripheral's advertising after disconnection when using new bt_le_ext_adv_* API
* :github:`31085` - networking / openthread: ipv6 mesh-local all-nodes multicast (ff03::1) packets are dropped by zephyr ipv6 stack
* :github:`31079` - Receiving extended scans on an Adafruit nRF 52840
* :github:`31071` - board: arm: SiliconLabs: add support to development kit efm32pg_stk3401a
* :github:`31069` - net: buf: remove data from end of buffer
* :github:`31067` - usb: cdc_acm: compilation error without UART
* :github:`31055` - nordic: west flash no longer supports changing ``CONFIG_GPIO_PINRESET`` when flashing
* :github:`31053` - LwM2M FOTA pull not working with modem (offloaded socket) driver using UART
* :github:`31044` - sample.bluetooth.peripheral_hr build fails on rv32m1_vega_ri5cy
* :github:`31110` - How can I overwrite west build in command?
* :github:`31028` - Cannot READ_BIT(RCC->CR, RCC_CR_PLL1RDY) on STM32H743 based board
* :github:`31027` - Google tests run twice
* :github:`31020` - CI build failed on intel_adsp_cavs18 when submitted a PR
* :github:`31019` - Bluetooth: Mesh: Thread competition leads to failure to open or close the scanning.
* :github:`31018` - up_squared: tests/kernel/pipe/pipe_api failed.
* :github:`31014` - Incorrect timing calculation in can_mcux_flexcan
* :github:`31008` - error: initializer element is not constant .attr = K_MEM_PARTITION_P_RX_U_RX
* :github:`30999` - updatehub with openthread build update pkg failed
* :github:`30997` - samples: net: sockets: echo_client: posix tls example
* :github:`30989` - driver : STM32 Ethernet : Pin definition for PH6
* :github:`30979` - up_squared_adsp: Twister can not capture testcases log correctly
* :github:`30972` - USB: SET_ADDRESS logic error
* :github:`30964` - Sleep calls are off on qemu_x86
* :github:`30961` - esp32 broken by devicetree device updates
* :github:`30955` - Bluetooth: userchan: k_sem_take failed with err -11
* :github:`30938` - samples/net/dhcpv4_client does not work with sam_e70_xplained
* :github:`30935` - tests: net: sockets: tcp: add a tls tests
* :github:`30921` - west flash failed with an open ocd error
* :github:`30918` - up_squared:  tests/kernel/mem_protect/mem_protect failed.
* :github:`30893` - Remove LEGACY_TIMEOUT_API
* :github:`30872` - Convert Intel GNA driver to devicetree
* :github:`30871` - "warning: compound assignment with 'volatile'-qualified left operand is deprecated" when building with C++20
* :github:`30870` - Convert Intel DMIC to devicetree
* :github:`30869` - Convert designware PWM driver to devicetree
* :github:`30862` - Nordic system timer driver incompatible with LEGACY_TIMEOUT_API
* :github:`30860` - legacy timeout ticks mishandled
* :github:`30857` - SDRAM not working on STM32H747I-DISCO
* :github:`30850` - iotdk: couldn't flash image into iotdk board using west flash.
* :github:`30846` - devicetree: unspecified phandle-array elements cause errors
* :github:`30822` - designator order for field 'zcan_filter::rtr' does not match declaration order in 'const zcan_filter'
* :github:`30819` - twister: --generate-hardware-map crashes and deletes map
* :github:`30810` - tests: kernel: kernel.threads.armv8m_mpu_stack_guard fails on nrf9160dk
* :github:`30809` - new testcase is failing after 3f134877 on mec1501modular_assy6885
* :github:`30808` - Blueooth: Controller Response COMMAND DISALLOWED
* :github:`30805` - Build error at tests/kernel/queue in mec15xxevb_assy6853(qemu) platform
* :github:`30800` - STM32 usb clock from PLLSAI1
* :github:`30792` - Cannot build network echo_server for nucleo_f767zi
* :github:`30752` - ARC: passed tests marked as failed when running sanitycheck on nsim_* platforms
* :github:`30750` - Convert i2s_cavs to devicetree
* :github:`30736` - Deadlock with usb_transfer_sync()
* :github:`30730` - tests: nrf: Tests in tests/drivers/timer/nrf_rtc_timer are flaky
* :github:`30723` - libc: malloc() returns unaligned pointer, causes CPU exception
* :github:`30713` - doc: "Variable ZEPHYR_TOOLCHAIN_VARIANT is not defined"
* :github:`30712` - "make zephyr_generated_headers" regressed again - ";" separator for Z_CFLAGS instead of spaces
* :github:`30705` - STM32 PWM driver generates signal with wrong frequency on STM32G4
* :github:`30702` - Shell module broken on LiteX/VexRiscv after release zephyr-v2.1.0
* :github:`30698` - OpenThread Kconfigs should more closely follow Zephyr Kconfig recommendations
* :github:`30688` - Using openthread based  lwm2m_client cannot ping the external network address unless reset once
* :github:`30686` - getaddrinfo() does not respect socket type
* :github:`30685` - reel_board: tests/kernel/fatal/exception/ failure
* :github:`30683` - intel_adsp_cavs15:running tests/kernel/sched/schedule_api failed
* :github:`30679` - puncover  worst-case stack analysis does not work
* :github:`30673` - cmake: zephyr_module.cmake included before ZEPHYR_EXTRA_MODULES is evaluated
* :github:`30663` - Support for TI's TMP117 Temperature Sensor.
* :github:`30657` - BT Mesh: Friendship ends if LPN publishes to a VA it is subscribed to
* :github:`30651` - sanitycheck samples/video/capture/sample.video.capture fails to build on mimxrt1064_evk
* :github:`30649` - Trouble with gpio callback on frdm k64f
* :github:`30638` - nrf pwm broken
* :github:`30636` - TCP stack locks irq's for too long
* :github:`30634` - frdm_kw41z: Current master fails compilation in drivers/pwm/pwm_mcux_tpm.c
* :github:`30624` - BLE : ATT Timeout occurred during multilink central connection
* :github:`30591` - build RAM usage printout uses prebuilt and not final binary
* :github:`30582` - Doxygen doesn't catch errors in argument names in callback functions that are @typedef'd
* :github:`30574` - up_squared: tests/kernel/semaphore/semaphore failed.
* :github:`30573` - up_squared: slowdown on test execution and timing out on multiple tests
* :github:`30566` - flashing issue with ST Nucleo board H745ZI-Q
* :github:`30557` - i2c slave driver removed
* :github:`30554` - tests/kernel/fatal/exception/sentinel test is failing for various nrf platforms
* :github:`30553` - kconfig.py exits with error when using multiple shields
* :github:`30548` - reel_board: tests/net/ieee802154/l2/ build failure
* :github:`30547` - reel_board: tests/net/ieee802154/fragment/ build failure
* :github:`30546` - LwM2M Execute arguments currently not supported
* :github:`30541` - l2m2m: writing to resources with pre_write callback fails
* :github:`30531` - When using ccache, compiler identity stored in ToolchainCapabilityDatabase is always the same
* :github:`30526` - tests: drivers: timer: Tests from drivers.timer.nrf_rtc_timer.basic fail on all nrf platforms
* :github:`30517` - Interrupt nesting is broken on ARMv7-R / LR_svc corrupted.
* :github:`30514` - reel_board: tests/benchmarks/sys_kernel/ fails
* :github:`30513` - reel_board: tests/benchmarks/latency_measure/ fails
* :github:`30509` - k_timer_remaining_get returns incorrect value on long timers
* :github:`30507` - nrf52_bsim fails on some tests after merging 29810
* :github:`30488` - Bluetooth: controller: swi.h should use CONFIG_SOC_NRF5340_CPUNET define
* :github:`30486` - updatehub demo for nrf52840dk
* :github:`30483` - Sanitycheck: When platform is nsim_hs_smp, process "west flash"  become defunct, the grandchild "cld" process can't be killed
* :github:`30480` - Bluetooth: Controller: Advertising can only be started 2^16 times
* :github:`30477` - frdm_k64f: testcase  samples/subsys/canbus/canopen/ failed to be ran
* :github:`30476` - frdm_k64f: testcase samples/net/cloud/tagoio_http_post/ failed to be ran
* :github:`30475` - frdm_k64f: testcase tests/kernel/fatal/exception/ failed to be ran
* :github:`30473` - mimxrt1050_evk: testcase tests/kernel/fatal/exception/ failed to be ran
* :github:`30472` - sam_e70_xplained: the samples/net/civetweb/http_server/. waits for interface unitl timeout
* :github:`30470` - sam_e70_xplained: tesecase tests/subsys/log_core failed to run
* :github:`30468` - mesh: cfg_svr.c app_key_del passes an incorrect parameter
* :github:`30467` - replace device define macros with devicetree-based macro
* :github:`30446` - fxas21002 gyroscope reading is in deg/s
* :github:`30435` - NRFX_CLOCK_EVT_HFCLKAUDIO_STARTED not handled in clock_control_nrf.c
* :github:`30434` - Memory map executing test case failed when code coverage enabled in x86_64 platform
* :github:`30433` - zephyr client automatic joiner failed on nRF52840dk
* :github:`30432` - No network interface was found when running socketcan sample
* :github:`30426` - Enforce all checkpatch warnings and move to 100 characters per line
* :github:`30423` - Devicetree: Child node of node on SPI bus itself needs reg property - Bug?
* :github:`30418` - Logging: Using asserts with LOG in high pri ISR context blocks output
* :github:`30408` - tests/kernel/sched/schedule_api is failing after 0875740 on m2gl025_miv
* :github:`30397` - tests:latency_measure is not counting semaphore results on the ARM boards
* :github:`30394` - TLS tests failing with sanitycheck (under load)
* :github:`30393` - kernel.threads.tls.userspace fails with SDK 0.12.0-beta on ARM Cortex-M
* :github:`30386` - Building confirmed images does not work
* :github:`30384` - Scheduler doesn't activate sleeping threads on native_posix
* :github:`30380` - Improve the use of CONFIG_KERNEL_COHERENCE
* :github:`30378` - Bluetooth: controller: tx buffer overflow error
* :github:`30364` - TCP2 does not implement queing for incoming packets
* :github:`30362` - adc_read_async callback parameters are dereferenced pointers, making use of CONTAINER_OF impossible
* :github:`30360` - reproducible qemu_x86_64 SMP failures
* :github:`30356` - DAC header file not included in stm32 soc.h
* :github:`30354` - Regression with 'local-mac-address' enet DTS property parsing (on i.MX K6x)
* :github:`30349` - Memory protection unit fault when running socket CAN program
* :github:`30344` - Bluetooth: host: Add support for multiple advertising sets for legacy advertising
* :github:`30338` - BT Mesh LPN max. poll timeout calculated incorrectly
* :github:`30330` - tests/subsys/usb/bos/usb.bos fails with native_posix and llvm/clang
* :github:`30328` - Openthread build issues with clang/llvm
* :github:`30322` - tests: benchmarks: latency_measure: timing measurement values are all 0
* :github:`30316` - updatehub with openthread
* :github:`30315` - Build failure: zephyr/include/generated/devicetree_unfixed.h:627:29: error: 'DT_N_S_leds_S_led_0_P_gpios_IDX_0_PH_P_label' undeclared
* :github:`30308` - Add optional user data field to device structure
* :github:`30307` - up_squared:  tests/kernel/device/ failed.
* :github:`30306` - up_squared: tests/kernel/mem_protect/userspace failed.
* :github:`30305` - up_squared:  tests/kernel/mem_protect/mem_protect failed.
* :github:`30304` - NRF52832 consumption too high 220uA
* :github:`30298` - regression/change in master: formatting floats and doubles
* :github:`30276` - Sanitycheck: can't find mdb.pid
* :github:`30275` - up_squared: tests/kernel/common failed (timeout error)
* :github:`30261` - File no longer at this location
* :github:`30257` - test: kernel: Test kernel.common.stack_protection_arm_fpu_sharing.fatal fails on nrf52 platforms
* :github:`30253` - tests: kernel: Test kernel.memory_protection.gap_filling fails on nrf5340dk_nrf5340_cpuapp
* :github:`30372` - WEST Support clean build
* :github:`30373` - out of tree （board soc doc subsystem ...)
* :github:`30240` - Bluetooth: Mesh: PTS Test failed in friend node
* :github:`30235` - MbedTLS X509 certificate not parsing
* :github:`30232` - CMake 3.19 doesn't work with Zephyr (tracking issue w/upstream CMake)
* :github:`30230` - printk and power management incompatibility
* :github:`30229` - BinaryHandler has no pid file
* :github:`30224` - stm32f4_disco: User button press is inverted
* :github:`30222` - boards: arm: nucleo_wb55rg: fails to build basic samples
* :github:`30219` - drivers: gpio: gpio_cc13xx_cc26xx: Add drive strength configurability
* :github:`30213` - usb: tests: Test usb.device.usb.device.usb_disable fails on nrf52840dk_nrf52840
* :github:`30211` - spi nor sfdp runtime: nph offset
* :github:`30207` - Mesh_demo with a nRF52840 not working
* :github:`30205` - Missing error check of function i2c_write_read() and dac_write_value()
* :github:`30194` - qemu_x86 crashes when printing floating point.
* :github:`30193` - reel_board: running tests/subsys/power/power_mgmt_soc failed
* :github:`30191` - Missing checks of return values of settings_runtime_set()
* :github:`30189` - Missing error check of function sensor_trigger_set()
* :github:`30187` - usb: stm32: MCU fall in deadlock when calling sleep API during USB transfer
* :github:`30183` - undefined reference to ``ring_buf_item_put``
* :github:`30179` - out of tree （board soc doc subsystem ...）
* :github:`30178` - Is there any plan to support NXP RT600 HIFI4 DSP in the zephyr project?
* :github:`30173` - OpenThread SED cannot join the network after "Update nRF5 ieee802154 driver to v1.9"
* :github:`30157` - SW based BLE Link Layer Random Advertise delay not as expected
* :github:`30153` - BSD recv() can not received huge package(may be 100kB) sustain .
* :github:`30148` - STM32G474: Write to flash Bank 2 address 0x08040000 does not work in 256K flash version
* :github:`30141` - qemu_x86 unexpected thread behavior
* :github:`30137` - TCP2: Handling of RST flag from server makes poll() call unable to return indefinitely
* :github:`30135` - LWM2M: Firmware URI writing does not work anymore
* :github:`30134` - tests: drivers: uart: Tests from tests/drivers/uart/uart_mix_fifo_poll fails on nrf platforms
* :github:`30133` - sensor: driver: lis2dh interrupt definitions
* :github:`30130` - nrf_radio_power_set() should use bool
* :github:`30129` - TCP2 send test
* :github:`30126` - xtensa-asm2-util.s hard coding
* :github:`30120` - sanitycheck fails for tests/bluetooth/init/bluetooth.init.test_ctlr_per_sync
* :github:`30117` - Cannot compile Zephyr project with standard macros INT8_C, UINT8_C, UINT16_C
* :github:`30106` - Refactor zcan_frame.
* :github:`30100` - twister test case selection numbers don't make any sense
* :github:`30099` - sanitycheck --build-only gets stuck
* :github:`30098` - > very few are even tested with CONFIG_NO_OPTIMIZATIONS. What is the general consensus about this?
* :github:`30094` - tests: kernel: fpu_sharing: Tests in tests/kernel/fpu_sharing fail on nrf platforms
* :github:`30075` - dfu: mcuboot: fail to build with CONFIG_BOOTLOADER_MCUBOOT=n and CONFIG_IMG_MANAGER=y
* :github:`30072` - tests/net/socket/socketpair appears to mis-use work queue APIs
* :github:`30066` - CI test build with RAM overflow
* :github:`30057` - LLVM built application crash
* :github:`30037` - Documentation: Fix getting started guide for macOS around homebrew install
* :github:`30031` - stm32f4 usb - bulk in endpoint does not work
* :github:`30029` - samples: net: cloud: tagoio_http_post: Undefined initialization levels used.
* :github:`30028` - sam_e70_xplained: MPU fault with CONFIG_NO_OPTIMIZATIONS=y
* :github:`30027` - sanitycheck failures on ``tests/bluetooth/init/bluetooth.init.test_ctlr_peripheral_ext``
* :github:`30022` - The mailbox message.info in the receiver thread is not updated.
* :github:`30014` - STM32F411RE PWM support
* :github:`30010` - util or toolchain: functions for reversing bits
* :github:`29999` - nrf52840 Slave mode is not supported on SPI_0
* :github:`29997` - format specifies type 'unsigned short' but the argument has type 'int' error in network stack
* :github:`29995` - Bluetooth: l2cap: L2CAP/LE/REJ/BI-02-C test failure
* :github:`29994` - High bluetooth ISR latency with CONFIG_BT_MAX_CONN=2
* :github:`29992` - dma tests fail with stm32wb55 and stm32l476  nucleo boards
* :github:`29991` - Watchdog Example not working as expected on a Nordic chip
* :github:`29977` - nrf9160: use 32Mhz HFCLK
* :github:`29969` - sanitycheck fails on tests/benchmarks/latency_measure/benchmark.kernel.latency
* :github:`29968` - sanitycheck fails a number of bluetooth tests on NRF
* :github:`29967` - sanitycheck fails to build samples/bluetooth/peripheral_hr/sample.bluetooth.peripheral_hr_rv32m1_vega_ri5cy
* :github:`29964` - net: lwm2m: Correctly Support Bootstrap-Delete Operation
* :github:`29963` - RFC: dfu/boot/mcuboot: consider usage of boootloader/mcuboot code
* :github:`29961` - Add i2c driver tests for microchip evaluation board
* :github:`29960` - Checkpatch compliance errors do not fail CI
* :github:`29958` - mcuboot hangs when CONFIG_BOOT_SERIAL_DETECT_PORT value not found
* :github:`29957` - BLE Notifications limited to 1 per connection event on Zephyr v2.4.0 Central
* :github:`29954` - intel_adsp_cavs18 fails with heap errors on current Zephyr
* :github:`29953` - Add the sofproject as a module
* :github:`29951` - ieee802154: cc13xx_cc26xx: raw mode support
* :github:`29945` - Missing error check of function sensor_sample_fetch() and sensor_channel_get()
* :github:`29943` - Missing error check of function isotp_send()
* :github:`29937` - XCC Build offsets.c ：FAILED
* :github:`29936` - XCC Build isr_tables.c fail
* :github:`29925` - pinctrl error for disco_l475_iot1 board:
* :github:`29921` - USB DFU with nrf52840dk (PCA10056)
* :github:`29916` - ARC: tests fail on nsim_hs with one register bank
* :github:`29913` - Question : Bluetooth mesh using long range
* :github:`29908` - devicetree: Allow all GPIO flags to be used by devicetree
* :github:`29896` - new documentation build warning
* :github:`29891` - mcumgr image upload (with smp_svr) does not work over serial/shell on the nrf52840dk
* :github:`29884` - x_nucleo_iks01a2 device tree overlay issue with stm32mp157c_dk2 board
* :github:`29883` - drivers: ieee802154: cc13xx_cc26xx: use multi-protocol radio patch
* :github:`29879` - samples/net/gptp compile failed on frdm_k64f board in origin/master (work well in origin/v2.4-branch)
* :github:`29877` - WS2812 SPI LED strip driver produces bad SPI data
* :github:`29869` - Missing error check of function entropy_get_entropy()
* :github:`29868` - Bluetooth: Mesh: DST not checked on send
* :github:`29858` - [v1.14, v2.4] Bluetooth: Mesh: RPL cleared on LPN disconnect
* :github:`29855` - Bluetooth: Mesh: TTL max not checked on send
* :github:`29853` - multiple PRs fail doc checks
* :github:`29842` - 'imgtool' absent in requirements.txt
* :github:`29833` - Test DT_INST_PROP_HAS_IDX() inside the macros for multi instances
* :github:`29831` - flash support for stm32h7 SoC
* :github:`29829` - On-PR CI needs to build a subset of tests for a subset of platforms regardless of the scope of the PR changes
* :github:`29826` - SNTP doesn't work on v2.4.0 on eswifi
* :github:`29822` - Redundant error check of function usb_set_config() in subsys/usb/class/usb_dfu.c
* :github:`29809` - gen_isr_tables.py does not check that the IRQ number is in bounds
* :github:`29805` - SimpleLink does not compile (simplelink_sockets.c)
* :github:`29796` - Zephyr API for writing to flash for STM32G474 doesn't work as expected
* :github:`29793` - Ninja generated error when setting PCAP option in west
* :github:`29791` - spi stm32 dma: spi
* :github:`29790` - The zephyr-app-commands macro does not honor :generator: option
* :github:`29782` - smp_svr: Flashing zephyr.signed.bin does not seem to work on nrf52840dk
* :github:`29780` - nRF SDK hci_usb sample disconnects after 40 seconds with extended connection via coded PHY
* :github:`29776` - Check vector number and pointer to ISR in "_isr_wrapper" routine for aarch64
* :github:`29775` - TCP socket stream
* :github:`29773` - sam_e70_xplained: running samples/net/sockets/civetweb/ failed
* :github:`29772` - sam_e70_xplained:running testcase tests/subsys/logging/log_core failed
* :github:`29771` - samples: net: sockets: tcp: tcp2 server not accepting with ipv6 bsd sockets
* :github:`29769` - mimxrt1050_evk: build error at tests/subsys/usb/device/
* :github:`29762` - nRF53 Network core cannot start LFClk when using empty_app_core
* :github:`29758` - edtlib not reporting proper matching_compat for led nodes (and other children nodes)
* :github:`29740` - OTA using Thread
* :github:`29737` - up_squared: tests/subsys/power/power_mgmt failed.
* :github:`29733` - SAM0 will wake up with interrupted execution after deep sleep
* :github:`29732` - issue with ST Nucleo H743ZI2
* :github:`29730` - drivers/pcie: In Kernel Mode pcie_conf_read crashes when used with newlib
* :github:`29722` - West flash is not able to flash with openocd
* :github:`29721` - drivers/sensor/lsm6dsl: assertion/UB during interrupt handling
* :github:`29720` - samples/display/lvgl/sample.gui.lvgl fails to build on several boards
* :github:`29716` - Dependency between userspace and memory protection features
* :github:`29713` - nRF5340 - duplicate unit-address
* :github:`29711` - Add BSD socket option SO_RCVTIMEO
* :github:`29710` - drivers: usb_dc_mcux_ehci: driver broken, build error at all USB test and samples
* :github:`29707` - xtensa  xt-xcc -Wno-unused-but-set-variable  not work
* :github:`29706` - xtensa xt-xcc inline warning
* :github:`29705` - reel_board: tests/kernel/sched/schedule_api/ fails on multiple boards
* :github:`29704` - [Coverity CID :215255] Dereference before null check in tests/subsys/fs/fs_api/src/test_fs.c
* :github:`29703` - [Coverity CID :215261] Explicit null dereferenced in subsys/emul/emul_bmi160.c
* :github:`29702` - [Coverity CID :215232] Dereference after null check in subsys/emul/emul_bmi160.c
* :github:`29701` - [Coverity CID :215226] Logically dead code in soc/xtensa/intel_adsp/common/bootloader/boot_loader.c
* :github:`29700` - [Coverity CID :215253] Unintentional integer overflow in drivers/timer/stm32_lptim_timer.c
* :github:`29699` - [Coverity CID :215249] Unused value in drivers/modem/ublox-sara-r4.c
* :github:`29698` - [Coverity CID :215248] Dereference after null check in drivers/modem/hl7800.c
* :github:`29697` - [Coverity CID :215243] Unintentional integer overflow in drivers/timer/stm32_lptim_timer.c
* :github:`29696` - [Coverity CID :215241] Buffer not null terminated in drivers/modem/hl7800.c
* :github:`29695` - [Coverity CID :215235] Dereference after null check in drivers/modem/hl7800.c
* :github:`29694` - [Coverity CID :215233] Logically dead code in drivers/modem/hl7800.c
* :github:`29693` - [Coverity CID :215224] Parse warning in drivers/modem/hl7800.c
* :github:`29692` - [Coverity CID :215221] Unchecked return value in drivers/regulator/regulator_fixed.c
* :github:`29690` - NUCLEO-H745ZI-Q + OpenOCD - connect under reset
* :github:`29684` - Can not make multiple BLE IPSP connection to the same host
* :github:`29683` - BLE IPSP sample doesn't work on raspberry pi 4 with nrf52840_mdk board
* :github:`29681` - Add NUCLEO-H723ZG board support
* :github:`29677` - stm32h747i_disco add ethernet support
* :github:`29675` - Remove pinmux dependency on STM32 boards
* :github:`29667` - RTT Tracing is not working using NXP mimxrt1064_evk
* :github:`29657` - enc28j60 on nRF52840 stalls during enc28j60_init_buffers in zephyr 2.4.0
* :github:`29654` - k_heap APIs have no tests
* :github:`29649` - net: context: add net_context api to check if a port is bound
* :github:`29639` - Bluetooth: host: Security procedure failure can terminate GATT client request
* :github:`29637` - 5g is microwave and 4LTE is radio or static?
* :github:`29636` - Bluetooth: Controller: Connection Parameter Update indication timeout
* :github:`29634` - Build error: (Bluetooth: Mesh: split prov.c into two separate modules #28457)
* :github:`29632` - GPIO interrupt support for IO expander
* :github:`29631` - kernel: provide aligned variant of k_heap_alloc
* :github:`29629` - Creating a k_thread as runtime instantiated kernel object using k_malloc causes general protection fault
* :github:`29616` - Lorawan subsystem stack: missing MLE_JOIN parameter set
* :github:`29611` - usb/class/dfu: void wait_for_usb_dfu() terminates before DFU operation is completed
* :github:`29608` - question: create runtime instantiated kernel objects in kernel mode
* :github:`29594` - x86_64: RBX being clobbered in the idle thread
* :github:`29590` - ARM: FPU: using Unshared FP Services mode can still result in corrupted floating point registers
* :github:`29589` - Creating a k_thread and k_sem as runtime instantiated kernel object causes general protection fault
* :github:`29574` - question: about CONFIG_NET_BUF_POOL_USAGE
* :github:`29567` - Using openthread based echo_client and lwm2m_client cannot ping the external network address
* :github:`29549` - doc: Zephyr module feature ``depends`` not documented.
* :github:`29544` - Bluetooth: Mesh: Friend node unable relay message for lpn
* :github:`29541` - CONFIG_THREAD_LOCAL_STORAGE=y build fails with ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb
* :github:`29538` - eswifi recvfrom() not properly implemented on disco_l475_iot1
* :github:`29534` - reel_board:running tests/kernel/workq/work_queue_api/ failed
* :github:`29533` - mec15xxevb_assy6853:running testcase tests/kernel/workq/work_queue_api/ failed.
* :github:`29532` - mec15xxevb_assy6853:running testcase tests/portability/cmsis_rtos_v2/ failed.
* :github:`29530` - display: nrf52840: adafruit_2_8_tft_touch_v2 shield not working with nrf-spim driver
* :github:`29519` - kernel: provide aligned variants for allocators
* :github:`29518` - sleep in qemu to short
* :github:`29499` - x86 thread stack guards persist after thread exit
* :github:`29497` - Warning in CR2
* :github:`29491` - usb: web USB sample fails Chapter9 USB3CV tests.
* :github:`29478` - fs: fs_open can corrupt fs_open_t object given via zfp parameter
* :github:`29468` - usb: ZEPHYR FATAL ERROR when running USB test for Nordic.
* :github:`29467` - nrf_qspi_nor.c Incorrect value used for checking start of RAM address space
* :github:`29446` - pwm: stm32: output signal delayed
* :github:`29444` - Network deadlock
* :github:`29442` - build failure w/sanitycheck for samples/bluetooth/hci_usb_h4/sample.bluetooth.hci_usb_h4
* :github:`29440` - Missing hw-flow-control; in hci_uart overlay files
* :github:`29435` - SDCard via SD/SDIO/MMC interfaces
* :github:`29430` - up_squared_adsp: Sanitycheck can not run test case on Up_Squared_ADSP board
* :github:`29429` - net: dns: enable dns service discovery for mdns service
* :github:`29418` - ieee802154: cc13xx_cc26xx: bug in rf driver library
* :github:`29412` - sanitycheck: skipped tests marked as failed due to the reason SKIPPED (SRAM overflow)
* :github:`29398` - ICMPv6 error sent with incorrect link layer addresses
* :github:`29386` - unexpected behavior when doing syscall with 7 or more arguments
* :github:`29382` - remove memory domain restriction on system RAM for memory partitions on MMU devices
* :github:`29376` - sanitycheck: "TypeError: 'NoneType' object is not iterable"
* :github:`29373` - Some altera DTS bindings have the wrong vendor prefix
* :github:`29368` - STM32: non F1 -pinctrl.dtsi generation files: Limit mode to variants
* :github:`29367` - usb: drivers: add USB support for UP squared
* :github:`29364` - cdc_acm_composite fails USB3CV test for Nordic platform.
* :github:`29363` - shell: inability to print 64-bit integers with newlib support
* :github:`29357` - RFC: API Change: Bluetooth: Update indication callback parameters
* :github:`29347` - Network deadlock because of mutex locking order
* :github:`29346` - west boards doesn't display the arcitecture.
* :github:`29330` - mec15xxevb_assy6853:running samples/boards/mec15xxevb_assy6853/power_management Sleep entry latency is higher than expected
* :github:`29329` - tests: kernel.workqueue.api tests fail on multiple platforms
* :github:`29328` - mec15xxevb_assy6853:running tests/kernel/workq/work_queue_api/ failed
* :github:`29327` - mec15xxevb_assy6853:region ``SRAM`` overflowed during build
* :github:`29319` - up_squared:  tests/kernel/timer/timer_api failed.
* :github:`29317` - mimxrt1015: kernel_threads_sched: application meet size issue
* :github:`29315` - twr_kv58f220m: all application build failure
* :github:`29312` - [RFC] [BOSSA] Improve offset parameter
* :github:`29310` - ble central Repeat read and write to three peripherals error USAGE FAULT
* :github:`29309` - ADC1 doesn't read correctly on STM32F7
* :github:`29308` - GPIO bit banging i2c init before gpio clock init in stm32f401 plantform,cause same gpio can't work.
* :github:`29307` - samples/bluetooth/mesh-demo unable to send vendor button message
* :github:`29300` - K_THREAD_DEFINE() uses const in a wrong way
* :github:`29298` - xlnx_psttc_timer driver has an imprecise z_clock_set_timeout() implementation
* :github:`29287` - spi: SPI_LOCK_ON does not hold the lock for multiple spi_transceive until spi_release
* :github:`29284` - compilation issues for MinnowBoard/ UpSquared on documentation examples
* :github:`29283` - quickfeather not listed in boards
* :github:`29274` - Can't get Coded PHY type(S2 or S8)
* :github:`29272` - nordic qspi: readoc / writeoc selection may not work
* :github:`29263` - tests/kernel/mem_protect/obj_validation fails build on some boards after recent changes
* :github:`29261` - boards: musca_b1: post build actions with TF-M might not be done in right order
* :github:`29259` - sanitycheck: sanitycheck defines test expected to fail as FAILED
* :github:`29258` - net: Unable to establish TCP connections from Windows hosts
* :github:`29257` - Race condition in k_queue_append and k_queue_alloc_append
* :github:`29248` - board: nrf52840_mdk: support for qspi flash missing
* :github:`29244` - k_thread_resume can cause k_sem_take with K_FOREVER to return -EAGAIN and crash
* :github:`29239` - i2c: mcux driver does not prevent simultaneous transactions
* :github:`29235` - Endless build loop after adding pinctrl dtsi
* :github:`29223` - BLE one central connect multiple peripherals
* :github:`29220` - ARC: tickless idle exit code destroy exception status
* :github:`29202` - core kernel depends on minimal libc ``z_prf()``
* :github:`29195` - west fails with custom manifest
* :github:`29194` - Sanitycheck block after passing some test
* :github:`29183` - DHCPv4 retransmission interval gets too large
* :github:`29175` - x86 fails all tests if CONFIG_X86_KPTI is disabled
* :github:`29173` - uart_nrfx_uart fails uart_async_api_test
* :github:`29166` - sanitycheck ``--test-only --device-testing --hardware-map`` shouldn't run tests on all boards from ``--build-only``
* :github:`29165` - shell_print doesn't support anymore %llx when used with newlib
* :github:`29164` - net: accept() doesn't return an immediately usable descriptor
* :github:`29162` - Data Access Violation when LOG_* is called on ISR context
* :github:`29155` - CAN BUS support on Atmel V71
* :github:`29150` - CONFIG_BT_SETTINGS_CCC_LAZY_LOADING never loads CCC
* :github:`29148` - MPU: twr_ke18f: many kernel application fails when allocate dynamic MPU region
* :github:`29146` - canisotp: mimxrt1064_evk: no DT_CHOSEN_ZEPHYR_CAN_PRIMARY_LABEL defined cause tests failure
* :github:`29145` - net: frdmk64f many net related applications meet hardfault, hal driver assert
* :github:`29139` - tests/kernel/fatal/exception failed on nsim_sem_mpu_stack_guard board
* :github:`29120` - STM32: Few issues on on pinctrl generation script
* :github:`29113` - Build failure with OSPD
* :github:`29111` - Atmel SAM V71 UART_0 fail
* :github:`29109` - HAL STM32 Missing ETH pin control configurations in DT files
* :github:`29101` - Bluetooth: assertion fail with basic repeated extended advertisement API
* :github:`29099` - net: dns: dns-sd: support for dns service discovery
* :github:`29098` - ATT timeout worker not canceled by destroy, and may operate on disposed object
* :github:`29095` - zefi.py has incorrect assertions
* :github:`29092` - tests/drivers/uart/uart_async_api fails on nrf52840dk_nrf52840 (and additional platforms)
* :github:`29089` - doc: boards: cc1352r_sensortag: fix minor rst issue
* :github:`29083` - Bluetooth: Host: Inconsistent permission value during discovery procedure
* :github:`29078` - nRF52840 doesn't start legacy advertisment after extended advertisment
* :github:`29074` - #27901 breaks mikroe_* shields overlay
* :github:`29070` - NXP LPC GPIO driver masked set does not use the mask
* :github:`29068` - chosen zephyr,code-partition has no effect on ELF linking start address
* :github:`29066` - kernel: k_sleep doesn't handle relative or absolute timeouts >INT_MAX
* :github:`29062` - samples/bluetooth/peripheral_hr/sample.bluetooth.peripheral_hr_rv32m1_vega_ri5cy fails to build on  rv32m1_vega_ri5cy
* :github:`29059` - HAL: mchp: Missing PCR ids to control PM for certain HW blocks
* :github:`29056` - tests/bluetooth/init/bluetooth.init.test_ctlr_dbg fails to build on nrf51dk_nrf51422
* :github:`29050` - Ugrade lvgl library
* :github:`29048` - Removing pwr-gpio of rt1052 from devicetree will cause build error
* :github:`29047` - Boards: nucleo_stm32g474re does not build
* :github:`29043` - dirvers: eth_stm32_hal: No interrupt is generated on the MII interface.
* :github:`29042` - CONFIG_SHELL_HELP=n fails to compile
* :github:`29034` - error in samples/subsys/usb/cdc_acm
* :github:`29025` - [Coverity CID :214882] Argument cannot be negative in tests/posix/eventfd/src/main.c
* :github:`29024` - [Coverity CID :214878] Argument cannot be negative in tests/posix/eventfd/src/main.c
* :github:`29023` - [Coverity CID :214877] Argument cannot be negative in tests/posix/eventfd/src/main.c
* :github:`29022` - [Coverity CID :214876] Argument cannot be negative in tests/posix/eventfd/src/main.c
* :github:`29021` - [Coverity CID :214874] Argument cannot be negative in tests/posix/eventfd/src/main.c
* :github:`29020` - [Coverity CID :214873] Argument cannot be negative in tests/posix/eventfd/src/main.c
* :github:`29019` - [Coverity CID :214871] Side effect in assertion in tests/kernel/sched/preempt/src/main.c
* :github:`29018` - [Coverity CID :214881] Unchecked return value in subsys/mgmt/ec_host_cmd/ec_host_cmd_handler.c
* :github:`29017` - [Coverity CID :214879] Explicit null dereferenced in subsys/emul/spi/emul_bmi160.c
* :github:`29016` - [Coverity CID :214875] Dereference after null check in subsys/emul/spi/emul_bmi160.c
* :github:`29015` - [Coverity CID :214880] Out-of-bounds access in subsys/net/ip/tcp2.c
* :github:`29014` - [Coverity CID :214872] Bad bit shift operation in drivers/ethernet/eth_w5500.c
* :github:`29008` - BLE Connection fails to establish between two nRF52840-USB Dongles with Zephyr controller
* :github:`29007` - OOT manifest+module discovery/builds fail
* :github:`29003` - memory corruption in pkt_alloc
* :github:`28999` - STM32: Transition to device tree based pinctrl configuration
* :github:`28990` - Docs: Dead links to sample source directories
* :github:`28979` - Automatic reviewer assignment for PR does not seem to work anymore
* :github:`28976` - sanitycheck failing all tests for nsim_em7d_v22
* :github:`28970` - clarify thread life-cycle documentation
* :github:`28956` - API-less devices aren't findable
* :github:`28955` - undesired kernel debug log
* :github:`28953` - winc1500 driver blocks on listen
* :github:`28948` - hci_usb: ACL transfer not restarted after USB Suspend - Resume
* :github:`28942` - ARC: nsim_hs_smp: huge zephyr.hex file generated on build
* :github:`28941` - Civetweb: create separate directory
* :github:`28938` - EFR32BGx Bluetooth Support
* :github:`28935` - support code coverage in unit tests
* :github:`28934` - pinmux: stm32: port remaining pinctrl DT serial definitions for STM32 based boards
* :github:`28933` - mcuboot: Brick when using BOOT_SWAP_USING_MOVE and reset happens during images swap
* :github:`28925` - west failed due to empty value in self.path
* :github:`28921` - MCUboot / smp_svr sample broken in 2.4.0
* :github:`28916` - net_if_down doesn't clear address
* :github:`28912` - Incorrect macro being used to init a sflist
* :github:`28908` - The same buffers are shared by the 2 Ethernet controllers in the eth_mcux driver
* :github:`28898` - lwm2m_client can't start if mcuboot is enabled
* :github:`28897` - SPI does not work for STM32 min dev board
* :github:`28893` - Double-dot in path's may cause problems with gcc under Windows
* :github:`28887` - Bluetooth encryption request overrides ongoing phy update
* :github:`28881` - tests/kernel/mem_protect/sys_sem: qemu_x86_64 intermittent failure
* :github:`28876` - -p doesn't run a pristine build
* :github:`28872` - Support ESP32 as Bluetooth controller
* :github:`28870` - Peripheral initiated connection parameter update is ignored
* :github:`28867` - ARM Cortex-M4: Semaphores could not be used in ISRs with priority 0?
* :github:`28865` - Doc: Generate documentation using dts bindings
* :github:`28854` - `CONFIG_STACK_POINTER_RANDOM` may be undefined
* :github:`28847` - code_relocation sample does not work on windows
* :github:`28844` - Double quote prepended when exporting CMAKE compile option using zephyr_get_compile_options_for_lang()
* :github:`28833` - STM32: SPI DMA Driver - HW CS handling not compatible with spi_nor (Winbond W25Q)
* :github:`28826` - nRF QSPI flash driver broken for GD25Q16
* :github:`28822` - Improve STM32 LL HAL usage
* :github:`28809` - Enable bt_gatt_notify() to overwrite notified value before sending rather than queue values
* :github:`28794` - RFC: API Change: k_work
* :github:`28791` - STM32: Clock recovery system (CRS) support
* :github:`28787` - lwm2m-client sample can't be build with openthread and DTLS
* :github:`28785` - shell: It should be possible to get list of commands without pressing tab
* :github:`28777` - Memory pool issue
* :github:`28775` - Update to TFM v1.2 release
* :github:`28774` - build failure: several bluetooth samples fail to build on nrf51dk_nrf51422
* :github:`28773` - Lower Link Layer code use upper link layer function have " undefined reference to"
* :github:`28758` - ASSERTION FAIL [conn->in_retransmission == 1] with civetweb sample application
* :github:`28745` - Bug in drivers/modem/hl7800.c
* :github:`28739` - Bluetooth: Mesh: onoff_level_lighting_vnd sample fails provisioning
* :github:`28735` - NULL pointer access in zsock_getsockname_ctx with TCP2
* :github:`28723` - Does not respect python virtualenv
* :github:`28722` - Bluetooth: provide ``struct bt_conn`` to ccc_changed callback
* :github:`28714` - Bluetooth: PTS upper tester: GAP/CONN/NCON/BV-02-C Fails because of usage of NRPA
* :github:`28709` - phandle-array doesn't allow array of just phandles
* :github:`28706` - west build -p auto -b nrf52840dk_nrf52840 error: HAS_SEGGER_RTT
* :github:`28701` - ASSERTION FAIL [!radio_is_ready()]
* :github:`28699` - Bluetooth: controller: Speed up disconnect process when slave latency is used
* :github:`28694` - k_wakeup follwed by k_thread_resume call causes system freeze
* :github:`28693` - FCB support for non-0xFF flash erase values
* :github:`28691` - tests: arch: arm: arm_thread_swap: fails with bus fault
* :github:`28688` - Bluetooth: provide ``struct bt_gatt_indicate_params`` to ``bt_gatt_indicate_func_t``
* :github:`28670` - drivers: flash: bluetooth: stm32wb: attempt to erase internal flash before enabling Bluetooth cause fatal error
* :github:`28664` - Decide whether to enable HW_STACK_PROTECTION by default
* :github:`28650` - GCC-10.2 link issue w/g++ on aarch64
* :github:`28629` - tests: kernel: common: and common.misra are failing on nrf52840dk
* :github:`28620` - 6LowPAN ipsp: ping host -> µc failes, ping µc -> host works. after that: ping host -> µc works
* :github:`28613` - cannot set GDB watchpoints on QEMU x86 with icount enabled
* :github:`28590` - up_squared_adsp:running tests/kernel/smp/ failed
* :github:`28589` - up_squared_adsp:running tests/kernel/workq/work_queue/ failed
* :github:`28587` - Data corruption while serving large files via HTTP with TCP2
* :github:`28556` - mec15xxevb_assy6853:running tests/kernel/sched/schedule_api/ failed
* :github:`28547` - up_squared: tests/subsys/debug/coredump failed using twister command.
* :github:`28544` - Null pointer dereference in ll_adv_aux_ad_data_set
* :github:`28533` - soc: ti_simplelink: cc13xx-cc26xx: kconfig for subghz 802.15.4
* :github:`28509` - series-push-hook.sh: Don't parse then-master-to-latest-master commits after rebase to lastest
* :github:`28504` - dfu: dfu libraries might fails to compile on redefined functions while building MCUBoot
* :github:`28502` - USB DFU class: support MCUBoot CONFIG_SINGLE_APPLICATION_SLOT
* :github:`28493` - Sanitycheck on ARC em_starterkit_em7d has many tests timeout
* :github:`28483` - Fix nanosleep(2) for sub-microsecond durations
* :github:`28473` - Mcuboot fails to compile when using single image and usb dfu
* :github:`28469` - Unable to capture adc signal at 8ksps when using nrf52840dk board.
* :github:`28462` - SHIELD not handled correct in CMake when using custom board
* :github:`28461` - ``HCI_CMD_TIMEOUT`` when setting ext adv data in the hci_usb project
* :github:`28456` - TOOLCHAIN_LD_FLAGS setting of -mabi/-march aren't propagated to linker invocation on RISC-V
* :github:`28442` - How handle IRQ_CONNECT const requirement?
* :github:`28406` - Condition variables
* :github:`28383` - bq274xx sample is not working
* :github:`28363` - ssd16xx: off-by-one with non-multiple-of-eight heights
* :github:`28355` - Document limitations of net_buf queuing functions
* :github:`28309` - Sample/subsys/fs/littlefs with board=nucleo_f429zi  don't work
* :github:`28299` - net: lwm2m: Improve token handling
* :github:`28298` - Deep sleep(system off) is not working with LVGL and display driver
* :github:`28296` - test_essential_thread_abort: lpcxpresso55s16_ns test failure
* :github:`28278` - PWM silently fails when changing output frequency on stm32
* :github:`28220` - flash: revise API to remove block restrictions on write operations
* :github:`28177` - gPTP gptp_priority_vector struct field ordering is wrong
* :github:`28176` - [Coverity CID :214217] Out-of-bounds access in tests/kernel/mem_protect/mem_map/src/main.c
* :github:`28175` - [Coverity CID :214214] Uninitialized pointer read in tests/benchmarks/data_structure_perf/rbtree_perf/src/rbtree_perf.c
* :github:`28170` - [Coverity CID :214222] Unrecoverable parse warning in include/ec_host_cmd.h
* :github:`28168` - [Coverity CID :214218] Unused value in subsys/mgmt/osdp/src/osdp.c
* :github:`28159` - [Coverity CID :214216] Logically dead code in drivers/pwm/pwm_stm32.c
* :github:`28155` - sam_e70_xplained:running samples/net/sockets/civetweb/ failed
* :github:`28124` - Linking external lib against POSIX API
* :github:`28117` - CoAP/LWM2M: Clean Packet Retransmission Concept
* :github:`28113` - Embed precise Zephyr version & platform name in sanitycheck output .xml
* :github:`28094` - samples: drivers: spi_flash_at45: Not work for boards without internal flash driver
* :github:`28092` - Make SPI speed of SDHC card configurable
* :github:`28014` - tests: kernel: mem_protect: sys_sem: failed when CONFIG_FPU is activated
* :github:`27999` - Add QSPI tests for microchip mec1521 board, drivers are in zephyr/drivers/spi, to be tested on mec15xxevb_assy6853
* :github:`27997` - Errors in copy paste lengthy script into Shell Console
* :github:`27981` - Low UART utilization for hci_uart
* :github:`27957` - flash signed binaries: key path and version
* :github:`27914` - frdm_k64f async uart api
* :github:`27892` - [v2.1.x] lib: updatehub: Improve download on slow networks and security fix
* :github:`27890` - [v2.2.x] lib: updatehub: Improve download on slow networks and security fix
* :github:`27881` - Zephyr requirements.txt fails to install on Python 3.9rc1
* :github:`27879` - Make i2c_slave callbacks public in the documentation
* :github:`27856` - Support per thread runtime statistics
* :github:`27846` - Weird ADC outliers on nrf52
* :github:`27841` - samples: disk: unable to access sd card
* :github:`27829` - sys_mutex and futex missing documentation
* :github:`27809` - Cannot enable MPU for nucleo_l552ze_q_ns
* :github:`27785` - memory domain arch implementation not correct with respect to SMP on ARC
* :github:`27716` - Bluetooth: Mesh: Devices relay their own messages (even with relay disabled!)
* :github:`27677` - RFC: Get rid of shell UART device selection in Kconfig
* :github:`27672` - [v2.2] BLE Transaction Collision
* :github:`27633` - arm64 SMP
* :github:`27628` - stm32: i2c bus failure when using USB
* :github:`27622` - Power management for modems using PPP
* :github:`27600` - JSON Api refuse to decode null value
* :github:`27596` - logging: backend inconsistency with console and shell
* :github:`27583` - sanitycheck does not fail on SRAM overflow, add option to make it fail on those cases.
* :github:`27574` - mec15xxevb_assy6853:arch.arm.arch.arm.no.multithreading failed to run
* :github:`27573` - coverage: When enable coverage, some testcases need more time
* :github:`27570` - up_squared:logging.add.logging.add.async failed
* :github:`27561` - drivers: gpio: pca9555 : Add GPIO driver enable interrupt support
* :github:`27559` - How to use stm32cubeIDE to build and debug?
* :github:`27525` - Including STM32Cube's USB PD support to Zephyr
* :github:`27510` - [v1.14] Bluetooth: controller: Fix uninit conn context after invalid channel map
* :github:`27506` - driver: peci: mchp: Ping command is failing due to improper tx wait timeout.
* :github:`27490` - arch/common/isr_tables.c compilation fails with CONFIG_NUM_IRQS=0
* :github:`27487` - storage/stream: should use only flash driver public API
* :github:`27468` - BT Host: Periodic Advertisement delayed receive
* :github:`27467` - BT Host: Periodic Advertiser List
* :github:`27466` - BT Host: Periodic Advertisement Sync Transfer (PAST)
* :github:`27457` - Add support for Nordic nrfx PDM driver for Nordic Thingy52
* :github:`27423` - RFC: API change: clock_control: Change parameters of clock_control_async_on
* :github:`27417` - CivetWeb Enable WebSocket
* :github:`27396` - samples/subsys/logging/logger timeout when sanitycheck enable coverage, it needs a filter
* :github:`27369` - spi: stm32: dma: rx transfer error when spi_write called
* :github:`27367` - Sprintf -  error while sending data to SD card
* :github:`27350` - ADC: adc shell failure when mismatch with dts device label
* :github:`27332` - [v2.3.x] lib: updatehub: Improve download on slow networks and security fix
* :github:`27299` - espi: xec: Whenever eSPI host indicates we are entering DnX mode, notification doesn't reach application
* :github:`27279` - CMSIS RTOS v1 Signals Implementation Issue
* :github:`27231` - Sending data to .csv file on SD card - ERROR CS control inhibited (no GPIO device)
* :github:`27195` - Sanitycheck: BinaryHandler can't kill children processes
* :github:`27176` - [v1.14] Restore socket descriptor permission management
* :github:`27174` - TCP Server don't get the right data from the client
* :github:`27146` - [Coverity CID :211510] Unchecked return value in lib/posix/semaphore.c
* :github:`27122` - Implement watchdog driver for mimxrt1050_evk
* :github:`27068` - ESP-IDF Bootloader bootloop
* :github:`27055` - BlueZ with ESP32 boards supported or not?
* :github:`27047` - zefi.py assumes host GCC is x86
* :github:`27031` - Zephyr OpenThread Simulation
* :github:`27020` - civetweb issues
* :github:`27006` - unsynchronized newlib uintptr_t and PRIxPTR definition on xtensa
* :github:`26987` - [Coverity CID :211475] Unintended sign extension in drivers/sensor/wsen_itds/itds.c
* :github:`26975` - Control never returns from stm32_i2c_msg_write(), when SCL is pulled low permanently (hardware fault occurs)
* :github:`26961` - occasional sanitycheck failures in samples/subsys/settings
* :github:`26949` - sanitycheck gets overwhelmed by console output
* :github:`26912` - arm: cortex_r: config_userspace: hang during early power-up
* :github:`26873` - WIFI_ESP: sockets left opened after unexpected reset of ESP
* :github:`26829` - GSM modem example on stm32f103 bluepill
* :github:`26819` - drivers: modem: SARA modem driver leaks sockets
* :github:`26807` - Bluetooth HCI USB sample is not working
* :github:`26799` - Introduce p99
* :github:`26794` - arc: smp: different sanitycheck results of ARC hsdk's 2 cores and 4 cores configuration
* :github:`26732` - Advertise only on single bluetooth channel
* :github:`26722` - uarte_nrfx_poll_out() in NRF UARTE driver does not work with hardware flow control
* :github:`26665` - Implement reset for ARC development boards
* :github:`26656` - SAM0 USB transfer slot leak
* :github:`26637` - How to identify sensor device?
* :github:`26584` - Multicast emission is only possible for ipv4 starting with 224.
* :github:`26533` - Support newlib for Aarch64 architecture
* :github:`26522` - Reported "Supported shields" list includes boards
* :github:`26515` - timers: platforms using cortex_m_systick does not enter indefinite wait on SLOPPY_IDLE
* :github:`26500` - sanitycheck reports failing tests with test_slice_scheduling()
* :github:`26488` - Bluetooth: Connection failure using frdm_kw41z shield
* :github:`26486` - possible SMP race with k_thread_join()
* :github:`26477` - GPIO sim driver
* :github:`26443` - sanitycheck shall generate results and detailed information about tests and environment in json format.
* :github:`26409` - Clearing of previously initialized data during IPv6 interface init causes infinite loop, memory corruption in timer ISR
* :github:`26383` - OpenThread NCP radio-only
* :github:`26372` - qspi driver does not work if multithreading is disabled
* :github:`26330` - tcp: low bulk receive performance due to window handling
* :github:`26315` - ieee802154: cc13xx_cc26xx: sub_ghz support
* :github:`26312` - drivers: ieee802154: cc13xx_cc26xx: adopt hal/ti rf driverlib
* :github:`26275` - USB MSC fails 'Command Set Test' from USB3CV.
* :github:`26272` - Cannot use alternative simulation runner with sanitycheck
* :github:`26227` - icount support for qemu_arc_{em,hs}
* :github:`26225` - x86_64 doesn't seem to be handling spurious interrupts properly
* :github:`26219` - sys/util: add support for mapping lists with per-element terminal symbols
* :github:`26172` - Zephyr Master/Slave not conforming with Core Spec. 5.2 connection policies
* :github:`26163` - qemu_arc_{em,hs} keeps failing in CI on tests/kernel/lifo/lifo_usage
* :github:`26142` - cmake warning: "Policy CMP0077 is not set"
* :github:`26084` - 2.4 Release Checklist
* :github:`26072` - refactor struct device to reduce RAM demands
* :github:`26051` - shell: uart: Allow a change in the shell initalisation to let routing it through USB UART
* :github:`26050` - devicetree: provide access to node ordinal
* :github:`26026` - RFC: drivers: pwm: add functions for capturing pwm pulse width and period
* :github:`25956` - Including header files from modules into app
* :github:`25927` - ARM: Core Stack Improvements/Bug fixes for 2.4 release
* :github:`25918` - qemu_nios2 crash when enabled icount
* :github:`25832` - [test][kernel][lpcxpresso55s69_ns] kernel cases meet ESF could not be retrieved successfully
* :github:`25604` - USB: enable/disable a class driver at runtime
* :github:`25600` - wifi_eswifi: Unable to start TCP/UDP client
* :github:`25592` - Remove checks on board undocumented DT strings
* :github:`25597` - west sign fails to find header size or padding
* :github:`25508` - tests: add additonal power management tests
* :github:`25507` - up_squared:tests/portability/cmsis_rtos_v2 failed.
* :github:`25482` - outdated recommendations for SYS_CLOCK_TICKS_PER_SEC
* :github:`25466` - log build errors are not helpful
* :github:`25434` - nRF5340 Bluetooth peripheral_hr sample high power consumption
* :github:`25413` - soc: ti_simplelink: kconfig: ble: placeholder for hal_ti
* :github:`25409` - Inconsistent naming of Kconfig options related to stack sizes of various Zephyr components
* :github:`25395` - Websocket Server API
* :github:`25379` - Bluetooth mesh example not working
* :github:`25314` - Bluetooth: controller: legacy: Backport conformance test related changes
* :github:`25302` - lwm2m client sample bootstrap server support
* :github:`25182` - Raspberry Pi 4B Support
* :github:`25173` - k_sem_give(struct k_sem \*sem) should report failure when the semaphore is full
* :github:`25164` - Remove ``default:`` functionality from devicetree bindings
* :github:`25015` - Bluetooth Isochronous Channels Support
* :github:`25010` - disco_l475_iot1 don't confirm MCUBoot slot-1 image
* :github:`24803` - ADC driver test hangs on atsame54_xpro
* :github:`24652` - sanitycheck doesn't keep my cores busy
* :github:`24453` - docs: Allow to use the exact current zephyr version number in place of /latest/ in documentation URLs
* :github:`24358` - deprecate and remove old k_mem_pool / sys_mem_pool APIs
* :github:`24338` - UICR & FICR missing from nRF52* devicetree files
* :github:`24211` - [v2.2.x] lib: updatehub: Not working on Zephyr 2.x
* :github:`23917` - Kconfig: problems with using backslash-escapes in "default" value of "string" option
* :github:`23874` - Test case to check registers data
* :github:`23866` - sample hci_usb fails with zephyr 2.2.0 (worked with zephyr 2.1.0)
* :github:`23729` - hifive1_revb fails to work with samples/subsys/console/getline
* :github:`23314` - Bluetooth: controller: Use defines for 625 us and 1250 us
* :github:`23302` - Poor TCP performance
* :github:`23212` - ARC: SMP: Enable use of ARConnect Inter-core Debug Unit
* :github:`23210` - mimxrt1050_evk:tests/net/iface failed with v1.14 branch.
* :github:`23063` - CMSIS v2 osThreadJoin does not work if thread exits with osThreadExit
* :github:`23062` - thread_num / thread_num_dynamic are never decremented in CMSIS v2 thread.c
* :github:`22942` - Missing TX power options for nRF52833
* :github:`22771` - z_x86_check_stack_bounds() doesn't work right for nested IRQs on x86-64
* :github:`22703` - Implement ADC driver for lpcxpresso55s69
* :github:`22411` - AArch64 / Cortex-A port improvements / TODO
* :github:`22333` - drivers: can: Check bus-timing values at compile-time
* :github:`22247` - Discussion: Supporting the Arduino ecosystem
* :github:`22198` - BMA280 Sample Code
* :github:`22185` - Add Thread Local Storage (TLS) support
* :github:`22060` - Build fails with gcc-arm-none-eabi-9-2019-q4-major
* :github:`21991` - memory domain locking may not be entirely correct
* :github:`21495` - x86_64: interrupt stack overflows are not caught
* :github:`21462` - x86_64 exceptions are not safely preemptible, and stack overflows are not caught
* :github:`21273` - devicetree: improved support for enum values
* :github:`21238` - Improve Zephyr HCI VS extension detection
* :github:`21216` - Ztest "1cpu" cases don't retarget interrupts on x86_64
* :github:`21179` - Reduce RAM consumption for civetweb HTTP sample
* :github:`20980` - ESP32 flash error with segment count error using esptool.py from esp-idf
* :github:`20925` - tests/kernel/fp_sharing/float_disable crashes on qemu_x86 with CONFIG_KPTI=n
* :github:`20821` - Do USE\_\(DT\_\)CODE\_PARTITION and FLASH_LOAD_OFFSET/SIZE need to be user-configurable?
* :github:`20792` - sys_pm: fragility in residency policy
* :github:`20775` - sys_pm_ctrl: fragility in managing state control
* :github:`20686` - tests/kernel/fp_sharing/float_disable failed when code coverage is enabled on qemu_x86.
* :github:`20518` - [Coverity CID :205647]Memory - illegal accesses in /tests/arch/x86/info/src/memmap.c
* :github:`20337` - sifive_gpio: gpio_basic_api test fails
* :github:`20262` - dt-binding for timers
* :github:`20118` - Zephyr BLE stack to work on the CC2650 Launchpad
* :github:`19850` - Bluetooth: Mesh: Modular settings handling
* :github:`19530` - Enhance sanitycheck/CI for separating build phase from testing
* :github:`19523` - Can't build fade-led example for any Nordic board
* :github:`19511` - net: TCP: echo server blocks after a packet with zero TCP options
* :github:`19497` - log subsystem APIs need to be clearly namespaced as public or private
* :github:`19467` - Error building samples with PWM
* :github:`19448` - devicetree: support disabled devices
* :github:`19435` - how to change uart tx rx pins in zephyr
* :github:`19380` - Potential bugs re. 'static inline' and static variables
* :github:`19244` - BLE throughput of DFU by Mcumgr is too slow
* :github:`19138` - zassert prints to UART even when RTT is selected
* :github:`19100` - LwM2M sample with DTLS: does not connect
* :github:`19003` - bluetooth : Mesh: adv thread can be replace by other methods.
* :github:`18927` - settings: deprecate base64 encoding
* :github:`18778` - soc: arm: nordic_nrf: Defining DT\_..._DEV_NAME
* :github:`18728` - sam_e70_xplained:tests/subsys/logging/log_core failed.
* :github:`18608` - PCA9685 PWM driver is broken
* :github:`18607` - DesignWare PWM driver is broken
* :github:`18601` - LwM2M client sample: an overlay for OpenThread support
* :github:`18551` - address-of-temporary idiom not allowed in C++
* :github:`18529` - fs/fcb: consider backport of mynewt fixes
* :github:`18276` - irq_connect_dynamic() is not tested on all arches
* :github:`17893` - dynamic threads don't work on x86 in some configurations
* :github:`17787` - openocd unable to flash hello_world to cc26x2r1_launchxl
* :github:`17743` - cross compiling for RISCV32 fails as compiler flags are not supplied by board but must be in target.cmake
* :github:`17645` - VSCode debugging Zephyr application
* :github:`17545` - Licensing and reference to public domain material
* :github:`17300` - [Zephyr v1.14.0] mcumgr: stm32: Strange Build warnings when counter is enabled
* :github:`17023` - userspace: thread indexes are not released when dynamic threads lose all references
* :github:`17014` - reentrancy protection in counter drivers?
* :github:`16766` - net: net_pkt_copy don't work as expected when data was pulled from destination
* :github:`16544` - drivers: spi: spi_mcux_lpspi: inconsistent chip select behaviour
* :github:`16438` - fs/FCB fs/NVS : requires unaligned flash read-out length capabilities,
* :github:`16237` - disco_l475_iot1: samples/bluetooth/ipsp ko since 3151d26
* :github:`16195` - k_uptime_delta(): Defective by design
* :github:`15944` - stm32: New Driver for FMC/FSMC
* :github:`15713` - MCUMGR_LOG build error
* :github:`15570` - Unbonded peripheral gets 'Tx Buffer Overflow' when erasing bond on master
* :github:`15372` - logging can't dump exceptions without losing data with CONFIG_LOG_PRINTK
* :github:`14591` - Infineon Tricore architecture support
* :github:`14571` - TCP: sending lots of data deadlocks with slow peer
* :github:`14300` - Bluetooth connection using central and peripheral samples in nrf52840
* :github:`13955` - stm32: Implement async uart api
* :github:`13591` - tests/blutooth/tester: ASSERTION FAIL due to Recursive spinlock when running bt tester on qemu-cortex-m3
* :github:`13396` - Cannot connect to Galaxy S8 via BLE
* :github:`13244` - How to encrypt advertise packet with zephyr and nrf52832 ？
* :github:`12368` - File descriptors: Compile fails with non-POSIX out-of-tree libc
* :github:`12353` - intel_s1000: SPI Flash Erase command doesn't work when booting from flash
* :github:`12239` - BLE400 / nRF51822 (nRF51) PWM clock too low (servo-motor example)
* :github:`12150` - Power management on nRF52 boards
* :github:`11912` - net: SimpleLink: Create real cross-platform port
* :github:`11770` - Multiprotocol feature for BLE/Thread
* :github:`10904` - Requirements for driver devices generation using device tree
* :github:`10857` - Migrating from CMSIS-Core to DeviceTree results in loss of type information
* :github:`10460` - Bluetooth: settings: No space to store CCC config after successful pairing
* :github:`10158` - Have support for probot with Zephyr
* :github:`10022` - Porting Modbus Library: Need some guidance
* :github:`9944` - sockets: Connect doesn't send new SYN in case the first connection attempt fails
* :github:`9883` - DTS processing generates unit_address_vs_reg warning on entries with 'reg'
* :github:`9875` - Bluetooth: Host LE Extended Advertising
* :github:`9783` - LwM2M: Cannot create object instance without specifying instance number
* :github:`9509` - Unable to upload firmware over serial with mcumgr
* :github:`9406` - Generate DTS 'compatible' based compilation flags
* :github:`9403` - is Support C++ standard library?
* :github:`9087` - driver: CAN interface should be compatible with CAN-FD interface
* :github:`8499` - Device Tree support overhaul
* :github:`8393` - ``CONFIG_MULTITHREADING=n`` builds call ``main()`` with interrupts locked
* :github:`8008` - net: Sockets (and likely net_context's) should not be closed behind application's back
* :github:`7939` - Connections to TI CC254x break after conn_update
* :github:`7404` - arch: arm: MSP initialization during early boot
* :github:`7331` - Precise time sync through BLE mesh.
* :github:`6857` - need to improve filtering and coverage in sanitycheck
* :github:`6818` - Question: Is  Lightweight OpenMP implementation supported in Zephyr? Or any plan?
* :github:`6648` - Trusted Execution Framework: practical use-cases (high-level overview)
* :github:`6199` - STM32: document dts porting rules
* :github:`6040` - Implement flash driver for LPC54114
* :github:`5626` - Building samples failed
* :github:`4420` - net: tcp: RST handling is weak
* :github:`3675` - LE Adv. Ext.: Extended Scan with PHY selection for non-conn non-scan un-directed without aux packets
* :github:`3674` - LE Adv. Ext.: Non-Connectable and Non-Scannable Undirected without auxiliary packet
* :github:`3893` - Enhance k_stack_push() to check k_stack->top to avoid corruption
* :github:`3719` - Multiple consoles support.
* :github:`3441` - IP stack: No TCP send window handling
* :github:`3102` - Make newlib libc the default c library
* :github:`2247` - LE Controller: Change hal/ into a set of drivers
