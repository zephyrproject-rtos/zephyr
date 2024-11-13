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

* Removed the ``CONFIG_MCUBOOT_CMAKE_WEST_SIGN_PARAMS`` Kconfig option as ``west sign`` is no
  longer called by the build system when signing images for MCUboot.

* The imgtool part of ``west sign`` has been deprecated, options to be supplied to imgtool when
  signing should be set in :kconfig:option:`CONFIG_MCUBOOT_EXTRA_IMGTOOL_ARGS` instead.

Kernel
******

* Removed the deprecated :kconfig:option:`CONFIG_MP_NUM_CPUS`, application should be updated to use
  :kconfig:option:`CONFIG_MP_MAX_NUM_CPUS` instead.

Boards
******

* :ref:`native_posix<native_posix>` has been deprecated in favour of
  :ref:`native_sim<native_sim>` (:github:`76898`).
* Nordic nRF53 and nRF91 based boards can use the common devicetree overlays in ``dts/common/nordic``
  to define default flash and ram partitioning based on TF-M.

* STM32WBA: The command used for fetching blobs required to build ble applications is now
  ``west blobs fetch hal_stm32`` instead of ``west blobs fetch stm32``.

* Board ``qemu_xtensa`` is deprecated. Use ``qemu_xtensa/dc233c`` instead.

Devicetree
**********

* The :c:macro:`DT_REG_ADDR` macro and its variants are now expanding into an
  unsigned literals (i.e. with a ``U`` suffix). To use addresses as devicetree
  indexes use the :c:macro:`DT_REG_ADDR_RAW` variants.
* The :c:macro:`DT_REG_SIZE` macro and its variants are also expanding into
  unsigned literals, no raw variants are provided at this stage.

STM32
=====

* On all official STM32 boards, ``west flash`` selects STM32CubeProgrammer as the default west runner.
  If you want to enforce the selection of another runner like OpenOCD or pyOCD for flashing, you should
  specify it using the west ``--runner`` or ``-r`` option. (:github:`75284`)
* ADC: Domain clock needs to be explicitly defined if property st,adc-clock-source = <ASYNC> is used.

Modules
*******

Mbed TLS
========

* The Kconfig options ``CONFIG_MBEDTLS_TLS_VERSION_1_0`` and ``CONFIG_MBEDTLS_TLS_VERSION_1_1``
  have been removed because Mbed TLS doesn't support TLS 1.0 and 1.1 anymore since v3.0. (:github:`76833`)
* The following Kconfig symbols were renamed (:github:`76408`):
  * ``CONFIG_MBEDTLS_ENTROPY_ENABLED`` is now :kconfig:option:`CONFIG_MBEDTLS_ENTROPY_C`,
  * ``CONFIG_MBEDTLS_ZEPHYR_ENTROPY`` is now :kconfig:option:`CONFIG_MBEDTLS_ENTROPY_POLL_ZEPHYR`.

* The Kconfig option ``CONFIG_MBEDTLS_SSL_EXPORT_KEYS`` was removed because the
  corresponding build symbol was removed in Mbed TLS 3.1.0 and is now assumed to
  be enabled. (:github:`77657`)

TinyCrypt
=========

Albeit the formal deprecation of TinyCrypt is not started yet, its removal from
the Zephyr codebase is. Formal deprecation will happen in the next release.

Trusted Firmware-M
==================

* The security counter used for the hardware rollback protection now comes explicitly from
  :kconfig:option:`CONFIG_TFM_IMAGE_SECURITY_COUNTER`, instead of being automatically determined from
  the image version. This has been changed as the implicit counter calculation is incompatible with
  versions larger than ``0.0.1024`` (:github:`78128`).

LVGL
====

zcbor
=====

* Updated the zcbor library to version 0.9.0.
  Full release notes at https://github.com/NordicSemiconductor/zcbor/blob/0.9.0/RELEASE_NOTES.md
  Migration guide at https://github.com/NordicSemiconductor/zcbor/blob/0.9.0/MIGRATION_GUIDE.md
  Migration guide copied here:

  * ``zcbor_simple_*()`` functions have been removed to avoid confusion about their use.
    They are still in the C file because they are used by other functions.
    Instead, use the specific functions for the currently supported simple values, i.e.
    ``zcbor_bool_*()``, ``zcbor_nil_*()``, and ``zcbor_undefined_*()``.
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

