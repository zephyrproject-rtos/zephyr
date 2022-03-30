Media control for Generic Audio Content Control
###############################################

This document describes how to run the media control functionality,
using the shell, both as a client and as a server.

The media control server consists of to parts. There is a media
player (mpl) that contains the logic to handle media, and there is a
media control service (mcs) that serves as a GATT-based interface to
the player. The media control client consists of one part, the GATT
based client (mcc).

The media control server may include an object transfer service (ots)
and the media control client may include an object transfer client
(otc). When these are included, a richer set of functionality is
available.

The media control server and client both implement the Generic Media
Control Service (only), and do not use any underlying Media Control
Services.

Note that in the examples below, in many cases the debug output has
been removed and long outputs may have been shortened to make the
examples shorter and clearer.

Also note that this documentation does not list all shell commands, it
just shows examples of some of them. The set of commands is
explorable from the mcc shell and the mpl shell, by typing :code:`mcc`
or :code:`mpl` and pressing TAB.  A help text for each command can be
found by doing :code:`mcc <command> help` or or :code:`mpl <command>
help`.

Overview
********

A media player has a *name* and an *icon* that allows identification
of the player for the user.

The content of the media player is structured into tracks and groups.
A media player has a number of groups. A group contains tracks and
other groups. (In this implementation, a group only contains tracks,
not other groups.) Tracks can be divided into segments.

An active player will have a *current track*. This is the track that
is playing now (if the player is playing). The current track has a
*title*, a *duration* (given in hundredths of a second) and a
*position* - the current position of the player within the track.

There is also a *current group* (the group of the current track), a
*parent group* (the parent group of the current group) and a *next
track*.

The media player is in a *state*, which will be one of playing,
paused, seeking or inactive. When playing, playback happens at a
given *playback speed*, and the tracks are played according to the
*playing order*, which is one of the *playing orders supported*.
Track changes are signalled as notifications of the *track changed*
characteristic. When seeking (fast forward or fast rewind), the track
position is moved according to the *seeking speed*.

The *opcodes supported* tells which operations are supported by the
player by writing to the *media control point*. There is also a
*search control point* that allows to search for groups and tracks
according to various criteria, with the result returned in the *search
results*.

Finally, the *content control ID* is used to associate the media
player with an audio stream.


Media Control Client (MCP)
**************************

The media control client is used to control, and to get information
from, a media control server. Control is done by writing to one of
the two control points, or by writing to other writable
characteristics. Getting information is done by reading
characteristics, or by configuring the server to send notifications.

Using the media control client
==============================

Before use, the media control client must be initialized by the
command :code:`mcc init`.

To achieve a connection to the peer, the :code:`bt` commands must be
used - :code:`bt init` followed by :code:`bt advertise on` (or
:code:`bt connect` if the server is advertising).

When the media control client is connected to a media control server,
the client can discover the server's Generic Media Control Service, by
giving the command :code:`mcc discover_mcs`. This will store the
handles of the service, and (optionally, but default) subscribe to all
notifications.

After discovery, the media control client can read and write
characteristics, including the media control point and the search
control point.


Example usage
=============

Setup
-----

.. code-block:: console

   uart:~$ bt init
   Bluetooth initialized

   uart:~$ mcc init
   MCC init complete

   uart:~$ bt advertise on
   Advertising started
   Connected: F6:58:DC:27:F3:57 (random)


When connected
--------------

Service discovery (GMCS and included OTS):

.. code-block:: console

   uart:~$ mcc discover_mcs
   <dbg> bt_mcc.bt_mcc_discover_mcs: start discovery of MCS primary service
   <dbg> bt_mcc.discover_primary_func: [ATTRIBUTE] handle 0x00ae
   <dbg> bt_mcc.discover_primary_func: Primary discovery complete
   <dbg> bt_mcc.discover_primary_func: UUID: 2800
   <dbg> bt_mcc.discover_primary_func: UUID: 8fd7
   <dbg> bt_mcc.discover_primary_func: Start discovery of MCS characteristics
   <dbg> bt_mcc.discover_mcs_char_func: [ATTRIBUTE] handle 0x00b0
   <dbg> bt_mcc.discover_mcs_char_func: Player name, UUID: 8fa0
   <dbg> bt_mcc.discover_mcs_char_func: [ATTRIBUTE] handle 0x00b2
   <dbg> bt_mcc.discover_mcs_char_func: Icon Object, UUID: 8fa1
   <dbg> bt_mcc.discover_mcs_char_func: [ATTRIBUTE] handle 0x00b4
   <dbg> bt_mcc.discover_mcs_char_func: Icon URI, UUID: 8fa2
   <dbg> bt_mcc.discover_mcs_char_func: [ATTRIBUTE] handle 0x00b6
   <dbg> bt_mcc.discover_mcs_char_func: Track Changed, UUID: 8fa3
   <dbg> bt_mcc.discover_mcs_char_func: Subscribing - handle: 0x00b6
   [...]
   <dbg> bt_mcc.discover_mcs_char_func: [ATTRIBUTE] handle 0x00ea
   <dbg> bt_mcc.discover_mcs_char_func: Content Control ID, UUID: 8fb5
   <dbg> bt_mcc.discover_mcs_char_func: Setup complete for MCS
   <dbg> bt_mcc.discover_mcs_char_func: Start discovery of included services
   <dbg> bt_mcc.discover_include_func: [ATTRIBUTE] handle 0x00af
   <dbg> bt_mcc.discover_include_func: Include UUID 1825
   <dbg> bt_mcc.discover_include_func: Discover include complete for MCS: OTS
   <dbg> bt_mcc.discover_include_func: Start discovery of OTS characteristics
   <dbg> bt_mcc.discover_otc_char_func: [ATTRIBUTE] handle 0x009c
   <dbg> bt_mcc.discover_otc_char_func: OTS Features
   [...]
   <dbg> bt_mcc.discover_otc_char_func: [ATTRIBUTE] handle 0x00ac
   <dbg> bt_mcc.discover_otc_char_func: Object Size
   Discovery complete
   <dbg> bt_otc.bt_otc_register: 0
   <dbg> bt_otc.bt_otc_register: L2CAP psm 0x  25 sec_level 1 registered
   <dbg> bt_mcc.discover_otc_char_func: Setup complete for OTS 1 / 1
   uart:~$


