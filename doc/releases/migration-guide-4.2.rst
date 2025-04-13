:orphan:

..
  See
  https://docs.zephyrproject.org/latest/releases/index.html#migration-guides
  for details of what is supposed to go into this document.

.. _migration_4.2:

Migration guide to Zephyr v4.2.0 (Working Draft)
################################################

This document describes the changes required when migrating your application from Zephyr v4.1.0 to
Zephyr v4.2.0.

Any other changes (not directly related to migrating applications) can be found in
the :ref:`release notes<zephyr_4.2>`.

.. contents::
    :local:
    :depth: 2

Build System
************

Kernel
******

Boards
******

* All boards based on Nordic ICs that used the ``nrfjprog`` Nordic command-line
  tool for flashing by default have been modified to instead default to the new
  nRF Util (``nrfutil``) tool. This means that you may need to `install nRF Util
  <https://www.nordicsemi.com/Products/Development-tools/nrf-util>`_ or, if you
  prefer to continue using ``nrfjprog``, you can do so by invoking west while
  specfying the runner: ``west flash -r nrfjprog``. The full documentation for
  nRF Util can be found
  `here <https://docs.nordicsemi.com/bundle/nrfutil/page/README.html>`_.

* All boards based on a Nordic IC of the nRF54L series now default to not
  erasing any part of the internal storage when flashing. If you'd like to
  revert to the previous default of erasing the pages that will be written to by
  the firmware to be flashed you can use the new ``--erase-pages`` command-line
  switch when invoking ``west flash``.
  Note that RRAM on nRF54L devices is not physically paged, and paging is
  only artificially provided, with a page size of 4096 bytes, for an easier
  transition of nRF52 software to nRF54L devices.

* The config option :kconfig:option:`CONFIG_NATIVE_POSIX_SLOWDOWN_TO_REAL_TIME` has been deprecated
  in favor of :kconfig:option:`CONFIG_NATIVE_SIM_SLOWDOWN_TO_REAL_TIME`.

* The DT binding :dtcompatible:`zephyr,native-posix-cpu` has been deprecated in favor of
  :dtcompatible:`zephyr,native-sim-cpu`.

* Zephyr now supports version 1.11.2 of the :zephyr:board:`neorv32`. NEORV32 processor (SoC)
  implementations need to be updated to this version to be compatible with Zephyr v4.2.0.

* The :zephyr:board:`neorv32` now targets NEORV32 processor (SoC) templates via board variants. The
  old ``neorv32`` board target is now named ``neorv32/neorv32/up5kdemo``.

* ``arduino_uno_r4_minima``, ``arduino_uno_r4_wifi``, and ``mikroe_clicker_ra4m1`` have migrated to
  new FSP-based configurations.
  While there are no major functional changes, the device tree structure has been significantly revised.
  The following device tree bindings are now removed:
  ``renesas,ra-gpio``, ``renesas,ra-uart-sci``, ``renesas,ra-pinctrl``,
  ``renesas,ra-clock-generation-circuit``, and ``renesas,ra-interrupt-controller-unit``.
  Instead, use the following replacements:
  - :dtcompatible:`renesas,ra-gpio-ioport`
  - :dtcompatible:`renesas,ra-sci-uart`
  - :dtcompatible:`renesas,ra-pinctrl-pfs`
  - :dtcompatible:`renesas,ra-cgc-pclk-block`

* Nucleo WBA52CG board (``nucleo_wba52cg``) is not supported anymore since it is NRND
  (Not Recommended for New Design) and it is not supported anymore in the STM32CubeWBA from
  version 1.1.0 (July 2023). The migration to :zephyr:board:`nucleo_wba55cg` (``nucleo_wba55cg``)
  is recommended and it could be done without any change.

Device Drivers and Devicetree
*****************************

DAI
===