* The ``power-domain`` property has been removed in favor of ``power-domains``.
  The new property allows to add more than one power domain.
  ``power-domain-names`` is also available to optionally name each entry in
  ``power-domains``. The number of cells in the ``power-domains`` property need
  to be defined using ``#power-domain-cells``.

Analog Digital Converter (ADC)
==============================

* For all STM32 ADC that selects an asynchronous clock through ``st,adc-clock-source`` property,
  it is now mandatory to also explicitly define a domain clock source using the ``clock`` property.

Clock control
=============

* LFXO/HFXO (High/Low Frequency Crystal Oscillator) present in nRF53 series can
  now be configured using devicetree. The Kconfig options
  :kconfig:option:`CONFIG_SOC_ENABLE_LFXO`,
  :kconfig:option:`CONFIG_SOC_LFXO_CAP_EXTERNAL`,
  :kconfig:option:`CONFIG_SOC_LFXO_CAP_INT_6PF`,
  :kconfig:option:`CONFIG_SOC_LFXO_CAP_INT_7PF`,
  :kconfig:option:`CONFIG_SOC_LFXO_CAP_INT_9PF`,
  :kconfig:option:`CONFIG_SOC_HFXO_CAP_DEFAULT`,
  :kconfig:option:`CONFIG_SOC_HFXO_CAP_EXTERNAL`,
  :kconfig:option:`CONFIG_SOC_HFXO_CAP_INTERNAL` and
  :kconfig:option:`CONFIG_SOC_HFXO_CAP_INT_VALUE_X2` have been deprecated.

  LFXO can now be configured like this:

  .. code-block:: devicetree

     /* use external capacitors */
     &lfxo {
           load-capacitors = "external";
     };

     /* use internal capacitors (value needs to be selected: 6, 7, 9pF)
     &lfxo {
           load-capacitors = "internal";
           load-capacitance-picofarad = <...>;
     };

  HFXO can now be configured like this:

  .. code-block:: devicetree

     /* use external capacitors */
     &hfxo {
           load-capacitors = "external";
     };

     /* use internal capacitors (value needs to be selected: 7pF...20pF in 0.5pF
      * steps, units: femtofarads)
      */
     &hfxo {
           load-capacitors = "internal";
           load-capacitance-femtofarad = <...>;
     };

Crypto
======

* Following the deprecation of the TinyCrypt library (:github:`79566`), the
  TinyCrypt-based shim driver was marked as deprecated (:github:`79653`).

Display
=======

Disk
====

* The SDMMC subsystem driver now requires a ``disk-name`` property be supplied
  with the definition of the disk, which is used when registering the
  SD device with the disk subsystem. This permits multiple SD devices to be
  registered simultaneously. If unsure, ``disk-name = "SD"`` may be used
  as a sane default.

* The MMC subsystem driver now requires a ``disk-name`` property be supplied
  with the definition of the disk, which is used when registering the
  MMC device with the disk subsystem. This permits multiple MMC devices to be
  registered simultaneously. If unsure, ``disk-name = "SD2"`` may be used
  as a sane default.


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

PWM
===

* The Raspberry Pi Pico PWM driver now configures frequency adaptively.
  This has resulted in a change in how device tree parameters are handled.
  If the :dtcompatible:`raspberry,pico-pwm`'s ``divider-int-0`` or variations
  for each channel are specified, or if these are set to 0,
  the driver dynamically configures the division ratio by specified cycles.
  The driver will operate at the specified division ratio if a non-zero value is
  specified for ``divider-int-0``.
  This is unchanged from previous behavior.
  Please specify ``divider-int-0`` explicitly to make the same behavior as before.

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
* The :dtcompatible:`current-sense-amplifier` sense resistor is now specified in milli-ohms
  (``sense-resistor-milli-ohms``) instead of micro-ohms in order to increase the maximum representable
  resistor from 4.2k to 4.2M.
