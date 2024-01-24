:orphan:

.. _migration_3.6:

Migration guide to Zephyr v3.6.0 (Working Draft)
################################################

This document describes the changes required or recommended when migrating your
application from Zephyr v3.5.0 to Zephyr v3.6.0.

Any other changes (not directly related to migrating applications) can be found in
the :ref:`release notes<zephyr_3.6>`.

Required changes
****************

Boards
======

* The deprecated Nordic SoC Kconfig option ``NRF_STORE_REBOOT_TYPE_GPREGRET`` has been removed,
  applications that use this should switch to using the :ref:`boot_mode_api` instead.

Build System
============

* The deprecated ``prj_<board>.conf`` Kconfig file support has been removed, projects that use
  this should switch to using board Kconfig fragments instead (``boards/<board>.conf``).

Kernel
======

* The system heap size and its availability is now determined by a ``K_HEAP_MEM_POOL_SIZE``
  define instead of the :kconfig:option:`CONFIG_HEAP_MEM_POOL_SIZE` Kconfig option. Subsystems
  can specify their own custom system heap size requirements by specifying Kconfig options with
  the prefix ``CONFIG_HEAP_MEM_POOL_ADD_SIZE_``. The old Kconfig option still exists, but will be
  overridden if the custom requirements are larger. To force the old Kconfig option to be used,
  even when its value is less than the indicated custom requirements, a new
  :kconfig:option:`CONFIG_HEAP_MEM_POOL_IGNORE_MIN` option has been introduced (which defaults
  being disabled).

C Library
=========

Optional Modules
================

The following modules have been made optional and are not downloaded with `west update` by default
anymore:

* ``canopennode`` (:github:`64139`)

To enable them again use the ``west config manifest.project-filter -- +<module
name>`` command, or ``west config manifest.group-filter -- +optional`` to
enable all optional modules, and then run ``west update`` again.

Device Drivers and Device Tree
==============================

* The :dtcompatible:`nxp,pcf8574` driver has been renamed to
  :dtcompatible:`nxp,pcf857x`. (:github:`67054`) to support pcf8574 and pcf8575.
  The Kconfig option has been renamed from :kconfig:option:`CONFIG_GPIO_PCF8574` to
  :kconfig:option:`CONFIG_GPIO_PCF857X`.
  The Device Tree can be configured as follows:

  .. code-block:: devicetree

    &i2c {
      status = "okay";
      pcf8574: pcf857x@20 {
          compatible = "nxp,pcf857x";
          status = "okay";
          reg = <0x20>;
          gpio-controller;
          #gpio-cells = <2>;
          ngpios = <8>;
      };

      pcf8575: pcf857x@21 {
          compatible = "nxp,pcf857x";
          status = "okay";
          reg = <0x21>;
          gpio-controller;
          #gpio-cells = <2>;
          ngpios = <16>;
      };
    };

* The :dtcompatible:`st,lsm6dsv16x` sensor driver has been changed to support
  configuration of both int1 and int2 pins. The DT attribute ``irq-gpios`` has been
  removed and substituted by two new attributes, ``int1-gpios`` and ``int2-gpios``.
  These attributes must be configured in the Device Tree similarly to the following
  example:

  .. code-block:: devicetree

    / {
        lsm6dsv16x@0 {
            compatible = "st,lsm6dsv16x";

            int1-gpios = <&gpioa 4 GPIO_ACTIVE_HIGH>;
            int2-gpios = <&gpiod 11 GPIO_ACTIVE_HIGH>;
            drdy-pin = <2>;
        };
    };
* The optional :c:func:`setup()` function in the Bluetooth HCI driver API (enabled through
  :kconfig:option:`CONFIG_BT_HCI_SETUP`) has gained a function parameter of type
  :c:struct:`bt_hci_setup_params`. By default, the struct is empty, but drivers can opt-in to
  :kconfig:option:`CONFIG_BT_HCI_SET_PUBLIC_ADDR` if they support setting the controller's public
  identity address, which will then be passed in the ``public_addr`` field.

  (:github:`62994`)

