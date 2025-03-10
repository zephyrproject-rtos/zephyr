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

Deprecated APIs and options
===========================

New APIs and options
====================

..
  Link to new APIs here, in a group if you think it's necessary, no need to get
  fancy just list the link, that should contain the documentation. If you feel
  like you need to add more details, add them in the API documentation code
  instead.

* Bluetooth

  * Host

    * :c:func:`bt_le_get_local_features`

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


Libraries / Subsystems
**********************

* Tracing

  * Add the :c:macro:`SYS_PORT_TRACK_FOREACH` macro for iterating over
    a tracking list.

  * Add the following wrapper for the :c:macro:`SYS_PORT_TRACK_NEXT` macro to
    provide a more visible way to access the global tracking list:

    * :c:macro:`SYS_PORT_TRACK_K_TIMER_NEXT` is equivalent to
      ``SYS_PORT_TRACK_NEXT(_track_list_k_timer)``.
    * :c:macro:`SYS_PORT_TRACK_K_MEM_SLAB_NEXT` is equivalent to
      ``SYS_PORT_TRACK_NEXT(_track_list_k_mem_slab)``.
    * :c:macro:`SYS_PORT_TRACK_K_SEM_NEXT` is equivalent to
      ``SYS_PORT_TRACK_NEXT(_track_list_k_sem)``.
    * :c:macro:`SYS_PORT_TRACK_K_MUTEX_NEXT` is equivalent to
      ``SYS_PORT_TRACK_NEXT(_track_list_k_mutex)``.
    * :c:macro:`SYS_PORT_TRACK_K_STACK_NEXT` is equivalent to
      ``SYS_PORT_TRACK_NEXT(_track_list_k_stack)``.
    * :c:macro:`SYS_PORT_TRACK_K_MSGQ_NEXT` is equivalent to
      ``SYS_PORT_TRACK_NEXT(_track_list_k_msgq)``.
    * :c:macro:`SYS_PORT_TRACK_K_MBOX_NEXT` is equivalent to
      ``SYS_PORT_TRACK_NEXT(_track_list_k_mbox)``.
    * :c:macro:`SYS_PORT_TRACK_K_PIPE_NEXT` is equivalent to
      ``SYS_PORT_TRACK_NEXT(_track_list_k_pipe)``.
    * :c:macro:`SYS_PORT_TRACK_K_QUEUE_NEXT` is equivalent to
      ``SYS_PORT_TRACK_NEXT(_track_list_k_queue)``.
    * :c:macro:`SYS_PORT_TRACK_K_EVENT_NEXT` is equivalent to
      ``SYS_PORT_TRACK_NEXT(_track_list_k_event)``.

Other notable changes
*********************

..
  Any more descriptive subsystem or driver changes. Do you really want to write
  a paragraph or is it enough to link to the api/driver/Kconfig/board page above?
