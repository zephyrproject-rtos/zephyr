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

* GPIO

  * Deprecated the ``GPIO_INT_DEBOUNCE`` flag and the ``GPIO_DS_*`` and
    ``GPIO_VOLTAGE_*`` groups of flags. Controller/SoC specific flags
    should now be used instead.

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

* Utilities

  * :c:macro:`UTIL_LISTIFY` has been deprecated. Use :c:macro:`LISTIFY` instead.

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

  * :c:func:`bt_disable` was added to enable caller to disable bluetooth stack.

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

* ARC

  * Add ARCv3 32 bit (HS5x) support - both GNU and MWDT toolchains, both UP and SMP
  * Workaround debug_select interference with MDB debugger
  * Switch to hs6x mcpu usage (GNU toolchain) for HS6x

* ARM

  * AARCH32

    * Added Cortex-R floating point support

  * AARCH64

    * Add support for GICv3 for the ARMv8 Xen Virtual Machine
    * Fix SMP boot code to take into account multiple cores booting at the same time
    * Add more memory mapping types for device memory
    * Simplify and optimize switching and user mode transition code
    * Add support for CONFIG_IRQ_OFFLOAD_NESTED
    * Fix booting issue with FVP V8R >= 11.16.16
    * Switch to the IRQ stack during ISR execution

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

  * Added support for a new optional :c:member:`bt_hci_driver.close` API which
    closes HCI transport.
  * Implemented :c:member:`bt_hci_driver.close` on stm32wb HCI driver.

Boards & SoC Support
********************

* Added support for these SoC series:

  * Added support for STM32H725/STM32H730/STM32H73B SoC variants

* Removed support for these SoC series:

* Made these changes in other SoC series:

  * Added Atmel SAM UPLL clock support
  * Raspberry Pi Pico: Added HWINFO support
  * Raspberry Pi Pico: Added I2C support
  * Raspberry Pi Pico: Added reset controller support
  * Raspberry Pi Pico: Added USB support

* Changes for ARC boards:

  * Add nsim_hs5x and nsim_hs5x_smp boards with ARCv3 32bit HS5x CPU
  * Add MWDT toolchain support for nsim_hs6x and nsim_hs6x_smp
  * Do memory layout overhaul for nSIM boards. Add the mechanism to switch between
    ICCM/DCCM memory layout and flat memory layout (i.e DDR).
  * Do required platform setup so nsim_hs5x, nsim_hs5x_smp, nsim_hs6x, nsim_hs6x_smp
    can be run on real HW (HAPS FPGA) with minimum additional configuration
  * Enable MWDT toolchain support for hsdk_2cores board
  * Adjust test duration for SMP nSIM boards with timeout_multiplier

* Added support for these ARM boards:

  * b_g474e_dpow1
  * stm32f401_mini

* Added support for these ARM64 boards:

  * NXP i.MX8MP EVK (i.MX8M Plus LPDDR4 EVK board)
  * NXP i.MX8MM EVK (i.MX8M Mini LPDDR4 EVK board)

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

  * Keyestudio CAN-BUS Shield (KS0411)
  * MikroElektronika WIFI and BLE Shield
  * X-NUCLEO-53L0A1 ranging and gesture detection sensor expansion board

Drivers and Sensors
*******************

* ADC

  * Atmel SAM0: Fixed adc voltage reference
  * STM32: Added support for :c:enumerator:`adc_reference.ADC_REF_INTERNAL`.

* CAN

  * Switched from transmitting CAN frames in FIFO/chronological order to transmitting according to
    CAN-ID priority (NXP FlexCAN, ST STM32 bxCAN, Bosch M_CAN, Microchip MCP2515).
  * Added support for ST STM32U5 to the ST STM32 FDCAN driver.
  * Renamed the base Bosch M_CAN devicetree binding compatible from ``bosch,m-can-base`` to
    :dtcompatible:`bosch,m_can-base`.
  * Added CAN controller statistics support (NXP FlexCAN, Renesas R-Car, ST STM32 bxCAN).
  * Added CAN transceiver support.
  * Added generic SocketCAN network interface and removed driver-specific implementations.