Reading characteristics - the player name and the track duration as
examples:

.. code-block:: console

   uart:~$ mcc read_player_name
   Player name: My media player
   4d 79 20 6d 65 64 69 61  20 70 6c 61 79 65 72    |My media  player

   uart:~$ mcc read_track_duration
   Track duration: 6300

Note that the value of some characteristics may be truncated due to
being too long to fit in the ATT packet. Increasing the ATT MTU may
help:

.. code-block:: console

   uart:~$ mcc read_track_title
   Track title: Interlude #1 (Song for

   uart:~$ gatt exchange-mtu
   Exchange pending
   Exchange successful

   uart:~$ mcc read_track_title
   Track title: Interlude #1 (Song for Alison)

Writing characteristics - track position as an example:

The track position is where the player "is" in the current track.
Read the track position, change it by writing to it, confirm by
reading it again.

.. code-block:: console

   uart:~$ mcc read_track_position
   Track Position: 0

   uart:~$ mcc set_track_position 500
   Track Position: 500

   uart:~$ mcc read_track_position
   Track Position: 500


Controlling the player via the control point:

Writing to the control point allows the client to request the server
to do operations like play, pause, fast forward, change track, change
group and so on. Some operations (e.g. goto track) take an argument.
Currently, raw opcode values are used as input to the control point
shell command. These opcode values can be found in the mpl.h header
file.

Send the play command (opcode "1"), the command to go to track (opcode
"52") number three, and the pause command (opcode "2"):

.. code-block:: console

   uart:~$ mcc set_cp 1
   Media State: 1
   Operation: 1, result: 1
   Operation: 1, param: 0

   uart:~$ mcc set_cp 52 3
   Track changed
   Track title: Interlude #3 (Levanto Seventy)
   Track duration: 7800
   Track Position: 0
   Current Track Object ID: 0x000000000104
   Next Track Object ID: 0x000000000105
   Operation: 52, result: 1
   Operation: 52, param: 3

   uart:~$ mcc set_cp 2
   Media State: 2
   Operation: 2, result: 1
   Operation: 2, param: 0



Using the included object transfer client
-----------------------------------------

When object transfer is supported by both the client and the server, a
larger set of characteristics is available. These include object IDs
for the various track and group objects. These IDs can be used to
select and download the corresponding objects from the server's object
transfer service.


Read the object ID of the current group object:

.. code-block:: console

   uart:~$ mcc read_current_group_obj_id
   Current Group Object ID: 0x000000000107


Select the object with that ID:

.. code-block:: console

   uart:~$ mcc ots_select 0x107
   Selecting object succeeded


Read the object's metadata:

.. code-block:: console

   uart:~$ mcc ots_read_metadata
   Reading object metadata succeeded
   <inf> bt_mcc: Object's meta data:
   <inf> bt_mcc:        Current size    :35
   <inf> bt_otc: --- Displaying 1 metadata records ---
   <inf> bt_otc: Object ID: 0x000000000107
   <inf> bt_otc: Object name: Joe Pass - Guitar Inte
   <inf> bt_otc: Object Current Size: 35
   <inf> bt_otc: Object Allocate Size: 35
   <inf> bt_otc: Type: Group Obj Type
   <inf> bt_otc: Properties:0x4
   <inf> bt_otc:  - read permitted


Read the object itself:

The object received is a group object. It consists of a series of
records consisting of a type (track or group) and an object ID.

.. code-block:: console

   uart:~$ mcc ots_read_current_group_object
   <dbg> bt_mcc.on_group_content: Object type: 0, object  ID: 0x000000000102
   <dbg> bt_mcc.on_group_content: Object type: 0, object  ID: 0x000000000103
   <dbg> bt_mcc.on_group_content: Object type: 0, object  ID: 0x000000000104
   <dbg> bt_mcc.on_group_content: Object type: 0, object  ID: 0x000000000105
   <dbg> bt_mcc.on_group_content: Object type: 0, object  ID: 0x000000000106


