:orphan:

.. _migration_3.7:

Migration guide to Zephyr v3.7.0 (Working Draft)
################################################

This document describes the changes required when migrating your application from Zephyr v3.6.0 to
Zephyr v3.7.0.

Any other changes (not directly related to migrating applications) can be found in
the :ref:`release notes<zephyr_3.7>`.

.. contents::
    :local:
    :depth: 2

Build System
************

* Completely overhauled the way SoCs and boards are defined. This requires all
  out-of-tree SoCs and boards to be ported to the new model. See the
  :ref:`hw_model_v2` for more detailed information. (:github:`69607`)

Kernel
******

Boards
******

* Reordered D1 and D0 in the `pro_micro` connector gpio-map for SparkFun Pro Micro RP2040 to match
  original Pro Micro definition. Out-of-tree shields must be updated to reflect this change. (:github:`69994`)
* ITE: Rename all SoC variant Kconfig options, e.g., ``CONFIG_SOC_IT82202_AX`` is renamed to
  ``CONFIG_SOC_IT82202AX``.
  All symbols are renamed as follows: ``SOC_IT81202BX``, ``SOC_IT81202CX``, ``SOC_IT81302BX``,
  ``SOC_IT81302CX``, ``SOC_IT82002AW``, ``SOC_IT82202AX``, ``SOC_IT82302AX``.
  And, rename the ``SOC_SERIES_ITE_IT8XXX2`` to ``SOC_SERIES_IT8XXX2``. (:github:`71680`)

Modules
*******

MbedTLS
=======

* The hash algorithms SHA-384, SHA-512, MD5 and SHA-1 are not enabled by default anymore.
  Their respective Kconfig options now need to be explicitly enabled to be able to use them.

MCUboot
=======

zcbor
=====

Device Drivers and Devicetree
*****************************

