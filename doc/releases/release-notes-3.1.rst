:orphan:

.. _zephyr_3.1:

Zephyr 3.1.0
############

The following sections provide detailed lists of changes by component.

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

* Kconfig preprocessor function ``dt_nodelabel_has_compat`` was redefined, for
  consistency with the ``dt_nodelabel_has_prop`` function and devicetree macros
  like :c:func:`DT_NODE_HAS_COMPAT`. Now the function does not take into account
  the status of the checked node. Its former functionality is provided by the
  newly introduced ``dt_nodelabel_enabled_with_compat`` function.

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

  * STM32H7: :kconfig:option:`CONFIG_NOCACHE_MEMORY` is no longer responsible for disabling
    data cache when defined. Use ``CONFIG_DCACHE=n`` instead.

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
* :c:func:`lwm2m_engine_set_res_data` is deprecated in favor of :c:func:`lwm2m_engine_set_res_buf`
* :c:func:`lwm2m_engine_get_res_data` is deprecated in favor of :c:func:`lwm2m_engine_get_res_buf`
* The TinyCBOR module has been deprecated in favor of the new zcbor CBOR
  library, included with Zephyr in this release.

* GPIO

  * Deprecated the ``GPIO_INT_DEBOUNCE`` flag and the ``GPIO_DS_*`` and
    ``GPIO_VOLTAGE_*`` groups of flags. Controller/SoC specific flags
    should now be used instead.

* SPI

  * Deprecated the ``gpio_dev``, ``gpio_pin``, and ``gpio_dt_flags`` members in
    struct :c:struct:`spi_cs_control` in favor of a new struct
    :c:struct:`gpio_dt_spec` member named ``gpio``.

* PWM

  * The ``pin`` prefix has been removed from all PWM API calls. So for example,
    ``pwm_pin_set_cycles`` is now ``pwm_set_cycles``. The old API calls are
    still provided, but are now deprecated.
  * PWM periods are now always set in nanoseconds, so ``_nsec`` and ``_usec``
    set functions such as ``pwm_pin_set_nsec()`` and ``pwm_pin_set_usec()``
    have been deprecated. Other units can be specified using, e.g.
    ``PWM_USEC()`` macros, which convert other units to nanoseconds.

* Utilities

  * :c:macro:`UTIL_LISTIFY` has been deprecated. Use :c:macro:`LISTIFY` instead.

Stable API changes in this release
==================================

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

* Host

  * The :c:enum:`bt_l2cap_chan_state` values ``BT_L2CAP_CONNECT`` and
    ``BT_L2CAP_DISCONNECT`` have been renamed to ``BT_L2CAP_CONNECTING`` and
    ``BT_L2CAP_DISCONNECTING`` respectively.

  * The callbacks :c:func:`pairing_complete`, :c:func:`pairing_failed`, and
    :c:func:`bond_delete` have been moved from struct :c:struct:`bt_auth_cb` to a
    newly created informational-only callback struct :c:struct:`bt_conn_auth_info_cb`.

  * :c:func:`bt_conn_index` now takes a ``const struct bt_conn*`` argument.

  * The :c:struct:`bt_gatt_subscribe_params` structure's ``write`` callback
    function has been deprecated.  Use the new ``subscribe`` callback
    instead.

  * :c:func:`bt_disable` was added to enable the caller to disable the Bluetooth stack.

  * Added new Kconfig options to select ISO Central and Peripheral role support
    separately

  * Added a new :c:func:`bt_get_appearance()` API call

  * Implemented support for dynamic appearance, including a new
    :c:func:`bt_set_appearance()` API call

  * Implemented support for L2CAP collision mitigation

  * Changed the scheduling of auto-initiated HCI commands so that they execute
    synchronously

  * Added a new :c:func:`bt_is_ready()` API call to find out if Bluetooth is
    currently enabled and initialized

  * Added support for automatic MTU exchange right after a connection is
    established

  * Created a new :c:struct:`bt_conn_auth_info_cb` to group the
    security-related callbacks under a single struct

  * Optimized the memory usage of the Object Transfer Service

  * Added a new :c:func:`bt_hci_le_rand()` API call to obtain a random number
    from the LE Controller

  * Added a new public API to connect EATT channels, :c:func:`bt_eatt_connect()`

  * Optimized L2CAP channels resource usage when not using dynamic channels

  * Added the ability to run the Bluetooth RX context from a workqueue, in order
    to optimize RAM usage. See :kconfig:option:`CONFIG_BT_RECV_CONTEXT`.

  * Added support for TX complete callback on EATT channels

  * Corrected the calling of the MTU callback to happen on any reconfiguration

Kernel
******

* Aborting an essential thread now causes a kernel panic, as the
  documentation has always promised but the kernel has never
  implemented.

* The k_timer handler can now correct itself for lost time due to very
  late-arriving interrupts.

* SMP interprocessor interrupts are deferred so that they are sent only at
  schedule points, and not synchronously when the scheduler state
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

  * Added ARCv3 32 bit (HS5x) support - both GNU and MWDT toolchains, both UP and SMP
  * Worked around debug_select interference with MDB debugger
  * Switched to hs6x mcpu usage (GNU toolchain) for HS6x

* ARM

  * AARCH32

    * Added Cortex-R floating point support

  * AARCH64

    * Added support for GICv3 for the ARMv8 Xen Virtual Machine
    * Fixed SMP boot code to take into account multiple cores booting at the same time
    * Added more memory mapping types for device memory
    * Simplified and optimize switching and user mode transition code
    * Added support for CONFIG_IRQ_OFFLOAD_NESTED
    * Fixed booting issue with FVP V8R >= 11.16.16
    * Switched to the IRQ stack during ISR execution

* Xtensa

  * Optimized context switches when KERNEL_COHERENCE is enabled to
    avoid needless stack invalidations for threads that have not
    migrated between CPUs.

  * Fixed a bug that could return directly into a thread context from a
    nested interrupt instead of properly returning to the preempted
    ISR.

* x64_64

  * UEFI devices can now use the firmware-initialized system console
    API as a printk/logging backend, simplifying platform bringup on
    devices without known-working serial port configurations.

Boards & SoC Support
********************

* Added support for these SoC series:

  * STM32H725/STM32H730/STM32H73B SoC variants

* Made these changes in other SoC series:

  * Added Atmel SAM UPLL clock support
  * Raspberry Pi Pico: Added HWINFO support
  * Raspberry Pi Pico: Added I2C support
  * Raspberry Pi Pico: Added reset controller support
  * Raspberry Pi Pico: Added USB support

* Changes for ARC boards:

  * Added nsim_hs5x and nsim_hs5x_smp boards with ARCv3 32bit HS5x CPU
  * Added MWDT toolchain support for nsim_hs6x and nsim_hs6x_smp
  * Overhauled memory layout for nSIM boards. Added a mechanism to switch between
    ICCM/DCCM memory layout and flat memory layout (i.e DDR).
  * Did required platform setup so nsim_hs5x, nsim_hs5x_smp, nsim_hs6x, nsim_hs6x_smp
    can be run on real HW (HAPS FPGA) with minimum additional configuration
  * Enabled MWDT toolchain support for hsdk_2cores board
  * Adjusted test duration for SMP nSIM boards with timeout_multiplier

* Added support for these ARM boards:

  * b_g474e_dpow1
  * stm32f401_mini

* Added support for these ARM64 boards:

  * NXP i.MX8MP EVK (i.MX8M Plus LPDDR4 EVK board)
  * NXP i.MX8MM EVK (i.MX8M Mini LPDDR4 EVK board)

* Added support for these RISC-V boards:

  * GigaDevice GD32VF103C-EVAL

* Made these changes in other boards:

  * sam4s_xplained: Added support for HWINFO
  * sam_e70_xlained: Added support for HWINFO and CAN-FD
  * sam_v71_xult: Added support for HWINFO and CAN-FD
  * gd32e103v_eval: Added prescaler to timer
  * longan_nano: Added support for TF-Card slot

* Added support for these following shields:

  * Keyestudio CAN-BUS Shield (KS0411)
  * MikroElektronika WIFI and BLE Shield
  * X-NUCLEO-53L0A1 ranging and gesture detection sensor expansion board

Drivers and Sensors
*******************

* ADC

  * Atmel SAM0: Fixed adc voltage reference
  * STM32: Added support for :c:enumerator:`adc_reference.ADC_REF_INTERNAL`.
  * Added the :c:struct:`adc_dt_spec` structure and associated helper macros,
    e.g. :c:macro:`ADC_DT_SPEC_GET`, to facilitate getting configuration of
    ADC channels from devicetree nodes.

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

  * Added support for STM32F1 SoCs to the STM32 DAC driver.

* Disk

  * Added a generic SDMMC disk driver, that uses the SD subsystem to interact with
    disk devices. This disk driver will be used with any disk device declared
    with the ``zephyr,sdmmc-disk`` compatible string.

* Display

  * STM32: Added basic support for LTDC driver. Currently supported on F4, F7, H7, L4+
    and MP1 series.

* DMA

  * Added a scatter gather test for DMAs that support it
  * Cleanly shared Synopsis DW-DMA driver and Intel cAVS GPDMA driver code.
  * Added support for Synposis DW-DMA transfer lists.
  * Added support for Intel HDA for audio device and host streams.
  * Fixes for NXP eDMA to pass scatter gather tests

* Entropy

  * STM32: Prevented the core from entering stop modes during entropy operations.

* Ethernet

  * eth_native_posix: Added support for setting MAC address.
  * eth_stm32_hal: Fixed a bug which caused a segfault in case of a failed RX
    buffer allocation.
  * eth_mcux: Added support for resetting PHY.
  * eth_liteeth: Refactored driver to use LiteX HAL.
  * eth_w5500: Fixed possible deadlock due to incorrect IRQ processing.

* Flash

  * Added STM32 OCTOSPI driver. Initial support is provided for L5 and U5
    series. Interrupt driven mode. Supports 1 and 8 lines in Single or Dual
    Transfer Modes.
  * STM32L5: Added support for Single Bank.
  * STM32 QSPI driver was extended with with QER (SFDP, DTS), custom quad write opcode
    and 1-1-4 read mode.
  * Added support for STM32U5 series.

* GPIO

  * Refactored GPIO devicetree flags. The upper 8 bits of ``gpio_dt_flags_t``
    are now reserved for controller/SoC specific flags. Certain
    hardware-specific flags previously defined as common configuration (IO
    voltage level, drive strength, and debounce filter) were replaced with ones
    defined in this controller/SoC specific space.
  * Added Xilinx PS MIO/EMIO GPIO controller driver.
  * Extended the NXP PCA95XX driver to support also PCAL95XX.

* HWINFO

  * Atmel SAM: Added RSTC support
  * Raspberry Pi Pico: Added Unique ID and reset cause driver

* I2C

  * Added arbitrary I2C clock speed support with :c:macro:`I2C_SPEED_DT`
  * NXP flexcomm now supports target (slave) mode
  * Fixed Atmel SAM/SAM0 exclusive bus access
  * Added ITE support

* I2S

  * Ported I2S drivers to pinctrl.
  * Fixed multiple bugs in the NXP I2S (SAI) driver, including problems with
    DMA transmission and FIFO under/overruns.

* MEMC

  * STM32: Extended FMC driver to support NOR/PSRAM. See :dtcompatible:`st,stm32-fmc-nor-psram.yaml`.

* Pin control

  * Platform support was added for:

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

  * STM32: It is now possible to configure plain GPIO pins using the pinctrl API.
    See :dtcompatible:`st,stm32-pinctrl` and :dtcompatible:`st,stm32f1-pinctrl` for
    more information.