* Various deprecated macros related to the deprecated devicetree label property
  were removed. These are listed in the following table. The table also
  provides replacements.

  However, if you are still using code like
  ``device_get_binding(DT_LABEL(node_id))``, consider replacing it with
  something like ``DEVICE_DT_GET(node_id)`` instead. The ``DEVICE_DT_GET()``
  macro avoids run-time string comparisons, and is also safer because it will
  fail the build if the device does not exist.

  .. list-table::
     :header-rows: 1

     * - Removed macro
       - Replacement

     * - ``DT_GPIO_LABEL(node_id, gpio_pha)``
       - ``DT_PROP(DT_GPIO_CTLR(node_id, gpio_pha), label)``

     * - ``DT_GPIO_LABEL_BY_IDX(node_id, gpio_pha, idx)``
       - ``DT_PROP(DT_GPIO_CTLR_BY_IDX(node_id, gpio_pha, idx), label)``

     * - ``DT_INST_GPIO_LABEL(inst, gpio_pha)``
       - ``DT_PROP(DT_GPIO_CTLR(DT_DRV_INST(inst), gpio_pha), label)``

     * - ``DT_INST_GPIO_LABEL_BY_IDX(inst, gpio_pha, idx)``
       - ``DT_PROP(DT_GPIO_CTLR_BY_IDX(DT_DRV_INST(inst), gpio_pha, idx), label)``

     * - ``DT_SPI_DEV_CS_GPIOS_LABEL(spi_dev)``
       - ``DT_PROP(DT_SPI_DEV_CS_GPIOS_CTLR(spi_dev), label)``

     * - ``DT_INST_SPI_DEV_CS_GPIOS_LABEL(inst)``
       - ``DT_PROP(DT_SPI_DEV_CS_GPIOS_CTLR(DT_DRV_INST(inst)), label)``

     * - ``DT_LABEL(node_id)``
       - ``DT_PROP(node_id, label)``

     * - ``DT_BUS_LABEL(node_id)``
       - ``DT_PROP(DT_BUS(node_id), label)``

     * - ``DT_INST_LABEL(inst)``
       - ``DT_INST_PROP(inst, label)``

     * - ``DT_INST_BUS_LABEL(inst)``
       - ``DT_PROP(DT_BUS(DT_DRV_INST(inst)), label)``

* The :dtcompatible:`st,stm32-lptim` lptim which is selected for counting ticks during
  low power modes is identified by **stm32_lp_tick_source** in the device tree as follows.
  The stm32_lptim_timer driver has been changed to support this.

  .. code-block:: devicetree

    stm32_lp_tick_source: &lptim1 {
            status = "okay";
    };

* The native Linux SocketCAN driver, which can now be used in both :ref:`native_posix<native_posix>`
  and :ref:`native_sim<native_sim>` with or without an embedded C-library, has been renamed to
  reflect this:

  * The devicetree compatible was renamed from ``zephyr,native-posix-linux-can`` to
    :dtcompatible:`zephyr,native-linux-can`.
  * The main Kconfig option was renamed from ``CONFIG_CAN_NATIVE_POSIX_LINUX`` to
    :kconfig:option:`CONFIG_CAN_NATIVE_LINUX`.

* Two new structures for holding common CAN controller driver configuration (``struct
  can_driver_config``) and data (``struct can_driver_data``) fields were introduced. Out-of-tree CAN
  controller drivers need to be updated to use these new, common configuration and data structures
  along with their initializer macros.

* The optional ``can_get_max_bitrate_t`` CAN controller driver callback was removed in favor of a
  common accessor function. Out-of-tree CAN controller drivers need to be updated to no longer
  supply this callback.

* The CAN transceiver API function :c:func:`can_transceiver_enable` now takes a :c:type:`can_mode_t`
  argument for propagating the CAN controller operational mode to the CAN transceiver. Out-of-tree
  CAN controller and CAN transceiver drivers need to be updated to match this new API function
  signature.

* The ``CAN_FILTER_FDF`` flag for filtering classic CAN/CAN FD frames was removed since no known CAN
  controllers implement support for this. Applications can still filter on classic CAN/CAN FD frames
  in their receive callback functions as needed.

* The ``CAN_FILTER_DATA`` and ``CAN_FILTER_RTR`` flags for filtering between Data and Remote
  Transmission Request (RTR) frames were removed since not all CAN controllers implement support for
  individual RX filtering based on the RTR bit. Applications can now use
  :kconfig:option:`CONFIG_CAN_ACCEPT_RTR` to either accept incoming RTR frames matching CAN filters
  or reject all incoming CAN RTR frames (the default). When :kconfig:option:`CONFIG_CAN_ACCEPT_RTR`
  is enabled, applications can still filter between Data and RTR frames in their receive callback
  functions as needed.