* Clock_control

  * STM32: Driver was cleaned up and overhauled for easier maintenance with a deeper integration
    of device tree inputs. Driver now takes into account individual activation of clock sources
    (High/Medium/Low Internal/external speed clocks, PLLs, ...)
  * STM32: Additionally to above change it is now possible for clock consumers to select an alternate
    source clock (Eg: LSE) by adding it to its 'clocks' property and then configure it using new
    clock_control_configure() API.
    See :dtcompatible:`st,stm32-rcc`, :dtcompatible:`st,stm32h7-rcc` and :dtcompatible:`st,stm32u5-rcc`
    for more information.

* Counter

  * Added driver for NXP QTMR.

* DAC

  *  support for ST STM32F1 to the ST STM32 DAC driver.

* Disk

  * Added generic SDMMC disk driver, that uses the SD subsystem to interact with
    disk devices. This disk driver will be used with any disk device declared
    with the :dtcompatible:`zephyr,sdmmc-disk` compatible string

* Display

  * STM32: Added basic support for LTDC driver. Currently supported on F4, F7, H7, L4+
    and MP1 series.

* DMA

  * Adds a scatter gather test for DMAs that support it
  * Cleanly share Synopsis DW-DMA driver and Intel cAVS GPDMA driver code.
  * Adds support for Synposis DW-DMA transfer lists.
  * Adds support for Intel HDA for audio device and host streams.
  * Fixes for NXP eDMA to pass scatter gather tests

* Entropy

  * STM32: Prevent  core to enter stop modes during entropy operations.

* Ethernet

  * eth_native_posix: Added support for setting MAC address.
  * eth_stm32_hal: Fixed a bug, which caused segfault in case of failed RX
    buffer allocation.
  * eth_mcux: Added support for resetting PHY.
  * eth_liteeth: Refactored driver to use LiteX HAL.
  * eth_w5500: Fixed possible deadlock due to incorrect IRQ processing.

* Flash

  * Added STM32 OCTOSPI driver: For now supports L5 and U5 series. Interrupt driven mode.
    Supports 1 and 8 lines in Single or Dual Transfer Modes.
  * STM32L5: Added support for Single Bank.
  * STM32 QSPI driver was extended with with QER (SFDP, DTS), custom quad write opcode
    and 1-1-4 read mode
  * Added support for STM32U5 series.

* GPIO

  * Refactored GPIO flags. Upper 8 bits of ``gpio_dt_flags_t`` is now reserved
    for controller/SoC specific flags and certain hardware-specific flags
    defined as common so far (IO voltage level, drive strength, debounce filter)
    were replaced with ones defined in this controller/SoC specific space.
  * Added Xilinx PS MIO/EMIO GPIO controller driver.
  * Extended the NXP PCA95XX driver to support also PCAL95XX.

* HWINFO

  * Atmel SAM: Added RSTC support
  * Raspberry Pi Pico: Added Unique ID and reset cause driver

* I2C

  * Arbitrary i2c clock speed support with :c:macro:`I2C_SPEED_DT`
  * NXP flexcomm supports target (slave) mode
  * Fixes for Atmel SAM/SAM0 exclusive bus access
  * Added ITE support

* I2S

  * Ported I2S drivers to pinctrl.
  * Fixed multiple bugs in the NXP I2S (SAI) driver, including problems with
    DMA transmission and FIFO under/overruns.

* Interrupt Controller

* MBOX

* MEMC

  * STM32: Extend FMC driver to support NOR/PSRAM. See :dtcompatible:`st,stm32-fmc-nor-psram.yaml`.

* Pin control

  * New platforms added to ``pinctrl`` state-based API:

    * Atmel SAM/SAM0
    * Espressif ESP32
    * ITE IT8XXX2
    * Microchip XEC
    * Nordic nRF (completed support)
    * Nuvoton NPCX Embedded Controller (EC)
    * NXP iMX
    * NXP Kinetis
    * NXP LPC
    * RV32M1
    * SiFive Freedom
    * Telink B91
    * TI CC13XX/CC26XX

  * STM32: It is now possible to configure plain GPIO pins using pinctrl API.
    See :dtcompatible:`st,stm32-pinctrl` and :dtcompatible:`st,stm32f1-pinctrl` for
    more information.