* PWM

  * Added :c:struct:`pwm_dt_spec` and associated helpers, e.g.
    :c:macro:`PWM_DT_SPEC_GET` or :c:func:`pwm_set_dt`. This addition makes it
    easier to use the PWM API when the PWM channel, period and flags are taken
    from a devicetree PWM cell.
  * STM32: Enabled complementary output for timer channel. A PWM consumer can now use
    :c:macro:`PWM_STM32_COMPLEMENTARY` to specify that PWM output should happen on a
    complementary channel pincfg (eg:``tim1_ch2n_pb14``).
  * STM32: Added counter mode support. See :dtcompatible:`st,stm32-timers`.
  * Aligned nRF PWM drivers (pwm_nrfx and pwm_nrf5_sw) with the updated PWM API.
    In particular, this means that the :c:func:`pwm_set` and
    :c:func:`pwm_set_cycles` functions need to be called with a PWM channel
    as a parameter, not with a pin number like it was for the deprecated
    ``pwm_pin_set_*`` functions. Also, the ``flags`` parameter is now supported
    by the drivers, so either the :c:macro:`PWM_POLARITY_INVERTED` or
    :c:macro:`PWM_POLARITY_NORMAL` flag must be provided in each call.

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

  * STM32: Added tx/rx pin swap and rx invert / tx invert capabilities.

* SPI

  * Ported all SPI drivers to pinctrl
  * Added support for SPI on the GD32 family

* Timer

  * Ported timer drivers to use pinctrl
  * LiteX: Ported the timer driver to use the HAL

* USB

  * Added RP2040 (Raspberry Pi Pico) USB device controller driver

Networking
**********

* CoAP:

  * Changed :c:struct:`coap_pending` allocation criteria. This now uses a data
    pointer instead of a timestamp, which does not give a 100% guarantee that
    structure is not in use already.

* Ethernet:

  * Added a
    :kconfig:option:`CONFIG_NET_ETHERNET_FORWARD_UNRECOGNISED_ETHERTYPE`
    option, which allows to forward frames with unrecognised EtherType to the
    netowrk stack.

* HTTP:

  * Removed a limitation where the maximum content length was limited up to
    100000 bytes.
  * Fixed ``http_client_req()`` return value. The function now correctly
    reports the number of bytes sent.
  * Clarified the expected behavior in case of empty response from the server.
  * Made use of ``shutdown`` to tear down HTTP connection instead of
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
  * Added :kconfig:option:`CONFIG_NET_DEFAULT_IF_UP` option which allows to make the
    first interface which is up the default choice.
  * Fixed packet leak in network shell TCP receive handler.
  * Added :c:func:`net_pkt_rx_clone` which allows to allocated packet from
    correct packet pool when cloning. This is used at the loopback interface.
  * Added :kconfig:option:`CONFIG_NET_LOOPBACK_SIMULATE_PACKET_DROP` option which
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

  * Added support for ``shutdown()`` function.
  * Fixed ``sendmsg()`` operation when TCP reported full transmission window.
  * Added support for ``getpeername()`` function.
  * Fixed userspace ``accept()`` argument validation.
  * Added support for :c:macro:`SO_SNDBUF` and :c:macro:`SO_RCVBUF` socket
    options.
  * Implemented ``POLLOUT`` reporting from ``poll()`` for STREAM
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
  * Updated ``FIN_TIMEOUT`` delay to correctly reflect time needed for
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

* Moved USB device stack code to own directory in preparation for upcoming
  rework of USB support.

Build System
************

* The build system's internals have been completely overhauled for increased
  modularity. This makes it easier to reuse individual components through the
  Zephyr CMake package mechanism.

  With the improved Zephyr CMake package, the following examples are now possible:

  * ``find_package(Zephyr)``: load a standard build system, as before
  * ``find_package(Zephyr COMPONENTS unittest)``: load a specific unittest
    build component
  * ``find_package(Zephyr COMPONENTS dts)``: only load the dts module and its
    direct dependencies
  * ``find_package(Zephyr COMPONENTS extensions west zephyr_module)``: load
    multiple specific modules and their dependencies

  Some use cases that this work intends to enable are:

  * The sysbuild proposal: `Zephyr sysbuild / multi image #40555
    <https://github.com/zephyrproject-rtos/zephyr/pull/40555>`_
  * Running Zephyr CMake configure stages individually. One example is only
    processing the devicetree steps of the build system, while skipping the
    rest. This is a required feature for extending twister to do test case
    filtering based on the devicetree contents without invoking a complete
    CMake configuration.

  For more details, see :zephyr_file:`cmake/package_helper.cmake`.

* A new Zephyr SDK has been created which now supports macOS and Windows in
  addition to Linux platforms.

  For more information, see: https://github.com/zephyrproject-rtos/sdk-ng

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
    * Fixed ``gmtime()`` access fault when userspace is enabled and
      ``gmtime()`` is called from a user mode thread. This function can be
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
  * Made the ``img_mgmt_vercmp()`` function public to allow application-
    level comparison of image versions.
  * mcumgr will now only return ``MGMT_ERR_ENOMEM`` when it fails to allocate
    a memory buffer for request processing, when previously it would wrongly
    report this error when the SMP response failed to fit into a buffer;
    now when encoding of response fails ``MGMT_ERR_EMSGSIZE`` will be
    reported. This addresses issue :github:`44535`.
  * Added :kconfig:option:`CONFIG_IMG_MGMT_USE_HEAP_FOR_FLASH_IMG_CONTEXT` that
    allows user to select whether the heap will be used for flash image context,
    when heap pool is configured. Previously usage of heap has been implicit,
    with no control from a developer, causing issues reported by :github:`44214`.
    The default, implicit, behaviour has not been kept and the above
    Kconfig option needs to be selected to keep previous behaviour.


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

  * The default policy was renamed from ``PM_POLICY_RESIDENCY`` to
    ``PM_POLICY_DEFAULT``, and ``PM_POLICY_APP`` was renamed to
    ``PM_POLICY_CUSTOM``.

  * The following functions were renamed:

    * :c:func:`pm_power_state_next_get()` is now :c:func:`pm_state_next_get()`
    * :c:func:`pm_power_state_force()` is now :c:func:`pm_state_force()`

  * Removed the deprecated function :c:func:`pm_device_state_set()`.

  * The state constraint APIs were moved (and renamed) to the policy
    API and accounts substates.

    * :c:func:`pm_constraint_get()` is now :c:func:`pm_policy_state_lock_is_active()`
    * :c:func:`pm_constraint_set()` is now :c:func:`pm_policy_state_lock_get()`
    * :c:func:`pm_constraint_release()` is now :c:func:`pm_policy_state_lock_put()`

  * Added a new API to set maximum latency requirements. The ``DEFAULT`` policy
    will account for latency when computing the next state.

    * :c:func:`pm_policy_latency_request_add()`
    * :c:func:`pm_policy_latency_request_update()`
    * :c:func:`pm_policy_latency_request_remove()`

  * The API to set a device initial state was changed to be usable independently of
    :kconfig:option:`CONFIG_PM_DEVICE_RUNTIME`.

    * :c:func:`pm_device_runtime_init_suspended()` is now :c:func:`pm_device_init_suspended()`
    * :c:func:`pm_device_runtime_init_off()` is now :c:func:`pm_device_init_off()`

* IPC

  * static_vrings: Fixed work queue (WQ) initialization
  * static_vrings: Introduced atomic helpers when accessing atomic_t variables
  * static_vrings: Moved to one WQ per instance
  * static_vrings: Added "zephyr,priority" property in the DT to set the WQ priority of the instance
  * static_vrings: Added configuration parameter to initialize shared memory to zero
  * Extended API with NOCOPY functions
  * static_vrings: Added support for NOCOPY operations
  * Introduced inter core messaging backend (icmsg) that relies on simple inter core messaging buffer

* Logging

  * Added UART frontend which supports binary dictionary logging.
  * Added support for MIPI SyS-T catalog messages.
  * Added cAVS HDA backend.

* Shell

  * Added API for creating subcommands from multiple files using memory section approach:

    * :c:macro:`SHELL_SUBCMD_SET_CREATE` for creating a subcommand set.
    * :c:macro:`SHELL_SUBCMD_COND_ADD` and :c:macro:`SHELL_SUBCMD_ADD` for adding subcommands
      to the set.

HALs
****

* Atmel

  * Added devicetree bindings, documentation, and scripts to support
    state-based pin control (``pinctrl``) API.
  * Imported new SoC header files for:

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
- Added an option for entering serial recovery mode with a timeout. See ``CONFIG_BOOT_SERIAL_WAIT_FOR_DFU``.
- Used a smaller sha256 implementation.
- Added support for the echo command in serial recovery. See ``CONFIG_BOOT_MGMT_ECHO``.
- Fixed image decryption for SoC flash with page sizes larger than 1024 B in single loader mode.
- Fixed a possible output buffer overflow in serial recovery.
- Added a GitHub workflow for verifying integration with Zephyr.
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
* Added a 'Language Support' sub-category under the 'Developing with Zephyr'
  category that provides details regarding C and C++ language and standard
  library support status.
* Added a 'Toolchain' sub-category under the 'Developing with Zephyr' category
  that lists all supported toolchains along with instructions for how to configure
  and use them.

Tests and Samples
*****************

  * A dedicated framework was added to test the STM32 clock_control driver.

Issue summary
*************

This section lists security vulnerabilities, other known bugs, and all issues
addressed during the v3.1.0 development period.

Security Vulnerability Related
==============================

The following CVEs are addressed by this release:

More detailed information can be found in:
https://docs.zephyrproject.org/latest/security/vulnerabilities.html

* CVE-2022-1841: Under embargo until 2022-08-18
* CVE-2022-1042: Under embargo until 2022-06-19
* CVE-2022-1041: Under embargo until 2022-06-19

Known bugs
==========