* Renamed the devicetree property ``dai_id`` to ``dai-id``.
* Renamed the devicetree property ``afe_name`` to ``afe-name``.
* Renamed the devicetree property ``agent_disable`` to ``agent-disable``.
* Renamed the devicetree property ``ch_num`` to ``ch-num``.
* Renamed the devicetree property ``mono_invert`` to ``mono-invert``.
* Renamed the devicetree property ``quad_ch`` to ``quad-ch``.
* Renamed the devicetree property ``int_odd`` to ``int-odd``.

DMA
===

* Renamed the devicetree property ``nxp,a_on`` to ``nxp,a-on``.
* Renamed the devicetree property ``dma_channels`` to ``dma-channels``.

Counter
=======

* ``counter_native_posix`` has been renamed ``counter_native_sim``, and with it its
  kconfig options and DT binding. :dtcompatible:`zephyr,native-posix-counter`  has been deprecated
  in favor of :dtcompatible:`zephyr,native-sim-counter`.
  And :kconfig:option:`CONFIG_COUNTER_NATIVE_POSIX` and its related options with
  :kconfig:option:`CONFIG_COUNTER_NATIVE_SIM` (:github:`86616`).

Entropy
=======

* ``fake_entropy_native_posix`` has been renamed ``fake_entropy_native_sim``, and with it its
  kconfig options and DT binding. :dtcompatible:`zephyr,native-posix-rng`  has been deprecated
  in favor of :dtcompatible:`zephyr,native-sim-rng`.
  And :kconfig:option:`CONFIG_FAKE_ENTROPY_NATIVE_POSIX` and its related options with
  :kconfig:option:`CONFIG_FAKE_ENTROPY_NATIVE_SIM` (:github:`86615`).

Eeprom
========

* :dtcompatible:`ti,tmp116-eeprom` has been renamed to :dtcompatible:`ti,tmp11x-eeprom` because it
  supports both tmp117 and tmp119.

Ethernet
========

* Removed Kconfig option ``ETH_STM32_HAL_MII`` (:github:`86074`).
  PHY interface type is now selected via the ``phy-connection-type`` property in the device tree.

* The :dtcompatible:`st,stm32-ethernet` driver now requires the ``phy-handle`` phandle to be
  set to the according PHY node in the device tree (:github:`87593`).

* The Kconfig options ``ETH_STM32_HAL_PHY_ADDRESS``, ``ETH_STM32_CARRIER_CHECK``,
  ``ETH_STM32_CARRIER_CHECK_RX_IDLE_TIMEOUT_MS``, ``ETH_STM32_AUTO_NEGOTIATION_ENABLE``,
  ``ETH_STM32_SPEED_10M``, ``ETH_STM32_MODE_HALFDUPLEX`` have been removed, as they are no longer
  needed, and the driver now uses the ethernet phy api to communicate with the phy driver, which
  is resposible for configuring the phy settings (:github:`87593`).

* ``ethernet_native_posix`` has been renamed ``ethernet_native_tap``, and with it its
  kconfig options: :kconfig:option:`CONFIG_ETH_NATIVE_POSIX` and its related options have been
  deprecated in favor of :kconfig:option:`CONFIG_ETH_NATIVE_TAP` (:github:`86578`).

* NuMaker Ethernet driver ``eth_numaker.c`` now supports ``gen_random_mac``,
  and the EMAC data flash feature has been removed (:github:`87953`).

Enhanced Serial Peripheral Interface (eSPI)
===========================================

* Renamed the devicetree property ``io_girq`` to ``io-girq``.
* Renamed the devicetree property ``vw_girqs`` to ``vw-girqs``.
* Renamed the devicetree property ``pc_girq`` to ``pc-girq``.
* Renamed the devicetree property ``poll_timeout`` to ``poll-timeout``.
* Renamed the devicetree property ``poll_interval`` to ``poll-interval``.
* Renamed the devicetree property ``consec_rd_timeout`` to ``consec-rd-timeout``.
* Renamed the devicetree property ``sus_chk_delay`` to ``sus-chk-delay``.
* Renamed the devicetree property ``sus_rsm_interval`` to ``sus-rsm-interval``.