* PWM

  * Added :c:struct:`pwm_dt_spec` and associated helpers, e.g.
    :c:macro:`PWM_DT_SPEC_GET` or :c:func:`pwm_set_dt`. This addition makes it
    easier to use the PWM API when the PWM channel, period and flags are taken
    from a Devicetree PWM cell.
  * STM32: Enabled complementary output for timer channel. A PWM consumer can now use
    :c:macro:`PWM_STM32_COMPLEMENTARY` to specify that PWM output should happen on a
    complementary channel pincfg (eg:``tim1_ch2n_pb14``).
  * STM32: Added counter mode support. See :dtcompatible:`st,stm32-timers`.

* Reset

  * Added reset controller driver API.
  * Raspberry Pi Pico: Added reset controller driver

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

  * STM32: Add tx/rx pin swap  and rx invert / tx invert capabilities.

* SPI

  * Ported all SPI drivers to pinctrl
  * Added support for SPI on the GD32 family

* Timer

  * Ported timer drivers to use pinctrl
  * LiteX: Ported the timer driver to use the HAL

* USB

  * Raspberry Pi Pico: Added USB driver

* Watchdog

Networking
**********

* CoAP:

  * Changed :c:struct:`coap_pending` allocation criteria - use data pointer
    instead of timestamp, which does not give 100% guarantee that structure
    is not in use already.

* Ethernet:

  * Added :kconfig:option:`NET_ETHERNET_FORWARD_UNRECOGNISED_ETHERTYPE` option
    which allows to forward frames with unrecognised EtherType to the netowrk
    stack.

* HTTP:

  * Removed a limitation, where the maximum content length was limited up to
    100000 bytes.
  * Fixed :c:func:`http_client_req` return value, the function did not report
    number of bytes sent correctly.
  * Clarify the expected behavior in case of empty response from the server.
  * Make use of :c:func:`shutdown` to tear down HTTP connection instead of
    closing the socket from a system work queue.

* LwM2M:

  * Various improvements towards LwM2M 1.1 support:

    * Added LwM2M 1.1 Discovery support.
    * Added attribute handling for Resource Instances.
    * Added support for Send, Read-composite, Write-composite, Observe-composite
      operations.
    * Added new content formats: SenML JSON, CBOR, SenML CBOR.
    * Added v1.1 implementation of core LwM2M objects.

  * Added support for dynamic Resource Instance allocation.
  * Added support for LwM2M Portfolio object (Object ID 16).
  * Added LwM2M shell module.
  * Added option to utilize DTLS session cache in queue mode.
  * Added :c:func:`lwm2m_engine_path_is_observed` API function.
  * Fixed a bug with hostname verification setting, which prevented DTLS
    connection in certain mbedTLS configurations.
  * Fixed a bug which could cause a socket descriptor leak, in case
    :c:func:`lwm2m_rd_client_start` was called immediately after
    :c:func:`lwm2m_rd_client_stop`.
  * Added error reporting from :c:func:`lwm2m_rd_client_start` and
    :c:func:`lwm2m_rd_client_stop`.

* Misc:

  * Added :c:func:`net_if_set_default` function which allows to set a default
    network interface at runtime.
  * Added :kconfig:option:`NET_DEFAULT_IF_UP` option which allows to make the
    first interface which is up the default choice.
  * Fixed packet leak in network shell TCP receive handler.
  * Added :c:func:`net_pkt_rx_clone` which allows to allocated packet from
    correct packet pool when cloning. This is used at the loopback interface.
  * Added :kconfig:option:`NET_LOOPBACK_SIMULATE_PACKET_DROP` option which
    allows to simulate packet drop at the loopback interface. This is used by
    certain test cases.

* MQTT:

  * Removed custom logging macros from MQTT implementation, in favour of the
    common networking logging.

