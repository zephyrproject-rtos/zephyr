:orphan:

.. _migration_4.0:

Migration guide to Zephyr v4.0.0 (Working Draft)
################################################

This document describes the changes required when migrating your application from Zephyr v3.7.0 to
Zephyr v4.0.0.

Any other changes (not directly related to migrating applications) can be found in
the :ref:`release notes<zephyr_4.0>`.

.. contents::
    :local:
    :depth: 2

Build System
************

Kernel
******

Boards
******

* :ref:`native_posix<native_posix>` has been deprecated in favour of
  :ref:`native_sim<native_sim>` (:github:`76898`).
* Nordic nRF53 and nRF91 based boards can use the common devicetree overlays in ``dts/common/nordic``
  to define default flash and ram partitioning based on TF-M.

* STM32WBA: The command used for fetching blobs required to build ble applications is now
  `west blobs fetch hal_stm32` instead of `west blobs fetch stm32`.

STM32
=====

* On all official STM32 boards, `west flash` selects STM32CubeProgrammer as the default west runner.
  If you want to enforce the selection of another runner like OpenOCD or pyOCD for flashing, you should
  specify it using the west `--runner` or `-r` option. (:github:`75284`)

Modules
*******

Mbed TLS
========

* The Kconfig options ``CONFIG_MBEDTLS_TLS_VERSION_1_0`` and ``CONFIG_MBEDTLS_TLS_VERSION_1_1``
  have been removed because Mbed TLS doesn't support TLS 1.0 and 1.1 anymore since v3.0. (:github:`76833`)

Trusted Firmware-M
==================

LVGL
====

zcbor
=====

* Updated the zcbor library to version 0.9.0.
  Full release notes at https://github.com/NordicSemiconductor/zcbor/blob/0.9.0/RELEASE_NOTES.md
  Migration guide at https://github.com/NordicSemiconductor/zcbor/blob/0.9.0/MIGRATION_GUIDE.md
  Migration guide copied here:

  * `zcbor_simple_*()` functions have been removed to avoid confusion about their use.
    They are still in the C file because they are used by other functions.
    Instead, use the specific functions for the currently supported simple values, i.e.
    `zcbor_bool_*()`, `zcbor_nil_*()`, and `zcbor_undefined_*()`.
    If a removed variant is strictly needed, add your own forward declaration in your code.

  * Code generation naming:

    * More C keywords are now capitalized to avoid naming collision.
      You might have to capitalize some instances if your code was generated to have those names.

    * A fix was made to the naming of bstr elements with a .size specifier, which might mean that these elements change name in your code when you regenerate.

Device Drivers and Devicetree
*****************************

* The ``compatible`` of the LiteX ethernet controller has been renamed from
  ``litex,eth0`` to :dtcompatible:`litex,liteeth`. (:github:`75433`)

* The ``compatible`` of the LiteX uart controller has been renamed from
  ``litex,uart0`` to :dtcompatible:`litex,uart`. (:github:`74522`)

* The devicetree bindings for the Microchip ``mcp23xxx`` series have been split up. Users of
  ``microchip,mcp230xx`` and ``microchip,mcp23sxx`` should change their devicetree ``compatible``
  values to the specific chip variant, e.g. :dtcompatible:`microchip,mcp23017`.
  The ``ngpios`` devicetree property has been removed, since it is implied by the model name.
  Chip variants with open-drain outputs (``mcp23x09``, ``mcp23x18``) now correctly reflect this in
  their driver API, users of these devices should ensure they pass appropriate values to
  :c:func:`gpio_pin_set`. (:github:`65797`)

Controller Area Network (CAN)
=============================

Display
=======

Enhanced Serial Peripheral Interface (eSPI)
===========================================

GNSS
====

* The u-blox M10 driver has been renamed to M8 as it only supports M8 based devices.
  Existing devicetree compatibles should be updated to :dtcompatible:`u-blox,m8`, and Kconfig
  symbols swapped to :kconfig:option:`CONFIG_GNSS_U_BLOX_M8`.

* The APIs :c:func:`gnss_set_periodic_config` and :c:func:`gnss_get_periodic_config` have
  been removed. (:github:`76392`)

Input
=====

* :c:macro:`INPUT_CALLBACK_DEFINE` has now an extra ``user_data`` void pointer
  argument that can be used to reference any user data structure. To restore
  the current behavior it can be set to ``NULL``. A ``void *user_data``
  argument has to be added to the callback function arguments.

