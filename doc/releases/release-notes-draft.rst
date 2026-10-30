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

  * Host

    * :c:func:`bt_addr_matches_irk`

  * Services

    * :kconfig:option:`CONFIG_BT_HIDS`
    * :c:func:`bt_hids_register`
    * :c:func:`bt_hids_unregister`
    * :c:func:`bt_hids_report_set`
    * :c:func:`bt_hids_report_get`
    * :c:func:`bt_hids_send_report`

  * Classic

    * :kconfig:option:`CONFIG_BT_VOICE_OVER_HCI`
    * :kconfig:option:`CONFIG_BT_HCI_SCO_FLOW_CONTROL`
    * :kconfig:option:`CONFIG_BT_SCO_RX_BUF_COUNT`
    * :kconfig:option:`CONFIG_BT_SCO_RX_BUF_SIZE`
    * :c:macro:`BT_BUF_SCO_SIZE`
    * :c:macro:`BT_BUF_SCO_RX_SIZE`
    * :c:macro:`BT_BUF_SCO_RX_COUNT`
    * :c:macro:`BT_SCO_CHAN_SEND_RESERVE`
    * :c:macro:`BT_SCO_SDU_SIZE`
    * :c:struct:`bt_sco_stream_ops`
    * :c:struct:`bt_sco_stream`
    * :c:func:`bt_sco_stream_cb_register`
    * :c:func:`bt_sco_stream_cb_unregister`
    * :c:func:`bt_sco_stream_connect`
    * :c:func:`bt_sco_stream_disconnect`
    * :c:func:`bt_sco_stream_send`
    * :c:member:`bt_conn_sco_info.mtu`
    * :c:member:`bt_conn_sco_info.interval`
    * :c:enumerator:`BT_BUF_SCO_OUT`
    * :c:enumerator:`BT_BUF_SCO_IN`
    * :c:macro:`BT_HCI_SCO_CORRECTLY_RECEIVED`
    * :c:macro:`BT_HCI_SCO_POSSIBLY_INVALID`
    * :c:macro:`BT_HCI_SCO_NO_DATA_RECEIVED`
    * :c:macro:`BT_HCI_SCO_DATA_PARTIALLY_LOST`
    * :c:macro:`bt_sco_handle`
    * :c:macro:`bt_sco_flag`
    * :c:macro:`bt_sco_handle_pack`
    * :c:macro:`BT_HCI_READ_SYNC_FLOW_ENABLE_SUPPORTED`
    * :c:macro:`BT_HCI_SYNC_FLOW_DISABLE`
    * :c:macro:`BT_HCI_SYNC_FLOW_ENABLE`
    * :c:macro:`BT_HCI_OP_READ_SYNC_FLOW_ENABLE`
    * :c:struct:`bt_hci_rp_read_sync_flow_enable`
    * :c:macro:`BT_HCI_WRITE_SYNC_FLOW_ENABLE_SUPPORTED`
    * :c:macro:`BT_HCI_OP_WRITE_SYNC_FLOW_ENABLE`
    * :c:struct:`bt_hci_cp_write_sync_flow_enable`
    * :c:macro:`BT_HCI_CTL_TO_HOST_FLOW_ACL_ON_SCO_OFF`
    * :c:macro:`BT_HCI_CTL_TO_HOST_FLOW_ACL_OFF_SCO_ON`
    * :c:macro:`BT_HCI_CTL_TO_HOST_FLOW_ACL_ON_SCO_ON`

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

Other notable changes
*********************

* Bluetooth

  * :kconfig:option:`CONFIG_BT_PRIVACY` can now be enabled in Broadcaster- and Observer-only builds,
    without :kconfig:option:`CONFIG_BT_SMP`.

..
  Any more descriptive subsystem or driver changes. Do you really want to write
  a paragraph or is it enough to link to the api/driver/Kconfig/board page above?