* OpenThread:

  * Updated OpenThread revision up to commit ``130afd9bb6d02f2a07e86b824fb7a79e9fca5fe0``.
  * Implemented ``otPlatCryptoRand`` platform API for OpenThread.
  * Added support for PSA MAC keys.
  * Multiple minor fixes/improvements to align with upstream OpenThread changes.

* Sockets:

  * Added support for :c:func:`shutdown` function.
  * Fixed :c:func:`sendmsg` operation when TCP reported full transmission window.
  * Added support for :c:func:`getpeername` function.
  * Fixed userspace :c:func:`accept` argument validation.
  * Added support for :c:macro:`SO_SNDBUF` and :c:macro:`SO_RCVBUF` socket
    options.
  * Implemented :c:macro:`POLLOUT` reporting from :c:func:`poll` for STREAM
    sockets.
  * Implemented socket dispatcher for offloaded sockets. This module allows to
    use multiple offloaded socket implementations at the same time.
  * Introduced a common socket priority for offloaded sockets
    (:kconfig:option:`CONFIG_NET_SOCKETS_OFFLOAD_PRIORITY`).
  * Moved socket offloading out of experimental.

* TCP:

  * Implemented receive window handling.
  * Implemented zero-window probe processing and sending.
  * Improved TCP stack throughput over loopback interface.
  * Fixed possible transmission window overflow in case of TCP retransmissions.
    This could led to TX buffer starvation when TCP entered retransmission mode.
  * Updated :c:macro:`FIN_TIMEOUT` delay to correctly reflect time needed for
    all FIN packet retransmissions.
  * Added proper error reporting from TCP to upper layers. This solves the
    problem of connection errors being reported to the application as graceful
    connection shutdown.
  * Added a mechanism which allows upper layers to monitor the TCP transmission
    window availability. This allows to improve throughput greatly in low-buffer
    scenarios.

* TLS:

  * Added :c:macro:`TLS_SESSION_CACHE` and :c:macro:`TLS_SESSION_CACHE_PURGE`
    socket options which allow to control session caching on a socket.
  * Fixed :c:macro:`TLS_CIPHERSUITE_LIST` socket option, which did not set the
    cipher list on a socket correctly.

USB
***

Build System
************

Devicetree
**********

* API

  * New macros for creating tokens in C from strings in the devicetree:
    :c:macro:`DT_STRING_UPPER_TOKEN_OR`, :c:macro:`DT_INST_STRING_TOKEN`,
    :c:macro:`DT_INST_STRING_UPPER_TOKEN`,
    :c:macro:`DT_INST_STRING_UPPER_TOKEN_OR`

  * :ref:`devicetree-can-api`: new

