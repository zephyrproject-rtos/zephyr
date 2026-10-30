:orphan:

..
  What goes here: new capabilities, major improvements and changes that
  materially affect users, with what changed and why it matters. See
  https://docs.zephyrproject.org/latest/releases/index.html#release-documentation
..
  Not here: lists of new, deprecated or removed APIs, Kconfig options,
  bindings, boards, drivers or samples. They are generated into the change
  inventory at release time. Not here either: bug fixes and minor changes,
  they are in the git log; and anything that requires the user to change their
  application, that goes in the migration guide.
..
  If you are describing a feature or functionality, consider adding it to the
  actual project documentation rather than the release notes, so that the
  information does not get lost in time, and link to it from here.

.. _zephyr_4.6:

Zephyr 4.6.0 (Working Draft)
############################

We are pleased to announce the release of Zephyr version 4.6.0.

Major enhancements with this release include:

An overview of the changes required or recommended when migrating your application from Zephyr
v4.5.0 to Zephyr v4.6.0 can be found in the separate :ref:`migration guide<migration_4.6>`.

The following sections describe the notable changes by component.

Security Vulnerability Related
******************************

..
  API changes with a user impact: describe the capability here and the
  required adaptation in the migration guide. The lists of new, deprecated and
  removed APIs and options, boards, shields, drivers and samples are generated
  into the change inventory at release time.

Libraries / Subsystems
**********************

* Bluetooth

  * Added a HID Service server (:kconfig:option:`CONFIG_BT_HIDS`), the service
    the HID Device role of the HID over GATT Profile is built on.
  * :c:func:`bt_addr_matches_irk` is now public, so a scanner can check whether a
    resolvable private address was generated from an IRK it holds.

Other notable changes
*********************

* Bluetooth

  * :kconfig:option:`CONFIG_BT_PRIVACY` can now be enabled in Broadcaster- and Observer-only builds,
    without :kconfig:option:`CONFIG_BT_SMP`.

..
  Any more descriptive subsystem or driver changes. Do you really want to write
  a paragraph or is it enough to link to the documentation of the change?