* The :dtcompatible:`nxp,kinetis-pit` pit driver has changed it's compatible
  to :dtcompatible:`nxp,pit` and has been updated to support multiple channels.
  To configure the individual channels, you must add a child node with the
  compatible :dtcompatible:`nxp,pit-channel` and configure as below.
  The :kconfig:option:`CONFIG_COUNTER_MCUX_PIT` has also been renamed to
  :kconfig:option:`CONFIG_COUNTER_NXP_PIT` with regards to the renaming
  of the binding for the pit. (:github:`66336`)
  example:

  .. code-block:: devicetree

    / {
        pit0: pit@40037000 {
            /* Other Pit DT Attributes */
            compatible = "nxp,pit";
            status = "disabled";
            num-channels = <1>;
            #address-cells = <1>;
            #size-cells = <0>;

            pit0_channel0: pit0_channel@0 {
                compatible = "nxp,pit-channel";
                reg = <0>;
                status = "disabled";
            };
    };

* The :dtcompatible:`nxp,kinetis-ethernet` has been deprecated in favor of
  :dtcompatible:`nxp,enet`. All in tree SOCs were converted to use this new schema.
  Thus, all boards using NXP's ENET peripheral will need to align to this binding
  in DT, which also comes with a different version driver. Alternatively,
  the Ethernet node can be deleted and redefined as the old binding to use
  the deprecated legacy driver. The primary advantage of the new binding
  is to be able to abstract an arbitrary phy through the mdio API. (:github:`70400`)
  Example of a basic board level ENET DT definition:

  .. code-block:: devicetree

    &enet_mac {
        status = "okay";
        pinctrl-0 = <&pinmux_enet>;
        pinctrl-names = "default";
        phy-handle = <&phy>;
        zephyr,random-mac-address;
        phy-connection-type = "rmii";
    };

    &enet_mdio {
        status = "okay";
        pinctrl-0 = <&pinmux_enet_mdio>;
        pinctrl-names = "default";
        phy: phy@3 {
            compatible = "ethernet-phy";
            reg = <3>;
            status = "okay";
        };
    };

* Some of the driver API structs have been rename to have the required ``_driver_api`` suffix. (:github:`72182`)
  The following types have been renamed:

  * ``emul_sensor_backend_api`` to :c:struct:`emul_sensor_driver_api`
  * ``emul_bbram_backend_api`` to :c:struct:`emul_bbram_driver_api`
  * ``usbc_ppc_drv`` to :c:struct:`usbc_ppc_driver_api`

* The driver for :dtcompatible:`maxim,max31790` got split up into a MFD and an
  actual PWM driver. (:github:`68433`)
  Previously, an instance of this device could have been defined like this:

  .. code-block:: devicetree

    max31790_max31790: max31790@20 {
        compatible = "maxim,max31790";
        status = "okay";
        reg = <0x20>;
        pwm-controller;
        #pwm-cells = <2>;
    };

  This can be converted to:

  .. code-block:: devicetree

    max31790_max31790: max31790@20 {
        compatible = "maxim,max31790";
        status = "okay";
        reg = <0x20>;

        max31790_max31790_pwm: max31790_max31790_pwm {
            compatible = "maxim,max31790-pwm";
            status = "okay";
            pwm-controller;
            #pwm-cells = <2>;
        };
    };

Analog-to-Digital Converter (ADC)
=================================

Bluetooth HCI
=============

 * The ``BT_HCI_VS_EXT`` Kconfig option was deleted and the feature is now included in the
   :kconfig:option:`BT_HCI_VS` Kconfig option.
 * The ``BT_HCI_VS_EVT`` Kconfig option was removed, since vendor event support is implicit if
   the :kconfig:option:`BT_HCI_VS` option is enabled.
 * The bt_read_static_addr() API was removed. This wasn't really a completely public API, but
   since it was exposed by the public hci_driver.h header file the removal is mentioned here.
   Enable the :kconfig:option:`BT_HCI_VS` Kconfig option instead, and use vendor specific HCI
   commands API to get the Controller's Bluetooth static address when available.

Charger
=======

* Dropped ``constant-charge-current-max-microamp`` property in ``charger_max20335`` driver because
  it did not reflect real chip functionality. (:github:`69910`)

* Added enum key to ``constant-charge-voltage-max-microvolt`` property in ``maxim,max20335-charger``
  binding to indicate invalid devicetree values at build time. (:github:`69910`)

Controller Area Network (CAN)
=============================

* Removed the following deprecated CAN controller devicetree properties. Out-of-tree boards using
  these properties need to switch to using the ``bus-speed``, ``sample-point``, ``bus-speed-data``,
  and ``sample-point-data`` devicetree properties for specifying the initial CAN bitrate:

  * ``sjw``
  * ``prop-seg``
  * ``phase-seg1``
  * ``phase-seg1``
  * ``sjw-data``
  * ``prop-seg-data``
  * ``phase-seg1-data``
  * ``phase-seg1-data``

  (:github:`68714`)

* Support for manual bus-off recovery was reworked (:github:`69460`):

  * Automatic bus recovery will always be enabled upon driver initialization regardless of Kconfig
    options. Since CAN controllers are initialized in "stopped" state, no unwanted bus-off recovery
    will be started at this point.
  * The Kconfig ``CONFIG_CAN_AUTO_BUS_OFF_RECOVERY`` was renamed (and inverted) to
    :kconfig:option:`CONFIG_CAN_MANUAL_RECOVERY_MODE`, which is disabled by default. This Kconfig
    option enables support for the :c:func:`can_recover()` API function and a new manual recovery mode
    (see the next bullet).
  * A new CAN controller operational mode :c:macro:`CAN_MODE_MANUAL_RECOVERY` was added. Support for
    this is only enabled if :kconfig:option:`CONFIG_CAN_MANUAL_RECOVERY_MODE` is enabled. Having
    this as a mode allows applications to inquire whether the CAN controller supports manual
    recovery mode via the :c:func:`can_get_capabilities` API function. The application can then
    either fail initialization or rely on automatic bus-off recovery. Having this as a mode
    furthermore allows CAN controller drivers not supporting manual recovery mode to fail early in
    :c:func:`can_set_mode` during application startup instead of failing when :c:func:`can_recover`
    is called at a later point in time.

Display
=======

* ST7735R based displays now use the MIPI DBI driver class. These displays
  must now be declared within a MIPI DBI driver wrapper device, which will
  manage interfacing with the display. Note that the `cmd-data-gpios` pin has
  changed polarity with this update, to align better with the new
  `dc-gpios` name. For an example, see below:

  .. code-block:: devicetree

    /* Legacy ST7735R display definition */
    &spi0 {
        st7735r: st7735r@0 {
            compatible = "sitronix,st7735r";
            reg = <0>;
            spi-max-frequency = <32000000>;
            reset-gpios = <&gpio0 6 GPIO_ACTIVE_LOW>;
            cmd-data-gpios = <&gpio0 12 GPIO_ACTIVE_LOW>;
            ...
        };
    };

    /* New display definition with MIPI DBI device */

    #include <zephyr/dt-bindings/mipi_dbi/mipi_dbi.h>

    ...

    mipi_dbi {
        compatible = "zephyr,mipi-dbi-spi";
        reset-gpios = <&gpio0 6 GPIO_ACTIVE_LOW>;
        dc-gpios = <&gpio0 12 GPIO_ACTIVE_HIGH>;
        spi-dev = <&spi0>;
        #address-cells = <1>;
        #size-cells = <0>;

        st7735r: st7735r@0 {
            compatible = "sitronix,st7735r";
            reg = <0>;
            mipi-max-frequency = <32000000>;
            mipi-mode = <MIPI_DBI_MODE_SPI_4WIRE>;
            ...
        };
    };

Enhanced Serial Peripheral Interface (eSPI)
===========================================

* The macros ``ESPI_SLAVE_TO_MASTER`` and ``ESPI_MASTER_TO_SLAVE`` were renamed to
  ``ESPI_TARGET_TO_CONTROLLER`` and ``ESPI_CONTROLLER_TO_TARGET`` respectively to reflect
  the new terminology in eSPI 1.5 specification.
  The enum values ``ESPI_VWIRE_SIGNAL_SLV_BOOT_STS``, ``ESPI_VWIRE_SIGNAL_SLV_BOOT_DONE`` and
  all ``ESPI_VWIRE_SIGNAL_SLV_GPIO_<NUMBER>`` signals were renamed to
  ``ESPI_VWIRE_SIGNAL_TARGET_BOOT_STS``, ``ESPI_VWIRE_SIGNAL_TARGET_BOOT_DONE`` and
  ``ESPI_VWIRE_SIGNAL_TARGET_GPIO_<NUMBER>`` respectively to reflect the new terminology
  in eSPI 1.5 specification. (:github:`68492`)

Flash
=====

General Purpose I/O (GPIO)
==========================

GNSS
====

* Basic power management support has been added to the ``gnss-nmea-generic`` driver.
  If ``CONFIG_PM_DEVICE=y`` the driver is now initialized in suspended mode and the
  application needs to call :c:func:`pm_device_action_run` with :c:macro:`PM_DEVICE_ACTION_RESUME`
  to start up the driver. (:github:`71774`)

Input
=====

* The ``analog-axis`` deadzone calibration value has been changed to be
  relative to the raw ADC values, similarly to min and max. The data structures
  and properties have been renamed to reflect that (from ``out-deadzone`` to
  ``in-deadzone``) and when migrating to the new definition the value should be
  scaled accordingly. (:github:`70377`)

* The ``holtek,ht16k33-keyscan`` driver has been converted to use the
  :ref:`input` subsystem, callbacks have to be migrated to use the input APIs,
  :dtcompatible:`zephyr,kscan-input` can be used for backward compatibility. (:github:`69875`)

Interrupt Controller
====================

LED Strip
=========

* The property ``in-gpios`` defined in :dtcompatible:`worldsemi,ws2812-gpio` has been
  renamed to ``gpios``. (:github:`68514`)

* The ``chain-length`` and ``color-mapping`` properties have been added to all LED strip bindings
  and are now mandatory.

* Added a new mandatory ``length`` function which returns the length (number of pixels) of an LED
  strip device.

* Made ``update_channels`` function optional and removed unimplemented functions.

Sensors
=======

Serial
======

Timer
=====

regulator
=========

* The :dtcompatible:`nxp,vref` driver no longer supports the ground selection function,
  as this setting should not be modified by the user. The DT property ``nxp,ground-select``
  has been removed, users should remove this property from their devicetree if it is present.
  (:github:`70642`)

Watchdog
========

* The ``nuvoton,npcx-watchdog`` driver has been changed to extend the max timeout period.
  The time of one watchdog count varies with the different pre-scalar settings.
  Removed :kconfig:option:`CONFIG_WDT_NPCX_DELAY_CYCLES` because it is no longer suitable to
  set the leading warning time.
  Instead, added the :kconfig:option:`CONFIG_WDT_NPCX_WARNING_LEADING_TIME_MS` to set
  the leading warning time in milliseconds.

Bluetooth
*********

Bluetooth Mesh
==============

* The model metadata pointer declaration of :c:struct:`bt_mesh_model` has been changed
  to add ``const`` qualifiers. The data pointer of :c:struct:`bt_mesh_models_metadata_entry`
  got ``const`` qualifier too. The model's metadata structure and metadata raw value
  can be declared as permanent constants in the non-volatile memory. (:github:`69679`)

* The model metadata pointer declaration of :c:struct:`bt_mesh_model` has been changed
  to a single ``const *`` and redundant metadata pointer from :c:struct:`bt_mesh_health_srv`
  is removed. Consequently, :code:`BT_MESH_MODEL_HEALTH_SRV` definition is changed
  to use variable argument notation. Now, when your implementation
  supports :kconfig:option:`CONFIG_BT_MESH_LARGE_COMP_DATA_SRV` and when you need to
  specify metadata for Health Server model, simply pass metadata as the last argument
  to the :code:`BT_MESH_MODEL_HEALTH_SRV` macro, otherwise omit the last argument. (:github:`71281`)

Bluetooth Audio
===============

* :kconfig:option:`CONFIG_BT_ASCS`, :kconfig:option:`CONFIG_BT_PERIPHERAL` and
  :kconfig:option:`CONFIG_BT_ISO_PERIPHERAL` are not longer `select`ed automatically when
  enabling :kconfig:option:`CONFIG_BT_BAP_UNICAST_SERVER`, and these must now be set explicitly
  in the project configuration file. (:github:`71993`)
* The discover callback functions :code:`bt_cap_initiator_cb.unicast_discovery_complete`` and
  :code:`bt_cap_commander_cb.discovery_complete`` for CAP now contain an additional parameter for
  the set member.
  This needs to be added to all instances of CAP discovery callback functions defined.
  (:github:`72797`)

Bluetooth Classic
=================

* The source files of Host BR/EDR have been moved to ``subsys/bluetooth/host/classic``.
  The Header files of Host BR/EDR have been moved to ``include/zephyr/bluetooth/classic``.
  Removed the :kconfig:option:`CONFIG_BT_BREDR`. It is replaced by new option
  :kconfig:option:`CONFIG_BT_CLASSIC`. (:github:`69651`)

Bluetooth Host
==============

* The advertiser options :code:`BT_LE_ADV_OPT_USE_NAME` and :code:`BT_LE_ADV_OPT_FORCE_NAME_IN_AD`
  are deprecated in this release. The application need to include the device name explicitly. One
  way to do it is by adding the following to the advertising data or scan response data passed to
  the host:

  .. code-block:: c

   BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1)

  (:github:`71686`)