* Bindings

  * Several new bindings were created to support :ref:`Pin Control
    <pinctrl-guide>` driver API implementations. This also affected many
    peripheral bindings, which now support ``pinctrl-0``, ``pinctrl-1``, ...,
    and ``pinctrl-names`` properties used to configure peripheral pin
    assignments in different system states, such as active and low-power
    states.

    In some cases, this resulted in the removal of old bindings, or other
    backwards incompatible changes affecting users of the old bindings. These
    changes include:

    * :dtcompatible:`atmel,sam-pinctrl` and :dtcompatible:`atmel,sam0-pinctrl`
      have been adapted to the new pinctrl bindings interface
    * :dtcompatible:`espressif,esp32-pinctrl` has replaced ``espressif,esp32-pinmux``
    * :dtcompatible:`ite,it8xxx2-pinctrl` and
      :dtcompatible:`ite,it8xxx2-pinctrl-func` have replaced
      ``ite,it8xxx2-pinmux`` and ``ite,it8xxx2-pinctrl-conf``
    * :dtcompatible:`microchip,xec-pinctrl`: new
    * :dtcompatible:`nuvoton,npcx-pinctrl`: new
    * :dtcompatible:`nxp,kinetis-pinctrl` has replaced the ``nxp,kinetis-port-pins`` property found in the ``nxp,kinetis-pinmux`` binding.
    * :dtcompatible:`nxp,mcux-rt-pinctrl`,
      :dtcompatible:`nxp,mcux-rt11xx-pinctrl`,
      :dtcompatible:`nxp,lpc-iocon-pinctrl`, :dtcompatible:`nxp,rt-iocon-pinctrl`,
      :dtcompatible:`nxp,lpc11u6x-pinctrl`, :dtcompatible:`nxp,imx7d-pinctrl`,
      :dtcompatible:`nxp,imx8m-pinctrl`, :dtcompatible:`nxp,imx8mp-pinctrl` and
      :dtcompatible:`nxp,imx-iomuxc`: new
    * :dtcompatible:`openisa,rv32m1-pinctrl`: new
    * :dtcompatible:`sifive,pinctrl` has replaced ``sifive,iof``
    * :dtcompatible:`telink,b91-pinctrl` has replaced ``telink,b91-pinmux``
    * :dtcompatible:`ti,cc13xx-cc26xx-pinctrl` has replaced ``ti,cc13xx-cc26xx-pinmux``

  * PWM bindings now generally have ``#pwm-cells`` set to 3, not 2 as it was in
    previous releases. This was done to follow the Linux convention that each
    PWM specifier should contain a channel, period, and flags cell, in that
    order. See pull request `#44523
    <https://github.com/zephyrproject-rtos/zephyr/pull/44523>`_ for more
    details on this change and its purpose.

  * Some bindings had their :ref:`compatible properties <dt-important-props>`
    renamed:

    * :dtcompatible:`nxp,imx-elcdif` has replaced ``fsl,imx6sx-lcdif``
    * :dtcompatible:`nxp,imx-gpr` has replaced ``nxp,imx-pinmux``
    * :dtcompatible:`nordic,nrf-wdt` has replaced ``nordic,nrf-watchdog``
    * :dtcompatible:`bosch,m_can-base` has replaced ``bosch,m-can-base``
    * :dtcompatible:`nxp,imx-usdhc` has replaced ``nxp,imx-sdhc``

  * Bindings with ``resets`` (and optionally ``reset-names``) properties were
    added to support the :ref:`reset_api` API. See the list of new bindings
    below for some examples.

  * The ``zephyr,memory-region-mpu`` property can be set to generate MPU
    regions from devicetree nodes. See commit `b91d21d32c
    <https://github.com/zephyrproject-rtos/zephyr/commit/b91d21d32ccc312558babe2cc363afbe44ea2de2>`_

  * The generic :zephyr_file:`dts/bindings/can/can-controller.yaml` include
    file used for defining CAN controller bindings no longer contains a ``bus:
    yaml`` statement. This was unused in upstream Zephyr; out of tree bindings
    relying on this will need updates.

  * Bindings for ADC controller nodes can now use a child binding to specify
    the initial configuration of individual channels in devicetree. See pull
    request `43030 <https://github.com/zephyrproject-rtos/zephyr/pull/43030>`_
    for details.

  * New bindings for the following compatible properties were added:

    * :dtcompatible:`arduino-nano-header-r3`
    * :dtcompatible:`arm,cortex-r52`
    * :dtcompatible:`atmel,sam-rstc`
    * :dtcompatible:`can-transceiver-gpio` (see also :ref:`devicetree-can-api`)
    * :dtcompatible:`gd,gd32-spi`
    * :dtcompatible:`hynitron,cst816s`
    * :dtcompatible:`intel,cavs-gpdma`
    * :dtcompatible:`intel,cavs-hda-host-in` and :dtcompatible:`intel,cavs-hda-host-out`
    * :dtcompatible:`intel,cavs-hda-link-in` and :dtcompatible:`intel,cavs-hda-link-out`
    * :dtcompatible:`intel,ssp-dai`
    * :dtcompatible:`intel,ssp-sspbase`
    * :dtcompatible:`invensense,icm42670`
    * :dtcompatible:`ite,enhance-i2c`
    * :dtcompatible:`ite,it8xxx2-vcmp`
    * :dtcompatible:`ite,it8xxx2-wuc` and :dtcompatible:`ite,it8xxx2-wuc-map`
    * :dtcompatible:`ite,peci-it8xxx2`
    * :dtcompatible:`maxim,max31875`
    * :dtcompatible:`microchip,cap1203`
    * :dtcompatible:`microchip,mcp4728`
    * :dtcompatible:`microchip,mpfs-qspi`
    * :dtcompatible:`microchip,xec-bbram`
    * :dtcompatible:`motorola,mc146818`
    * :dtcompatible:`nordic,nrf-acl`
    * :dtcompatible:`nordic,nrf-bprot`
    * :dtcompatible:`nordic,nrf-ccm`
    * :dtcompatible:`nordic,nrf-comp`
    * :dtcompatible:`nordic,nrf-ctrlapperi`
    * :dtcompatible:`nordic,nrf-dcnf`
    * :dtcompatible:`nordic,nrf-gpio-forwarder`
    * :dtcompatible:`nordic,nrf-lpcomp`
    * :dtcompatible:`nordic,nrf-mpu`
    * :dtcompatible:`nordic,nrf-mutex`
    * :dtcompatible:`nordic,nrf-mwu`
    * :dtcompatible:`nordic,nrf-nfct`
    * :dtcompatible:`nordic,nrf-oscillators`
    * :dtcompatible:`nordic,nrf-ppi`
    * :dtcompatible:`nordic,nrf-reset`
    * :dtcompatible:`nordic,nrf-swi`
    * :dtcompatible:`nordic,nrf-usbreg`
    * :dtcompatible:`nuvoton,adc-cmp`
    * :dtcompatible:`nxp,imx-mipi-dsi`
    * :dtcompatible:`nxp,imx-qtmr`
    * :dtcompatible:`nxp,imx-tmr`
    * :dtcompatible:`raspberrypi,pico-reset`
    * :dtcompatible:`raspberrypi,pico-usbd`
    * :dtcompatible:`raydium,rm68200`
    * :dtcompatible:`riscv,sifive-e31`, :dtcompatible:`riscv,sifive-e51`,
      and :dtcompatible:`riscv,sifive-s7` CPU bindings
    * :dtcompatible:`seeed,grove-lcd-rgb`
    * :dtcompatible:`st,lsm6dso32`
    * :dtcompatible:`st,stm32-clock-mux`
    * :dtcompatible:`st,stm32-fmc-nor-psram`
    * :dtcompatible:`st,stm32-lse-clock`
    * :dtcompatible:`st,stm32-ltdc`
    * :dtcompatible:`st,stm32-ospi` and :dtcompatible:`st,stm32-ospi-nor`
    * :dtcompatible:`st,stm32h7-fmc`
    * TI ADS ADCs: :dtcompatible:`ti,ads1013`, :dtcompatible:`ti,ads1015`,
      :dtcompatible:`ti,ads1113`, :dtcompatible:`ti,ads1114`,
      :dtcompatible:`ti,ads1115`, :dtcompatible:`ti,ads1014`
    * :dtcompatible:`ti,tlc5971`
    * :dtcompatible:`xlnx,fpga`
    * :dtcompatible:`xlnx,ps-gpio` and :dtcompatible:`xlnx,ps-gpio-bank`
    * :dtcompatible:`zephyr,bt-hci-entropy`
    * :dtcompatible:`zephyr,ipc-icmsg`
    * :dtcompatible:`zephyr,memory-region`
    * :dtcompatible:`zephyr,sdhc-spi-slot`

  * Bindings for the following compatible properties were removed:

    * ``bosch,m-can``
    * ``nxp,imx-usdhc``
    * ``shared-multi-heap``
    * ``snps,creg-gpio-mux-hsdk``
    * ``snps,designware-pwm``
    * ``zephyr,mmc-spi-slot``

  * Numerous other additional properties were added to bindings throughout the tree.

