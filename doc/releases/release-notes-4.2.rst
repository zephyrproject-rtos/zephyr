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

.. _zephyr_4.2:

Zephyr 4.2.0 (Working Draft)
############################

We are pleased to announce the release of Zephyr version 4.2.0.

Major enhancements with this release include:

An overview of the changes required or recommended when migrating your application from Zephyr
v4.1.0 to Zephyr v4.2.0 can be found in the separate :ref:`migration guide<migration_4.2>`.

The following sections provide detailed lists of changes by component.

Security Vulnerability Related
******************************
The following CVEs are addressed by this release:

More detailed information can be found in:
https://docs.zephyrproject.org/latest/security/vulnerabilities.html

API Changes
***********

Removed APIs and options
========================

* Removed the deprecated the ``net_buf_put()`` and ``net_buf_get()`` API functions.

* Removed the deprecated ``include/zephyr/net/buf.h`` header file.

Deprecated APIs and options

* The scheduler Kconfig options CONFIG_SCHED_DUMB and CONFIG_WAITQ_DUMB were
  renamed and deprecated. Use :kconfig:option:`CONFIG_SCHED_SIMPLE` and
  :kconfig:option:`CONFIG_WAITQ_SIMPLE` instead.

===========================

* ``arduino_uno_r4_minima`` and ``arduino_uno_r4_wifi`` board targets have been deprecated in favor
  of a new ``arduino_uno_r4`` board with revisions (``arduino_uno_r4@minima`` and
  ``arduino_uno_r4@wifi``).

New APIs and options
====================

* I2C

  * :c:func:`i2c_configure_dt`.

..
  Link to new APIs here, in a group if you think it's necessary, no need to get
  fancy just list the link, that should contain the documentation. If you feel
  like you need to add more details, add them in the API documentation code
  instead.

* Bluetooth

  * Audio

    * :c:macro:`BT_BAP_ADV_PARAM_CONN_QUICK`
    * :c:macro:`BT_BAP_ADV_PARAM_CONN_REDUCED`
    * :c:macro:`BT_BAP_CONN_PARAM_SHORT_7_5`
    * :c:macro:`BT_BAP_CONN_PARAM_SHORT_10`
    * :c:macro:`BT_BAP_CONN_PARAM_RELAXED`
    * :c:macro:`BT_BAP_ADV_PARAM_BROADCAST_FAST`
    * :c:macro:`BT_BAP_ADV_PARAM_BROADCAST_SLOW`
    * :c:macro:`BT_BAP_PER_ADV_PARAM_BROADCAST_FAST`
    * :c:macro:`BT_BAP_PER_ADV_PARAM_BROADCAST_SLOW`
    * :c:func:`bt_csip_set_member_set_size_and_rank`
    * :c:func:`bt_csip_set_member_get_info`

  * Host

    * :c:func:`bt_le_get_local_features`
    * :c:func:`bt_le_bond_exists`
    * :c:func:`bt_br_bond_exists`
    * :c:func:`bt_conn_lookup_addr_br`
    * :c:func:`bt_conn_get_dst_br`
    * LE Connection Subrating is no longer experimental.

* Display

  * :c:func:`display_clear`

* Networking:

  * IPv4

    * :kconfig:option:`CONFIG_NET_IPV4_MTU`

* Stepper

  * :c:func:`stepper_stop()`

New Boards
**********

..
  You may update this list as you contribute a new board during the release cycle, in order to make
  it visible to people who might be looking at the working draft of the release notes. However, note
  that this list will be recomputed at the time of the release, so you don't *have* to update it.
  In any case, just link the board, further details go in the board description.

New Drivers
***********

..
  Same as above for boards, this will also be recomputed at the time of the release.
  Just link the driver, further details go in the binding description

New Samples
***********

..
  Same as above for boards and drivers, this will also be recomputed at the time of the release.
 Just link the sample, further details go in the sample documentation itself.

* :zephyr:code-sample:`stepper`

Other notable changes
*********************

..
  Any more descriptive subsystem or driver changes. Do you really want to write
  a paragraph or is it enough to link to the api/driver/Kconfig/board page above?

* Added support for Armv8.1-M MPU's PXN (Privileged Execute Never) attribute.
  With this, the MPU attributes for ``__ramfunc`` and ``__ram_text_reloc`` were modified such that,
  PXN attribute is set for these regions if compiled with ``CONFIG_ARM_MPU_PXN`` and ``CONFIG_USERSPACE``.
  This results in a change in behaviour for code being executed from these regions because,
  if these regions have pxn attribute set in them, they cannot be executed in privileged mode.