* The field :code:`init_credits` in :c:type:`bt_l2cap_le_endpoint` has been removed as it was no
  longer used in Zephyr 3.4.0 and later. Any references to this field should be removed. No further
  action is needed.

Networking
**********

* Deprecate the :kconfig:option:`CONFIG_NET_SOCKETS_POSIX_NAMES` option. It is a legacy option
  and was used to allow user to call BSD socket API while not enabling POSIX API.
  This could cause complications when building applications that wanted to enable the
  :kconfig:option:`CONFIG_POSIX_API` option. This means that if the application wants to use
  normal BSD socket interface, then it needs to enable :kconfig:option:`CONFIG_POSIX_API`.
  If the application does not want or is not able to enable that option, then the socket API
  calls need to be prefixed by a ``zsock_`` string.
  All the sample applications that use BSD socket interface are changed to enable
  :kconfig:option:`CONFIG_POSIX_API`. Internally the network stack will not enable POSIX API
  option which means that various network libraries that use sockets, are converted to
  use the ``zsock_*`` API calls. (:github:`69950`)

* The zperf zperf_results struct is changed to support 64 bits transferred bytes (total_len)
  and test duration (time_in_us and client_time_in_us), instead of 32 bits. This will make
  the long-duration zperf test show with correct throughput result. (:github:`69500`)