* The :dtcompatible:`current-sense-amplifier` properties ``sense-gain-mult`` and ``sense-gain-div``
  are now limited to a maximum value of ``UINT16_MAX`` to enable smaller rounding errors in internal
  calculations.

* The ``nxp,`` prefixed properties in :dtcompatible:`nxp,kinetis-acmp` have been deprecated in favor
  of properties without the prefix. The sensor based driver for the :dtcompatible:`nxp,kinetis-acmp`
  has been updated to support both the new and deprecated property names. Uses of the deprecated
  property names should be updated to the new property names.

Serial
======

 * Users of :c:func:`uart_irq_tx_ready` now need to check for ``ret > 0`` to ensure that the FIFO
   can accept data bytes, instead of ``ret == 1``. The function now returns a lower bound on the
   number of bytes that can be provided to :c:func:`uart_fifo_fill` without truncation.

 * LiteX: ``CONFIG_UART_LITEUART`` has been renamed to :kconfig:option:`CONFIG_UART_LITEX`.

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

* The ``bt-hci-bus`` and ``bt-hci-quirks`` devicetree properties for HCI bindings have been changed
  to use lower-case strings without the ``BT_HCI_QUIRK_`` and ``BT_HCI_BUS_`` prefixes.
* The Kconfig option :kconfig:option:`BT_SPI` is now automatically selected based on devicetree
  compatibles and can be removed from board ``.defconfig`` files.

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
  and more specifically prior to calling :c:func:`bt_bap_unicast_server_register_cb` for the first
  time. It does not need to be called again until the new function
  :c:func:`bt_bap_unicast_server_unregister` has been called.
  (:github:`76632`)

* The Coordinated Set Coordinator functions :c:func:`bt_csip_set_coordinator_lock` and
  :c:func:`bt_csip_set_coordinator_release` now require that :kconfig:option:`CONFIG_BT_BONDABLE`
  is enabled and that all members are bonded, to comply with the requirements from the CSIP spec.
  (:github:`78877`)

* The callback structure provided to :c:func:`bt_bap_unicast_client_register_cb` is no longer
  :code:`const`, and now multiple callback structures can be registered.
  (:github:`78999`)

* The Broadcast Audio Scan Service (BASS) shall now be registered and unregistered dynamically
  at runtime within the scan delegator. Two new APIs, :c:func:`bt_bap_scan_delegator_register()`
  and :c:func:`bt_bap_scan_delegator_unregister()`, have been introduced to manage both BASS and
  scan delegator registration and initialization dynamically. It should also be mentioned that
  the previous callback registration function, :c:func:`bt_bap_scan_delegator_register_cb()` has
  now been removed and merged with :c:func:`bt_bap_scan_delegator_register()`.
  This change allows more flexibility when registering or unregistering scan delegator and BASS
  related functionality without requiring build-time configuration. Existing need to be updated
  to use these new APIs.
  (:github:`78751`)

* The Telephone Bearer Service (TBS) and Generic Telephone Bearer Service (GTBS) shall now be
  registered dynamically at runtime with :c:func:`bt_tbs_register_bearer`. The services can also be
  unregistered with :c:func:`bt_tbs_unregister_bearer`.
  (:github:`76108`)

* There has been a rename from ``bt_audio_codec_qos`` to ``bt_bap_qos_cfg``. This effects all
  structs, enums and defines that used the ``bt_audio_codec_qos`` name. To use the new naming simply
  do a search-and-replace for ``bt_audio_codec_qos`` to ``bt_bap_qos_cfg`` and
  ``BT_AUDIO_CODEC_QOS`` to ``BT_BAP_QOS_CFG``. (:github:`76633`)