* The io-channel cells of the following devicetree bindings were reduced from 2 (``positive`` and
  ``negative``) to the common ``input``, making it possible to use the various ADC DT macros with TI
  LMP90xxx ADC devices:

  * :dtcompatible:`ti,lmp90077`
  * :dtcompatible:`ti,lmp90078`
  * :dtcompatible:`ti,lmp90079`
  * :dtcompatible:`ti,lmp90080`
  * :dtcompatible:`ti,lmp90097`
  * :dtcompatible:`ti,lmp90098`
  * :dtcompatible:`ti,lmp90099`
  * :dtcompatible:`ti,lmp90100`

* The io-channel cells of the :dtcompatible:`microchip,mcp3204` and
  :dtcompatible:`microchip,mcp3208` devicetree bindings were renamed from ``channel`` to the common
  ``input``, making it possible to use the various ADC DT macros with Microchip MCP320x ADC devices.

* The :dtcompatible:`st,stm32h7-fdcan` CAN controller driver now supports configuring the
  domain/kernel clock via devicetree. Previously, the driver only supported using the PLL1_Q clock
  for kernel clock, but now it defaults to the HSE clock, which is the chip default. Boards that
  use the PLL1_Q clock for FDCAN will need to override the ``clocks`` property as follows:

  .. code-block:: devicetree

    &fdcan1 {
            clocks = <&rcc STM32_CLOCK_BUS_APB1_2 0x00000100>,
                     <&rcc STM32_SRC_PLL1_Q FDCAN_SEL(1)>;
    };

* Runtime configuration is now disabled by default for Nordic UART drivers. The motivation for the
  change is that this feature is rarely used and disabling it significantly reduces the memory
  footprint.

* For platforms that enabled :kconfig:option:`CONFIG_MULTI_LEVEL_INTERRUPTS`, the ``IRQ`` variant
  of the Devicetree macros now return the as-seen value in the devicetree instead of the Zephyr
  multilevel-encoded IRQ number. To get the IRQ number in Zephyr multilevel-encoded format, use
  ``IRQN`` variant instead. For example, consider the following devicetree:

  .. code-block:: devicetree

    plic: interrupt-controller@c000000 {
            riscv,max-priority = <7>;
            riscv,ndev = <1024>;
            reg = <0x0c000000 0x04000000>;
            interrupts-extended = <&hlic0 11>;
            interrupt-controller;
            compatible = "sifive,plic-1.0.0";
            #address-cells = <0x0>;
            #interrupt-cells = <0x2>;
    };

    uart0: uart@10000000 {
            interrupts = <10 1>;
            interrupt-parent = <&plic>;
            clock-frequency = <0x384000>;
            reg = <0x10000000 0x100>;
            compatible = "ns16550";
            reg-shift = <0>;
    };

  ``plic`` is a second level interrupt aggregator and ``uart0`` is a child of ``plic``.
  ``DT_IRQ_BY_IDX(DT_NODELABEL(uart0), 0, irq)`` will return ``10``
  (as-seen value in the devicetree), while ``DT_IRQN_BY_IDX(DT_NODELABEL(uart0), 0)`` will return
  ``(((10 + 1) << CONFIG_1ST_LEVEL_INTERRUPT_BITS) | 11)``.

  Drivers and applications that are supposed to work in multilevel-interrupt configurations should
  be updated to use the ``IRQN`` variant, i.e.:

  * ``DT_IRQ(node_id, irq)`` -> ``DT_IRQN(node_id)``
  * ``DT_IRQ_BY_IDX(node_id, idx, irq)`` -> ``DT_IRQN_BY_IDX(node_id, idx)``
  * ``DT_IRQ_BY_NAME(node_id, name, irq)`` -> ``DT_IRQN_BY_NAME(node_id, name)``
  * ``DT_INST_IRQ(inst, irq)`` -> ``DT_INST_IRQN(inst)``
  * ``DT_INST_IRQ_BY_IDX(inst, idx, irq)`` -> ``DT_INST_IRQN_BY_IDX(inst, idx)``
  * ``DT_INST_IRQ_BY_NAME(inst, name, irq)`` -> ``DT_INST_IRQN_BY_NAME(inst, name)``

Power Management
================

Shell
=====