GPIO
====

* To support the RP2350B, which has many pins, the Raspberry Pi-GPIO configuration has
  been changed. The previous role of :dtcompatible:`raspberrypi,rpi-gpio` has been migrated to
  :dtcompatible:`raspberrypi,rpi-gpio-port`, and :dtcompatible:`raspberrypi,rpi-gpio` is
  now left as a placeholder and mapper.
  The labels have also been changed along, so no changes are necessary for regular use.

Sensors
=======

* ``ltr`` vendor prefix has been renamed to ``liteon``, and with it the
  :dtcompatible:`ltr,f216a` name has been replaced by :dtcompatible:`liteon,ltrf216a`.
  The choice :kconfig:option:`DT_HAS_LTR_F216A_ENABLED` has been replaced with
  :kconfig:option:`DT_HAS_LITEON_LTRF216A_ENABLED` (:github:`85453`)

* :dtcompatible:`ti,tmp116` has been renamed to :dtcompatible:`ti,tmp11x` because it supports
  tmp116, tmp117 and tmp119.

Serial
=======

* ``uart_native_posix`` has been renamed ``uart_native_pty``, and with it its
  kconfig options and DT binding. :dtcompatible:`zephyr,native-posix-uart`  has been deprecated
  in favor of :dtcompatible:`zephyr,native-pty-uart`.
  :kconfig:option:`CONFIG_UART_NATIVE_POSIX` and its related options with
  :kconfig:option:`CONFIG_UART_NATIVE_PTY`.
  The choice :kconfig:option:`CONFIG_NATIVE_UART_0` has been replaced with
  :kconfig:option:`CONFIG_UART_NATIVE_PTY_0`, but now, it is also possible to select if a UART is
  connected to the process stdin/out instead of a PTY at runtime with the command line option
  ``--<uart_name>_stdinout``.
  :kconfig:option:`CONFIG_NATIVE_UART_AUTOATTACH_DEFAULT_CMD` has been replaced with
  :kconfig:option:`CONFIG_UART_NATIVE_PTY_AUTOATTACH_DEFAULT_CMD`.
  :kconfig:option:`CONFIG_UART_NATIVE_WAIT_PTS_READY_ENABLE` has been deprecated. The functionality
  it enabled is now always enabled as there is no drawbacks from it.
  :kconfig:option:`CONFIG_UART_NATIVE_POSIX_PORT_1_ENABLE` has been deprecated. This option does
  nothing now. Instead users should instantiate as many :dtcompatible:`zephyr,native-pty-uart` nodes
  as native PTY UART instances they want. (:github:`86739`)

Timer
=====

* ``native_posix_timer`` has been renamed ``native_sim_timer``, and so its kconfig option
  :kconfig:option:`CONFIG_NATIVE_POSIX_TIMER` has been deprecated in favor of
  :kconfig:option:`CONFIG_NATIVE_SIM_TIMER`, (:github:`86612`).

Modem
=====

* Removed Kconfig option :kconfig:option:`CONFIG_MODEM_CELLULAR_CMUX_MAX_FRAME_SIZE` in favor of
  :kconfig:option:`CONFIG_MODEM_CMUX_WORK_BUFFER_SIZE` and :kconfig:option:`CONFIG_MODEM_CMUX_MTU`.


Stepper
=======

* Refactored the ``stepper_enable(const struct device * dev, bool enable)`` function to
  :c:func:`stepper_enable` & :c:func:`stepper_disable`.

Bluetooth
*********

Bluetooth Audio
===============

* ``CONFIG_BT_CSIP_SET_MEMBER_NOTIFIABLE`` has been renamed to
  :kconfig:option:`CONFIG_BT_CSIP_SET_MEMBER_SIRK_NOTIFIABLE``. (:github:`86763``)

