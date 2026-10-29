:orphan:

..
  See
  https://docs.zephyrproject.org/latest/releases/index.html#migration-guides
  for details of what is supposed to go into this document.

.. _migration_4.6:

Migration guide to Zephyr v4.6.0 (Working Draft)
################################################

This document describes the changes required when migrating your application from Zephyr v4.5.0 to
Zephyr v4.6.0.

Any other changes (not directly related to migrating applications) can be found in
the :ref:`release notes<zephyr_4.6>`.

.. contents::
    :local:
    :depth: 2

Common
******

* The :file:`scripts/footprint/size_report`, :file:`scripts/footprint/compare_footprint`, and
  :file:`scripts/kconfig/diffconfig` scripts have been renamed to
  :file:`scripts/footprint/size_report.py`, :file:`scripts/footprint/compare_footprint.py`, and
  :file:`scripts/kconfig/diffconfig.py`, respectively. Users must update their invocations.

Build System
************

Kernel
******

Boards
******

* On the Nordic nRF54L and nRF71 Series SoCs, the push area that the Key Management Unit (KMU) of
  CRACEN uses to push keys is now reserved at the start of the SRAM by the ``kmu_push_area``
  devicetree node of the SoC. The ``zephyr,sram`` chosen node of the in-tree boards is now
  ``cpuapp_sram_app``, a child node of ``cpuapp_sram`` that starts after the push area. The nodes
  are not present when ``USE_NON_SECURE_ADDRESS_MAP`` is used. Out-of-tree boards that use the
  CRACEN KMU key push should be updated, as otherwise the application RAM overlaps the push area
  without any diagnostic. Boards that do not use it can keep using the whole RAM:

  * Replace ``zephyr,sram = &cpuapp_sram`` with ``zephyr,sram = &cpuapp_sram_app``.
  * To limit the SRAM available to the application, resize ``cpuapp_sram_app`` and keep its
    offset, for example ``reg = <0x60 (DT_SIZE_K(184) - 0x60)>``. Changing ``reg`` of
    ``cpuapp_sram`` no longer changes the SRAM that ``zephyr,sram`` refers to.
  * If the memory layout deletes and redefines ``cpuapp_sram``, assign the redefined node to
    ``zephyr,sram`` and leave the first ``0x60`` bytes of the SRAM free.
  * Remove custom nodes and linker sections that reserve the push area, such as
    ``nrf_kmu_reserved_push_area``. They would overlap the ``kmu_push_area`` node.

* :zephyr:board:`stm32_min_dev` now declares two SoCs, so the SoC qualifier is
  mandatory. Applications and test configurations that used ``stm32_min_dev``,
  ``stm32_min_dev@blue`` or ``stm32_min_dev@black`` must use
  ``stm32_min_dev@blue/stm32f103x8`` or ``stm32_min_dev@black/stm32f103x8``
  instead. ``stm32_min_dev_defconfig`` was renamed accordingly to
  :file:`stm32_min_dev_stm32f103x8_defconfig`.

* :zephyr:board:`cy8ckit_041s_max`: the console moved from SCB2 to SCB1 on the same pins (P0.4,
  P0.5), and its node label changed from ``uart2`` to ``uart1``. The ``spi1`` node on SCB1
  (P2.0 to P2.2) was removed. SCB2 is now ``arduino_spi`` on the Arduino header pins D11 to D13
  (P10.0 to P10.2), disabled by default.

* LiteX: The deprecated ``CONFIG_LITEX_CSR_DATA_WIDTH`` Kconfig option has been removed. Only the
  32 bit CSR data width is supported. Designs that use an 8 bit CSR data width must be rebuilt with
  a 32 bit CSR data width. (:github:`124793`)