* The :dtcompatible:`analog-axis` ``invert`` property has been renamed to
  ``invert-input`` (there's now an ``invert-output`` available as well).

Interrupt Controller
====================

LED Strip
=========

SDHC
====

* The NXP USDHC driver now assumes a card is present if no card detect method
  is configured, instead of using the peripheral's internal card detect signal
  to check for card presence. To use the internal card detect signal, the
  devicetree property ``detect-cd`` should be added to the USDHC node in use.

Sensors
=======

* The existing driver for the Microchip MCP9808 temperature sensor transformed and renamed
  to support all JEDEC JC 42.4 compatible temperature sensors. It now uses the
  :dtcompatible:`jedec,jc-42.4-temp` compatible string instead to the ``microchip,mcp9808`` string.

Serial
======

Regulator
=========

* Internal regulators present in nRF52/53 series can now be configured using
  devicetree. The Kconfig options :kconfig:option:`CONFIG_SOC_DCDC_NRF52X`,
  :kconfig:option:`CONFIG_SOC_DCDC_NRF52X_HV`,
  :kconfig:option:`CONFIG_SOC_DCDC_NRF53X_APP`,
  :kconfig:option:`CONFIG_SOC_DCDC_NRF53X_NET` and
  :kconfig:option:`CONFIG_SOC_DCDC_NRF53X_HV` selected by board-level Kconfig
  options have been deprecated.

  Example for nRF52 series:

  .. code-block:: devicetree

      /* configure REG/REG1 in DC/DC mode */
      &reg/reg1 {
          regulator-initial-mode = <NRF5X_REG_MODE_DCDC>;
      };

      /* enable REG0 (HV mode) */
      &reg0 {
          status = "okay";
      };

  Example for nRF53 series:

  .. code-block:: devicetree

      /* configure VREGMAIN in DC/DC mode */
      &vregmain {
          regulator-initial-mode = <NRF5X_REG_MODE_DCDC>;
      };

      /* configure VREGRADIO in DC/DC mode */
      &vregradio {
          regulator-initial-mode = <NRF5X_REG_MODE_DCDC>;
      };

      /* enable VREGH (HV mode) */
      &vregh {
          status = "okay";
      };

Bluetooth
*********

Bluetooth HCI
=============

Bluetooth Mesh
==============

Bluetooth Audio
===============

* The Volume Renderer callback functions :code:`bt_vcp_vol_rend_cb.state` and
  :code:`bt_vcp_vol_rend_cb.flags` for VCP now contain an additional parameter for
  the connection.
  This needs to be added to all instances of VCP Volume Renderer callback functions defined.
  (:github:`76992`)

* The Unicast Server has a new registration function :c:func:`bt_bap_unicast_server_register` which
  takes a :c:struct:`bt_bap_unicast_server_register_param` as argument. This allows the Unicast
  Server to dynamically register Source and Sink ASE count at runtime. The old
  :kconfig:option:`CONFIG_BT_ASCS_ASE_SRC_COUNT` and :kconfig:option:`CONFIG_BT_ASCS_ASE_SNK_COUNT`
  has been renamed to :kconfig:option:`CONFIG_BT_ASCS_MAX_ASE_SRC_COUNT` and
  :kconfig:option:`CONFIG_BT_ASCS_MAX_ASE_SNK_COUNT` to reflect that they now serve as a
  compile-time maximum configuration of ASEs to be used.
  :c:func:`bt_bap_unicast_server_register` needs to be called once before using the Unicast Server,
  and more specfically prior to calling :c:func:`bt_bap_unicast_server_register_cb` for the first
  time. It does not need to be called again until the new function
  :c:func:`bt_bap_unicast_server_unregister` has been called.
  (:github:`76632`)

Bluetooth Classic
=================

Bluetooth Host
==============

Bluetooth Crypto
================

Networking
**********

* The CoAP public API functions :c:func:`coap_get_block1_option` and
  :c:func:`coap_get_block2_option` have changed. The ``block_number`` pointer
  type has changed from ``uint8_t *`` to ``uint32_t *``. Additionally,
  :c:func:`coap_get_block2_option` now accepts an additional ``bool *has_more``
  parameter, to store the value of the more flag. (:github:`76052`)

* The Ethernet bridge shell is moved under network shell. This is done so that
  all the network shell activities can be found under ``net`` shell command.
  After this change the bridge shell is used by ``net bridge`` command.

* The Ethernet bridging code is changed to allow similar configuration experience
  as in Linux. The bridged Ethernet interface can be used normally even if bridging
  is enabled. The actual bridging is done by a separate virtual network interface that
  directs network packets to bridged Ethernet interfaces.
  The :c:func:`eth_bridge_iface_allow_tx` is removed as it is not needed because the
  bridged Ethernet interface can send and receive data normally.
  The :c:func:`eth_bridge_listener_add` and :c:func:`eth_bridge_listener_remove` are
  removed as same functionality can be achieved using promiscuous API.
  Because the bridge interface is a normal network interface,
  the :c:func:`eth_bridge_iface_add` and :c:func:`eth_bridge_iface_remove`
  will take network interface pointer as a first parameter.

* To facilitate use outside of the networking subsystem, the network buffer header file was renamed
  from :zephyr_file:`include/zephyr/net/buf.h` to :zephyr_file:`include/zephyr/net_buf.h` and the
  implementation moved to :zephyr_file:`lib/net_buf/`. (:github:`78009`)

Other Subsystems
****************

Flash map
=========

 * ``CONFIG_SPI_NOR_IDLE_IN_DPD`` has been removed from the :kconfig:option:`CONFIG_SPI_NOR`
   driver. An enhanced version of this functionality can be obtained by enabling
   :ref:`pm-device-runtime` on the device (Tunable with
   :kconfig:option:`CONFIG_SPI_NOR_ACTIVE_DWELL_MS`).

hawkBit
=======

MCUmgr
======

Modem
=====

Shell
=====

* ``kernel threads`` and ``kernel stacks`` shell command have been renamed to
  ``kernel thread list`` & ``kernel thread stacks``

Architectures
*************