Libraries / Subsystems
**********************

* C Library

  * Minimal libc

    * Added ``[U]INT_{FAST,LEAST}N_{MIN,MAX}`` minimum and maximum value
      macros in ``stdint.h``.
    * Added ``PRIx{FAST,LEAST}N`` and ``PRIxMAX`` format specifier macros in
      ``inttypes.h``.
    * Fixed :c:func:`gmtime` access fault when userspace is enabled and
      :c:func:`gmtime` is called from a user mode thread. This function can be
      safely called from both kernel and user mode threads.

  * Newlib

    * Fixed access fault when calling the newlib math functions from a user
      mode thread. All ``libm.a`` globals are now placed into the
      ``z_libc_partition`` when userspace is enabled.

* C++ Subsystem

  * Renamed all C++ source and header files to use the ``cpp`` and ``hpp``
    extensions, respectively. All Zephyr upstream C++ source and header files
    are now required to use these extensions.

* Management

  * MCUMGR has been migrated from using TinyCBOR, for CBOR encoding, to zcbor.
  * MCUMGR :kconfig:option:`CONFIG_FS_MGMT_UL_CHUNK_SIZE` and
    :kconfig:option:`CONFIG_IMG_MGMT_UL_CHUNK_SIZE` have been deprecated as,
    with the introduction of zcbor, it is no longer needed to use an intermediate
    buffer to copy data out of CBOR encoded buffer. The file/image chunk size
    is now limited by :kconfig:option:`CONFIG_MCUMGR_BUF_SIZE` minus all the
    other command required variables.
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