Search
------

The search control point takes as its input a sequence of search
control items, each consisting of length, type (e.g. track name or
artist name) and parameter (the track name or artist name to search
for). If the result is successful, the search results are stored in
an object in the object transfer service. The ID of the search
results ID object can be read from the search results object ID
characteristic. The search result object can then be downloaded as
for the current group object above. (Note that the search results
object ID is empty until a search has been done.)

This implementation has a working implementation of the search
functionality interface and the server-side search control point
parameter parsing. But the **actual searching is faked**, the same
results are returned no matter what is searched for.

There are two commands for search, one (:code:`mcc set_scp_raw`)
allows to input the search control point parameter (the sequence of
search control items) as a string. The other (:code:`mcc
set_scp_ioptest`) does preset IOP test searches and takes the round
number of the IOP search control point test as a parameter.

Before the search, the search results object ID is empty

.. code-block:: console

   uart:~$ mcc read_search_results_obj_id
   Search Results Object ID: 0x000000000000
   <dbg> bt_mcc.mcc_read_search_results_obj_id_cb: Zero-length Search Results Object ID

Run the search corresponding to the fourth round of the IOP test:

The search control point parameter generated by this command and
parameter has one search control item. The length field (first octet)
is 16 (0x10). (The length of the length field itself is not
included.) The type field (second octet) is 0x04 (search for a group
name). The parameter (the group name to search for) is
"TSPX_Group_Name".

.. code-block:: console

   uart:~$ mcc set_scp_ioptest 4
   Search string:
   00000000: 10 04 54 53 50 58 5f 47  72 6f 75 70 5f 4e 61 6d |..TSPX_G roup_Nam|
   00000010: 65                                               |e                |
   Search control point notification result code: 1
   Search Results Object ID: 0x000000000107
   Search Control Point set

After the successful search, the search results object ID has a value:

.. code-block:: console

   uart:~$ mcc read_search_results_obj_id
   Search Results Object ID: 0x000000000107


Media Control Service (MCS)
***************************

The media control service (mcs) and the associated media player (mpl)
typically reside on devices that can provide access to, and serve,
media content, like PCs and smartphones.

As mentioned above, the media player (mpl) has the player logic, while
the media control service (mcs) has the GATT-based interface. This
separation is done so that the media player can also be used without
the GATT-based interface.


Using the media control service and the media player
====================================================

The media control service and the media player are in general
controlled remotely, from the media control client.

Before use, the media control client must be initialized by the
command :code:`mpl init`.

As for the client, the :code:`bt` commands are used for connecting -
:code:`bt init` followed by :code:`bt connect <address> <address
type>` (or :code:`bt advertise on` if the server is advertising).


Example Usage
=============

Setup
-----

.. code-block:: console

   uart:~$ bt init
   Bluetooth initialized

   uart:~$ mpl init
   [Large amounts of debug output]

   uart:~$ bt connect F9:33:3B:67:D2:A7 (random)
   Connection pending
   Connected: F9:33:3B:67:D2:A7 (random)


When connected
--------------

Control is done from the client.

The server will give debug output related to the various operations
performed by the client.

Example: Debug output by the server when the client gives the "next
track" command:

.. code-block:: console

   [00:13:29.932,373] <dbg> bt_mcs.control_point_write: Opcode: 49
   [00:13:29.932,403] <dbg> bt_mpl.mpl_operation_set: opcode: 49, param: 536880068
   [00:13:29.932,403] <dbg> bt_mpl.paused_state_operation_handler: Operation opcode: 49
   [00:13:29.932,495] <dbg> bt_mpl.do_next_track: Track ID before: 0x000000000104
   [00:13:29.932,586] <dbg> bt_mpl.do_next_track: Track ID after: 0x000000000105
   [00:13:29.932,617] <dbg> bt_mcs.mpl_track_changed_cb: Notifying track change
   [00:13:29.932,708] <dbg> bt_mcs.mpl_track_title_cb: Notifying track title: Interlude #4 (Vesper Dreams)
   [00:13:29.932,800] <dbg> bt_mcs.mpl_track_duration_cb: Notifying track duration: 13500
   [00:13:29.932,861] <dbg> bt_mcs.mpl_track_position_cb: Notifying track position: 0
   [00:13:29.933,044] <dbg> bt_mcs.mpl_current_track_id_cb: Notifying current track ID: 0x000000000105
   [00:13:29.933,258] <dbg> bt_mcs.mpl_next_track_id_cb: Notifying next track ID: 0x000000000106
   [00:13:29.933,380] <dbg> bt_mcs.mpl_operation_cb: Notifying control point - opcode: 49, result: 1


Some server commands are available. These commands force
notifications of the various characterstics, for testing that the
client receives notifications. The values sent in the notifications
caused by these testing commands are independent of the media player,
so they do not correspond the actual values of the characteristics nor
to the actual state of the media player.

Example: Force (fake value) notification of the track duration:

.. code-block:: console

   uart:~$ mpl duration_changed_cb
   [00:15:17.491,058] <dbg> bt_mcs.mpl_track_duration_cb: Notifying track duration: 12000