* The following subsystem and driver shell modules are now disabled by default. Each required shell
  module must now be explicitly enabled via Kconfig (:github:`65307`):

  * :kconfig:option:`CONFIG_ACPI_SHELL`
  * :kconfig:option:`CONFIG_ADC_SHELL`
  * :kconfig:option:`CONFIG_AUDIO_CODEC_SHELL`
  * :kconfig:option:`CONFIG_CAN_SHELL`
  * :kconfig:option:`CONFIG_CLOCK_CONTROL_NRF_SHELL`
  * :kconfig:option:`CONFIG_DAC_SHELL`
  * :kconfig:option:`CONFIG_DEBUG_COREDUMP_SHELL`
  * :kconfig:option:`CONFIG_EDAC_SHELL`
  * :kconfig:option:`CONFIG_EEPROM_SHELL`
  * :kconfig:option:`CONFIG_FLASH_SHELL`
  * :kconfig:option:`CONFIG_HWINFO_SHELL`
  * :kconfig:option:`CONFIG_I2C_SHELL`
  * :kconfig:option:`CONFIG_LOG_CMDS`
  * :kconfig:option:`CONFIG_LORA_SHELL`
  * :kconfig:option:`CONFIG_MCUBOOT_SHELL`
  * :kconfig:option:`CONFIG_MDIO_SHELL`
  * :kconfig:option:`CONFIG_OPENTHREAD_SHELL`
  * :kconfig:option:`CONFIG_PCIE_SHELL`
  * :kconfig:option:`CONFIG_PSCI_SHELL`
  * :kconfig:option:`CONFIG_PWM_SHELL`
  * :kconfig:option:`CONFIG_REGULATOR_SHELL`
  * :kconfig:option:`CONFIG_SENSOR_SHELL`
  * :kconfig:option:`CONFIG_SMBUS_SHELL`
  * :kconfig:option:`CONFIG_STATS_SHELL`
  * :kconfig:option:`CONFIG_USBD_SHELL`
  * :kconfig:option:`CONFIG_USBH_SHELL`
  * :kconfig:option:`CONFIG_W1_SHELL`
  * :kconfig:option:`CONFIG_WDT_SHELL`

* The ``SHELL_UART_DEFINE`` macro now only requires a ``_name`` argument. In the meantime, the
  macro accepts additional arguments (ring buffer TX & RX size arguments) for compatibility with
  previous Zephyr version, but they are ignored, and will be removed in future release.

Bootloader
==========

* MCUboot's deprecated ``CONFIG_ZEPHYR_TRY_MASS_ERASE`` Kconfig option has been removed. If an
  erase is needed when flashing MCUboot, this should now be provided directly to the ``west``
  command e.g. ``west flash --erase``. (:github:`64703`)

Bluetooth
=========

* ATT now has its own TX buffer pool.
  If extra ATT buffers were configured using :kconfig:option:`CONFIG_BT_L2CAP_TX_BUF_COUNT`,
  they now instead should be configured through :kconfig:option:`CONFIG_BT_ATT_TX_COUNT`.
* The HCI implementation for both the Host and the Controller sides has been
  renamed for the IPC transport. The ``CONFIG_BT_RPMSG`` Kconfig option is now
  :kconfig:option:`CONFIG_BT_HCI_IPC`, and the ``zephyr,bt-hci-rpmsg-ipc``
  Devicetree chosen is now ``zephyr,bt-hci-ipc``. The existing sample has also
  been renamed, from ``samples/bluetooth/hci_rpmsg`` to
  ``samples/bluetooth/hci_ipc``. (:github:`64391`)
* The BT GATT callback list, appended to by :c:func:`bt_gatt_cb_register`, is no longer
  cleared on :c:func:`bt_enable`. Callbacks can now be registered before the initial
  call to :c:func:`bt_enable`, and should no longer be re-registered after a :c:func:`bt_disable`
  :c:func:`bt_enable` cycle. (:github:`63693`)
