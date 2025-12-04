:orphan:

..
  What goes here: removed/deprecated apis, new boards, new drivers, notable
  features. If you feel like something new can be useful to a user, put it
  under "Other Enhancements" in the first paragraph, if you feel like something
  is worth mentioning in the project media (release blog post, release
  livestream) put it under "Major enhancement".
..
  If you are describing a feature or functionality, consider adding it to the
  actual project documentation rather than the release notes, so that the
  information does not get lost in time.
..
  No list of bugfixes, minor changes, those are already in the git log, this is
  not a changelog.
..
  Does the entry have a link that contains the details? Just add the link, if
  you think it needs more details, put them in the content that shows up on the
  link.
..
  Are you thinking about generating this? Don't put anything at all.
..
  Does the thing require the user to change their application? Put it on the
  migration guide instead. (TODO: move the removed APIs section in the
  migration guide)

.. _zephyr_4.4:

Zephyr 4.4.0 (Working Draft)
############################

We are pleased to announce the release of Zephyr version 4.4.0.

Major enhancements with this release include:

An overview of the changes required or recommended when migrating your application from Zephyr
v4.3.0 to Zephyr v4.4.0 can be found in the separate :ref:`migration guide<migration_4.4>`.

The following sections provide detailed lists of changes by component.

Security Vulnerability Related
******************************

API Changes
***********

..
  Only removed, deprecated and new APIs. Changes go in migration guide.

Removed APIs and options
========================

Deprecated APIs and options
===========================

* Bluetooth

  * Mesh

    * The function :c:func:`bt_mesh_input_number` was deprecated. Applications should use
      :c:func:`bt_mesh_input_numeric` instead.
    * The callback :c:member:`output_number` in :c:struct:`bt_mesh_prov` structure was deprecated.
      Applications should use :c:member:`output_numeric` callback instead.

  * Host

    * :c:member:`bt_conn_le_info.interval` has been deprecated. Use
      :c:member:`bt_conn_le_info.interval_us` instead. Note that the units have changed:
      ``interval`` was in units of 1.25 milliseconds, while ``interval_us`` is in microseconds.

New APIs and options
====================
..
  Link to new APIs here, in a group if you think it's necessary, no need to get
  fancy just list the link, that should contain the documentation. If you feel
  like you need to add more details, add them in the API documentation code
  instead.

.. zephyr-keep-sorted-start re(^\* \w)

* Bluetooth

  * Host

    * :c:func:`bt_gatt_cb_unregister` Added an API to unregister GATT callback handlers.

  * Mesh

    * :c:func:`bt_mesh_input_numeric` to provide provisioning numeric input OOB value.
    * :c:member:`output_numeric` callback in :c:struct:`bt_mesh_prov` structure to
      output numeric values during provisioning.

  * Services

    * Introduced Alert Notification Service (ANS) :kconfig:option:`CONFIG_BT_ANS`

* Ethernet

  * Driver MAC address configuration with support for NVMEM cell.

    * :c:func:`net_eth_mac_load`
    * :c:struct:`net_eth_mac_config`
    * :c:macro:`NET_ETH_MAC_DT_CONFIG_INIT` and :c:macro:`NET_ETH_MAC_DT_INST_CONFIG_INIT`

* Flash

  * :dtcompatible:`jedec,mspi-nor` now allows MSPI configuration of read, write and
    control commands separately via devicetree.

* NVMEM

  * Flash device support

    * :kconfig:option:`CONFIG_NVMEM_FLASH`
    * :kconfig:option:`CONFIG_NVMEM_FLASH_WRITE`

* Settings

   * :kconfig:option:`CONFIG_SETTINGS_SAVE_SINGLE_SUBTREE_WITHOUT_MODIFICATION`
   * :kconfig:option:`CONFIG_SETTINGS_SAVE_SINGLE_SUBTREE_WITHOUT_MODIFICATION_VALUE_SIZE`

.. zephyr-keep-sorted-stop

New Boards
**********

..
  You may update this list as you contribute a new board during the release cycle, in order to make
  it visible to people who might be looking at the working draft of the release notes. However, note
  that this list will be recomputed at the time of the release, so you don't *have* to update it.
  In any case, just link the board, further details go in the board description.

New Shields
***********

..
  Same as above, this will also be recomputed at the time of the release.

New Drivers
***********

..
  Same as above, this will also be recomputed at the time of the release.
  Just link the driver, further details go in the binding description

* Radio

   * :dtcompatible:`radio-fem-two-ctrl-pins` (renamed from ``generic-fem-two-ctrl-pins``)
   * :dtcompatible:`radio-gpio-coex` (renamed from ``gpio-radio-coex``)

New Samples
***********

* :zephyr:code-sample:`ble_peripheral_ans`

..
  Same as above, this will also be recomputed at the time of the release.
 Just link the sample, further details go in the sample documentation itself.


Libraries / Subsystems
**********************

Other notable changes
*********************

..
  Any more descriptive subsystem or driver changes. Do you really want to write
  a paragraph or is it enough to link to the api/driver/Kconfig/board page above?