* IPC

  * static_vrings: Fixed WQ initialization
  * static_vrings: Introduced atomic helpers when accessing atomic_t variables
  * static_vrings: Moved to one WQ per instance
  * static_vrings: Added "zephyr,priority" property in the DT to set the WQ priority of the instance
  * static_vrings: Added configuration parameter to initialize shared memory to zero
  * Extended API with NOCOPY functions
  * static_vrings: Added support for NOCOPY operations
  * Introduced inter core messaging backend (icmsg) that relies on simple inter core messaging buffer

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

* STM32:

  * Updated stm32f4 to new STM32cube version V1.27.0
  * Updated stm32f7 to new STM32cube version V1.16.2
  * Updated stm32g4 to new STM32cube version V1.5.0
  * Updated stm32h7 to new STM32cube version V1.10.0
  * Updated stm32l4 to new STM32cube version V1.17.1
  * Updated stm32u5 to new STM32cube version V1.1.0
  * Updated stm32wb to new STM32cube version V1.13.2 (including hci lib)

MCUboot
*******

- Added initial support for devices with a write alignment larger than 8B.
- Addeed optiona for enter to the serial recovery mode with timeout, see ``CONFIG_BOOT_SERIAL_WAIT_FOR_DFU``.
- Use a smaller sha256 implementation.
- Added support for the echo command in serial recovery, see ``CONFIG_BOOT_MGMT_ECHO``.
- Fixed image decryption for any SoC flash of the pages size which not fitted in 1024 B in single loader mode.
- Fixed possible output buffer overflow in serial recovery.
- Added GH workflow for verifying integration with the Zephyr.
- Removed deprecated ``DT_CHOSEN_ZEPHYR_FLASH_CONTROLLER_LABEL``.
- Fixed usage of ``CONFIG_LOG_IMMEDIATE``.

Trusted Firmware-m
******************

* Updated to TF-M 1.6.0

Documentation
*************

* Reorganised and consolidated documentation for improved readability and
  user experience.
* Replaced the existing statically rendered Kconfig documentation with the new
  Kconfig documentation engine that dynamically renders the Kconfig contents
  for improved search performance.
* Added 'Language Support' sub-category under the 'Developing with Zephyr'
  category that provides details regarding C and C++ language and standard
  library support status.
* Added 'Toolchain' sub-category under the 'Developing with Zephyr' category
  that lists all supported toolchains and the instructions on how to configure
  and use them.

Tests and Samples
*****************

  * A dedicated framework was added to test STM32 clock_control driver.

Issue Related Items
*******************

These GitHub issues were addressed since the previous 3.0.0 tagged
release:
