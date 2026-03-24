Bluetooth: Classic: AVRCP Shell
################################

This document describes how to use the Bluetooth Classic AVRCP (Audio/Video Remote Control Profile)
functionality via shell commands. The :code:`avrcp` command exposes both Controller (CT) and
Target (TG) roles for exercising AVRCP control and browsing features.

There are two sub-commands: :code:`avrcp ct` and :code:`avrcp tg`.
The :code:`avrcp ct` sub-command provides **Controller (CT)** functionality,
and :code:`avrcp tg` sub-command provides **Target (TG)** functionality.

Prerequisites
-------------

Before running the :code:`avrcp` shell, make sure your build enables Bluetooth Classic and the shell.
An ACL BR/EDR connection to the peer device must be established first (typically via the general
:code:`bt` shell commands) before AVRCP control or browsing connections can be created.

Commands
********

All commands can only be used after the ACL connection has been established except
:code:`avrcp ct register_cb` and :code:`avrcp tg register_cb`.

The :code:`avrcp` commands:

.. code-block:: console

   uart:~$ avrcp
   avrcp - Bluetooth AVRCP shell commands
   Subcommands:
     connect                : connect AVRCP
     disconnect             : disconnect AVRCP
     browsing_connect       : connect browsing AVRCP
     browsing_disconnect    : disconnect browsing AVRCP
     ct                     : AVRCP CT shell commands
     tg                     : AVRCP TG shell commands

The :code:`avrcp ct` commands:

.. code-block:: console

   uart:~$ avrcp ct
   ct - AVRCP CT shell commands
   Subcommands:
     register_cb                 : register avrcp ct callbacks
     get_unit                    : get unit info
     get_subunit                 : get subunit info
     get_caps                    : get capabilities <cap_id: company or events>
     play                        : request a play at the remote player
     pause                       : request a pause at the remote player
     register_notification       : register notify <event_id> [playback_interval]
     set_browsed_player          : set browsed player <player_id>
     get_folder_items            : [none]
     change_path                 : [none]
     get_item_attrs              : get item attrs [scope]
     get_total_number_of_items   : get total number of items [scope]
     search                      : search [search_string]
     list_app_attrs              : [none]
     list_app_vals               : List App vals <attr_id>
     get_app_curr                : Get curr player app setting val [attr1] [attr2] ...
     set_app_val                 : Set app setting Val <attr1> <val1> [<attr2> <val2>] ...
     get_app_attr_text           : Get app setting attrs text <attr1> [attr2] ...
     get_app_val_text            : Get setting vals Text <attr_id> <val1> [val2] ...
     inform_displayable_char     : Inform Displayable Character Set <charset_id1> [charset_id2] ...
     inform_batt                 : Inform Battery Status Of CT <Battery status>
     set_absolute_volume         : set absolute volume <volume>
     get_element_attrs           : get element attrs [identifier] [attr1] [attr2] ...
     get_play_status             : [none]
     set_addressed_player        : set addressed player <player_id>
     play_item                   : PlayItem <scope> <uid_hex> <uid_counter>
     add_to_now_playing          : AddToNowPlaying <scope> <uid_hex> <uid_counter>

The :code:`avrcp tg` commands:

.. code-block:: console

   uart:~$ avrcp tg
   tg - AVRCP TG shell commands
   Subcommands:
     register_cb                                : register avrcp tg callbacks
     send_unit_rsp                              : send unit info response
     send_subunit_rsp                           : [none]
     send_get_caps_rsp                          : send get capabilities response [status]
     send_notification_rsp                      : send notify rsp <event_id> <type> [value...]
     send_browsed_player_rsp                    : Send SetBrowsedPlayer response
     send_get_folder_items_rsp                  : send get folder items [status]
     send_change_path_rsp                       : send change path [status]
     send_get_item_attrs_rsp                    : send get item attrs [status]
     send_get_total_number_of_items_rsp         : send get total number of items [status]
     send_search_rsp                            : search [status]
     send_browsing_general_reject               : send browsing general reject [reason]
     send_passthrough_rsp                       : send_passthrough_rsp <op/opvu> <opid> <state>
     send_list_player_app_setting_attrs_rsp     : send attrs rsp <num> [attr_id...]
     send_list_player_app_setting_vals_rsp      : send vals rsp <num> [val_id...]
     send_get_curr_player_app_setting_val_rsp   : send current vals rsp <num_pairs> [attr val]...
     send_set_player_app_setting_val_rsp        : set app setting val rsp [status]
     send_get_player_app_setting_attr_text_rsp  : send get player app setting attr text rsp [status]
     send_get_player_app_setting_val_text_rsp   : send get player app setting val text rsp [status]
     send_inform_displayable_char_rsp           : send displayable char rsp [status]
     send_inform_batt_status_of_ct_rsp          : send inform batt rsp [status]
     send_get_element_attrs_rsp                 : send get element attrs response<large: 1>
     send_absolute_volume_rsp                   : send absolute volume rsp <volume>
     send_get_play_status_rsp                   : send get play status [status]
     send_set_addressed_player_rsp              : send set addressed player rsp [status]
     send_play_item_rsp                         : send play item rsp [status]
     send_add_to_now_playing_rsp                : send add to now playing rsp [status]

.. _avrcp_basic_operations:

Basic AVRCP Operations
**********************

Demonstrate the flow of basic AVRCP operations:
 * Both sides register AVRCP callbacks using :code:`avrcp ct register_cb` and :code:`avrcp tg register_cb`.
 * Create an AVRCP connection using :code:`avrcp connect`.
 * CT gets capabilities from TG using :code:`avrcp ct get_caps events`.
 * CT registers for notifications using :code:`avrcp ct register_notification 0x01`.
 * CT requests playback control using :code:`avrcp ct play`.
 * CT gets current play status using :code:`avrcp ct get_play_status`.
 * CT gets element attributes (metadata) using :code:`avrcp ct get_element_attrs`.
 * CT sets volume using :code:`avrcp ct set_absolute_volume 50`.
 * CT pauses playback using :code:`avrcp ct pause`.

.. note::
   CT (Controller) sends commands and TG (Target) responds. For notification-capable events,
   TG typically sends an **INTERIM** response immediately after registration to indicate the
   current value, and later a **CHANGED** response when the value actually changes.

.. tabs::

        .. group-tab:: Device A (CT - Controller)

                .. code-block:: console

                        uart:~$ avrcp ct register_cb
                        AVRCP CT callbacks registered
                        uart:~$ avrcp connect
                        AVRCP CT connected
                        uart:~$ avrcp ct get_caps events
                        GetCapabilities : status=0x04
                        Remote supported EventID = 0x01
                        Remote supported EventID = 0x02
                        Remote supported EventID = 0x0d
                        uart:~$ avrcp ct register_notification 0x01
                        Get capabilities command sent successfully: cap_id=events
                        Sent register notification event_id=0x01
                        AVRCP notification rsp: tid=0x01, status=0x04, event_id=0x01
                         Notification type: INTERIM
                         PLAYBACK_STATUS_CHANGED: status=0x00
                        uart:~$ avrcp ct play
                        Passthrough PRESSED command sent successfully: opid=0x44
                        Passthrough RELEASED command sent successfully: opid=0x44
                        <input `avrcp tg send_passthrough_rsp op play pressed` in TG side>
                        AVRCP passthough command accepted, operation id = 0x44, state = 0
                        <input `avrcp tg send_passthrough_rsp op play released` in TG side>
                        AVRCP passthough command accepted, operation id = 0x44, state = 1
                        <input `avrcp tg send_notification_rsp 0x01 changed 1` in TG side>
                        AVRCP notification rsp: tid=0x01, status=0x04, event_id=0x01
                         Notification type: CHANGED
                         PLAYBACK_STATUS_CHANGED: status=0x01
                        uart:~$ avrcp ct get_play_status
                        AVRCP GetPlayStatus
                        getplaystatus : status=0x04
                        GetPlayStatus: len=180000 ms, pos=30000 ms, status=0x01
                         status: PLAYING
                        uart:~$ avrcp ct get_element_attrs
                        Requesting element attributes: identifier=0x0000000000000000, num_attrs=0
                        AVRCP CT get element attrs command sent
                        GetElementAttributes : status=0x04
                        AVRCP GetElementAttributes response received, tid=0x05, num_attrs=7
                         Attr[0]: ID=0x00000001 (TITLE), charset=0x006a, len=11
                           Value: "Test Title"
                         Attr[1]: ID=0x00000002 (ARTIST), charset=0x006a, len=11
                           Value: "Test Artist"
                        uart:~$ avrcp ct set_absolute_volume 50
                        set absolute volume absolute_volume=0x32
                        AVRCP set absolute volume rsp: tid=0x02, status=0x04, volume=0x32
                        uart:~$ avrcp ct pause
                        Passthrough PRESSED command sent successfully: opid=0x46
                        Passthrough RELEASED command sent successfully: opid=0x46
                        <input `avrcp tg send_passthrough_rsp op pause pressed` in TG side>
                        AVRCP passthough command accepted, operation id = 0x46, state = 0
                        <input `avrcp tg send_passthrough_rsp op pause released` in TG side>
                        AVRCP passthough command accepted, operation id = 0x46, state = 1

        .. group-tab:: Device B (TG - Target)

                .. code-block:: console

                        uart:~$ avrcp tg register_cb
                        AVRCP TG callbacks registered
                        <input `avrcp connect` in CT side>
                        AVRCP TG connected
                        <input `avrcp ct get_caps events` in CT side>
                        AVRCP get capabilities command received: cap_id 0x03 (EVENTS_SUPPORTED)
                        uart:~$ avrcp tg send_get_caps_rsp
                        Get capabilities response sent successfully
                        <input `avrcp ct register_notification 0x01` in CT side>
                        receive register notification request event_id=0x01
                        uart:~$ avrcp tg send_notification_rsp 0x01 interim 0
                        Sent notification rsp event_id=0x01 type=interim
                        <input `avrcp ct play` in CT side>
                        receive passthrough command: op_id=0x44 (PLAY)
                        uart:~$ avrcp tg send_passthrough_rsp op play pressed
                        Passthrough opid=0x44 (STANDARD), state=pressed sent successfully
                        uart:~$ avrcp tg send_passthrough_rsp op play released
                        Passthrough opid=0x44 (STANDARD), state=released sent successfully
                        uart:~$ avrcp tg send_notification_rsp 0x01 changed 1
                        Sent notification rsp event_id=0x01 type=changed
                        <input `avrcp ct get_play_status` in CT side>
                        receive get play status request
                        uart:~$ avrcp tg send_get_play_status_rsp
                        GetPlayStatus rsp sent
                        <input `avrcp ct get_element_attrs` in CT side>
                        AVRCP GetElementAttributes command received
                        uart:~$ avrcp tg send_get_element_attrs_rsp 0
                        Sending standard GetElementAttributes response (7 attrs)
                        GetElementAttributes response sent successfully
                        <input `avrcp ct set_absolute_volume 50` in CT side>
                        AVRCP set_absolute_volume_req: tid=0x06, absolute_volume=0x32
                        uart:~$ avrcp tg send_absolute_volume_rsp 50
                        Set absolute volume response sent successfully
                        <input `avrcp ct pause` in CT side>
                        AVRCP passthrough command received: opid = 0x46
                        uart:~$ avrcp tg send_passthrough_rsp op pause pressed
                        send passthrough response
                        uart:~$ avrcp tg send_passthrough_rsp op pause released
                        send passthrough response

AVRCP Connection
****************

The AVRCP profile supports both control and browsing connections. The control connection
is used for basic remote control functionality, while the browsing connection allows
browsing of media content.

Control Connection
==================

Establish AVRCP control connection:

1. Register callbacks (CT side):

.. code-block:: console

   uart:~$ avrcp ct register_cb
   AVRCP CT callbacks registered

2. Register callbacks (TG side):

.. code-block:: console

   uart:~$ avrcp tg register_cb
   AVRCP TG callbacks registered

3. Connect AVRCP:

.. code-block:: console

   uart:~$ avrcp connect
   AVRCP CT connected
   AVRCP TG connected

4. Disconnect AVRCP:

.. code-block:: console

   uart:~$ avrcp disconnect
   AVRCP CT disconnected
   AVRCP TG disconnected

Browsing Connection
===================

After control connection is established, browsing connection can be initiated:

1. Connect browsing:

.. code-block:: console

   uart:~$ avrcp browsing_connect
   AVRCP browsing connect request sent
   AVRCP CT browsing connected
   AVRCP TG browsing connected

2. Disconnect browsing:

.. code-block:: console

   uart:~$ avrcp browsing_disconnect
   AVRCP browsing disconnect request sent
   AVRCP CT browsing disconnected
   AVRCP TG browsing disconnected

Basic Playback Control
**********************

Control playback from CT side:

.. tabs::

   .. group-tab:: Play Command

      .. code-block:: console

         uart:~$ avrcp ct play
         Passthrough PRESSED command sent successfully: opid=0x44
         Passthrough RELEASED command sent successfully: opid=0x44

   .. group-tab:: Pause Command

      .. code-block:: console

         uart:~$ avrcp ct pause
         Passthrough PRESSED command sent successfully: opid=0x46
         Passthrough RELEASED command sent successfully: opid=0x46

Get Capabilities
****************

Query supported capabilities:

.. tabs::

   .. group-tab:: Company ID

      .. code-block:: console

         uart:~$ avrcp ct get_caps company
         Get capabilities command sent successfully: cap_id=company
         GetCapabilities : status=0x04
         Remote CompanyID = 0x001958

   .. group-tab:: Events Supported

      .. code-block:: console

         uart:~$ avrcp ct get_caps events
         Get capabilities command sent successfully: cap_id=events
         GetCapabilities : status=0x04
         Remote supported EventID = 0x01
         Remote supported EventID = 0x02
         Remote supported EventID = 0x03
         Remote supported EventID = 0x04
         Remote supported EventID = 0x0d

   .. group-tab:: TG Response

      .. code-block:: console

         uart:~$ avrcp tg send_get_caps_rsp
         Sending company ID capability rsp: 0x001958

Media Metadata Operations
*************************

Get Element Attributes
======================

Retrieve metadata for the currently playing media:

.. tabs::

   .. group-tab:: CT Request

      .. code-block:: console

         uart:~$ avrcp ct get_element_attrs
         Requesting element attributes: identifier=0x0000000000000000, num_attrs=0
         AVRCP CT get element attrs command sent
         GetElementAttributes : status=0x04
         AVRCP GetElementAttributes response received, tid=0x00, num_attrs=7
          Attr[0]: ID=0x00000001 (TITLE), charset=0x006a, len=11
            Value: "Test Title"
          Attr[1]: ID=0x00000002 (ARTIST), charset=0x006a, len=11
            Value: "Test Artist"
          Attr[2]: ID=0x00000003 (ALBUM), charset=0x006a, len=10
            Value: "Test Album"
          Attr[3]: ID=0x00000004 (TRACK_NUMBER), charset=0x006a, len=1
            Value: "1"
          Attr[4]: ID=0x00000005 (TOTAL_TRACKS), charset=0x006a, len=2
            Value: "10"
          Attr[5]: ID=0x00000006 (GENRE), charset=0x006a, len=4
            Value: "Rock"
          Attr[6]: ID=0x00000007 (PLAYING_TIME), charset=0x006a, len=6
            Value: "240000"

   .. group-tab:: TG Response

      .. code-block:: console

         uart:~$ avrcp tg send_get_element_attrs_rsp 0
         Sending standard GetElementAttributes response (7 attrs)
         GetElementAttributes response sent successfully

Play Status
===========

Get current playback status:

.. tabs::

   .. group-tab:: CT Request

      .. code-block:: console

         uart:~$ avrcp ct get_play_status
         AVRCP GetPlayStatus
         getplaystatus : status=0x04
         GetPlayStatus: len=180000 ms, pos=30000 ms, status=0x01
          status: PLAYING

   .. group-tab:: TG Response

      .. code-block:: console

         uart:~$ avrcp tg send_get_play_status_rsp
         GetPlayStatus rsp sent

