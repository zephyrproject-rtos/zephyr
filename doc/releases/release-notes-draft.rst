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

.. _zephyr_4.6:

Zephyr 4.6.0 (Working Draft)
############################

We are pleased to announce the release of Zephyr version 4.6.0.

Major enhancements with this release include:

An overview of the changes required or recommended when migrating your application from Zephyr
v4.5.0 to Zephyr v4.6.0 can be found in the separate :ref:`migration guide<migration_4.6>`.

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

New APIs and options
====================
..
  Link to new APIs here, in a group if you think it's necessary, no need to get
  fancy just list the link, that should contain the documentation. If you feel
  like you need to add more details, add them in the API documentation code
  instead.

.. zephyr-keep-sorted-start re(^\* \w) ignorecase

* Bluetooth

  * Services

    * :kconfig:option:`CONFIG_BT_HIDS`
    * :c:func:`bt_hids_register`
    * :c:func:`bt_hids_unregister`
    * :c:func:`bt_hids_report_set`
    * :c:func:`bt_hids_report_get`
    * :c:func:`bt_hids_send_report`

* Networking

  * Ethernet bridge

    * :c:group:`eth_bridge_stp`
    * :kconfig:option:`CONFIG_NET_ETHERNET_BRIDGE_STP`

.. zephyr-keep-sorted-stop

.. _boards_added_in_zephyr_4_6:

New Boards
**********

..
  You may update this list as you contribute a new board during the release cycle, in order to make
  it visible to people who might be looking at the working draft of the release notes. However, note
  that this list will be recomputed at the time of the release, so you don't *have* to update it.
  In any case, just link the board, further details go in the board description.

.. _shields_added_in_zephyr_4_6:

New Shields
***********

..
  Same as above, this will also be recomputed at the time of the release.

New Drivers
***********

..
  Same as above, this will also be recomputed at the time of the release.
  Just link the driver, further details go in the binding description

New Samples
***********

..
  Same as above, this will also be recomputed at the time of the release.
  Just link the sample, further details go in the sample documentation itself.

Libraries / Subsystems
**********************

* Bluetooth

  * Added a HID Service server (:kconfig:option:`CONFIG_BT_HIDS`), the service
    the HID Device role of the HID over GATT Profile is built on.

* Networking

  * Ethernet bridge

    * The bridge learns the port of each source MAC address into its FDB
      (:kconfig:option:`CONFIG_NET_ETHERNET_BRIDGE_FDB_LEARNING`), so that a frame to a known
      destination is sent out of one port instead of being flooded.
      Learned entries age out after
      :kconfig:option:`CONFIG_NET_ETHERNET_BRIDGE_FDB_AGEING_TIME` seconds without a frame from
      their address, and :c:struct:`eth_bridge_fdb_entry` records when that frame was last seen
      (FDB API version 0.2.0).
    * Experimental support for the Rapid Spanning Tree Protocol of IEEE Std 802.1Q-2014
      (:kconfig:option:`CONFIG_NET_ETHERNET_BRIDGE_STP`). Once started on a bridge with
      :c:func:`eth_bridge_stp_enable`, it blocks the bridge ports that would make a loop and
      falls back to STP BPDUs on a port where an STP bridge is attached. The
      ``NET_EVENT_ETHERNET_BRIDGE_PORT_STATE``, ``NET_EVENT_ETHERNET_BRIDGE_NEW_ROOT`` and
      ``NET_EVENT_ETHERNET_BRIDGE_TOPOLOGY_CHANGE`` net_mgmt events report port role and state
      changes, a new root bridge and the start of a topology change. The protocol does not
      run on ports that forward frames in hardware, DSA switch ports and Ethernet devices that
      report the new :c:enumerator:`ETHERNET_HW_PORT_FORWARDING` capability, as it cannot
      block them.

Other notable changes
*********************

* Bluetooth

  * :kconfig:option:`CONFIG_BT_PRIVACY` can now be enabled in Broadcaster- and Observer-only builds,
    without :kconfig:option:`CONFIG_BT_SMP`.

..
  Any more descriptive subsystem or driver changes. Do you really want to write
  a paragraph or is it enough to link to the api/driver/Kconfig/board page above?
