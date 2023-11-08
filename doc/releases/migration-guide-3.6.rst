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

* ``canopennode``

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

Power Management
================

Bootloader
==========

* MCUboot's deprecated ``CONFIG_ZEPHYR_TRY_MASS_ERASE`` Kconfig option has been removed. If an
  erase is needed when flashing MCUboot, this should now be provided directly to the ``west``
  command e.g. ``west flash --erase``.

Bluetooth
=========

* The HCI implementation for both the Host and the Controller sides has been
  renamed for the IPC transport. The ``CONFIG_BT_RPMSG`` Kconfig option is now
  :kconfig:option:`CONFIG_BT_HCI_IPC`, and the ``zephyr,bt-hci-rpmsg-ipc``
  Devicetree chosen is now ``zephyr,bt-hci-ipc``. The existing sample has also
  been renamed, from ``samples/bluetooth/hci_rpmsg`` to
  ``samples/bluetooth/hci_ipc``.

Networking
==========

* The CoAP public API has some minor changes to take into account. The
  :c:func:`coap_remove_observer` now returns a result if the observer was removed. This
  change is used by the newly introduced :ref:`coap_server_interface` subsystem. Also, the
  ``request`` argument for :c:func:`coap_well_known_core_get` is made ``const``.

Other Subsystems
================

* MCUmgr applications that make use of serial transports (shell or UART) must now select
  :kconfig:option:`CONFIG_CRC`, this was previously erroneously selected if MCUmgr was enabled,
  when for non-serial transports it was not needed.

* Touchscreen drivers :dtcompatible:`focaltech,ft5336` and
  :dtcompatible:`goodix,gt911` were using the incorrect polarity for the
  respective ``reset-gpios``. This has been fixed so those signals now have to
  be flagged as :c:macro:`GPIO_ACTIVE_LOW` in the devicetree.`

Recommended Changes
*******************