* Each IPv4 address assigned to a network interface has an IPv4 netmask
  tied to it instead of being set for the whole interface.
  If there is only one IPv4 address specified for a network interface,
  nothing changes from the user point of view. But, if there is more than
  one IPv4 address / network interface, the netmask must be specified
  for each IPv4 address separately. (:github:`68419`)

* Virtual network interface API no longer has the `input` callback. The input callback was
  used to read the inner IPv4/IPv6 packets in an IP tunnel. This incoming tunnel read is now
  implemented in `recv` callback. (:github:`70549`)

* Virtual LAN (VLAN) implementation is changed to use the Virtual network interfaces.
  There are no API changes, but the type of a VLAN network interface is changed from `ETHERNET`
  to `VIRTUAL`. This could require changes to the code that sets the VLAN tags to a network
  interface. For example in the `net_eth_is_vlan_enabled()` API, the 2nd interface parameter
  must point to the main Ethernet interface, and not to the VLAN interface. (:github:`70345`)

* Modified the ``wifi connect`` command to use key-value format for the arguments. In the
  previous implementation, we were identifying an option using its position in the argument string.
  This made it difficult to deal with optional arguments or extending the support
  for other options. Having this key-value format makes it easier to extend the options that
  can be passed to the connect command.
  ``wifi -h`` will give more information about the usage of connect command.
  (:github:`70024`)

