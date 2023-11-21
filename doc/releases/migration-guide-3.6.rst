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

Kernel
======

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

  (:github:`62994`)

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

Bootloader
==========

* MCUboot's deprecated ``CONFIG_ZEPHYR_TRY_MASS_ERASE`` Kconfig option has been removed. If an
  erase is needed when flashing MCUboot, this should now be provided directly to the ``west``
  command e.g. ``west flash --erase``. (:github:`64703`)

Bluetooth
=========

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
* The Bluetooth Mesh ``model`` declaration has been changed to add prefix ``const``.
  The ``model->user_data``, ``model->elem_idx`` and ``model->mod_idx`` field has been changed to
  the new runtime structure, replaced by ``model->rt->user_data``, ``model->rt->elem_idx`` and
  ``model->rt->mod_idx`` separately. (:github:`65152`)
* The Bluetooth Mesh ``element`` declaration has been changed to add prefix ``const``.
  The ``elem->addr`` field has been changed to the new runtime structure, replaced by
  ``elem->rt->addr``. (:github:`65388`)

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

* The IGMP multicast library now supports IGMPv3. This results in a minor change to the existing
  api. The :c:func:`net_ipv4_igmp_join` now takes an additional argument of the type
  ``const struct igmp_param *param``. This allows IGMPv3 to exclude/include certain groups of
  addresses. If this functionality is not used or available (when using IGMPv2), you can safely pass
  a NULL pointer. IGMPv3 can be enabled using the Kconfig ``CONFIG_NET_IPV4_IGMPV3``.
  (:github:`65293`)

Other Subsystems
================

* MCUmgr applications that make use of serial transports (shell or UART) must now select
  :kconfig:option:`CONFIG_CRC`, this was previously erroneously selected if MCUmgr was enabled,
  when for non-serial transports it was not needed. (:github:`64078`)

* Touchscreen drivers :dtcompatible:`focaltech,ft5336` and
  :dtcompatible:`goodix,gt911` were using the incorrect polarity for the
  respective ``reset-gpios``. This has been fixed so those signals now have to
  be flagged as :c:macro:`GPIO_ACTIVE_LOW` in the devicetree. (:github:`64800`)

Recommended Changes
*******************

* New macros available for ST sensor DT properties setting. These macros have a self-explanatory
  name that helps in recognizing what the property setting means (e.g. LSM6DSV16X_DT_ODR_AT_60Hz).
  (:github:`65410`)