* The Andes AE350 SoC is now provided per AndesCore core, for example ``ae350_a25`` for the A25
  core. The ``ae350`` SoC and :kconfig:option:`CONFIG_SOC_AE350` are deprecated and will be
  removed in a future release. Kconfig, CMake and code checking
  :kconfig:option:`CONFIG_SOC_AE350` should check :kconfig:option:`CONFIG_SOC_SERIES_AE350`
  instead. (:github:`122288`)

  * The AE350 board targets of :zephyr:board:`adp_xc7k` and
    :zephyr:board:`tang_mega_138k_pro_dock` have been renamed after the core. The previous board
    targets are deprecated and map to the new board targets:

    * ``adp_xc7k/ae350`` to ``adp_xc7k/ae350_a25``
    * ``adp_xc7k/ae350/clic`` to ``adp_xc7k/ae350_d23/clic``
    * ``tang_mega_138k_pro_dock/ae350`` to ``tang_mega_138k_pro_dock/ae350_a25``
    * ``tang_mega_138k_pro_dock/ae350/demo`` to ``tang_mega_138k_pro_dock/ae350_a25/demo``

  * Applications must rename their board specific files after the new board targets, for
    example :file:`boards/adp_xc7k_ae350.overlay` to :file:`boards/adp_xc7k_ae350_a25.overlay`,
    as the files named after the previous board targets are no longer applied. Kconfig, CMake
    and code checking ``CONFIG_BOARD_ADP_XC7K_AE350`` must check
    ``CONFIG_BOARD_ADP_XC7K_AE350_A25`` instead.
  * Out-of-tree boards using the ``ae350`` SoC should use the ``ae350_<core>`` SoC of their core
    in ``board.yml``, rename the board files named after the SoC, select ``SOC_AE350_<core>``
    instead of ``SOC_AE350``, and include the :file:`riscv/andes/andes_v5_ae350_<core>.dtsi` of
    their core. Boards describing their own CPU nodes must use the compatible of the core, for
    example :dtcompatible:`andestech,a25`, which enables the ``CONFIG_CPU_ANDES_<core>`` option
    selecting the CPU features of the core.
  * :file:`riscv/andes/andes_v5_ae350.dtsi` now describes a single CPU instead of eight, and the
    PLIC also has the supervisor mode context of the hart. Out-of-tree boards including it must
    drop the deletes of the other CPU nodes. Multicore boards must add the other CPU nodes and
    add them to the ``interrupts-extended`` of ``plic0``, ``mtimer`` and ``plic_sw``, or include
    the :file:`riscv/andes/andes_v5_ae350_<core>mp.dtsi` of their core.

* The RAKwireless ``rak3172`` board is renamed to :zephyr:board:`rak3272s`. It
  was named after the WisDuo stamp module it carries rather than the board it
  describes, which is the RAK3272S Breakout Board. The other carrier for the
  same module is the new :zephyr:board:`rak3372` WisBlock Core Module.
  :file:`boards/deprecated.cmake` keeps ``-b rak3172`` working with a warning,
  but out-of-tree code testing ``CONFIG_BOARD_RAK3172`` must switch to
  ``CONFIG_BOARD_RAK3272S``.

* :zephyr:board:`rak3272s` no longer defines ``led0``. The RAK3272S carries no
  LEDs and leaves the two module pins the nodes used unrouted. Applications
  that relied on the alias, such as :zephyr:code-sample:`blinky`, need an
  overlay pointing at a LED wired to the headers.

Device Drivers and Devicetree
*****************************

.. Only place contents common to all device drivers here. Contents specific to one driver subsystem
   goes into its own subsection, below.

.. Group contents in this section by subsystem, e.g.:
..
.. ADC
.. ===
..
.. ...

.. zephyr-keep-sorted-start re(^\w) ignorecase

ADC
===

* :dtcompatible:`nxp,lpc-lpadc` channels that use ``ADC_GAIN_1_2`` for the 30/64 partial scale
  should use ``ADC_GAIN_15_32``: with ``ADC_GAIN_1_2``, which still selects that scale, the
  conversions of :c:func:`adc_raw_to_millivolts_dt` and :c:func:`adc_raw_to_millivolts` are
  6.25% too low.

Audio Codec
===========

* The :dtcompatible:`wolfson,wm8904` binding now requires the ``clocks`` and ``clock-names``
  properties. The MCLK input must always be described in devicetree, either by a clock controller
  or by a :dtcompatible:`fixed-clock` node when it is not backed by a controllable clock source.
  The driver no longer falls back to computing MCLK as ``fs-ratio * <sample rate>``.

  The ``fs-ratio`` property has been replaced by ``fll-sysclk-fs-ratio``, which now only sets the
  SYSCLK to sample rate ratio generated by the FLL when ``clock-source`` is ``"FLL"``. It is
  ignored when ``clock-source`` is ``"MCLK"``.

  Out-of-tree boards and overlays must be updated, for example:

  .. code-block:: devicetree

     mclk: mclk {
             compatible = "fixed-clock";
             clock-frequency = <12288000>;
             #clock-cells = <0>;
     };

     &wm8904 {
             clocks = <&mclk>;
             clock-names = "mclk";
     };

  When using a :dtcompatible:`fixed-clock` node,
  :kconfig:option:`CONFIG_CLOCK_CONTROL_FIXED_RATE_CLOCK` must be enabled.