* The Bluetooth UUID has been modified to rodata in ``BT_UUID_DECLARE_16``, ``BT_UUID_DECLARE_32`
  and ``BT_UUID_DECLARE_128`` as the return value has been changed to `const`.
  Any pointer to a UUID must be prefixed with `const`, otherwise there will be a compilation warning.
  For example change ``struct bt_uuid *uuid = BT_UUID_DECLARE_16(xx)`` to
  ``const struct bt_uuid *uuid = BT_UUID_DECLARE_16(xx)``. (:github:`66136`)
* The :c:func:`bt_l2cap_chan_send` API no longer allocates buffers from the same pool as its `buf`
  parameter when segmenting SDUs into PDUs. In order to reproduce the previous behavior, the
  application should register the `alloc_seg` channel callback and allocate from the same pool as
  `buf`.
* The :c:func:`bt_l2cap_chan_send` API now requires the application to reserve
  enough bytes for the L2CAP headers. Call ``net_buf_reserve(buf,
  BT_L2CAP_SDU_CHAN_SEND_RESERVE);`` at buffer allocation time to do so.

* Mesh

  * The Bluetooth Mesh ``model`` declaration has been changed to add prefix ``const``.
    The ``model->user_data``, ``model->elem_idx`` and ``model->mod_idx`` field has been changed to
    the new runtime structure, replaced by ``model->rt->user_data``, ``model->rt->elem_idx`` and
    ``model->rt->mod_idx`` separately. (:github:`65152`)
  * The Bluetooth Mesh ``element`` declaration has been changed to add prefix ``const``.
    The ``elem->addr`` field has been changed to the new runtime structure, replaced by
    ``elem->rt->addr``. (:github:`65388`)
  * Deprecated :kconfig:option:`CONFIG_BT_MESH_PROV_DEVICE`. This option is
    replaced by new option :kconfig:option:`CONFIG_BT_MESH_PROVISIONEE` to
    be aligned with Mesh Protocol Specification v1.1, section 5.4. (:github:`64252`)
  * Removed the ``CONFIG_BT_MESH_V1d1`` Kconfig option.
  * Removed the ``CONFIG_BT_MESH_TX_SEG_RETRANS_COUNT``,
    ``CONFIG_BT_MESH_TX_SEG_RETRANS_TIMEOUT_UNICAST``,
    ``CONFIG_BT_MESH_TX_SEG_RETRANS_TIMEOUT_GROUP``, ``CONFIG_BT_MESH_SEG_ACK_BASE_TIMEOUT``,
    ``CONFIG_BT_MESH_SEG_ACK_PER_HOP_TIMEOUT``, ``BT_MESH_SEG_ACK_PER_SEGMENT_TIMEOUT``
    Kconfig options. They are superseded by the
    :kconfig:option:`CONFIG_BT_MESH_SAR_TX_SEG_INT_STEP`,
    :kconfig:option:`CONFIG_BT_MESH_SAR_TX_UNICAST_RETRANS_COUNT`,
    :kconfig:option:`CONFIG_BT_MESH_SAR_TX_UNICAST_RETRANS_WITHOUT_PROG_COUNT`,
    :kconfig:option:`CONFIG_BT_MESH_SAR_TX_UNICAST_RETRANS_INT_STEP`,
    :kconfig:option:`CONFIG_BT_MESH_SAR_TX_UNICAST_RETRANS_INT_INC`,
    :kconfig:option:`CONFIG_BT_MESH_SAR_TX_MULTICAST_RETRANS_COUNT`,
    :kconfig:option:`CONFIG_BT_MESH_SAR_TX_MULTICAST_RETRANS_INT`,
    :kconfig:option:`CONFIG_BT_MESH_SAR_RX_SEG_THRESHOLD`,
    :kconfig:option:`CONFIG_BT_MESH_SAR_RX_ACK_DELAY_INC`,
    :kconfig:option:`CONFIG_BT_MESH_SAR_RX_SEG_INT_STEP`,
    :kconfig:option:`CONFIG_BT_MESH_SAR_RX_DISCARD_TIMEOUT`,
    :kconfig:option:`CONFIG_BT_MESH_SAR_RX_ACK_RETRANS_COUNT` Kconfig options.


LoRaWAN
=======

* The API to register a callback to provide battery level information to the LoRaWAN stack has been
  renamed from ``lorawan_set_battery_level_callback`` to
  :c:func:`lorawan_register_battery_level_callback` and the return type is now ``void``. This
  is more consistent with similar functions for downlink and data rate changed callbacks.
  (:github:`65103`)

Networking
==========

* The CoAP public API has some minor changes to take into account. The
  :c:func:`coap_remove_observer` now returns a result if the observer was removed. This
  change is used by the newly introduced :ref:`coap_server_interface` subsystem. Also, the
  ``request`` argument for :c:func:`coap_well_known_core_get` is made ``const``.
  (:github:`64265`)

* CoAP observer events have moved from a callback function in a CoAP resource to the Network Events
  subsystem. The ``CONFIG_COAP_OBSERVER_EVENTS`` configuration option has been removed.
  (:github:`65936`)

* The CoAP public API function :c:func:`coap_pending_init` has changed. The parameter
  ``retries`` is replaced with a pointer to :c:struct:`coap_transmission_parameters`. This allows to
  specify retransmission parameters of the confirmable message. It is safe to pass a NULL pointer to
  use default values.
  (:github:`66482`)

* The CoAP public API functions :c:func:`coap_service_send` and :c:func:`coap_resource_send` have
  changed. An additional parameter pointer to :c:struct:`coap_transmission_parameters` has been
  added. It is safe to pass a NULL pointer to use default values. (:github:`66540`)

* The IGMP multicast library now supports IGMPv3. This results in a minor change to the existing
  api. The :c:func:`net_ipv4_igmp_join` now takes an additional argument of the type
  ``const struct igmp_param *param``. This allows IGMPv3 to exclude/include certain groups of
  addresses. If this functionality is not used or available (when using IGMPv2), you can safely pass
  a NULL pointer. IGMPv3 can be enabled using the Kconfig ``CONFIG_NET_IPV4_IGMPV3``.
  (:github:`65293`)

* The network stack now uses a separate IPv4 TTL (time-to-live) value for multicast packets.
  Before, the same TTL value was used for unicast and multicast packets.
  The IPv6 hop limit value is also changed so that unicast and multicast packets can have a
  different one. (:github:`65886`)

* The Ethernet phy APIs defined in ``<zephyr/net/phy.h>`` are removed from syscall list.
  The APIs were marked as callable from usermode but in practice this does not work as the device
  cannot be accessed from usermode thread. This means that the API calls will need to made
  from supervisor mode thread.

zcbor
=====

* If you have zcbor-generated code that relies on the zcbor libraries through Zephyr, you must
  regenerate the files using zcbor 0.8.0. Note that the names of generated types and members has
  been overhauled, so the code using the generated code must likely be changed.
  For example:

  * Leading single underscores and all double underscores are largely gone,
  * Names sometimes gain suffixes like ``_m`` or ``_l`` for disambiguation.
  * All enum (choice) names have now gained a ``_c`` suffix, so the enum name no longer matches
    the corresponding member name exactly (because this broke C++ namespace rules).

* The function :c:func:`zcbor_new_state`, :c:func:`zcbor_new_decode_state` and the macro
  :c:macro:`ZCBOR_STATE_D` have gained new parameters related to decoding of unordered maps.
  Unless you are using that new functionality, these can all be set to NULL or 0.

* The functions :c:func:`zcbor_bstr_put_term` and :c:func:`zcbor_tstr_put_term` have gained a new
  parameter ``maxlen``, referring to the maximum length of the parameter ``str``.
  This parameter is passed directly to :c:func:`strnlen` under the hood.

* The function :c:func:`zcbor_tag_encode` has been renamed to :c:func:`zcbor_tag_put`.

* Printing has been changed significantly, e.g. :c:func:`zcbor_print` is now called
  :c:func:`zcbor_log`, and :c:func:`zcbor_trace` with no parameters is gone, and in its place are
  :c:func:`zcbor_trace_file` and :c:func:`zcbor_trace`, both of which take a ``state`` parameter.

Other Subsystems
================

* MCUmgr applications that make use of serial transports (shell or UART) must now select
  :kconfig:option:`CONFIG_CRC`, this was previously erroneously selected if MCUmgr was enabled,
  when for non-serial transports it was not needed. (:github:`64078`)

* Touchscreen drivers :dtcompatible:`focaltech,ft5336` and
  :dtcompatible:`goodix,gt911` were using the incorrect polarity for the
  respective ``reset-gpios``. This has been fixed so those signals now have to
  be flagged as :c:macro:`GPIO_ACTIVE_LOW` in the devicetree. (:github:`64800`)

* The :kconfig:option:`ZBUS_MSG_SUBSCRIBER_NET_BUF_DYNAMIC`
  and :kconfig:option:`ZBUS_MSG_SUBSCRIBER_NET_BUF_STATIC`
  zbus options are renamed. Instead, the new :kconfig:option:`ZBUS_MSG_SUBSCRIBER_BUF_ALLOC_DYNAMIC`
  and :kconfig:option:`ZBUS_MSG_SUBSCRIBER_BUF_ALLOC_STATIC` options should be used.

Xtensa
======

* :kconfig:option:`CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC` no longer has a default in
  the architecture layer. Instead, SoCs or boards will need to define it.

Recommended Changes
*******************

* New macros available for ST sensor DT properties setting. These macros have a self-explanatory
  name that helps in recognizing what the property setting means (e.g. LSM6DSV16X_DT_ODR_AT_60Hz).
  (:github:`65410`)

* Users of :ref:`native_posix<native_posix>` are recommended to migrate to
  :ref:`native_sim<native_sim>`. :ref:`native_sim<native_sim>` supports all its use cases,
  and should be a drop-in replacement for most.