- :github:`23302` - Poor TCP performance
- :github:`25917` - Bluetooth: Deadlock with TX of ACL data and HCI commands (command blocked by data)
- :github:`30348` - XIP can't be enabled with ARC MWDT toolchain
- :github:`31298` - tests/kernel/gen_isr_table failed on hsdk and nsim_hs_smp sometimes
- :github:`33747` - gptp does not work well on NXP rt series platform
- :github:`34226` - Compile error when building civetweb samples for posix_native
- :github:`34600` - Bluetooth: L2CAP: Deadlock when there are no free buffers while transmitting on multiple channels
- :github:`36358` - Potential issue with CMAKE_OBJECT_PATH_MAX
- :github:`37193` - mcumgr: Probably incorrect error handling with udp backend
- :github:`37704` - hello world doesn't work on qemu_arc_em when CONFIG_ISR_STACK_SIZE=1048510
- :github:`37731` - Bluetooth: hci samples: Unable to allocate command buffer
- :github:`38041` - Logging-related tests fails on qemu_arc_hs6x
- :github:`38544` - drivers: wifi: esWIFI: Regression due to 35815
- :github:`38654` - drivers: modem: bg9x: Has no means to update size of received packet.
- :github:`38880` - ARC: ARCv2: qemu_arc_em / qemu_arc_hs don't work with XIP disabled
- :github:`38947` - Issue with SMP commands sent over the UART
- :github:`39347` - Static object constructors do not execute on the NATIVE_POSIX_64 target
- :github:`39888` - STM32L4: usb-hid: regression in hal 1.17.0
- :github:`40023` - Build fails for ``native_posix`` board when using C++ <atomic> header
- :github:`41281` - Style Requirements Seem to Be Inconsistent with Uncrustify Configuration
- :github:`41286` - Bluetooth SDP: When the SDP attribute length is greater than SDP_MTU, the attribute is discarded
- :github:`41606` - stm32u5: Re-implement VCO input and EPOD configuration
- :github:`41622` - Infinite mutual recursion when SMP and ATOMIC_OPERATIONS_C are set
- :github:`41822` - BLE IPSP sample cannot handle large ICMPv6 Echo Request
- :github:`42030` - can: "bosch,m-can-base": Warning "missing or empty reg/ranges property"
- :github:`42134` - TLS handshake error using DTLS on updatehub
- :github:`42574` - i2c: No support for bus recovery imx.rt and or timeout on bus busy
- :github:`42629` - stm32g0: Device hang/hard fault with AT45 + ``CONFIG_PM_DEVICE``
- :github:`42842` - BBRAM API is missing a documentation reference page
- :github:`43115` - Data corruption in STM32 SPI driver in Slave Mode
- :github:`43246` - Bluetooth: Host: Deadlock with Mesh and Ext Adv on native_posix
- :github:`43249` - MBEDTLS_ECP_C not build when MBEDTLS_USE_PSA_CRYPTO
- :github:`43308` - driver: serial: stm32: uart will lost data when use dma mode[async mode]
- :github:`43390` - gPTP broken in Zephyr 3.0
- :github:`43515` - reel_board: failed to run tests/kernel/workq/work randomly
- :github:`43555` - Variables not properly initialized when using data relocation with SDRAM
- :github:`43562` - Setting and/or documentation of Timer and counter use/requirements for Nordic Bluetooth driver
- :github:`43646` - mgmt/mcumgr/lib: OS taskstat may give shorter list than expected
- :github:`43655` - esp32c3: Connection fail loop
- :github:`43811` - ble: gatt: db_hash_work runs for too long and makes serial communication fail
- :github:`43828` - Intel CAVS: multiple tests under tests/boards/intel_adsp/smoke are failing
- :github:`43836` - stm32: g0b1: RTT doesn't work properly after stop mode
- :github:`43887` - SystemView tracing with STM32L0x fails to compile
- :github:`43910` - civetweb/http_server - DEBUG_OPTIMIZATIONS enabled
- :github:`43928` - pm: going to PM_STATE_SOFT_OFF in pm_policy_next_state causes assert in some cases
- :github:`43933` - llvm: twister: multiple errors with set but unused variables
- :github:`44062` - Need a way to deal with stack size needed when running coverage report.
- :github:`44214` - mgmt/mcumgr/lib: Parasitic use of CONFIG_HEAP_MEM_POOL_SIZE in image management
- :github:`44219` - mgmt/mcumgr/lib: Incorrect processing of img_mgmt_impl_write_image_data leaves mcumgr in broken state in case of error
- :github:`44228` - drivers: modem: bg9x: bug on cmd AT+QICSGP
- :github:`44324` - Compile error in byteorder.h
- :github:`44377` - ISO Broadcast/Receive sample not working with coded PHY
- :github:`44403` - MPU fault and ``CONFIG_CMAKE_LINKER_GENERATOR``
- :github:`44410` - drivers: modem: shell: ``modem send`` doesn't honor line ending in modem cmd handler
- :github:`44579` - MCC: Discovery cannot complete with success
- :github:`44622` - Microbit v2 board dts file for lsm303agr int line
- :github:`44725` - drivers: can: stm32: can_add_rx_filter() does not respect CONFIG_CAN_MAX_FILTER
- :github:`44898` - mgmt/mcumgr: Fragmentation of responses may cause mcumgr to drop successfully processed response
- :github:`44925` - intel_adsp_cavs25: multiple tests failed after running tests/boards/intel_adsp
- :github:`44948` - cmsis_dsp: transofrm: error during building cf64.fpu and rf64.fpu for mps2_an521_remote
- :github:`44996` - logging: transient strings are no longer duplicated correctly
- :github:`44998` - SMP shell exec command causes BLE stack breakdown if buffer size is too small to hold response
- :github:`45105` - ACRN: failed to run testcase tests/kernel/fifo/fifo_timeout/
- :github:`45117` - drivers: clock_control: clock_control_nrf
- :github:`45157` - cmake: Use of -ffreestanding disables many useful optimizations and compiler warnings
- :github:`45168` - rcar_h3ulcb: failed to run test case tests/drivers/can/timing
- :github:`45169` - rcar_h3ulcb: failed to run test case tests/drivers/can/api
- :github:`45218` - rddrone_fmuk66: I2C configuration incorrect
- :github:`45222` - drivers: peci: user space handlers not building correctly
- :github:`45241` - (Probably) unnecessary branches in several modules
- :github:`45270` - CMake - TEST_BIG_ENDIAN
- :github:`45304` - drivers: can: CAN interfaces are brought up with default bitrate at boot, causing error frames if bus bitrate differs
- :github:`45315` - drivers: timer: nrf_rtc_timer: NRF boards take a long time to boot application in CONFIG_TICKLESS_KERNEL=n mode after OTA update
- :github:`45349` - ESP32: fails to chain-load sample/board/esp32/wifi_station from MCUboot
- :github:`45374` - Creating the unicast group before both ISO connections have been configured might cause issue
- :github:`45441` - SPI NOR driver assume all SPI controller HW is implemnted in an identical way
- :github:`45509` - ipc: ipc_icmsg: Can silently drop buffer if message is too big
- :github:`45532` - uart_msp432p4xx_poll_in() seems to be a blocking function
- :github:`45564` - Zephyr does not boot with CONFIG_PM=y
- :github:`45581` - samples: usb: mass: Sample.usb.mass_flash_fatfs fails on non-secure nrf5340dk
- :github:`45596` - samples: Code relocation nocopy sample has some unusual failure on nrf5340dk
- :github:`45647` - test: drivers: counter: Test passes even when no instances are found
- :github:`45666` - Building samples about BLE audio with nrf5340dk does not work
- :github:`45675` - testing.ztest.customized_output: mismatch twister results in json/xml file
- :github:`45678` - Lorawan: Devnonce has already been used
- :github:`45760` - Running twister on new board files
- :github:`45774` - drivers: gpio: pca9555: Driver is writing to output port despite all pins having been configured as input
- :github:`45802` - Some tests reported as PASSED (device) but they were only build
- :github:`45807` - CivetWeb doesn't build for CC3232SF
- :github:`45814` - Armclang build fails due to missing source file
- :github:`45842` - drivers: modem: uart_mux errors after second call to gsm_ppp_start
- :github:`45844` - Not all bytes are downloaded with HTTP request
- :github:`45845` - tests: The failure test case number increase significantly in CMSIS DSP tests on ARM boards.
- :github:`45848` - tests: console harness: inaccuracy testcases report
- :github:`45866` - drivers/entropy: stm32: non-compliant RNG configuration on some MCUs
- :github:`45914` - test: tests/kernel/usage/thread_runtime_stats/ test fail
- :github:`45929` - up_squared：failed to run test case tests/posix/common
- :github:`45951` - modem: ublox-sara-r4: outgoing datagrams are truncated if they do not fit MTU
- :github:`45953` - modem: simcom-sim7080: sendmsg() should result in single outgoing datagram
- :github:`46008` - stm32h7: gptp sample does not work at all
- :github:`46049` - Usage faults on semaphore usage in driver (stm32l1)
- :github:`46066` - TF-M: Unable to trigger NMI interrupt from non-secure
- :github:`46072` - [ESP32] Debug log error in hawkbit example "CONFIG_LOG_STRDUP_MAX_STRING"
- :github:`46073` - IPSP (IPv6 over BLE) example stop working after a short time
- :github:`46121` - Bluetooth: Controller: hci: Wrong periodic advertising report data status
- :github:`46124` - stm32g071 ADC drivers apply errata during sampling config
- :github:`46126` - pm_device causes assertion error in sched.c with lis2dh
- :github:`46157` - ACRN: some cases still failed because of the log missing
- :github:`46158` - frdm_k64f：failed to run test case tests/subsys/modbus/modbus.rtu/server_setup_low_none
- :github:`46167` - esp32: Unable to select GPIO for PWM LED driver channel
- :github:`46170` - ipc_service: open-amp backend may never leave
- :github:`46173` - nRF UART callback is not passed correct index via evt->data.rx.offset sometimes
- :github:`46186` - ISO Broadcaster fails silently on unsupported RTN/SDU_Interval combination
- :github:`46199` - LIS2DW12 I2C driver uses invalid write command
- :github:`46206` - it8xxx2_evb: tests/kernel/fatal/exception/ assertion failed -- "thread was not aborted"
- :github:`46208` - it8xxx2_evb: tests/kernel/sleep failed, elapsed_ms = 2125
- :github:`46234` - samples: lsm6dso: prints incorrect anglular velocity units
- :github:`46235` - subsystem: Bluetooth LLL: ASSERTION FAIL [!link->next]
- :github:`46255` - imxrt1010 wrong device tree addresses
- :github:`46263` - Regulator Control

Addressed issues
================