Clock control
=============

* NXP MCXC and K32L2B3 SoCs now use the :dtcompatible:`nxp,mcg-lite` binding and driver
  for the MCG-Lite clock generator. Out-of-tree devicetrees must update the mcg node to use
  the new compatible and replace any use of ``KINETIS_MCG_OUT_CLK`` with
  ``NXP_MCG_LITE_OUT_CLK``. The ``lircdiv2`` property is no longer supported by
  :dtcompatible:`nxp,kinetis-mcg` and should be removed from existing devicetrees.

Comparator
==========

* The deprecated ``mcux_lpcmp`` sensor driver, its public header
  ``zephyr/drivers/sensor/mcux_lpcmp.h``, the ``CONFIG_MCUX_LPCMP_TRIGGER`` option and its sample
  have been removed. Applications using ``CONFIG_MCUX_LPCMP`` must enable
  :kconfig:option:`CONFIG_COMPARATOR` and
  :kconfig:option:`CONFIG_COMPARATOR_NXP_LPCMP` and migrate to the :ref:`comparator_api` instead of
  the sensor API.

  The :dtcompatible:`nxp,lpcmp` devicetree compatible is unchanged, but existing overlays must be
  updated for the comparator driver. Legacy sensor properties remain in the binding but are not
  consumed by the comparator driver:

  * Replace ``hysteresis-level`` with ``hysteresis-mode``, converting integer values ``0`` through
    ``3`` to strings ``"LEVEL0"`` through ``"LEVEL3"``.
  * ``power-level = "low"`` becomes ``power-mode = "low_speed"`` and
    ``power-level = "high"`` becomes ``power-mode = "high_speed"``. The old ``"nano"`` value has
    no corresponding ``power-mode`` setting.
  * Replace ``enable-output-pin`` with ``enable-pin-out``. Use ``invert-output`` for output
    inversion.
  * The ``function-clock`` property is not used by the comparator driver. Review the SoC clock
    configuration and the standard ``clocks`` property separately; ``clocks`` is not a direct
    replacement for the LPCMP internal functional clock-source selection.

  Refer to :zephyr_file:`dts/bindings/comparator/nxp,lpcmp.yaml` for the property definitions. The
  :zephyr_file:`tests/drivers/comparator/gpio_loopback` test provides an example of the comparator
  API. (:github:`124232`).

Ethernet
========

* :kconfig:option:`CONFIG_ETH_PHY_DRIVER` is no longer enabled by default. It is now selected by
  the Ethernet drivers that need a PHY driver. Out-of-tree Ethernet drivers that use a PHY driver
  must select it as well. (:github:`120871`)

* The Kconfig option ``CONFIG_ETH_INTEL_IGC_NET_MTU`` has been removed. The Intel IGC Ethernet
  driver now always uses the standard Ethernet MTU of 1500 bytes, as the Ethernet L2 does not
  support jumbo frames. (:github:`122863`)

* The ``reset-gpios`` timing properties of the Ethernet PHY bindings, such as
  :dtcompatible:`ethernet-phy` and :dtcompatible:`ethernet-phy-fixed-link`, have been renamed to
  align with the Linux Ethernet PHY binding, which uses the same names.
  Replace ``reset-assert-duration-us`` with ``reset-assert-us``, and replace
  ``reset-deassertion-timeout-ms`` with ``reset-deassert-us``, multiplying its value by 1000 as
  it is now in microseconds. (:github:`124061`)

* The ``ETHERNET_LLDP`` value of :c:enum:`ethernet_hw_caps` has been removed. LLDP is now enabled
  on every Ethernet interface when :kconfig:option:`CONFIG_NET_LLDP` is set. Out-of-tree Ethernet
  drivers must stop reporting it. (:github:`124787`)