Bluetooth Host
==============

* The symbols ``BT_LE_CS_TONE_ANTENNA_CONFIGURATION_INDEX_<NUMBER>`` in
  :zephyr_file:`include/zephyr/bluetooth/conn.h` have been renamed
  to ``BT_LE_CS_TONE_ANTENNA_CONFIGURATION_A<NUMBER>_B<NUMBER>``.

* The ISO data paths are not longer setup automatically, and shall explicitly be setup and removed
  by the application by calling :c:func:`bt_iso_setup_data_path` and
  :c:func:`bt_iso_remove_data_path` respectively. (:github:`75549`)

* ``BT_ISO_CHAN_TYPE_CONNECTED`` has been split into ``BT_ISO_CHAN_TYPE_CENTRAL`` and
  ``BT_ISO_CHAN_TYPE_PERIPHERAL`` to better describe the type of the ISO channel, as behavior for
  each role may be different. Any existing uses/checks for ``BT_ISO_CHAN_TYPE_CONNECTED``
  can be replaced with an ``||`` of the two. (:github:`75549`)

Bluetooth Classic
=================

* The parameters of HFP AG callback ``sco_disconnected`` of the struct :c:struct:`bt_hfp_ag_cb`
  have been changed to SCO connection object ``struct bt_conn *sco_conn`` and the disconnection
  reason of the SCO connection ``uint8_t reason``.

Networking
**********

* The struct ``net_linkaddr_storage`` has been renamed to struct
  :c:struct:`net_linkaddr` and the old struct ``net_linkaddr`` has been removed.
  The struct :c:struct:`net_linkaddr` now contains space to store the link
  address instead of having pointer that point to the link address. This avoids
  possible dangling pointers when cloning struct :c:struct:`net_pkt`. This will
  increase the size of struct :c:struct:`net_pkt` by 4 octets for IEEE 802.15.4,
  but there is no size increase for other network technologies like Ethernet.
  Note that any code that is using struct :c:struct:`net_linkaddr` directly, and
  which has checks like ``if (lladdr->addr == NULL)``, will no longer work as expected
  (because the addr is not a pointer) and must be changed to ``if (lladdr->len == 0)``
  if the code wants to check that the link address is not set.

* TLS credential type ``TLS_CREDENTIAL_SERVER_CERTIFICATE`` was renamed to
  more generic :c:enumerator:`TLS_CREDENTIAL_PUBLIC_CERTIFICATE` to better
  reflect the purpose of this credential type.

* The MQTT public API function :c:func:`mqtt_disconnect` has changed. The function
  now accepts additional ``param`` parameter to support MQTT 5.0 case. The parameter
  is optional and not used with older MQTT versions - MQTT 3.1.1 users should pass
  NULL as an argument.

* The ``AF_PACKET/SOCK_RAW/IPPROTO_RAW`` socket combination is no longer supported,
  as ``AF_PACKET`` sockets should only accept IEEE 802.3 protocol numbers. As an
  alternative, ``AF_PACKET/SOCK_DGRAM/ETH_P_ALL`` or ``AF_INET(6)/SOCK_RAW/IPPROTO_IP``
  sockets can be used, depending on the actual use case.

* The HTTP server now respects the configured ``_concurrent`` and  ``_backlog`` values. Check that
  you provide applicable values to :c:macro:`HTTP_SERVICE_DEFINE_EMPTY`,
  :c:macro:`HTTPS_SERVICE_DEFINE_EMPTY`, :c:macro:`HTTP_SERVICE_DEFINE` and
  :c:macro:`HTTPS_SERVICE_DEFINE`.

SPI
===

* Renamed the device tree property ``port_sel`` to ``port-sel``.
* Renamed the device tree property ``chip_select`` to ``chip-select``.

Other subsystems
****************

Modules
*******

Architectures
*************