* :github:`46241` - Bluetooth: Controller: ISO: Setting CONFIG_BT_CTLR_ISO_TX_BUFFERS=4 breaks non-ISO data
* :github:`46140` - Custom driver offload socket creation failing
* :github:`46138` - Problem with building zephyr/samples/subsys/mgmt/mcumgr/smp_svr  using atsame70
* :github:`46137` - RFC: Integrate u8g2 monochrome graphcial library as module to Zephyr OS (https://github.com/olikraus/u8g2)
* :github:`46129` - net: lwm2m: Object Update Callbacks
* :github:`46102` - samples: net: W5500 implementation
* :github:`46097` - b_l072z_lrwan1 usart dma doesn't work
* :github:`46093` - get a run error "Fatal exception (28): LoadProhibited" while enable CONFIG_NEWLIB_LIBC=y
* :github:`46091` - samples: net: cloud: tagoio: Drop pinmux dependency
* :github:`46059` - LwM2M: Software management URI resource not updated properly
* :github:`46056` - ``unexpected eof`` with twister running ``tests/subsys/logging/log_api/logging.log2_api_immediate_printk_cpp`` on ``qemu_leon3``
* :github:`46037` - ESP32 :  fails to build the mcuboot, zephyr v3.1.0 rc2,  sdk 0.14.2
* :github:`46034` - subsys settings: should check the return value of function cs->cs_itf->csi_load(cs, &arg).
* :github:`46033` - twister: incorrect display of test results
* :github:`46027` - tests: rpi_pico tests fails on twister with: No rule to make target 'bootloader/boot_stage2.S
* :github:`46026` - Bluetooth: Controller: llcp: Wrong effective time calculation if PHY changed
* :github:`46023` - drivers: reset: Use of reserved identifier ``assert``
* :github:`46020` - module/mcuboot: doesn't build with either RSA or ECISE-X25519 image encryption
* :github:`46017` - Apply for contributor
* :github:`46002` - NMP timeout when i am using  any mcumgr command
* :github:`45996` - stm32F7: DCache configuration is not correctly implemented
* :github:`45948` - net: socket: dtls: sendmsg() should result in single outgoing datagram
* :github:`45946` - net: context: outgoing datagrams are truncated if not enough memory was allocated
* :github:`45942` - tests: twister: harness: Test harness report pass when there is no console output
* :github:`45933` - webusb sample code linking error for esp32 board
* :github:`45932` - tests: subsys/logging/log_syst : failed to build on rpi_pico
* :github:`45916` - USART on STM32: Using same name for different remapping configurations
* :github:`45911` - LVGL sample cannot be built with CONFIG_LEGACY_INCLUDE_PATH=n
* :github:`45904` - All tests require full timeout period to pass after twister overhaul when executed on HW platform
* :github:`45894` - up_squared：the test shows pass in the twister.log it but does not seem to finish
* :github:`45893` - MCUboot authentication failure with RSA-3072 key on i.MX RT 1160 EVK
* :github:`45886` - ESP32: PWM parameter renaming broke compilation
* :github:`45883` - Bluetooth: Controller: CCM reads data before Radio stores them when DF enabled on PHY 1M
* :github:`45882` - Zephyr minimal C library contains files licensed with BSD-4-Clause-UC
* :github:`45878` - doc: release: Update release notes with CVE
* :github:`45876` - boards: h747/h745: Update dual core flash and debug instructions
* :github:`45875` - bluetooth: hci_raw: avoid possible memory overflow in bt_buf_get_tx()
* :github:`45873` - soc: esp32: use PYTHON_EXECUTABLE from build system
* :github:`45872` - ci: make git credentials non-persistent
* :github:`45871` - ci: split Bluetooth workflow
* :github:`45870` - drivers: virt_ivshmem: Allow multiple instances of ivShMem devices
* :github:`45869` - doc: update requirements
* :github:`45865` - CODEOWNERS has errors
* :github:`45862` - USB ECM/RNDIS Can't receive broadcast messages
* :github:`45856` - blinky built with asserts on arduino nano
* :github:`45855` - Runtime fault when running with CONFIG_NO_OPTIMIZATIONS=y
* :github:`45854` - Bluetooth: Controller: llcp: Assert if LL_REJECT_IND PDU received while local and remote control procedure is pending
* :github:`45851` - For native_posix programs, k_yield doesn't yield to k_msleep threads
* :github:`45839` - Bluetooth: Controller: df: Possible memory overwrite if requested number of CTE is greater than allowed by configuration
* :github:`45836` - samples: Bluetooth: unicast_audio_server invalid check for ISO flags
* :github:`45834` - SMP Server Sample needs ``-DDTC_OVERLAY_FILE=usb.overlay`` for CDC_ACM
* :github:`45828` - mcumgr: img_mgmt_dfu_stopped is called on a successful erase
* :github:`45827` - bluetooth: bluetooth host: Adding the same device to resolving list
* :github:`45826` - Bluetooth: controller: Assert in lll.c when executing LL/CON/INI/BV-28-C
* :github:`45821` - STM32U5: clock_control: Issue to get rate of alt clock source
* :github:`45820` - bluetooth: host: Failed to set security right after reconnection with bonded Central
* :github:`45800` - Clock control settings for MCUX Audio Clock are Incorrect
* :github:`45799` - LED strip driver flips colors on stm32h7
* :github:`45795` - driver: pinctrl: npcx: get build error when apply pinctrl mechanism to a DT node without reg prop.
* :github:`45791` - drivers/usb: stm32: Superfluous/misleading Kconfig option
* :github:`45790` - drivers: can: stm32h7: wrong minimum timing values
* :github:`45784` - nominate me as zephyr contributor
* :github:`45783` - drivers/serial: ns16550: message is garbled
* :github:`45779` - Implementing ARCH_EXCEPT on Xtensa unmasks nested interrupt handling bug
* :github:`45778` - Unable to use thread aware debugging with STM32H743ZI
* :github:`45761` - MCUBoot with multi-image support on Zephyr project for i.MX RT1165 EVK
* :github:`45755` - ESP32 --defsym:1: undefined symbol \`printf' referenced in expression - using CONFIG_NEWLIB_LIBC
* :github:`45750` - tests-ci : kernel: timer: tickless test_sleep_abs Failed
* :github:`45751` - tests-ci : drivers: counter: basic_api test_multiple_alarms  Failed
* :github:`45739` - stm32h7: DCache configuration is not correctly implemented
* :github:`45735` - Ethernet W5500 Driver via SPI is deadlocking
* :github:`45725` - Bluetooth: Controller: df: CTE request not disabled if run in single shot mode
* :github:`45714` - Unable to get TCA9548A to work
* :github:`45713` - twister: map generation fails
* :github:`45708` - Bluetooth: Controller: llcp: CTE request control procedure has missing support for LL_UNKNOWN_RSP
* :github:`45706` - tests: error_hook: mismatch testcases in testplan.json
* :github:`45702` - Reboot instead of halting the system
* :github:`45697` - RING_BUF_DECLARE broken for C++
* :github:`45691` - missing testcase tests/drivers/watchdog on nucleo stm32 boards
* :github:`45686` - missing testcase samples/drivers/led_pwm on nucleo stm32 boards
* :github:`45672` - Bluetooth: Controller: can't cancel periodic advertising sync create betwee ll_sync_create and reception of AUX__ADV_IND with SyncInfo
* :github:`45670` - Intel CAVS: log missing of tests/lib/p4workq/
* :github:`45664` - mqtt_publisher does not work in atsame54_xpro board
* :github:`45648` - pm: device_runtime: API functions fault when PM not supported
* :github:`45632` - ESP32   get error "undefined reference to \`sprintf' "  while CONFIG_NEWLIB_LIBC=y
* :github:`45630` - ipc_service: Align return codes for available backends.
* :github:`45611` - GD32 build failure: CAN_MODE_NORMAL is redefined
* :github:`45593` - tests: newlib:  test_malloc_thread_safety fails on nrf9160dk_nrf9160_ns
* :github:`45583` - Typo in definition of lsm6ds0.h
* :github:`45580` - ESP32-C3: CONFIG_ESP32_PHY_MAX_TX_POWER undeclared error when building with CONFIG_BT=y
* :github:`45578` - cmake: gcc --print-multi-directory doesn't print full path and checks fails
* :github:`45577` - STM32L4: USB MSC doesn't work with SD card
* :github:`45568` - STM32H7xx: Driver for internal flash memory partially uses a fixed flash program word size, which doesn't fit for all STM32H7xx SOCs (e.g. STM32H7A3, STM32H7B0, STM32H7B3) leading to potential flash data corruption
* :github:`45557` - doc: Some generic yaml bindings don't show up in dts/api/bindings.html#dt-no-vendor
* :github:`45549` - bt_gatt_write_without_response_cb doesn't use callback
* :github:`45545` - K_ESSENTIAL option doesn't have any effect on k_create_thread
* :github:`45543` - Build samples/bluetooth/broadcast_audio_sink raises an error
* :github:`45542` - Implementing firmware image decompression in img_mgmt_upload()
* :github:`45533` - uart_imx_poll_in() seems to be a blocking function
* :github:`45529` - GdbStub get_mem_region bug
* :github:`45518` - LPCXpresso55S69 incorrect device name for JLink runner
* :github:`45514` - UDP Packet socket doesn't do L2 header processing
* :github:`45505` - NXP MIMXRT1050-EVKB: MCUBoot Serial Recover: mcumgr hangs when trying to upload image
* :github:`45488` - Build warnings when no GPIO ports enabled
* :github:`45486` - MCUBootloader can't building for imxrt1160_evk_cm7 core
* :github:`45482` - Adding, building and linking Lua in a project
* :github:`45468` - Is uart_poll_in() blocking or not?
* :github:`45463` - null function pointer called when using shell logger backend under heavy load
* :github:`45458` - it8xxx2_evb: tests/drivers/pwm/pwm_api assertion fail
* :github:`45443` - SAMD21: Wrong voltage reference set by enum adc_reference
* :github:`45440` - Intel CAVS: intel_adsp_hda testsuite is failing due to time out on intel_adsp_cavs15
* :github:`45431` - Bluetooth: Controller: df: Wrong antenna identifier inserted after switch pattern exhausted
* :github:`45426` - Data buffer allocation: TCP stops working
* :github:`45421` - Zephyr build image(sample blinky application) not getting flash through NXP Secure Provisioning Tool V4.0 for i.MX RT 1166EVK
* :github:`45407` - Support for flashing the Zephyr based application on i.MX RT 1160 EVK through SDP Mode(USB-HID/ UART) & PyOCD runner
* :github:`45405` - up_squared: most of the test case timeout
* :github:`45404` - Bluetooth: Controller: Periodic advertising scheduling is broken, TIFS/TMAFS maintenance corrupted
* :github:`45401` - test-ci: adc: lpcxpresso55s28: adc pinctl init error
* :github:`45394` - Bug when sending a BLE proxy mesh msg of length exactly 2x the MTU size
* :github:`45390` - MinGW-w64: Cannot build Zephyr project
* :github:`45395` - Programming NXP i.MX RT OTP fuse with west
* :github:`45372` - PWM not working
* :github:`45371` - frdm_k64f: failed to run test case tests/net/socket/offload_dispatcher
* :github:`45367` - net: tcp: Scheduling dependent throughput
* :github:`45365` - Zephyr IP Stack Leaks in Promiscuous Mode
* :github:`45362` - sample/net/sockets/dumb_http_server not working with enc28j60
* :github:`45361` - samples/bluetooth/hci_usb doesn't build for nucleo_wb55rg
* :github:`45359` - USB DFU sample does not work on RT series boards
* :github:`45355` - Twister fails when west is not present
* :github:`45345` - Make FCB work with sectors larger than 16K
* :github:`45337` - timing: missing extern "C" in timing.h
* :github:`45336` - newlib: PRIx8 inttype incorrectly resolves to ``hh`` with newlib-nano
* :github:`45324` - NET_TCP_BACKLOG_SIZE is unused, it has to be either implemented or deleted
* :github:`45322` - tests: drivers: pwm_api fails with stm32 devices
* :github:`45316` - drivers: timer: nrf_rtc_timer: SYS_CLOCK_TICKS_PER_SEC too high for when CONFIG_KERNEL_TICKLESS=n
* :github:`45314` - subsystem: Bluetooth LLL: ASSERTION FAIL [!link->next] @ ZEPHYR_BASE/subsys/bluetooth/controller/ll_sw/ull_conn.c:1952
* :github:`45303` - drivers: can: CAN classic and CAN-FD APIs are mixed together and CAN-FD is a compile-time option
* :github:`45302` - Bus Fault with Xilinx UART Lite
* :github:`45280` - GPIO Configuration Issue
* :github:`45278` - twister: Run_id check feature breaks workflows with splitted building and testing.
* :github:`45276` - Add support for multiple zero-latency irq priorities
* :github:`45268` - Error newlibc ESP32
* :github:`45267` - kernel: Recursive spinlock in k_msgq_get() in the context of a k_work_poll handler
* :github:`45266` - teensy41: pwm sample unable to build
* :github:`45261` - mcumgr: conversion of version to string fails (snprintf format issue)
* :github:`45248` - Avoid redefining 32-bit integer types like __UINT32_TYPE__
* :github:`45237` - RFC: API Change: Bluetooth - replace callback in bt_gatt_subscribe_param
* :github:`45229` - sample: spi: bitbang: spi_bitbang sample has improper definition of its test
* :github:`45226` - samples/drivers/led_pwm: Build failure
* :github:`45219` - drivers: can: transceivers are initialized after controllers
* :github:`45209` - Minimal LIBC missing macros
* :github:`45189` - sam_e70b_xplained: failed to run test case tests/benchmarks/cmsis_dsp/basicmath
* :github:`45186` - Building Zephyr on Ubuntu fails when ZEPHYR_TOOLCHAIN_VARIANT is set to llvm
* :github:`45185` - Intel CAVS: tests under tests/ztest/register/ are failing
* :github:`45182` - MCUBoot Usage Fault on RT1060 EVK
* :github:`45172` - Bluetooth: attr->user_data is NULL when doing discovery with BT_GATT_DISCOVER_ATTRIBUTE
* :github:`45155` - STM32 serial port asynchronous initialization TX DMA channel error
* :github:`45152` - ``tests/subsys/logging/log_stack`` times out on ``qemu_arc_hs6x`` with twister
* :github:`45129` - mimxrt1050_evk: GPIO button pushed only once
* :github:`45123` - driver: can_stm32fd: STM32U5 series support
* :github:`45118` - Error claiming older doc is the latest
* :github:`45112` - Cannot install watchdog timeout on STM32WB
* :github:`45111` - fvp_base_revc_2xaemv8a: multiple test failures
* :github:`45110` - fvp_baser_aemv8r_smp: multiple test failures
* :github:`45108` - fvp_baser_aemv8r: multiple test failures
* :github:`45089` - stm32: usart: rx pin inversion missing
* :github:`45073` - nucleo_h743zi  failing twister builds due to NOCACHE_MEMORY warning
* :github:`45072` - [Coverity CID: 248346] Copy into fixed size buffer in /subsys/bluetooth/shell/bt.c
* :github:`45045` - mec172xevb_assy6906: tests/arch/arm/arm_irq_vector_table failed to run
* :github:`45012` - sam_e70b_xplained: failed to run test case tests/drivers/can/timing/drivers.can.timing
* :github:`45009` - twister: many tests failed with "mismatch error" after met a SerialException.
* :github:`45008` - esp32: i2c_read() error was returned successfully at the bus nack
* :github:`45006` - Bluetooth HCI SPI fault
* :github:`44997` - zcbor build error when ZCBOR_VERBOSE is set
* :github:`44985` - tests: drivers: can: timing: failure to set bitrate of 800kbit/s on nucleo_g474re
* :github:`44977` - samples: modules: canopennode: failure to initialize settings subsystem on nucleo_g474re
* :github:`44966` - build fails for nucleo wb55 rg board.
* :github:`44956` - Deprecate the old spi_cs_control fields
* :github:`44947` - cmsis_dsp: matrix: error during building libraries.cmsis_dsp.matrix.unary_f64 for qemu_cortex_m3
* :github:`44940` - rom_report creates two identical identifier but for different path in rom.json
* :github:`44938` - Pin assignments SPIS nrf52
* :github:`44931` - Bluetooth: Samples: broadcast_audio_source stack overflow
* :github:`44927` - Problems in using STM32 Hal Library
* :github:`44926` - intel_adsp_cavs25: can not build multiple tests under tests/posix/ and tests/lib/newlib/
* :github:`44921` - Can't run hello_world using mps_an521_remote
* :github:`44913` - Enabling BT_CENTRAL breaks MESH advertising
* :github:`44910` - Issue when installing Python additional dependencies
* :github:`44904` - PR#42879 causes a hang in the shell history
* :github:`44902` - x86: FPU registers are not initialised for userspace (eager FPU sharing)
* :github:`44887` - it8xxx2_evb: tests/kernel/sched/schedule_api/ assertion fail
* :github:`44886` - Unable to boot Zephyr on FVP_BaseR_AEMv8R
* :github:`44882` - doc: Section/chapter "Supported Boards" missing from pdf documentation
* :github:`44874` - error log for locking a mutex in an ISR
* :github:`44872` - k_timer callback timing incorrect with multiple lightly loaded cores
* :github:`44871` - mcumgr endless loop in mgmt_find_handler
* :github:`44864` - tcp server tls error：server has no certificate
* :github:`44856` - Various kernel timing-related tests fail on hifive1 board
* :github:`44837` - drivers: can: mcp2515: can_set_timing() performs a soft-reset of the MCP2515, discarding configured mode
* :github:`44834` - Add support for gpio expandeux NXP PCAL95xx
* :github:`44831` - west flash for nucleo_u575zi_q is failing
* :github:`44830` - Unable to set compiler warnings on app exclusively
* :github:`44822` - STM32F103 Custom Board Clock Config Error
* :github:`44811` - STRINGIFY does not work with mcumgr
* :github:`44798` - promote Michael to the Triage permission level
* :github:`44797` - x86: Interrupt handling not working for cores <> core0 - VMs not having core 0 assigned cannot handle IRQ events.
* :github:`44778` - stdint types not recognized in soc_common.h
* :github:`44777` - disco_l475_iot1 default CONFIG_BOOT_MAX_IMG_SECTORS should be 512 not 256
* :github:`44758` - intel_adsp: kernel.common tests are failing
* :github:`44752` - Nominate @brgl as contributor
* :github:`44750` - Using STM32 internal ADC with interrupt:
* :github:`44737` - Configurable LSE driving capability on H735
* :github:`44734` - regression in GATT/SR/GAS/BV-06-C qualification test case
* :github:`44731` - mec172xevb_assy6906: test/drivers/adc/adc_api test case build fail
* :github:`44730` - zcbor ARRAY_SIZE conflict with zephyr include
* :github:`44728` - Fresh Build and Flash of Bluetooth Peripheral Sample Produces Error on P-Nucleo-64 Board (STM32WBRG)
* :github:`44724` - can: drivers: mcux: flexcan: correctly handle errata 5461 and 5829
* :github:`44722` - lib: posix: support for pthread_attr_setstacksize
* :github:`44721` - drivers: can: mcan: can_mcan_add_rx_filter() unconditionally adds offset for extended CAN-ID filters
* :github:`44706` - drivers: can: mcp2515: mcp2515_set_mode() silently ignores unsupported modes
* :github:`44705` - Windows getting started references wget usage without step for installing wget
* :github:`44704` - Bootloader linking error while building for RPI_PICO
* :github:`44701` - advertising with multiple advertising sets fails with BT_HCI_ERR_MEM_CAPACITY_EXCEEDED
* :github:`44691` - west sign fails to find header size or padding
* :github:`44690` - ST kit b_u585i_iot02a and OCTOSPI flash support
* :github:`44687` - drivers: can: missing syscall verifier for can_get_max_filters()
* :github:`44680` - drivers: can: mcux: flexcan: can_set_mode() resets IP, discarding installed RX filters
* :github:`44678` - mcumgr: lib: cmd: img_mgmt: Warning about struct visibility emitted with certain Kconfig options
* :github:`44676` - mimxrt1050_evk_qspi crash or freeze when accessing flash
* :github:`44670` - tests-ci : kernel: tickless: concept test Timeout
* :github:`44671` - tests-ci : kernel: scheduler: deadline test failed
* :github:`44672` - tests-ci : drivers: counter: basic_api test failed
* :github:`44659` - Enhancement to k_thread_state_str()
* :github:`44621` - ASCS: Sink ASE stuck in Releasing state
* :github:`44600` - NMI testcase fails on tests/arch/arm/arm_interrupt with twister
* :github:`44586` - nrf5340: Random crashes when a lot of interrupts is triggered
* :github:`44584` - SWO log output does not compile for STM32WB55
* :github:`44573` - Do we have complete RNDIS stack available for STM32 controller in zephyr ?
* :github:`44558` - Possible problem with timers
* :github:`44557` - tests: canbus: isotp: implementation: fails on mimxrt1024_evk
* :github:`44553` - General Question: Compilation Time >15 Minutes?
* :github:`44546` - Bluetooth: ISO: Provide stream established information
* :github:`44544` - shell_module/sample.shell.shell_module.usb fails for thingy53_nrf5340_cpuapp_ns
* :github:`44539` - twister fails on several stm32 boards with tests/arch/arm testcases
* :github:`44535` - mgmt/mcumgr/lib: Incorrect use of MGMT_ERR_ENOMEM, in most cases where it is used
* :github:`44531` - bl654_usb without mcuboot maximum image size is not limited
* :github:`44530` - xtensa xcc build usb stack fail (newlib)
* :github:`44519` - Choosing CONFIG_CHIP Kconfig breaks LwM2M client client example build
* :github:`44507` - net: tcp: No retries of a TCP FIN message
* :github:`44504` - net: tcp: Context still open after timeout on connect
* :github:`44497` - Add guide for disabling MSD on JLink OB devices and link to from smp_svr page
* :github:`44495` - sys_slist_append_list and sys_slist_merge_slist corrupt target slist if appended or merged list is empty
* :github:`44489` - Docs: missing documentation related to MCUBOOT serial recovery feature
* :github:`44488` - Self sensor library from private git repository
* :github:`44486` - nucleo_f429zi: multiple networking tests failing
* :github:`44484` - drivers: can: mcp2515: The MCP2515 driver uses wrong timing limits
* :github:`44483` - drivers: can: mcan: data phase prescaler bounds checking uses wrong value
* :github:`44482` - drivers: can: mcan: CAN_SJW_NO_CHANGE not accepted with CONFIG_ASSERT=y
* :github:`44480` - bt_le_adv_stop null pointer exception
* :github:`44478` - Zephyr on Litex/Vexriscv not booting
* :github:`44473` - net: tcp: Connection does not properly terminate when connection is lost
* :github:`44453` - Linker warnings in watchdog samples and tests built for twr_ke18f
* :github:`44449` - qemu_riscv32 DHCP fault
* :github:`44439` - Bluetooth: Controller: Extended and Periodic Advertising HCI Component Conformance Test Coverage
* :github:`44427` - SYS_CLOCK_HW_CYCLES_PER_SEC not correct for hifive1_revb / FE310
* :github:`44404` - Porting stm32h745 for zephyr
* :github:`44397` - twister: test case error number discrepancy in the result
* :github:`44391` - tests-ci : peripheral: gpio: 1pin test Timeout
* :github:`44438` - tests-ci : arch: interrupt: arm.nmi test Unknown
* :github:`44386` - Zephyr SDK 0.14.0 does not contain a sysroots directory
* :github:`44374` - Twister: Non-intact handler.log files when running tests and samples folders
* :github:`44361` - drivers: can: missing syscall verifier for can_set_mode()
* :github:`44349` - Nordic BLE fails assertion when logging is enabled
* :github:`44348` - drivers: can: z_vrfy_can_recover() does not compile
* :github:`44347` - ACRN: multiple tests failed due to incomplete log
* :github:`44345` - drivers: can: M_CAN bus recovery function has the wrong signature
* :github:`44344` - drivers: can: mcp2515 introduces a hard dependency on CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
* :github:`44338` - intel_adsp_cavs18: multiple tests failed due to non-intact log
* :github:`44314` - rddrone_fmuk66: fatal error upon running basic samples
* :github:`44307` - LE Audio: unicast stream/ep or ACL disconnect reset should not terminate the CIG
* :github:`44296` - Bluetooth: Controller: DF: IQ sample of CTE signals are not valid if PHY is 1M
* :github:`44295` - Proposal for subsystem for media
* :github:`44284` - LE Audio: Missing recv_info for BAP recv
* :github:`44283` - Bluetooth: ISO: Add TS flag for ISO receive
* :github:`44274` - direction_finding_connectionless_rx/tx U-Blox Nora B106 EVK
* :github:`44271` - mgmt/mcumgr: BT transport: Possible buffer overflow (and crash) when reciving SMP when CONFIG_MCUMGR_BUF_SIZE < transport MTU
* :github:`44262` - mimxrt1050_evk: build time too long for this platform
* :github:`44261` - twister: some changes make test cases work abnormally.
* :github:`44259` - intel_adsp_cavs18: tests/lib/icmsg failed
* :github:`44255` - kernel: While thread is running [thread_state] is in _THREAD_QUEUED
* :github:`44251` - ``CONFIG_USB_DEVICE_REMOTE_WAKEUP`` gets default value `y` if not set
* :github:`44250` - Can't build WiFi support on esp32, esp32s2, esp32c3
* :github:`44247` - west build -b nrf52dk_nrf52832 samples/boards/nrf/clock_skew failed
* :github:`44244` - Bluetooth: Controller: ISO BIS payload counter rollover
* :github:`44240` - tests: drivers: pwm_api: PWM driver test doesn't compile for mec172xevb_assy6906
* :github:`44239` - boards: arm: mec152x/mec172x CONFIG_PWM=y doesn't compile PWM driver
* :github:`44231` - Problems trying to configure the environment
* :github:`44218` - libc: minimal: qsort_r not working as expected
* :github:`44216` - tests: drivers: counter_basic_api: Build failing on LPCxpresso55s69_cpu
* :github:`44215` - tests: subsys: cpp: over half of tests failing on macOS but do not fail on Linux
* :github:`44213` - xtensa arch_cpu_idle not correct on cavs18+ platforms
* :github:`44199` - (U)INT{32,64}_C macro constants do not match the Zephyr stdint types
* :github:`44192` - esp32 flash custom partition table
* :github:`44186` - Possible race condition in TCP connection establishment
* :github:`44145` - Zephyr Panic dump garbled on Intel cAVS platforms
* :github:`44134` - nRF52833 current consumption too high
* :github:`44128` - Deprecate DT_CHOSEN_ZEPHYR_FLASH_CONTROLLER_LABEL
* :github:`44125` - drivers/ethernet/eth_stm32_hal.c: eth_stm32_hal_set_config() always returns -ENOTSUP (-134)
* :github:`44110` - Bluetooth: synced callback may have wrong addr type
* :github:`44109` - Device tree error while porting zephyr for a custom board
* :github:`44108` - ``CONFIG_ZTEST_NEW_API=y`` broken with ``CONFIG_TEST_USERSPACE=y``
* :github:`44107` - The SMP nsim boards are started incorrectly when launching on real HW
* :github:`44106` - test of dma drivers fails on dma_m2m_loop_test
* :github:`44101` - a build error when CONFIG_MULTITHREADING=n
* :github:`44092` - rand32_ctr_drbg fails to call the respective initialization routing
* :github:`44089` - logging: shell backend: null-deref when logs are dropped
* :github:`44072` - mcumgr smp source is checking variable without it being set and causing automated test failures
* :github:`44070` - west spdx TypeError: 'NoneType' object is not iterable
* :github:`44043` - Usage fault when running flash shell sample on RT1064 EVK
* :github:`44029` - Unexpected behavior of CONFIG_LOG_OVERRIDE_LEVEL
* :github:`44018` - net: tcp: Running out of buffers by packet loss
* :github:`44012` - net: tcp: Cooperative scheduling transfer size limited
* :github:`44010` - frdm_k64f: failed to run testcase samples/kernel/metairq_dispatch/
* :github:`44006` - intel_adsp_cavs25: tests/drivers/dma/loop_transfer failed
* :github:`44004` - Bluetooth: ascs: Invalid ASE state transition: Releasing -> QoS Configured
* :github:`43993` - doc: Fix minor display issue for west spdx extension command
* :github:`43990` - How to make civetweb run on a specified network card
* :github:`43988` - Extracting the index of a child node referenced using alias
* :github:`43980` - No PWM signal on Nucleo F103RB using TIM1 CH2 PA9
* :github:`43976` - [lwm2m_engine / sockets] Possibility to decrease timeout on connect()
* :github:`43975` - tests: kernel: scheduler: Test from kernel.scheduler.slice_perthread fails on some nrf platforms
* :github:`43972` - UART: uart_poll_in() not working in Shell application
* :github:`43964` - k_timer callback timing gets unreliable with more cores active
* :github:`43950` - code_relocation: Add NOCOPY feature breaks windows builds
* :github:`43949` - drivers: espi: mec172x: ESPI flash write and erase operations not working
* :github:`43948` - drivers: espi: xec: MEC172x: Driver enables all bus interrupts but doesn't handle them causing starvation
* :github:`43946` - Bluetooth: Automatic ATT MTU negotiation
* :github:`43940` - Support for CH32V307 devices
* :github:`43930` - nRF52833 High Power Consumption with 32.768kHz RC Oscillator
* :github:`43924` - ipc_service: Extend API with zero-copy send
* :github:`43899` - can: stm32: Build issue on g4 target
* :github:`43898` - Twister:  test case number discrepancy in the result xml.
* :github:`43891` - networking: detect initialisation failures of backing drivers
* :github:`43888` - adc: stm32: compilation broken on G4 targets
* :github:`43874` - mec172xevb_assy6906: tests/drivers/spi/spi_loopback  test case UART output wrong.
* :github:`43873` - tests:ci:lpcxpresso55s06: portability.posix.common.newlib meet hard fault
* :github:`43872` - tests:ci:lpcxpresso55s06:libraries.cmsis_dsp.matrix.unary_f32 test fails
* :github:`43870` - test:ci:lpcxpresso55s06: hwinfo test meet hardfault
* :github:`43867` - mec172xevb_assy6906: tests/drivers/pwm/pwm_api  test case build fail.
* :github:`43865` - Add APDS-9250 I2C Driver
* :github:`43864` - mec172xevb_assy6906: tests/drivers/pwm/pwm_loopback  test case failed to build
* :github:`43858` - mcumgr seems to lock up when it receives command for group that does not exist
* :github:`43856` - mec172xevb_assy6906: tests/drivers/i2c/i2c_api  i2c_test failed
* :github:`43851` - LE Audio: Make PACS location optional
* :github:`43838` - mec172xevb_assy6906: tests/drivers/adc/adc_dma  test case build fail
* :github:`43842` - tests-ci : libraries: encoding: jwt test Timeout
* :github:`43841` - tests-ci : net: socket: tls.preempt test Timeout
* :github:`43835` - ``zephyr_library_compile_options()`` fails to apply if the same setting is set for multiple libraries in a single project
* :github:`43834` - DHCP not work in ``Intel@PSE`` on ``Intel@EHL``
* :github:`43830` - LPC55S69 Not flashing to second core.
* :github:`43829` - http_client: http_client_req() returns incorrect number of bytes sent
* :github:`43818` - lib: os: ring_buffer: recent changes cause UART shell to fail on qemu_cortex_a9
* :github:`43816` - tests: cmsis_dsp: rf16 and cf16 tests are not executed on Native POSIX
* :github:`43807` - Test "cpp.libcxx.newlib.exception" failed on platforms which use zephyr.bin to run tests.
* :github:`43794` - BMI160 Driver: Waiting time between SPI activation and reading CHIP IP is too low
* :github:`43793` - Alllow callbacks to CDC_ACM events
* :github:`43792` - mimxrt1050_evk: failed to run tests/net/socket/tls and tests/subsys/jwt
* :github:`43786` - [Logging] log context redefined with XCC when use zephyr logging api with SOF
* :github:`43757` - it8xxx2_evb: k_busy_wait is not working accurately for ITE RISC-V
* :github:`43756` - drivers: gpio: pca95xx does not compile with CONFIG_GPIO_PCA95XX_INTERRUPT
* :github:`43750` - ADC Driver build is broken for STM32L412
* :github:`43745` - Xtensa XCC Build spi_nor.c fail
* :github:`43742` - BT510 lis2dh sensor does not disconnect SAO pull-up resistor
* :github:`43739` - tests: dma: random failure on dma loopback suspend and resume case on twr_ke18f
* :github:`43732` - esp32: MQTT publisher sample stuck for both TLS and non-TLS sample.
* :github:`43728` - esp32 build error while applicaton in T2 topology
* :github:`43718` - Bluetooth: bt_conn: Unable to allocate buffer within timeout
* :github:`43715` - ESP32 UART devicetree binding design issue
* :github:`43713` - intel_adsp_cavs: tests are not running with twister
* :github:`43711` - samples: tfm: psa Some TFM/psa samples fail on nrf platforms
* :github:`43702` - samples/arch/smp/pktqueue not working on ESP32
* :github:`43700` - mgmt/mcumgr: Strange Kconfig names for MCUMGR_GRP_ZEPHYR_BASIC log levels
* :github:`43699` - Bluetooth Mesh working with legacy and extended advertising simultaneously
* :github:`43693` - LE Audio: Rename enum bt_audio_pac_type
* :github:`43669` - LSM6DSL IMU driver - incorrect register definitions
* :github:`43663` - stm32f091 test   tests/kernel/context/ test_kernel_cpu_idle  fails
* :github:`43661` - Newlib math library not working with user mode threads
* :github:`43656` - samples:bluetoooth:direction_finding_connectionless_rx antenna switching not working with nRF5340
* :github:`43654` - Nominate Mehmet Alperen Sener as Bluetooth Mesh Collaborator
* :github:`43649` - Best practice for "external libraries" and cmake
* :github:`43647` - Bluetooth: LE multirole: connection as central is not totally unreferenced on disconnection
* :github:`43640` - stm32f1: Convert ``choice GPIO_STM32_SWJ`` to dt
* :github:`43636` - Documentation incorrectly states that C++ new and delete operators are unsupported
* :github:`43630` - Zperf tcp download stalls with window size becoming 0 on Zephyr side
* :github:`43618` - Invalid thread indexes out of userspace generation
* :github:`43600` - tests: mec15xxevb_assy6853: most of the test cases failed
* :github:`43587` - arm: trustzone: Interrupts using FPU causes usage fault when ARM_NONSECURE_PREEMPTIBLE_SECURE_CALLS is disabled
* :github:`43580` - hl7800: tcp stack freezes on slow response from modem
* :github:`43573` - return const struct device \* for device_get_binding(const char \*name)
* :github:`43568` - ITE eSPI driver expecting OOB header also along with OOB data from app code - espi_it8xxx2_send_oob() & espi_it8xxx2_receive_oob
* :github:`43567` - Bluetooth: Controller: ISO data packet dropped on payload array wraparound
* :github:`43553` - Request to configure SPBTLE-1S of STEVAL-MKSBOX1V1
* :github:`43552` - samples: bluetooth: direction_finding: Sample fails on nrf5340
* :github:`43543` - RFC: API Change: Bluetooth: struct bt_auth_cb field removal
* :github:`43525` - Default network interface selection by up-state
* :github:`43518` - 'DT_N_S_soc_S_timers_40012c00_S_pwm' undeclared
* :github:`43513` - it8xxx2_evb: tests/kernel/sleep failed
* :github:`43512` - wifi: esp_at: sockets not cleaned up on close
* :github:`43511` - lvgl: upgrade to 8.2 build problem
* :github:`43505` - ``py`` command not found when using nanopb on windows
* :github:`43503` - Build Version detection not working when Zephyr Kernel is a Git Submodule
* :github:`43490` - net: sockets: userspace accept() crashes with NULL addr/addrlen pointer
* :github:`43487` - LE Audio: Broadcast audio sample
* :github:`43476` - tests: nrf: Output of nrf5340dk_nrf5340_cpuapp_ns not available
* :github:`43470` - wifi: esp_at: race condition on mutex's leading to deadlock
* :github:`43469` - USBD_CLASS_DESCR_DEFINE section name bug
* :github:`43465` - 'Malformed data' on bt_data_parse() for every ble adv packet on bbc_microbit
* :github:`43456` - winc1500 wifi driver fails to build
* :github:`43452` - Missing SPI SCK on STM32F103vctx
* :github:`43448` - Deadlock detection in ``bt_att_req_alloc`` ineffective when ``CONFIG_BT_RECV_IS_RX_THREAD=n``
* :github:`43440` - Bluetooth: L2CAP send le data lack calling net_buf_unref() function
* :github:`43430` - Is there any plan to develop zephyr to mircrokenrel architecture?
* :github:`43425` - zephyr+Linux+hypervisor on Raspberry Pi 4
* :github:`43419` - Pull request not updated after force push the original branch
* :github:`43411` - STM32 SPI DMA issue
* :github:`43409` - frdm_k64f: USB connection gets lost after continuous testing
* :github:`43400` - nrf board system_off sample application does not work on P1 buttons
* :github:`43392` - Bluetooth: ISO: unallocated memory written during mem_init
* :github:`43389` - LoRaWAN on Nordic and SX1276 & SX1262 Shield
* :github:`43382` - mgmt/mcumgr/lib: Echo OS command echoes back empty string witn no error when string is too long to handle
* :github:`43378` - TLS availability misdetection when ZEPHYR_TOOLCHAIN_VARIANT is not set
* :github:`43372` - pm: lptim: stm32h7: pending irq stops STANDBY
* :github:`43369` - Use Zephyr crc implementation for LittleFS
* :github:`43359` - Bluetooth: ASCS QoS config should not fail for preferred settings
* :github:`43348` - twister:skipped case num issue when use --only-failed.
* :github:`43345` - Bluetooth: Controller: Extended and Periodic Advertising Link Layer Component Test Coverage
* :github:`43344` - intel_adsp_cavs25: samples/subsys/logging/syst is failing with a timeout when the sample is enabled to run on intel_adsp_cavs25
* :github:`43333` - RFC: Bring zcbor as CBOR decoder/encoder in replacement for TinyCBOR
* :github:`43326` - Unstable SD Card performance on Teensy 4.1
* :github:`43319` - Hardware reset cause api sets reset pin bit everytime the api is called
* :github:`43316` - stm32wl55 cannot enable PLL source as MSI
* :github:`43314` - LE Audio: BAP ``sent`` callback missing
* :github:`43310` - disco_l475_iot1: BLE not working
* :github:`43306` - sam_e70b_xplained: the platform will be not normal after running test case tests/subsys/usb/desc_sections/
* :github:`43305` - wifi: esp_at: shell command "wifi scan" not working well
* :github:`43295` - mimxrt685_evk_cm33: Hard fault with ``CONFIG_FLASH=y``
* :github:`43292` - NXP RT11xx devicetree missing GPIO7, GPIO8, GPIO12
* :github:`43285` - nRF5x System Off demo fails to put the nRF52840DK into system off
* :github:`43284` - samples: drivers: watchdog failed in mec15xxevb_assy6853
* :github:`43277` - usb/dfu: upgrade request is not called while used from mcuboot, update doesn't happen
* :github:`43276` - tests: up_squared:  testsuite tests/kernel/sched/deadline/ failed
* :github:`43271` - tests: acrn_ehl_crb:  tests/arch/x86/info failed
* :github:`43268` - LE Audio: Add stream ops callbacks for unicast server
* :github:`43258` - HCI core data buffer overflow with ESP32-C3 in Peripheral HR sample
* :github:`43248` - Bluetooth: Mesh: Unable used with ext adv on native_posix
* :github:`43235` - STM32 platform does not handle large i2c_write() correctly
* :github:`43230` - Deprecate DT_CHOSEN_ZEPHYR_ENTROPY_LABEL
* :github:`43229` - nvs: change nvs_init to accept a device reference
* :github:`43218` - nucleo_wb55rg: Partition update required to use 0.13.0 BLE firmware
* :github:`43205` - UART console broken since 099850e916ad86e99b3af6821b8c9eb73ba91abf
* :github:`43203` - BLE: With BT_SETTINGS and BT_SMP, second connection blocks the system in connection event notification
* :github:`43192` - lvgl: upgrade LVGL to 8.1 build error
* :github:`43190` - Bluetooth: audio: HCI command timeout on LE Setup Isochronous Data Path
* :github:`43186` - Bluetooth: import nrf ble_db_discovery library to zephyr
* :github:`43172` - CONFIG_BT_MESH_ADV_EXT doesn't build without CONFIG_BT_MESH_RELAY
* :github:`43163` - Applications not pulling LVGL cannot be configured or compiled
* :github:`43159` - hal: stm32: ltdc pins should be very-high-speed
* :github:`43142` - Ethernet and PPP communication conflicts
* :github:`43136` - STM32 Uart log never take effect
* :github:`43132` - Thingy:52 i2c_nrfx_twim: Error 0x0BAE0001 occurred for message
* :github:`43131` - LPCXPresso55S69-evk dtsi file incorrect
* :github:`43130` - STM32WL ADC idles / doesn't work
* :github:`43117` - Not possible to create more than one shield.
* :github:`43109` - drivers:peci:xec: PECI Command 'Ping' does not work properly
* :github:`43099` - CMake: ARCH roots issue
* :github:`43095` - Inconsistent logging config result resulted from menuconfig.
* :github:`43094` - CMake stack overflow after changing the build/zephyr/.config, even just timestamp.
* :github:`43090` - mimxrt685_evk_cm33: USB examples not working on Zephyr v3.0.0
* :github:`43087` - XCC build failures for all intel_adsp tests/platforms
* :github:`43081` - [Slack] Slack invite works only on very few mail addresses - this should be changed!
* :github:`43066` - stm32wl55 true RNG  falls in seed error
* :github:`43058` - PACS: Fix PAC capabilities to be exposed in PAC Sink/Source characteristic
* :github:`43057` - twister: error while executing twister script on windows machine for sample example code
* :github:`43046` - Wifi sample not working with disco_l475_iot1
* :github:`43034` - Documentation for ``console_putchar`` function is incorrect
* :github:`43024` - samples: tests task wdt fails on some stm32 nucleo target boards
* :github:`43020` - samples/subsys/fs/littlefs does not work with native_posix board on WSL2
* :github:`43016` - Self inc/dec works incorrectly with logging API.
* :github:`42997` - Bluetooth: Controller: Receiving Periodic Advertising Reports with larger AD Data post v3.0.0-rc2
* :github:`42988` - Specify and standardize undefined behavior on empty response from server for http_client
* :github:`42960` - Bluetooth: Audio: Codec config parsing and documentation
* :github:`42953` - it8xxx2_evb: Test in tests/kernel/timer/timer_api fail.
* :github:`42940` - Please add zsock_getpeername
* :github:`42928` - CSIS: Invalid usage of bt_conn_auth_cb callbacks
* :github:`42888` - Bluetooth: Controller: Extended Advertising - Advertising Privacy Support
* :github:`42881` - Arduino due missing 'arduino_i2c' alias.
* :github:`42877` -  k_cycle_get_32 returns 0 on start-up on native_posix
* :github:`42874` - ehl_crb: samples/kernel/metairq_dispatch fails when it is run multiple times
* :github:`42870` - Build error due to minimal libc qsort callback cast
* :github:`42865` - openocd configurations missing for stm32mp157c_dk2 board
* :github:`42857` - sam_e70b_xplained: failed to run test cases tests/net/npf and tests/net/bridge
* :github:`42856` - Bluetooth: BAP: Unicast client sample cannot connect
* :github:`42854` - k_busy_wait() never returns when called - litex vexriscv soc and cpu on xilinx ac701 board
* :github:`42851` - it8xxx2_evb: Mutlitple tests in tests/kernel/contex fail.
* :github:`42850` - CONFIG items disappeared in zephyr-3.0-rc3
* :github:`42848` - it8xxx2_evb: Test in /tests/subsys/cpp/libcxx fail.
* :github:`42847` - it8xxx2_evb: Multiple tests in tests/subsys/portability/cmsis_rtos_v2 fail.
* :github:`42831` - Do the atomic* functions require protection from optimization?
* :github:`42829` - GATT: bt_gatt_is_subscribed does not work as expected when called from bt_conn_cb->connected
* :github:`42825` - MQTT client disconnection (EAGAIN) on publish with big payload
* :github:`42817` - ADC on ST Nucleo H743ZI board with DMA
* :github:`42800` - gptp_mi neighbor_prop_delay is not included in sync_receipt_time calculation due cast from double to uint64_t
* :github:`42799` - gptp correction field in sync follow up message does not have correct endianness
* :github:`42774` - pinctrl-0 issue in device tree building
* :github:`42723` - tests: kernel.condvar: child thread is not running
* :github:`42702` - upsquared: drivers.counter.cmos.seconds_rate is failing with busted maximum bound when run multiple times
* :github:`42685` - Socket echo server sample code not working in Litex Vexriscv cpu (Xilinx AC701 board)
* :github:`42680` - Missing bt_conn_(un)ref for LE Audio and tests
* :github:`42599` - tests: kernel: mem_protect: mem_protect fails after reset on stm32wb55 nucleo
* :github:`42588` - lsm6dso
* :github:`42587` - LE Audio: BAP Unicast API use array of pointers instead of array of streams
* :github:`42559` - 6LoCAN samples fail due to null pointer dereference
* :github:`42548` - acrn_ehl_crb:  twister failed to run tests/subsys/logging due to UnicodeEncodeError after switching to log v2
* :github:`42544` - Bluetooth: controller: llcp: handling of remote procedures with and without instant
* :github:`42534` - BLE Testing functions do not work properly
* :github:`42530` - Possibility to define pinmux item for Pin Control as a plain input/output
* :github:`42524` - Wrong implementation of SPI driver
* :github:`42520` - bt_ots Doxygen documentation does not seem to be included in the Zephyr project documentation.
* :github:`42518` - Bluetooth Ext Adv:Sync: While simultaneous advertiser are working, and skip is non-zero, sync terminates repeatedly
* :github:`42508` - TWIHS hangs
* :github:`42496` - ARM M4 MPU backed userspace livelocks on stack overflow when FPU enabled
* :github:`42478` - Unable to build mcuboot for b_u585i_iot02a
* :github:`42453` - Unable to update Firmware using MCUBoot on STM32G0 series
* :github:`42436` - NXP eDMA overrun errors on SAI RX
* :github:`42434` - NXP I2S (SAI) driver bugs
* :github:`42432` - i2c: unable to configure SAMD51 i2c clock frequency for standard (100 KHz) speeds
* :github:`42425` - i2c: sam0 driver does not prevent simultaneous transactions
* :github:`42351` - stm32H743 nucleo board cannot flash after tests/drivers/flash
* :github:`42343` - LE Audio: PACS: Server change location
* :github:`42342` - LE Audio: PACS notify changes to locations
* :github:`42333` - Cannot write to qspi flash in adafruit feather nrf52840, device tree is wrong
* :github:`42310` - Support for TCA6408A gpio expander, which existing driver as a base?
* :github:`42306` - Bluetooth: Host: More than ``CONFIG_BT_EATT_MAX`` EATT channels may be created
* :github:`42290` - ESP32 - Heltec Wifi - Possibly invalid CONFIG_ESP32_XTAL_FREQ setting (40MHz). Detected 26 MHz
* :github:`42235` - SocketCAN not supported for NUCLEO H743ZI
* :github:`42227` - Teensy41 support SDHC - Storage init Error
* :github:`42189` - Sub 1GHz Support for CC1352
* :github:`42181` - Ethernet PHY imxrt1060 Teensy not working, sample with DHCPv4_client fails
* :github:`42113` - Modbus RTU allow non-compliant client configuration
* :github:`42108` - upsquared: isr_dynamic & isr_regular test is failing
* :github:`42102` - doc: searches for sys_reboot() are inconsistent
* :github:`42096` - LE Audio: Media: Pass structs by reference and not value
* :github:`42090` - Bluetooth: Audio: MCS BSIM notification length warning
* :github:`42083` - Bluetooth: ISO: Packet Sequence Number should be incremented for each channel
* :github:`42081` - Direction finding code support for nrf52811?
* :github:`42072` - west: spdx: Blank FileChecksum field for missing build file
* :github:`42050` - printk bug: A function called from printk is invoked three times given certain configuration variables
* :github:`42015` - LED api can't be called from devicetree phandle
* :github:`42011` - Establish guidelines for TSC working groups
* :github:`42000` - BQ274xx driver not working correctly
* :github:`41995` - tracing: riscv: Missing invoking the sys_trace_isr_exit()
* :github:`41947` - lpcxpresso55s16 SPI hardware chip select not working
* :github:`41946` - Bluetooth: ISO: Sending on RX-only CIS doesn't report error
* :github:`41944` - Assertion triggered when system is going to PM_STATE_SOFT_OFF
* :github:`41931` - drivers: audio: tlv320dac310x: device config used as non-const
* :github:`41924` - drivers: dma/i2c: nios2: config used as non-const
* :github:`41921` - Fast USB DFU workflow
* :github:`41899` - ESP32 Wifi mDNS
* :github:`41874` - Recursive spinlock error on ARM in specific circumstances
* :github:`41864` - ESP32 Wifi AP Mode DHCP Service
* :github:`41823` - Bluetooth: Controller: llcp: Remote request are dropped due to lack of free proc_ctx
* :github:`41788` - Bluetooth: Controller: llcp: Refectored PHY Update procedure asserts while waiting for free buffers to send notifications
* :github:`41787` - Alignment issue on Cortex M7
* :github:`41777` - periodic_adv periodic_sync lost data
* :github:`41773` - LoRaWAN: Unable to correctly join networks of any version on LTS
* :github:`41742` - stm32g0: stm32_temp: not working
* :github:`41710` - tests: ztest: ztress: Test randomly fails on qemu_cortex_a9
* :github:`41677` - undefined reference to \`__device_dts_ord_xx'
* :github:`41667` - doc: arm: mec172x:  MEC172x EVB documentation points to some inexistent jumpers
* :github:`41652` - Bluetooth: Controller: BIG: Channel map update BIG: Generation of BIG_CHANNEL_MAP_IND (sent 6 times)
* :github:`41651` - Bluetooth: Controller: BIG Sync: Channel map update of BIG
* :github:`41650` - STM32H7 SPI123 incorrect clock source used for prescaler calculation
* :github:`41642` - Deploy generated docs from PRs
* :github:`41628` - Move LVGL glue code to zephyr/modules/
* :github:`41613` - Process: Review and update Milestone Definitions
* :github:`41597` - Unable to build mcuboot for BL654_DVK
* :github:`41596` - Split connected ISO client and server by Kconfig
* :github:`41594` - LE Audio: Upstream CCP/TBS
* :github:`41593` - LE Audio: Upstream BASS
* :github:`41592` - Object Transfer Service Client made "official"
* :github:`41590` - LE Audio: CAP API - Acceptor
* :github:`41517` - Hard fault if ``CONFIG_LOG2_MODE_DEFERRED`` is enabled
* :github:`41472` - Unable to mount fat file system on nucleo_f429zi
* :github:`41449` - PWM capture with STM32
* :github:`41408` - Low power states for STM32 H7
* :github:`41388` - tests: coverage: test code coverage report failed on mps2_an385
* :github:`41382` - nordic nrf52/nrf53 and missing cpu-power-states (dts) for automatic device PM control
* :github:`41375` - hal_nordic: update 15.4 driver to newest version
* :github:`41297` - QSPI flash need read, write via 4 lines not 1 line
* :github:`41285` - pthread_once has incorrect behavior
* :github:`41230` - LE Audio: API Architecture and documentation for GAF
* :github:`41228` - LE Audio: Add a codec to Zephyr
* :github:`41220` - STM32H7: Check for VOSRDY instead of ACTVOSRDY
* :github:`41201` - LE Audio: Improved media_proxy internal data structure
* :github:`41200` - LE Audio: Other postponed MCS cleanups
* :github:`41196` - LE Audio: Reconfigure Unicast Group after creation
* :github:`41194` - LE Audio: Remove support for bidirectional audio streams
* :github:`41192` - LE Audio: Change PACS from indicate to notify
* :github:`41191` - LE Audio: Update pac_indicate to actually send data
* :github:`41188` - LE Audio: Remove stream (dis)connected callback from stream ops
* :github:`41186` - LE Audio: CAP API - Initiator
* :github:`41169` - twister: program get stuck when serial in hardware map is empty string
* :github:`41151` - RFC: Provide k_realloc()
* :github:`41093` - Kconfig.defconfig:11: error: couldn't parse 'default $(dt_node_int_prop_int,/cpus/cpu@0,clock-frequency)'
* :github:`40970` - Upgrade qemu to fix breakage in mps3-an547
* :github:`40920` - Bluetooth audio: client/server naming scheme
* :github:`40901` - RFC: API Change: update LVGL from v7 to v8
* :github:`40874` - mps2_an521_ns: fail to handle user_string_alloc_copy() with null parameter
* :github:`40856` - PPP: gsm_modem: LCP never gets past REQUEST_SENT phase
* :github:`40775` - stm32: multi-threading broken after #40173
* :github:`40679` - libc/minimal: static variable of gmtime() does not located to z_libc_partition at usermode.
* :github:`40657` - Cannot enable secondary pwm out channels on stm32f3
* :github:`40635` - gen_app_partitions.py may not include all object files produced by build system
* :github:`40620` - zephyr with cadence xtensa core dsp LX7 ，helloworld program  cannot be entered after the program is executed
* :github:`40593` - tests: lib: cmsis_dsp: Overflows in libraries.cmsis_dsp.matrix
* :github:`40591` - RFC: Replace TinyCBOR with ZCBOR within Zephyr
* :github:`40588` - mgmg/mcumg/lib: Replace TinyCBOR with zcbor
* :github:`40559` - Move LittlefFS configuration header and CMakeLists.txt from module to zephyr/modules
* :github:`40371` - modem: uart interface does not disable TX interrupt in ISR
* :github:`40360` - Error messages with the sample: Asynchronous Socket Echo Server Using select()
* :github:`40306` - ESP32 BLE transmit error
* :github:`40298` - Bluetooth assertions in lll_conn.c
* :github:`40204` - Bluetooth: ll_sync_create_cancel fails with BT_HCI_ERR_CMD_DISALLOWED before BT_HCI_EVT_LE_PER_ADV_SYNC_ESTABLISHED is generated
* :github:`40195` - CONFIG_BOARD default value using cmake -DBOARD define value
* :github:`39948` - kernel.common.stack_sentinel fails on qemu_cortex_a9
* :github:`39922` - Instruction fetch fault happens on RISC-V with XIP and userspace enabled
* :github:`39834` - [Coverity CID: 240669] Unrecoverable parse warning in subsys/jwt/jwt.c
* :github:`39738` - twister: tests: samples: Skips on integration_platforms in CI
* :github:`39520` - Add support for the BlueNRG-LP SoC
* :github:`39432` - Periodic adv. syncing takes longer and bt_le_per_adv_sync_delete returns error after commit ecf761b4e9
* :github:`39314` - Invalid CONTROLLER_ID in usb_dc_mcux.c for LPC54114
* :github:`39194` - Process: investigate GitHub code review replacements
* :github:`39184` - HawkBit hash mismatch
* :github:`39176` - overflow in sensor_value_from_double
* :github:`39132` - subsys/net/ip/tcp2: Missing feature to decrease Receive Window size sent in the ACK messge
* :github:`38978` - Esp32 compilation error after enabling CONFIG_NEWLIB_LIBC
* :github:`38966` - Please add STM32F412VX
* :github:`38747` - data/json: encoding issues with array in object_array
* :github:`38632` - Multiple potential dead-locks modem_socket_wait_data
* :github:`38570` - Process: binary blobs in Zephyr
* :github:`38567` - Process: legitimate signed-off-by lines
* :github:`38548` - stm32: QSPI flash driver concurrent access issue
* :github:`38305` - Update to LVGL v8
* :github:`38279` - Bluetooth: Controller: assert LL_ASSERT(!radio_is_ready()) in lll_conn.c
* :github:`38268` - Multiple defects in "Multi Producer Single Consumer Packet Buffer" library
* :github:`38179` - twister: only report failures in merged junit output
* :github:`37798` - Change nRF5340DK board files to handle CPUNET pin configuration with DTS nodes
* :github:`37730` - http_client_req: Timeout likely not working as expected
* :github:`37710` - Bluetooth advert packet size is size of maximum packet not size of actual data
* :github:`37683` - STM32 Eth Tx DMA always uses first descriptor instead of going through circular buffer
* :github:`37324` - subsys/mgmt/hawkbit: Unable to finish download if CPU blocking function (i.e. ``flash_img_buffered_write``) is used
* :github:`37294` - RTT logs not found with default west debug invocation on jlink runner
* :github:`37191` - nrf5340: Support +3dBm TX power
* :github:`37186` - entropy: Bluetooth derived entropy device
* :github:`36905` - Improve (k\_)malloc and heap documentation
* :github:`36882` - MCUMGR: fs upload fail for first time file upload
* :github:`36645` - minimal libc: add strtoll and strtoull functions
* :github:`36571` - LoRa support for random DevNonce and NVS stack state storage
* :github:`36266` - kernel timeout_list NULL pointer access
* :github:`35316` - log_panic() hangs kernel
* :github:`34737` - Can't compile CIVETWEB with CONFIG_NO_OPTIMIZATIONS or CONFIG_DEBUG
* :github:`34590` - Functions getopt_long and getopt_long_only from the FreeBSD project
* :github:`34256` - Add support for FVP in CI / SDK
* :github:`34218` - Civetweb server crashing when trying to access invalid resource
* :github:`34204` - nvs_write: Bad documented return value.
* :github:`33876` - Lora sender sample build error for esp32
* :github:`32885` - Zephyr C++ support documentation conflicts to the code
* :github:`31613` - Undefined reference errors when using External Library with k_msgq_* calls
* :github:`30724` - CAN J1939 Support
* :github:`30152` - Settings nvs subsystem uses a hardcoded flash area label
* :github:`29981` - Improve clock initialization on LPC & MXRT600
* :github:`29941` - Unable to connect Leshan LwM2M server using x86 based LwM2M client
* :github:`29199` - github integration: ensure maintainers are added to PRs that affect them
* :github:`29107` - Bluetooth: hci-usb uses non-standard interfaces
* :github:`28009` - Add connection status to the connection info
* :github:`27841` - samples: disk: unable to access sd card
* :github:`27177` - Unable to build samples/bluetooth/st_ble_sensor for steval_fcu001v1 board
* :github:`26731` - Single channel selection - Bluetooth - Zephyr
* :github:`26038` - build zephyr with llvm fail
* :github:`25362` - better support for posix api read write in socketpair tests
* :github:`24733` - Misconfigured environment
* :github:`23347` - net: ieee802154_radio: API improvements
* :github:`22870` - Add Cortex-M4 testing platform
* :github:`22455` - How to assign USB endpoint address manually in stm32f4_disco for CDC ACM class driver
* :github:`22247` - Discussion: Supporting the Arduino ecosystem
* :github:`22161` - add shell command for the settings subsystem
* :github:`21994` - Bluetooth: controller: split: Fix procedure complete event generation
* :github:`21409` - sanitycheck: cmd.exe colorized output
* :github:`20269` - Add support for opamps in MCUs
* :github:`19979` - Implement Cortex-R floating-point support
* :github:`19244` - BLE throughput of DFU by Mcumgr is too slow
* :github:`17893` - dynamic threads don't work on x86 in some configurations
* :github:`17743` - cross compiling for RISCV32 fails as compiler flags are not supplied by board but must be in target.cmake
* :github:`17005` - Upstreamability of SiLabs RAIL support
* :github:`16406` - west: runners: Add --id and --chiperase options
* :github:`16205` - Add support to west to flash w/o a build, but given a binary
* :github:`15820` - mcumgr: taskstat show name & used size
* :github:`14649` - CI testing must be retry-free
* :github:`14591` - Infineon Tricore architecture support
* :github:`13318` - k_thread_foreach api breaks real time semantics
* :github:`9578` - Windows installation of SDK needs 'just works' installer
* :github:`8536` - imxrt1050: Replace systick with gpt or other system timer
* :github:`8481` - Remove the Kconfig helper options for nRF ICs once DT can replace them
* :github:`8139` - Driver for BMA400 accelerometer
* :github:`6654` - efm32wg_stk3800 bluetooth sample do not compile (add support)
* :github:`6162` - LwM2M: Support Queue Mode Operation
* :github:`1495` - esp32: newlibc errors
* :github:`1392` - No module named 'elftools'
* :github:`3192` - Shutting down BLE support
* :github:`3150` - Si1153 Ambient Light Sensor, Proximity, and Gesture detector support