I2S
===

* :kconfig:option:`CONFIG_I2S_NATIVE_SIM_RX_FILE_PATH` and
  :kconfig:option:`CONFIG_I2S_NATIVE_SIM_TX_FILE_PATH` are deprecated. Use
  :kconfig:option:`CONFIG_I2S_NATIVE_SIM_RX_PATH` and
  :kconfig:option:`CONFIG_I2S_NATIVE_SIM_TX_PATH`, which also accept ALSA devices. Without the
  ALSA backend the old options still provide their default. (:github:`123474`)

Modem
=====

* Sierra Wireless HL78xx: the ``CONFIG_HL78XX_EVT_MONITOR_HEAP_SIZE`` option has been replaced
  by :kconfig:option:`CONFIG_HL78XX_EVT_MONITOR_QUEUE_DEPTH`. The queue of notifications
  waiting for the deferred event monitors is now sized as a number of notifications instead of
  a heap size in bytes. Applications that set the old option must remove it, and set the new
  one if the default of 16 notifications is not enough. A notification that does not fit is
  dropped and counted; the count is available from
  :c:func:`hl78xx_evt_monitor_dropped_count`. The slots are reserved statically, one queued
  notification each, so the default of 16 uses more RAM than the 256-byte heap it replaces.
  (:github:`120425`)

OPAMP
=====

* The NXP OPAMP drivers (:dtcompatible:`nxp,opamp` and :dtcompatible:`nxp,opamp-fast`) hold a
  power state constraint while the device is active, and the OPAMP nodes declare
  ``zephyr,disabling-power-states``. With :kconfig:option:`CONFIG_PM_POLICY_DEVICE_CONSTRAINTS`
  an enabled OPAMP therefore keeps the system out of Power Down and Deep Power Down for as long
  as it is active, which without :kconfig:option:`CONFIG_PM_DEVICE_RUNTIME` is from init onwards.
  Enable device runtime PM for the node, or suspend it explicitly, to let the system reach those
  states. (:github:`122135`)

* :dtcompatible:`nxp,opamp-fast` no longer enables an instance marked with
  ``is-reference-source`` from its init function; the instance follows its own power state like
  every other instance. With :kconfig:option:`CONFIG_PM_DEVICE_RUNTIME` an application has to hold
  a runtime reference on the reference source while it measures an amplifier fed by it. Builds
  without device runtime PM are unaffected: the device is resumed at init and the instance comes
  up enabled as before. (:github:`122135`)

PTP Clock
=========

* :c:func:`ptp_clock_rate_adjust` and the :c:member:`ptp_clock_driver_api.rate_adjust` driver
  operation, which take the clock rate as a floating-point ratio, have been deprecated. Use
  :c:func:`ptp_clock_adjust_rate` and implement :c:member:`ptp_clock_driver_api.adjust_rate`
  instead, which take the rate offset from the nominal frequency as scaled parts per million,
  that is parts per million with a 16-bit binary fractional field:
  ``scaled_ppm = (ratio - 1) * 1000000 * 65536``. :c:macro:`PTP_CLOCK_SCALED_PPM_ONE` is 1 ppm.

  A driver only has to implement one of the two operations. Both functions work with either of
  them and convert the argument if necessary, so applications and drivers can be migrated
  independently.

  Drivers can use :c:func:`ptp_clock_scaled_ppm_to_ppb` if the hardware takes the offset in
  parts per billion, and :c:func:`ptp_clock_adjust_by_scaled_ppm` to compute the value of an
  addend register from its nominal value. All in-tree drivers have been converted.
  (:github:`120828`)

Timers
======

* The deprecated Cortex-M SysTick low-power companion Kconfig options have been removed.
  Replace them with their generic system timer equivalents:

  .. list-table::
     :header-rows: 1

     * - Removed option
       - Replacement
     * - ``CONFIG_CORTEX_M_SYSTICK_LPM_TIMER_NONE``
       - :kconfig:option:`CONFIG_SYSTEM_TIMER_LPM_COMPANION_NONE`
     * - ``CONFIG_CORTEX_M_SYSTICK_LPM_TIMER_COUNTER``
       - :kconfig:option:`CONFIG_SYSTEM_TIMER_LPM_COMPANION_COUNTER`
     * - ``CONFIG_CORTEX_M_SYSTICK_LPM_TIMER_HOOKS``
       - :kconfig:option:`CONFIG_SYSTEM_TIMER_LPM_COMPANION_HOOKS`
     * - ``CONFIG_CORTEX_M_SYSTICK_RESET_BY_LPM``
       - :kconfig:option:`CONFIG_SYSTEM_TIMER_RESET_BY_LPM`

