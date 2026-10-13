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

* Management

  * MCUmgr

    * :dtcompatible:`zephyr,smp-uart` and :dtcompatible:`zephyr,smp-uart-raw` select the UARTs of
      the MCUmgr UART transports, see :ref:`mcumgr_smp_transport_uart_config`.

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

* Management

  * MCUmgr

    * The UART transports can run on several UARTs at once, each selected with a
      :dtcompatible:`zephyr,smp-uart` or :dtcompatible:`zephyr,smp-uart-raw` node, and
      :kconfig:option:`CONFIG_MCUMGR_TRANSPORT_UART` and
      :kconfig:option:`CONFIG_MCUMGR_TRANSPORT_RAW_UART` can be enabled together. Without a node of
      an enabled transport, they use the ``zephyr,uart-mcumgr`` chosen UART as before.

    * With :kconfig:option:`CONFIG_MCUMGR_TRANSPORT_UART_ASYNC`, UARTs without the asynchronous UART
      API, such as USB CDC ACM, use the interrupt-driven API instead of not receiving.

Other notable changes
*********************

..
  Any more descriptive subsystem or driver changes. Do you really want to write
  a paragraph or is it enough to link to the api/driver/Kconfig/board page above?
