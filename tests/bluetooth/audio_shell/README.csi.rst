Coordinated Set Identification for Generic Audio
#################################################

This document describes how to run the coordinated set identification
functionality, both as a client and as a server.
Note that in the examples below, some lines of debug have been removed to make
this shorter and provide a better overview.

Coordinated Set Coordinator (Client)
************************************

The client will typically be a resource-rich device, such as a smartphone
or a laptop. The client is able to lock and release members of a coordinated
set. While the coordinated set is locked, no other clients may lock the set.

To lock a set, the client must connect to each of the set members it wants to
lock. This implementation will always try to to connect to all the members of
the set, and at the same time. Thus if the set size is 3, then
:code:`BT_MAX_CONN` shall be at least 3.

If the locks on set members shall persists through disconnects, it is
necessary to bond with the set members. If you need to bond with multiple
set members, make sure that :code:`BT_MAX_PAIRED` is correctly configured.

Using the Set Coordinator
=========================

When the btaudio stack has been initialized (:code:`btaudio init`),
and a set member device has been connected, the call control client can be
initialized by calling :code:`btaudio csip init`, which will start a discovery
for the TBS uuids and store the handles, and optionally subscribe to all
notifications (default is to subscribe to all).

Once the client has connected and discovered the handles, then it can
read the set information, which is needed to identify other set members.
The client can then scan for and connect to the remaining set members, and once
all the members has been connected to, it can lock and release the set.

It is necessary to enable :code:`BT_DEBUG_CSIP` to properly use the set
coordinator.

Example usage
=============

Setup
-----

.. code-block:: console

   uart:~$ btaudio init
   uart:~$ bt connect xx:xx:xx:xx:xx:xx public

When connected
--------------

Discovering sets on a device:

.. code-block:: console

   uart:~$ btaudio csip init
   <dbg> bt_csip.primary_discover_func: [ATTRIBUTE] handle 0x0048
   <dbg> bt_csip.primary_discover_func: Discover complete, found 1 instances
   <dbg> bt_csip.discover_func: Setup complete for 1 / 1
   Found 1 sets on device
   uart:~$ btaudio csip discover_sets
   <dbg> bt_csip.Set SIRK
   36 04 9a dc 66 3a a1 a1 |6...f:..
   1d 9a 2f 41 01 73 3e 01 |../A.s>.
   <dbg> bt_csip.csip_discover_sets_read_set_size_cb: 2
   <dbg> bt_csip.csip_discover_sets_read_set_lock_cb: 1
   <dbg> bt_csip.csip_discover_sets_read_rank_cb: 1
   Set size 2 (pointer: 0x566fdfe8)

Discover set members, based on the set pointer above:

.. code-block:: console

   uart:~$ btaudio csip discover_members 0x566fdfe8
   <dbg> bt_csip.csis_found: Found CSIS advertiser with address 34:02:86:03:86:c0 (public)
   <dbg> bt_csip.is_set_member: hash: 0x33ccb1, prand 0x5bfe6a
   <dbg> bt_csip.is_discovered: 34:02:86:03:86:c0 (public)
   <dbg> bt_csip.is_discovered: 34:13:e8:b3:7f:9e (public)
   <dbg> bt_csip.csis_found: Found member (2 / 2)
   Discovered 2/2 set members

Lock set members:

.. code-block:: console

   uart:~$ btaudio csip lock_set
   <dbg> bt_csip.bt_csip_lock_set: Connecting to 34:02:86:03:86:c0 (public)
   <dbg> bt_csip.csip_connected: Connected to 34:02:86:03:86:c0 (public)
   <dbg> bt_csip.discover_func: Setup complete for 1 / 1
   <dbg> bt_csip.csip_lock_set_init_cb:
   <dbg> bt_csip.Set SIRK
   36 04 9a dc 66 3a a1 a1 |6...f:..
   1d 9a 2f 41 01 73 3e 01 |../A.s>.
   <dbg> bt_csip.csip_discover_sets_read_set_size_cb: 2
   <dbg> bt_csip.csip_discover_sets_read_set_lock_cb: 1
   <dbg> bt_csip.csip_discover_sets_read_rank_cb: 2
   <dbg> bt_csip.csip_write_lowest_rank: Locking member with rank 1
   <dbg> bt_csip.notify_func: Instance 0 lock was locked
   <dbg> bt_csip.csip_write_lowest_rank: Locking member with rank 2
   <dbg> bt_csip.notify_func: Instance 0 lock was locked
   Set locked

Release set members:

.. code-block:: console

   uart:~$ btaudio csip release_set
   <dbg> bt_csip.csip_release_highest_rank: Releasing member with rank 2
   <dbg> bt_csip.notify_func: Instance 0 lock was released
   <dbg> bt_csip.csip_release_highest_rank: Releasing member with rank 1
   <dbg> bt_csip.notify_func: Instance 0 lock was released
   Set released

Disconnect set members:

.. code-block:: console

   uart:~$ btaudio csip disconnect
   <dbg> bt_csip.bt_csip_disconnect: member 0
   <dbg> bt_csip.bt_csip_disconnect: Disconnecting 34:13:e8:b3:7f:9e (public)
   <dbg> bt_csip.bt_csip_disconnect: member 1
   <dbg> bt_csip.bt_csip_disconnect: Disconnecting 34:02:86:03:86:c0 (public)


Coordinated Set Member (Server)
**********************************************
The server on devices that are part of a set,
consisting of at least two devices, e.g. a pair of earbuds.

Using the Set Member
=====================
The server itself does not expose any APIs to change the values currently.

Example Usage
=============

Setup
-----

.. code-block:: console

   uart:~$ btaudio init
   uart:~$ btaudio csis advertise on