* The deprecated chosen ``/chosen/zephyr,cortex-m-idle-timer`` is no longer supported. Use the
  :ref:`zephyr,system-timer-companion <devicetree-chosen-nodes>` chosen instead to select the
  low-power companion counter.

* The deprecated ``drivers/timer/cortex_m_systick.h`` compatibility header has been removed.
  Include :zephyr_file:`include/zephyr/drivers/timer/system_timer_lpm.h` instead, and replace
  ``z_cms_lptim_hook_on_lpm_entry`` and ``z_cms_lptim_hook_on_lpm_exit`` with
  :c:func:`z_sys_clock_lpm_enter` and :c:func:`z_sys_clock_lpm_exit`, respectively. (:github:`124238`).

.. zephyr-keep-sorted-stop

Bluetooth
*********

Bluetooth Audio
===============

.. zephyr-keep-sorted-start re(^\* \w)

* MCP

  * The media proxy API has been removed and users will need to use the ``bt_mcs``, ``bt_mcc`` or
    ``bt_mcp_media_control_server`` APIs. Users of the media player API like
    ``media_proxy_pl_init`` can seamlessly switch to using
    :c:func:`bt_mcp_media_control_server_register`.
    Users using the ``bt_mcc`` API can continue as is, but ``struct mpl_cmd`` is now
    ``struct bt_mcs_cmd``, and ``struct mpl_search`` is now ``struct bt_mcs_search``.
    Uses of ``media_proxy_ctrl_`` for local operations can in most places just search-and-replace
    with ``bt_mcp_media_control_server_``. Uses of ``media_proxy_ctrl_`` for remote operations will
    need to switch to the ``bt_mcc`` API. (:github:`102994`)

.. zephyr-keep-sorted-stop

Bluetooth Classic
=================

* The L2CAP BR/EDR ECHO API is now only available with :kconfig:option:`CONFIG_BT_TESTING`:
  :c:struct:`bt_l2cap_br_echo_cb`, :c:func:`bt_l2cap_br_echo_cb_register`,
  :c:func:`bt_l2cap_br_echo_cb_unregister`, :c:func:`bt_l2cap_br_echo_req` and
  :c:func:`bt_l2cap_br_echo_rsp`. The stack answers a received ECHO request with an ECHO response
  without data, unless a registered ``req`` callback handles it.

Bluetooth HCI
=============

* The STM32WB HCI driver now constructs its generated public Bluetooth address using the low
  16 bits of the device UDN, the device ID, and the full 24-bit ST company ID. This corrects the
  previous address layout and changes the generated address on affected devices. Previously bonded
  peers may need to remove the old bond and pair with the device again.

Bluetooth Host
==============

* The ATT prepare write buffers (:kconfig:option:`CONFIG_BT_ATT_PREPARE_COUNT`) are now shared
  equally between connections: a single connection holds at most
  :kconfig:option:`CONFIG_BT_ATT_PREPARE_COUNT_PER_CONN` of them, which defaults to
  ``CONFIG_BT_ATT_PREPARE_COUNT / CONFIG_BT_MAX_CONN`` but at least 1 (a pool smaller than
  ``CONFIG_BT_MAX_CONN`` therefore stays over-subscribed), and its further Prepare Write
  Requests are rejected with Prepare Queue Full. Applications with ``CONFIG_BT_MAX_CONN``
  above 1 whose long or reliable writes need more buffers than that share must either raise
  ``CONFIG_BT_ATT_PREPARE_COUNT`` or set ``CONFIG_BT_ATT_PREPARE_COUNT_PER_CONN`` explicitly,
  accepting in the latter case that one connection can hold buffers the others need. The
  recommended sizing is ``CONFIG_BT_ATT_PREPARE_COUNT`` = buffers needed by one client
  multiplied by ``CONFIG_BT_MAX_CONN``, which keeps every connection's capacity independent
  of the other connections.

