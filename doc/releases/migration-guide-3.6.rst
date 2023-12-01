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

* The network stack now uses a separate IPv4 TTL (time-to-live) value for multicast packets.
  Before, the same TTL value was used for unicast and multicast packets.
  The IPv6 hop limit value is also changed so that unicast and multicast packets can have a
  different one. (:github:`65886`)

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

Recommended Changes
*******************

* New macros available for ST sensor DT properties setting. These macros have a self-explanatory
  name that helps in recognizing what the property setting means (e.g. LSM6DSV16X_DT_ODR_AT_60Hz).
  (:github:`65410`)

* Users of :ref:`native_posix<native_posix>` are recommended to migrate to
  :ref:`native_sim<native_sim>`. :ref:`native_sim<native_sim>` supports all its use cases,
  and should be a drop-in replacement for most.