* The generation of broadcast ID inside of zephyr stack has been removed, it is now up the
  application to generate a broadcast ID. This means that the application can now fully decide
  whether to use a static or random broadcast ID. Reusing and statically defining a broadcast ID was
  added to the Basic Audio Profile in version 1.0.2, which is the basis for this change. All
  instances of :c:func:`bt_cap_initiator_broadcast_get_id` and
  :c:func:`bt_bap_broadcast_source_get_id` has been removed(:github:`80228`)

* ``BT_AUDIO_BROADCAST_CODE_SIZE`` has been removed and ``BT_ISO_BROADCAST_CODE_SIZE`` should be
  used instead. (:github:`80217`)

Bluetooth Classic
=================

Bluetooth Host
==============

Automatic advertiser resumption is deprecated
---------------------------------------------

.. note::

   This deprecation is compiler-checked. If you get no warnings,
   you should not be affected.

Deprecated symbols:
   * :c:enumerator:`BT_LE_ADV_OPT_CONNECTABLE`
   * :c:enumerator:`BT_LE_ADV_OPT_ONE_TIME`
   * :c:macro:`BT_LE_ADV_CONN`

New symbols:
   * :c:enumerator:`BT_LE_ADV_OPT_CONN`
   * :c:macro:`BT_LE_ADV_CONN_FAST_1`
   * :c:macro:`BT_LE_ADV_CONN_FAST_2`

:c:enumerator:`BT_LE_ADV_OPT_CONNECTABLE` is a combined
instruction to make the advertiser connectable and to enable
automatic resumption. To disable the automatic resumption, use
:c:enumerator:`BT_LE_ADV_OPT_CONN`.

Extended Advertising API with shorthands
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Extended Advertising API ``bt_le_ext_adv_*`` implicitly assumes
:c:enumerator:`BT_LE_ADV_OPT_ONE_TIME` and never automatically
resume advertising. Therefore, the following search/replace can
be applied without thinking:

Replace all

.. code-block:: diff

   -bt_le_ext_adv_create(BT_LE_ADV_CONN, ...)
   +bt_le_ext_adv_create(BT_LE_ADV_FAST_2, ...)

.. code-block:: diff

   -bt_le_ext_adv_update_param(..., BT_LE_ADV_CONN)
   +bt_le_ext_adv_update_param(..., BT_LE_ADV_FAST_2)

Extended Advertising API with custom parameters
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

You may have uses of :c:enumerator:`BT_LE_ADV_OPT_CONNECTABLE`
in assignments to a :c:struct:`bt_le_adv_param`. If your struct
is never passed to :c:func:`bt_le_adv_start`, you should:

* replace :c:enumerator:`BT_LE_ADV_OPT_CONNECTABLE` with
  :c:enumerator:`BT_LE_ADV_OPT_CONN`.
* remove :c:enumerator:`BT_LE_ADV_OPT_ONE_TIME`.

Legacy Advertising API not using automatic resumption
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Any calls to :c:func:`bt_le_adv_start` that use the combination
:c:enumerator:`BT_LE_ADV_OPT_CONNECTABLE` and
:c:enumerator:`BT_LE_ADV_OPT_ONE_TIME` should have that
combination replaced with :c:enumerator:`BT_LE_ADV_OPT_CONN`.

Legacy Advertising API using automatic resumption
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

For this case, the application has to take over the
responsibility of restarting the advertiser.

Refer to the extended advertising sample for an example
implementation of advertiser restarting. The same technique can
be used for legacy advertising.

Bluetooth Crypto
================

Networking
**********

* The CoAP public API functions :c:func:`coap_get_block1_option` and
  :c:func:`coap_get_block2_option` have changed. The ``block_number`` pointer
  type has changed from ``uint8_t *`` to ``uint32_t *``. Additionally,
  :c:func:`coap_get_block2_option` now accepts an additional ``bool *has_more``
  parameter, to store the value of the more flag. (:github:`76052`)

* The struct :c:struct:`coap_transmission_parameters` has a new field ``ack_random_percent`` if
  :kconfig:option:`CONFIG_COAP_RANDOMIZE_ACK_TIMEOUT` is enabled. (:github:`79058`)