Bluetooth Mesh
==============

* The deprecated legacy provisioning OOB API was removed together with the
  ``CONFIG_BT_MESH_PROV_OOB_API_LEGACY`` Kconfig option. The maximum numeric OOB
  authentication size is now always 32 bytes. Applications must replace:

  * ``bt_mesh_input_number()`` with :c:func:`bt_mesh_input_numeric`, which takes the
    numeric value as a little-endian byte array and its size.
  * The ``output_number`` callback of :c:struct:`bt_mesh_prov` with
    :c:member:`bt_mesh_prov.output_numeric`, which receives the numeric value as a
    little-endian byte array and its size.

Networking
**********

* The ``ZFD_IOCTL_POLL_PREPARE``, ``ZFD_IOCTL_POLL_UPDATE``, and
  ``ZFD_IOCTL_POLL_OFFLOAD`` requests have been deprecated. Implement ``poll_prepare``,
  ``poll_update``, and ``poll_offload`` in :c:struct:`fd_op_vtable` instead. Out-of-tree ioctl
  handlers that forward these requests to an in-tree socket should use
  ``zvfs_fdtable_call_poll_prepare()``, ``zvfs_fdtable_call_poll_update()``, and
  ``zvfs_fdtable_call_poll_offload()``, as in-tree sockets no longer handle the ``ZFD_IOCTL_POLL_*``
  requests in their ioctl handlers.

* For ``AF_UNSPEC`` lookups, :c:func:`zsock_getaddrinfo` now returns IPv6 addresses before IPv4
  ones, unless only the IPv4 addresses are reachable, and queries AAAA records before A records.
  Applications that use the first result now connect over IPv6 where they used IPv4. Enable
  :kconfig:option:`CONFIG_NET_SOCKETS_DNS_PREFER_IPV4` to keep IPv4 first, for example with
  offloaded network interfaces, where reachability cannot be determined.

* Network interface driver APIs are now device API classes. Out-of-tree network drivers must define
  their API instances with :c:macro:`DEVICE_API` and use the renamed structs:

  * ``struct net_if_api`` is now :c:struct:`net_if_driver_api`.
  * ``struct ethernet_api`` is now :c:struct:`ethernet_driver_api`.
  * ``struct dummy_api`` is now :c:struct:`dummy_driver_api`.
  * ``struct ppp_api`` is now :c:struct:`ppp_driver_api`.
  * ``struct canbus_api`` is now :c:struct:`canbus_driver_api`.
  * ``struct offloaded_if_api`` is now :c:struct:`offloaded_if_driver_api`.
  * ``struct virtual_interface_api`` is now :c:struct:`virtual_interface_driver_api`.
  * ``struct hdlc_api`` is now :c:struct:`hdlc_driver_api`.
  * ``struct ieee802154_radio_api`` is now :c:struct:`ieee802154_radio_driver_api`.

  For example, ``static const struct ethernet_api api = {...}`` becomes
  ``static DEVICE_API(ethernet, api) = {...}``. API instances are now always constant, and cannot
  be modified at runtime. (:github:`124334`)

* Wi-Fi drivers must define their API with ``DEVICE_API(wifi_native, name)`` when
  :kconfig:option:`CONFIG_WIFI_USE_NATIVE_NETWORKING` is enabled, or with
  ``DEVICE_API(wifi_offload, name)`` otherwise. ``struct net_wifi_mgmt_offload`` remains available
  to refer to the selected type. (:github:`124334`)

Other subsystems
****************

Shell
=====

* Calling a shell command with an unknown subcommand, such as ``kernel foo``, now returns
  ``-ENOEXEC`` instead of ``SHELL_CMD_HELP_PRINTED`` (1). The help is still printed. Update code
  that checks the return value of :c:func:`shell_execute_cmd` for this case.

Modules
*******

* The `zephyr-lang-rust <https://github.com/zephyrproject-rtos/zephyr-lang-rust>`_ module providing
  Rust language support is no longer an optional module of the Zephyr manifest. It is now an
  :ref:`external module <external_module_rust>`; add it to the application manifest to keep using
  Rust. The ``manifest.project-filter +zephyr-lang-rust`` west setting no longer has an effect.

Architectures
*************