Volume Control
**************

Set absolute volume:

.. tabs::

   .. group-tab:: CT Set Volume

      .. code-block:: console

         uart:~$ avrcp ct set_absolute_volume 50
         set absolute volume absolute_volume=0x32
         AVRCP set absolute volume rsp: tid=0x01, status=0x04, volume=0x32

   .. group-tab:: TG Response

      .. code-block:: console

         uart:~$ avrcp tg send_absolute_volume_rsp 50
         Set absolute volume response sent successfully

Event Notifications
*******************

Register for notifications and handle events:

Register for Volume Change Notification
========================================

.. tabs::

   .. group-tab:: CT Register

      .. code-block:: console

         uart:~$ avrcp ct register_notification 0x0d
         Sent register notification event_id=0x0d
         AVRCP notification rsp: tid=0x02, status=0x04, event_id=0x0d
          Notification type: INTERIM
          VOLUME_CHANGED: absolute_volume=0x0a
         AVRCP notify_changed_cb received: event_id=0x0d
          Notification type: CHANGED
          VOLUME_CHANGED: absolute_volume=0x14

   .. group-tab:: TG Send Notification

      .. code-block:: console

         uart:~$ avrcp tg send_notification_rsp 0x0d interim 10
         Sent notification rsp event_id=0x0d type=interim

         uart:~$ avrcp tg send_notification_rsp 0x0d changed 20
         Sent notification rsp event_id=0x0d type=changed

Browsing Operations
*******************

Set Browsed Player
==================

.. tabs::

   .. group-tab:: CT Request

      .. code-block:: console

         uart:~$ avrcp ct set_browsed_player 1
         AVRCP send set browsed player req
         AVRCP set browsed player success, tid = 0
           UID Counter: 1
           Number of Items: 100
           Charset ID: 0x006A
           Folder Depth: 1
           charset_id  : 0x006A
           Get folder Name (hex)  :
         00000000: 4d 75 73 69 63                                   |Music            |

   .. group-tab:: TG Response

      .. code-block:: console

         uart:~$ avrcp tg send_browsed_player_rsp
         Send set browsed player response, status = 0x04

Get Folder Items
================

.. tabs::

   .. group-tab:: CT Request

      .. code-block:: console

         uart:~$ avrcp ct get_folder_items
         Sent GetFolderItems command
         AVRCP get folder items success, tid = 1
           UID Counter: 1
           Number of Items: 1
         Media Player Item:
           item_len   : 28
           player_id   : 1
           major_type  : 0x01
           sub_type    : 0x00000000
           play_status : 0x00
           charset_id  : 0x006A
           name_len    : 4
           charset_id  : 0x006A
           Name (hex)  :
         00000000: 44:65:6d:6f                                      |Demo

   .. group-tab:: TG Response

      .. code-block:: console

         uart:~$ avrcp tg send_get_folder_items_rsp
         TG: Sent GetFolderItems response

Player Application Settings
***************************

List and get configure player application settings:

List Available Settings
=======================

.. tabs::

   .. group-tab:: CT Request

      .. code-block:: console

         uart:~$ avrcp ct list_app_attrs
         Sent list player app setting attrs
         list player app setting attrs : status=0x04
         attr =0x01 (EQUALIZER)
         attr =0x02 (REPEAT_MODE)

   .. group-tab:: TG Response

      .. code-block:: console

         uart:~$ avrcp tg send_list_player_app_setting_attrs_rsp 2 0x01 0x02
         TG: Sent list player app setting attrs response

Get Current Settings
====================

.. tabs::

   .. group-tab:: CT Request

      .. code-block:: console

         uart:~$ avrcp ct get_app_curr 1 2
         Sent get_curr_player_app_setting_val num=2
         get curr player app setting val : status=0x04
         attr_id :1 val 1
         attr_id :2 val 1

   .. group-tab:: TG Response

      .. code-block:: console

         uart:~$ avrcp tg send_get_curr_player_app_setting_val_rsp 2 0x01 0x01 0x02 0x02
         TG: Send get curr player app setting val rsp (num=2)