* The Ethernet bridge shell is moved under network shell. This is done so that
  all the network shell activities can be found under ``net`` shell command.
  After this change the bridge shell is used by ``net bridge`` command. (:github:`77235`)

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
  will take network interface pointer as a first parameter. (:github:`77987`)

* To facilitate use outside of the networking subsystem, the network buffer header file was renamed
  from :zephyr_file:`include/zephyr/net/buf.h` to :zephyr_file:`include/zephyr/net_buf.h` and the
  implementation moved to :zephyr_file:`lib/net_buf/`. (:github:`78009`)

* The ``work_q`` parameter to ``NET_SOCKET_SERVICE_SYNC_DEFINE`` and
  ``NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC`` has been removed as it was always ignored. (:github:`79446`)

* The callback function for the socket service has changed. The
  ``struct k_work *work`` parameter has been replaced with a pointer to the
  ``struct net_socket_service_event *pev`` parameter. (:github:`80041`)

* Deprecated the :kconfig:option:`CONFIG_NET_SOCKETS_POLL_MAX` option in favour of
  :kconfig:option:`CONFIG_ZVFS_POLL_MAX`.

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

* :c:func:`hawkbit_autohandler` now takes one argument. This argument has to be set to
  ``true`` for the same behavior as before the change. (:github:`71037`)

* ``<zephyr/mgmt/hawkbit.h>`` is deprecated in favor of ``<zephyr/mgmt/hawkbit/hawkbit.h>``.
  The old header will be removed in future releases and its usage should be avoided.
  The hawkbit autohandler has been separated into ``<zephyr/mgmt/hawkbit/autohandler.h>``.
  The configuration part of hawkbit is now in ``<zephyr/mgmt/hawkbit/config.h>``. (:github:`71037`)

MCUmgr
======

* The ``MCUMGR_TRANSPORT_BT_AUTHEN`` Kconfig option from the :kconfig:option:`CONFIG_MCUMGR_TRANSPORT_BT` MCUmgr transport has been replaced with the :kconfig:option:`CONFIG_MCUMGR_TRANSPORT_BT_PERM_RW` Kconfig choice.
  The requirement for Bluetooth authentication is now indicated by the :kconfig:option:`CONFIG_MCUMGR_TRANSPORT_BT_PERM_RW_AUTHEN` Kconfig option.
  To remove the default requirement for Bluetooth authentication it is necessary to enable the :kconfig:option:`CONFIG_MCUMGR_TRANSPORT_BT_PERM_RW` Kconfig option in the project configuration.

Modem
=====

Random
======

* Following the deprecation of the TinyCrypt library (:github:`79566`), usage
  of TinyCrypt in the CTR-DRBG random number generator was removed. From now on
  Mbed TLS is required to enable :kconfig:option:`CONFIG_CTR_DRBG_CSPRNG_GENERATOR`.
  (:github:`79653`)

Shell
=====

* ``kernel threads`` and ``kernel stacks`` shell command have been renamed to
  ``kernel thread list`` & ``kernel thread stacks``

JWT (JSON Web Token)
====================

* By default, the signature is now computed using the PSA Crypto API for both RSA and ECDSA
  (:github:`78243`). The conversion to the PSA Crypto API is part of the adoption
  of a standard interface for crypto operations (:github:`43712`). Moreover,
  following the deprecation of the TinyCrypt library (:github:`79566`), usage
  of TinyCrypt was removed from the JWT subsystem (:github:`79653`).

* The following new symbols were added to allow specifying both the signature
  algorithm and crypto library:

  * :kconfig:option:`CONFIG_JWT_SIGN_RSA_PSA` (default) RSA signature using the PSA Crypto API;
  * :kconfig:option:`CONFIG_JWT_SIGN_RSA_LEGACY` RSA signature using Mbed TLS;
  * :kconfig:option:`CONFIG_JWT_SIGN_ECDSA_PSA` ECDSA signature using the PSA Crypto API.

  They replace the previously-existing Kconfigs ``CONFIG_JWT_SIGN_RSA`` and
  ``CONFIG_JWT_SIGN_ECDSA``. (:github:`79653`)

Architectures
*************