* The Kconfig ``CONFIG_NET_TCP_ACK_TIMEOUT`` has been deprecated. Its usage was
  limited to TCP handshake only, and in such case the total timeout should depend
  on the total retransmission timeout (as in other cases) making the config
  redundant and confusing. Use ``CONFIG_NET_TCP_INIT_RETRANSMISSION_TIMEOUT`` and
  ``CONFIG_NET_TCP_RETRY_COUNT`` instead to control the total timeout at the
  TCP level. (:github:`70731`)

* In LwM2M API, the callback type :c:type:`lwm2m_engine_set_data_cb_t` has now an additional
  parameter ``offset``. This parameter is used to indicate the offset of the data
  during a Coap Block-wise transfer. Any post write, validate or some firmware callbacks
  should be updated to include this parameter. (:github:`72590`)



Other Subsystems
****************

hawkBit
=======

* :kconfig:option:`CONFIG_HAWKBIT_PORT` is now an int instead of a string.
  :kconfig:option:`CONFIG_SETTINGS` needs to be enabled to use hawkBit, as it now uses the
  settings subsystem to store the hawkBit configuration. (:github:`68806`)

LoRaWAN
=======

MCUmgr
======

* The support for SHA-256 (when using checksum/hash functions), previously provided
  by either TinyCrypt or MbedTLS, is now provided by either PSA or MbedTLS.
  PSA is the recommended API going forward, however, if it is not already enabled
  (:kconfig:option:`CONFIG_MBEDTLS_PSA_CRYPTO_CLIENT`) and you have tight code size
  constraints, you may be able to save 1.3 KB by using MbedTLS instead.

Modem
=====

* The ``CONFIG_MODEM_CHAT_LOG_BUFFER`` Kconfig option was
  renamed to :kconfig:option:`CONFIG_MODEM_CHAT_LOG_BUFFER_SIZE`. (:github:`70405`)

Shell
=====

State Machine Framework
=======================

* The :c:macro:`SMF_CREATE_STATE` macro now always takes 5 arguments. The amount of arguments is
  now independent of the values of :kconfig:option:`CONFIG_SMF_ANCESTOR_SUPPORT` and
  :kconfig:option:`CONFIG_SMF_INITIAL_TRANSITION`. If the additional arguments are not used, they
  have to be set to ``NULL``. (:github:`71250`)
* SMF now follows a more UML-like transition flow when the transition source is a parent of the
  state called by :c:func:`smf_run_state`. Exit actions up to (but not including) the Least Common
  Ancestor of the transition source and target state will be executed, as will entry actions from
  (but not including) the LCA down to the target state. (:github:`71675`)
* Previously, calling :c:func:`smf_set_state` with a ``new_state`` set to NULL would execute all
  exit actions from the current state to the topmost parent, with the expectation the topmost exit
  action would terminate the state machine. Passing ``NULL`` is now not allowed. Instead create a
  'terminate' state at the top level, and call :c:func:`smf_set_terminate` from its entry action.

ZBus
====

Userspace
*********

Architectures
*************

* Function :c:func:`arch_start_cpu` has been renamed to :c:func:`arch_cpu_start`. (:github:`64987`)

* x86

  * Kconfigs ``CONFIG_DISABLE_SSBD`` and ``CONFIG_ENABLE_EXTENDED_IBRS``
    are deprecated. Use :kconfig:option:`CONFIG_X86_DISABLE_SSBD` and
    :kconfig:option:`CONFIG_X86_ENABLE_EXTENDED_IBRS` instead. (:github:`69690`)

* POSIX arch:

  * LLVM fuzzing support has been refactored. A test application now needs to provide its own
    ``LLVMFuzzerTestOneInput()`` hook instead of relying on a board provided one. Check
    ``samples/subsys/debug/fuzz/`` for an example. (:github:`71378`)

Xtensa
======
