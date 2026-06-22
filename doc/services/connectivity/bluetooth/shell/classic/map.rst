Bluetooth: MAP Shell
####################

This document describes how to run the Bluetooth Classic MAP (Message Access Profile)
shell functionality. The :code:`map` command exposes the Bluetooth Classic MAP shell commands.

Commands
********

Main Commands
=============

The :code:`map` commands:

.. code-block:: console

   uart:~$ map
   map - Bluetooth MAP shell commands
   Subcommands:
     add_header  : Adding header sets
     mce         : MCE commands
     mse         : MSE commands

MCE Commands
============

The :code:`map mce` commands:

.. code-block:: console

   uart:~$ map mce
   mce - MCE commands
   Subcommands:
     mas  : MCE MAS commands
     mns  : MCE MNS commands

MCE MAS Commands
----------------

The :code:`map mce mas` commands:

.. code-block:: console

   uart:~$ map mce mas
   mas - MCE MAS commands
   Subcommands:
     discover            : [none]
     select              : <instance_id>
     rfcomm_connect      : <channel> <instance_id> [supported_features]
     rfcomm_disconnect   : [none]
     l2cap_connect       : <psm> <instance_id> [supported_features]
     l2cap_disconnect    : [none]
     connect             : [none]
     disconnect          : [none]
     abort               : [none]
     set_folder          : <path: "/" | ".." | "../folder" | "folder">
     set_ntf_reg         : [none]
     get_folder_listing  : [srmp]
     get_msg_listing     : [name <folder_name>] [srmp]
     get_msg             : <msg_handle> [srmp]
     set_msg_status      : <msg_handle>
     push_msg            : [folder_name]
     update_inbox        : [none]
     get_mas_inst_info   : [srmp]
     set_owner_status    : [none]
     get_owner_status    : [srmp]
     get_convo_listing   : [srmp]
     set_ntf_filter      : [none]

MCE MNS Commands
----------------

The :code:`map mce mns` commands:

.. code-block:: console

   uart:~$ map mce mns
   mns - MCE MNS commands
   Subcommands:
     rfcomm_register    : [none]
     rfcomm_disconnect  : [none]
     l2cap_register     : [none]
     l2cap_disconnect   : [none]
     connect            : <rsp: success, error> [rsp_code]
     disconnect         : <rsp: success, error> [rsp_code]
     abort              : <rsp: success, error> [rsp_code]
     send_event         : <rsp: noerror, error> [rsp_code] [srmp]

MSE Commands
============

The :code:`map mse` commands:

.. code-block:: console

   uart:~$ map mse
   mse - MSE commands
   Subcommands:
     mas  : MSE MAS commands
     mns  : MSE MNS commands

MSE MAS Commands
----------------

The :code:`map mse mas` commands:

.. code-block:: console

   uart:~$ map mse mas
   mas - MSE MAS commands
   Subcommands:
     select              : <instance_id>
     rfcomm_register     : [none]
     rfcomm_disconnect   : [none]
     l2cap_register      : [none]
     l2cap_disconnect    : [none]
     connect             : <rsp: success, error> [rsp_code]
     disconnect          : <rsp: success, error> [rsp_code]
     abort               : <rsp: success, error> [rsp_code]
     set_folder          : <rsp: success, error> [rsp_code]
     set_ntf_reg         : <rsp: noerror, error> [rsp_code]
     get_folder_listing  : <rsp: noerror, error> [rsp_code] [srmp]
     get_msg_listing     : <rsp: noerror, error> [rsp_code] [srmp]
     get_msg             : <rsp: noerror, error> [rsp_code] [srmp]
     set_msg_status      : <rsp: noerror, error> [rsp_code]
     push_msg            : <rsp: noerror, error> [rsp_code] [srmp]
     update_inbox        : <rsp: noerror, error> [rsp_code]
     get_mas_inst_info   : <rsp: noerror, error> [rsp_code] [srmp]
     set_owner_status    : <rsp: noerror, error> [rsp_code]
     get_owner_status    : <rsp: noerror, error> [rsp_code] [srmp]
     get_convo_listing   : <rsp: noerror, error> [rsp_code] [srmp]
     set_ntf_filter      : <rsp: noerror, error> [rsp_code]

MSE MNS Commands
----------------

The :code:`map mse mns` commands:

.. code-block:: console

   uart:~$ map mse mns
   mns - MSE MNS commands
   Subcommands:
     discover            : [none]
     rfcomm_connect      : <channel> [supported_features]
     rfcomm_disconnect   : [none]
     l2cap_connect       : <psm> [supported_features]
     l2cap_disconnect    : [none]
     connect             : [none]
     disconnect          : [none]
     abort               : [none]
     send_event          : [none]

Application Parameter Commands
==============================

The :code:`map add_header app_param` commands:

.. code-block:: console

   uart:~$ map add_header app_param
   app_param - Application parameter commands
   Subcommands:
     list                          : List all added application parameters
     clear                         : Clear all application parameters
     max_list_count                : <count: 0-65535>
     list_start_offset             : <offset: 0-65535>
     filter_msg_type               : <type_mask: hex>
     filter_period_begin           : <time: YYYYMMDDTHHMMSS[+/-HHMM]>
     filter_period_end             : <time: YYYYMMDDTHHMMSS[+/-HHMM]>
     filter_read_status            : <no_filter/unread/read>
     filter_recipient              : <recipient_address>
     filter_originator             : <originator_address>
     filter_priority               : <no_filter/high/non_high>
     attachment                    : <on/off>
     transparent                   : <on/off>
     retry                         : <on/off>
     new_message                   : <on/off>
     notification_status           : <on/off>
     mas_instance_id               : <id: 0-255>
     parameter_mask                : <mask: hex>
     folder_listing_size           : <size: 0-65535>
     listing_size                  : <size: 0-65535>
     subject_length                : <length: 0-255>
     charset                       : <native/utf8>
     fraction_request              : <first/next>
     fraction_deliver              : <more/last>
     status_indicator              : <read/deleted/extended>
     status_value                  : <yes/no>
     mse_time                      : <time: YYYYMMDDTHHMMSS[+/-HHMM]>
     database_id                   : <id_string: max 32 chars>
     convo_listing_ver             : <version_string: max 32 chars>
     presence_availability         : <unknown/offline/online/away/dnd/busy/meeting>
     presence_text                 : <text>
     last_activity                 : <time: YYYYMMDDTHHMMSS[+/-HHMM]>
     filter_last_activity_begin    : <time: YYYYMMDDTHHMMSS[+/-HHMM]>
     filter_last_activity_end      : <time: YYYYMMDDTHHMMSS[+/-HHMM]>
     chat_state                    : <unknown/inactive/active/composing/paused/gone>
     conversation_id               : <id_string: max 32 chars>
     folder_ver                    : <version_string: max 32 chars>
     filter_msg_handle             : <handle_string: max 16 chars>
     notification_filter_mask      : <mask: hex>
     convo_parameter_mask          : <mask: hex>
     owner_uci                     : <uci_string>
     extended_data                 : <data_string>
     map_supported_features        : [none]
     message_handle                : <handle_string: max 16 chars>
     modify_text                   : <prepend/replace>

Usage Examples
**************

SDP Discovery
=============

MCE Discover MSE
----------------

The MCE can discover MSE services on the remote device:

.. code-block:: console

   uart:~$ map mce mas discover
   Found RFCOMM channel 0x06
   Found GOEP L2CAP PSM 0x1003
   Found MAP profile version 0x0104
   Found MAP features 0x007fffff
   Found service name: MAP MAS 0
   Found MAP instance ID 0
   Found MAP MSG type 0x1f
   uart:~$

MSE Discover MCE
----------------

The MSE can discover MCE services on the remote device:

.. code-block:: console

   uart:~$ map mse mns discover
   Found RFCOMM channel 0x06
   Found GOEP L2CAP PSM 0x1003
   Found MAP profile version 0x0104
   Found MAP features 0x0077ffff
   Found service name: MAP MNS
   uart:~$

Connect MAP Transport
=====================

The ACL connection should be established before creating the MAP transport connection.

MAS Transport Connection
------------------------

RFCOMM Transport
~~~~~~~~~~~~~~~~

.. tabs::

   .. group-tab:: MCE

      .. code-block:: console

         uart:~$ map mce mas rfcomm_connect 6 0 7fffff
         Security changed: XX:XX:XX:XX:XX:XX level 2
         MCE MAS RFCOMM connected: 0x20005a00, addr: XX:XX:XX:XX:XX:XX, instance_id: 0
         Selected MCE MAS instance_id 0 as default
         uart:~$

   .. group-tab:: MSE

      .. code-block:: console

         uart:~$ map mse mas rfcomm_register
         RFCOMM server (channel 06, mas_instance_id 0, map_supported_features 007fffff) is registered
         MSE MAS SDP record 0 registered
         RFCOMM server (channel 07, mas_instance_id 1, map_supported_features 007fffff) is registered
         MSE MAS SDP record 1 registered
         Security changed: XX:XX:XX:XX:XX:XX level 2
         MSE MAS RFCOMM connected: 0x20005c00, addr: XX:XX:XX:XX:XX:XX, instance_id: 0
         Selected MSE MAS instance_id 0 as default
         uart:~$

L2CAP Transport
~~~~~~~~~~~~~~~

.. tabs::

   .. group-tab:: MCE

      .. code-block:: console

         uart:~$ map mce mas l2cap_connect 1003 0 7fffff
         Security changed: XX:XX:XX:XX:XX:XX level 2
         MCE MAS L2CAP connected: 0x20005a00, addr: XX:XX:XX:XX:XX:XX, instance_id: 0
         Selected MCE MAS instance_id 0 as default
         uart:~$

   .. group-tab:: MSE

      .. code-block:: console

         uart:~$ map mse mas l2cap_register
         L2CAP server (psm 1003, mas_instance_id 0, map_supported_features 007fffff) is registered
         MSE MAS SDP record 0 registered
         L2CAP server (psm 1005, mas_instance_id 1, map_supported_features 007fffff) is registered
         MSE MAS SDP record 1 registered
         Security changed: XX:XX:XX:XX:XX:XX level 2
         MSE MAS L2CAP connected: 0x20005c00, addr: XX:XX:XX:XX:XX:XX, instance_id: 0
         Selected MSE MAS instance_id 0 as default
         uart:~$

MNS Transport Connection
------------------------

RFCOMM Transport
~~~~~~~~~~~~~~~~

.. tabs::

   .. group-tab:: MCE

      .. code-block:: console

         uart:~$ map mce mns rfcomm_register
         RFCOMM server (channel 06, map_supported_features 0077ffff) is registered
         MCE MNS SDP record registered
         MCE MNS RFCOMM connected: 0x20005b00, addr: XX:XX:XX:XX:XX:XX
         uart:~$

   .. group-tab:: MSE

      .. code-block:: console

         uart:~$ map mse mns rfcomm_connect 6 77ffff
         MSE MNS RFCOMM connected: 0x20005d00, addr: XX:XX:XX:XX:XX:XX
         uart:~$

L2CAP Transport
~~~~~~~~~~~~~~~

.. tabs::

   .. group-tab:: MCE

      .. code-block:: console

         uart:~$ map mce mns l2cap_register
         L2CAP server (psm 1003, map_supported_features 0077ffff) is registered
         MCE MNS SDP record registered
         MCE MNS L2CAP connected: 0x20005b00, addr: XX:XX:XX:XX:XX:XX
         uart:~$

   .. group-tab:: MSE

      .. code-block:: console

         uart:~$ map mse mns l2cap_connect 1003 77ffff
         MSE MNS L2CAP connected: 0x20005d00, addr: XX:XX:XX:XX:XX:XX
         uart:~$

Disconnect MAP Transport
========================

MAS Transport Disconnection
---------------------------

RFCOMM Transport (MCE initiates)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. tabs::

   .. group-tab:: MCE

      .. code-block:: console

         uart:~$ map mce mas rfcomm_disconnect
         MCE MAS RFCOMM disconnected: 0x20005a00
         No connected MCE MAS instance available for conn XX:XX:XX:XX:XX:XX
         uart:~$

   .. group-tab:: MSE

      .. code-block:: console

         MSE MAS RFCOMM disconnected: 0x20005c00
         No connected MSE MAS instance available for conn XX:XX:XX:XX:XX:XX
         uart:~$

RFCOMM Transport (MSE initiates)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. tabs::

   .. group-tab:: MCE

      .. code-block:: console

         MCE MAS RFCOMM disconnected: 0x20005a00
         No connected MCE MAS instance available for conn XX:XX:XX:XX:XX:XX
         uart:~$

   .. group-tab:: MSE

      .. code-block:: console

         uart:~$ map mse mas rfcomm_disconnect
         MSE MAS RFCOMM disconnected: 0x20005c00
         No connected MSE MAS instance available for conn XX:XX:XX:XX:XX:XX
         uart:~$

L2CAP Transport (MCE initiates)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. tabs::

   .. group-tab:: MCE

      .. code-block:: console

         uart:~$ map mce mas l2cap_disconnect
         MCE MAS L2CAP disconnected: 0x20005a00
         No connected MCE MAS instance available for conn XX:XX:XX:XX:XX:XX
         uart:~$

   .. group-tab:: MSE

      .. code-block:: console

         MSE MAS L2CAP disconnected: 0x20005c00
         No connected MSE MAS instance available for conn XX:XX:XX:XX:XX:XX
         uart:~$

L2CAP Transport (MSE initiates)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. tabs::

   .. group-tab:: MCE

      .. code-block:: console

         MCE MAS L2CAP disconnected: 0x20005a00
         No connected MCE MAS instance available for conn XX:XX:XX:XX:XX:XX
         uart:~$

   .. group-tab:: MSE

      .. code-block:: console

         uart:~$ map mse mas l2cap_disconnect
         MSE MAS L2CAP disconnected: 0x20005c00
         No connected MSE MAS instance available for conn XX:XX:XX:XX:XX:XX
         uart:~$

MNS Transport Disconnection
---------------------------

RFCOMM Transport (MSE initiates)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. tabs::

   .. group-tab:: MCE

      .. code-block:: console

         MCE MNS RFCOMM disconnected: 0x20005b00
         uart:~$

   .. group-tab:: MSE

      .. code-block:: console

         uart:~$ map mse mns rfcomm_disconnect
         MSE MNS RFCOMM disconnected: 0x20005d00
         uart:~$

RFCOMM Transport (MCE initiates)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. tabs::

   .. group-tab:: MCE

      .. code-block:: console

         uart:~$ map mce mns rfcomm_disconnect
         MCE MNS RFCOMM disconnected: 0x20005b00
         uart:~$

   .. group-tab:: MSE

      .. code-block:: console

         MSE MNS RFCOMM disconnected: 0x20005d00
         uart:~$

L2CAP Transport (MSE initiates)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. tabs::

   .. group-tab:: MCE

      .. code-block:: console

         MCE MNS L2CAP disconnected: 0x20005b00
         uart:~$

   .. group-tab:: MSE

      .. code-block:: console

         uart:~$ map mse mns l2cap_disconnect
         MSE MNS L2CAP disconnected: 0x20005d00
         uart:~$

L2CAP Transport (MCE initiates)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. tabs::

   .. group-tab:: MCE

      .. code-block:: console

         uart:~$ map mce mns l2cap_disconnect
         MCE MNS L2CAP disconnected: 0x20005b00
         uart:~$

   .. group-tab:: MSE

      .. code-block:: console

         MSE MNS L2CAP disconnected: 0x20005d00
         uart:~$

Connect to MAP Server
=====================

MCE MAS Connect to MSE MAS
--------------------------

.. tabs::

   .. group-tab:: MCE

      .. code-block:: console

         uart:~$ map mce mas connect
         MCE MAS 0x20005a00 conn rsp, rsp_code Success, version 10, mopl 0fff
         HI 4a Len 16
         00000000: bb 58 2b 40 42 0c 11 db  b0 de 08 00 20 0c 9a 66 |.X+@B... .... ..f|
         HI cb Len 4
         Conn ID: 00000001
         uart:~$

   .. group-tab:: MSE

      .. code-block:: console

         MSE MAS 0x20005c00 conn req, version 10, mopl 0fff
         HI 46 Len 16
         00000000: bb 58 2b 40 42 0c 11 db  b0 de 08 00 20 0c 9a 66 |.X+@B... .... ..f|
         HI 4c Len 6
         T 29 L 4
         00000000: 00 77 ff ff                                      |.w..             |
         uart:~$ map mse mas connect success
         uart:~$

MSE MNS Connect to MCE MNS
--------------------------

.. tabs::

   .. group-tab:: MSE

      .. code-block:: console

         uart:~$ map mse mns connect
         MSE MNS 0x20005d00 conn rsp, rsp_code Success, version 10, mopl 0fff
         HI 4a Len 16
         00000000: bb 58 2b 41 42 0c 11 db  b0 de 08 00 20 0c 9a 66 |.X+AB... .... ..f|
         HI cb Len 4
         Conn ID: 00000001
         uart:~$

   .. group-tab:: MCE

      .. code-block:: console

         MCE MNS 0x20005b00 conn req, version 10, mopl 0fff
         HI 46 Len 16
         00000000: bb 58 2b 41 42 0c 11 db  b0 de 08 00  20 0c 9a 66 |.X+AB... .... ..f|
         uart:~$ map mce mns connect success
         uart:~$

Disconnect from MAP Server
==========================

MCE MAS Disconnect from MSE MAS
-------------------------------

.. tabs::

   .. group-tab:: MCE

      .. code-block:: console

         uart:~$ map mce mas disconnect
         MCE MAS 0x20005a00 disconn rsp, rsp_code Success
         uart:~$

   .. group-tab:: MSE

      .. code-block:: console

         MSE MAS 0x20005c00 disconn req
         HI c0 Len 4
         Conn ID: 00000001
         uart:~$ map mse mas disconnect success
         uart:~$

MSE MNS Disconnect from MCE MNS
-------------------------------

.. tabs::

   .. group-tab:: MSE

      .. code-block:: console

         uart:~$ map mse mns disconnect
         MSE MNS 0x20005d00 disconn rsp, rsp_code Success
         uart:~$

   .. group-tab:: MCE

      .. code-block:: console

         MCE MNS 0x20005b00 disconn req
         HI c0 Len 4
         Conn ID: 00000001
         uart:~$ map mce mns disconnect success
         uart:~$

MAP Operations
==============

Set Notification Registration
-----------------------------

.. tabs::

   .. group-tab:: MCE

      .. code-block:: console

         uart:~$ map mce mas set_ntf_reg
         Adding default application parameters:
           Notification status: 0x01
         MCE MAS 0x20005a00 set_ntf_reg rsp, rsp_code Success
         uart:~$

   .. group-tab:: MSE

      .. code-block:: console

         MSE MAS 0x20005c00 set_ntf_reg req, final true, data len 52
         HI cb Len 4
         Conn ID: 00000001
         HI 42 Len 34
         00000000: 78 2d 62 74 2f 4d 41 50  2d 4e 6f 74 69 66 69 63 |x-bt/MAP -Notific|
         00000010: 61 74 69 6f 6e 52 65 67  69 73 74 72 61 74 69 6f |ationReg istratio|
         00000020: 6e 00                                            |n.               |
         HI 4c Len 3
         T 0e L 1
         00000000: 01                                               |.                |
         HI 49 Len 1
         00000000: 30                                               |0                |
         uart:~$ map mse mas set_ntf_reg noerror
         uart:~$

Set Folder
----------

.. tabs::

   .. group-tab:: MCE

      .. code-block:: console

         uart:~$ map mce mas set_folder /
         MCE MAS 0x20005a00 set_folder rsp, rsp_code Success
         uart:~$ map mce mas set_folder telecom
         MCE MAS 0x20005a00 set_folder rsp, rsp_code Success
         uart:~$ map mce mas set_folder ..
         MCE MAS 0x20005a00 set_folder rsp, rsp_code Success
         uart:~$

   .. group-tab:: MSE

      .. code-block:: console

         MSE MAS 0x20005c00 set_folder req, flags 2, data len 8
         HI cb Len 4
         Conn ID: 00000001
         HI 01 Len 0
         uart:~$ map mse mas set_folder success
         MSE MAS 0x20005c00 set_folder req, flags 2, data len 24
         HI cb Len 4
         Conn ID: 00000001
         HI 01 Len 16
         00000000: 00 74 00 65 00 6c 00 65  00 63 00 6f 00 6d 00 00 |.t.e.l.e .c.o.m..|
         uart:~$ map mse mas set_folder success
         MSE MAS 0x20005c00 set_folder req, flags 3, data len 8
         HI cb Len 4
         Conn ID: 00000001
         HI 01 Len 0
         uart:~$ map mse mas set_folder success
         uart:~$

Get Folder Listing
------------------

RFCOMM Transport
~~~~~~~~~~~~~~~~

When using RFCOMM transport, the same shell command can be called multiple times
until the complete message is delivered.

.. tabs::

   .. group-tab:: MCE

      .. code-block:: console

         uart:~$ map mce mas get_folder_listing
         MCE MAS 0x20005a00 get_folder_listing rsp, rsp_code Continue, data len 252
         HI 48 Len 249
         00000000: 3c 3f 78 6d 6c 20 76 65  72 73 69 6f 6e 3d 27 31 |<?xml ve rsion='1|
         00000010: 2e 30 27 20 65 6e 63 6f  64 69 6e 67 3d 27 75 74 |.0' enco ding='ut|
         ...
         uart:~$ map mce mas get_folder_listing
         MCE MAS 0x20005a00 get_folder_listing rsp, rsp_code Success, data len 4
         HI 49 Len 1
         00000000: 00                                               |.                |
         uart:~$

   .. group-tab:: MSE

      .. code-block:: console

         MSE MAS 0x20005c00 get_folder_listing req, final true, data len 30
         HI cb Len 4
         Conn ID: 00000001
         HI 42 Len 22
         00000000: 78 2d 6f 62 65 78 2f 66  6f 6c 64 65 72 2d 6c 69 |x-obex/f older-li|
         00000010: 73 74 69 6e 67 00                                |sting.           |
         uart:~$ map mse mas get_folder_listing noerror
         MSE MAS 0x20005c00 get_folder_listing req, final true, data len 0
         uart:~$ map mse mas get_folder_listing noerror
         uart:~$

L2CAP Transport with SRM
~~~~~~~~~~~~~~~~~~~~~~~~

When using L2CAP transport with SRM enabled by default, the GET request does not
need to be sent again.

.. tabs::

   .. group-tab:: MCE

      .. code-block:: console

         uart:~$ map mce mas get_folder_listing
         MCE MAS 0x20005c98 get_folder_listing rsp, rsp_code Continue, data len 252
         HI 97 Len 1
         OBEX SRM: 01
         HI 48 Len 247
         00000000: 3c 3f 78 6d 6c 20 76 65  72 73 69 6f 6e 3d 27 31 |<?xml ve rsion='1|
         00000010: 2e 30 27 20 65 6e 63 6f  64 69 6e 67 3d 27 75 74 |.0' enco ding='ut|
         ...
         MCE MAS 0x20005a00 get_folder_listing rsp, rsp_code Success, data len 6
         HI 49 Len 3
         00000000: 67 3e 00                                         |g>.              |
         ...
         uart:~$

   .. group-tab:: MSE

      .. code-block:: console

         MSE MAS 0x20005c00 get_folder_listing req, final true, data len 32
         HI cb Len 4
         Conn ID: 00000001
         HI 97 Len 1
         OBEX SRM: 01
         HI 42 Len 22
         00000000: 78 2d 6f 62 65 78 2f 66  6f 6c 64 65 72 2d 6c 69 |x-obex/f older-li|
         00000010: 73 74 69 6e 67 00                                |sting.           |
         ...
         uart:~$ map mse mas get_folder_listing noerror
         uart:~$ map mse mas get_folder_listing noerror
         uart:~$

L2CAP Transport with SRM and SRMP
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

When using L2CAP transport with SRM enabled and SRMP enabled, the GET request needs
to continue sending until SRMP is disabled.

.. tabs::

   .. group-tab:: MCE

      .. code-block:: console

         uart:~$ map mce mas get_folder_listing srmp
         MCE MAS 0x20005a00 get_folder_listing rsp, rsp_code Continue, data len 252
         HI 97 Len 1
         OBEX SRM: 01
         HI 48 Len 247
         00000000: 3c 3f 78 6d 6c 20 76 65  72 73 69 6f 6e 3d 27 31 |<?xml ve rsion='1|
         00000010: 2e 30 27 20 65 6e 63 6f  64 69 6e 67 3d 27 75 74 |.0' enco ding='ut|
         ...
         MCE MAS 0x20005a00 get_folder_listing rsp, rsp_code Success, data len 6
         HI 49 Len 3
         00000000: 67 3e 00                                         |g>.              |
         ...
         uart:~$

   .. group-tab:: MSE

      .. code-block:: console

         MSE MAS 0x20005c00 get_folder_listing req, final true, data len 34
         HI cb Len 4
         Conn ID: 00000001
         HI 97 Len 1
         OBEX SRM: 01
         HI 98 Len 1
         OBEX SRMP: 01
         HI 42 Len 22
         00000000: 78 2d 6f 62 65 78 2f 66  6f 6c 64 65 72 2d 6c 69 |x-obex/f older-li|
         00000010: 73 74 69 6e 67 00                                |sting.           |
         uart:~$ map mse mas get_folder_listing noerror
         MSE MAS 0x20005c00 get_folder_listing req, final true, data len 0
         uart:~$ map mse mas get_folder_listing noerror
         uart:~$

Get Message Listing
-------------------

RFCOMM Transport
~~~~~~~~~~~~~~~~

.. tabs::

   .. group-tab:: MCE

      .. code-block:: console

         uart:~$ map mce mas get_msg_listing
         MCE MAS 0x20005a00 get_msg_listing rsp, rsp_code Continue, data len 252
         HI 4c Len 29
         T 0d L 1
         00000000: 00                                               |.                |
         T 19 L 20
         00000000: 32 30 32 35 30 31 30 31  54 31 32 30 30 30 30 2b |20250101 T120000+|
         00000010: 30 30 30 30                                      |0000             |
         T 12 L 2
         00000000: 00 02                                            |..               |
         HI 48 Len 217
         00000000: 3c 3f 78 6d 6c 20 76 65  72 73 69 6f 6e 3d 27 31 |<?xml ve rsion='1|
         00000010: 2e 30 27 20 65 6e 63 6f  64 69 6e 67 3d 27 75 74 |.0' enco ding='ut|
         ...
         uart:~$ map mce mas get_msg_listing
         MCE MAS 0x20005a00 get_msg_listing rsp, rsp_code Success, data len 52
         HI 49 Len 49
         00000000: 22 20 73 65 6e 74 3d 22  6e 6f 22 20 70 72 6f 74 |" sent=" no" prot|
         ...
         uart:~$

   .. group-tab:: MSE

      .. code-block:: console

         MSE MAS 0x20005c00 get_msg_listing req, final true, data len 32
         HI cb Len 4
         Conn ID: 00000001
         HI 42 Len 21
         00000000: 78 2d 62 74 2f 4d 41 50  2d 6d 73 67 2d 6c 69 73 |x-bt/MAP -msg-lis|
         00000010: 74 69 6e 67 00                                   |ting.            |
         HI 01 Len 0
         uart:~$ map mse mas get_msg_listing noerror
         Adding default application parameters:
           New message: 0
           MSE time: 20250101T120000+0000
           Listing size: 2
         MSE MAS 0x20005c00 get_msg_listing req, final true, data len 0
         uart:~$ map mse mas get_msg_listing noerror
         uart:~$

L2CAP Transport with SRM
~~~~~~~~~~~~~~~~~~~~~~~~

.. tabs::

   .. group-tab:: MCE

      .. code-block:: console

         uart:~$ map mce mas get_msg_listing
         MCE MAS 0x20005a00 get_msg_listing rsp, rsp_code Continue, data len 252
         HI 97 Len 1
         OBEX SRM: 01
         HI 4c Len 29
         T 0d L 1
         00000000: 00                                               |.                |
         T 19 L 20
         00000000: 32 30 32 35 30 31 30 31  54 31 32 30 30 30 30 2b |20250101 T120000+|
         00000010: 30 30 30 30                                      |0000             |
         T 12 L 2
         00000000: 00 02                                            |..               |
         HI 48 Len 215
         00000000: 3c 3f 78 6d 6c 20 76 65  72 73 69 6f 6e 3d 27 31 |<?xml ve rsion='1|
         ...
         MCE MAS 0x20005a00 get_msg_listing rsp, rsp_code Continue, data len 252
         HI 48 Len 249
         00000000: 2e 63 6f 6d 22 20 72 65  63 69 70 69 65 6e 74 5f |.com" re cipient_|
         ...
         MCE MAS 0x20005c98 get_msg_listing rsp, rsp_code Success, data len 54
         HI 49 Len 51
         00000000: 65 73 22 20 73 65 6e 74  3d 22 6e 6f 22 20 70 72 |es" sent ="no" pr|
         00000010: 6f 74 65 63 74 65 64 3d  22 6e 6f 22 2f 3e 0d 0a |otected= "no"/>..|
         ...
         uart:~$

   .. group-tab:: MSE

      .. code-block:: console

         MSE MAS 0x20005c00 get_msg_listing req, final true, data len 34
         HI cb Len 4
         Conn ID: 00000001
         HI 97 Len 1
         OBEX SRM: 01
         HI 42 Len 21
         00000000: 78 2d 62 74 2f 4d 41 50  2d 6d 73 67 2d 6c 69 73 |x-bt/MAP -msg-lis|
         00000010: 74 69 6e 67 00                                   |ting.            |
         uart:~$ map mse mas get_msg_listing noerror
         Adding default application parameters:
           New message: 0
           MSE time: 20250101T120000+0000
           Listing size: 2
         uart:~$ map mse mas get_msg_listing noerror
         uart:~$ map mse mas get_msg_listing noerror
         uart:~$

L2CAP Transport with SRM and SRMP
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. tabs::

   .. group-tab:: MCE

      .. code-block:: console

         uart:~$ map mce mas get_msg_listing srmp
         MCE MAS 0x20005a00 get_msg_listing rsp, rsp_code Continue, data len 252
         HI 97 Len 1
         OBEX SRM: 01
         HI 4c Len 29
         T 0d L 1
         00000000: 00                                               |.                |
         T 19 L 20
         00000000: 32 30 32 35 30 31 30 31  54 31 32 30 30 30 30 2b |20250101 T120000+|
         00000010: 30 30 30 30                                      |0000             |
         T 12 L 2
         00000000: 00 02                                            |..               |
         HI 48 Len 215
         00000000: 3c 3f 78 6d 6c 20 76 65  72 73 69 6f 6e 3d 27 31 |<?xml ve rsion='1|
         ...
         uart:~$ map mce mas get_msg_listing
         MCE MAS 0x20005a00 get_msg_listing rsp, rsp_code Continue, data len 252
         HI 48 Len 249
         00000000: 2e 63 6f 6d 22 20 72 65  63 69 70 69 65 6e 74 5f |.com" re cipient_|
         00000010: 6e 61 6d 65 3d 22 42 6f  62 22 20 72 65 63 69 70 |name="Bo b" recip|
         ...
         MCE MAS 0x20005a00 get_msg_listing rsp, rsp_code Success, data len 54
         HI 49 Len 51
         00000000: 65 73 22 20 73 65 6e 74  3d 22 6e 6f 22 20 70 72 |es" sent ="no" pr|
         00000010: 6f 74 65 63 74 65 64 3d  22 6e 6f 22 2f 3e 0d 0a |otected= "no"/>..|
         ...
         uart:~$

   .. group-tab:: MSE

      .. code-block:: console

         MSE MAS 0x20005c00 get_msg_listing req, final true, data len 36
         HI cb Len 4
         Conn ID: 00000001
         HI 97 Len 1
         OBEX SRM: 01
         HI 98 Len 1
         OBEX SRMP: 01
         HI 42 Len 21
         00000000: 78 2d 62 74 2f 4d 41 50  2d 6d 73 67 2d 6c 69 73 |x-bt/MAP -msg-lis|
         00000010: 74 69 6e 67 00                                   |ting.            |
         HI 01 Len 0
         uart:~$ map mse mas get_msg_listing noerror
         Adding default application parameters:
           New message: 0
           MSE time: 20250101T120000+0000
           Listing size: 2
         MSE MAS 0x20005c00 get_msg_listing req, final true, data len 0
         uart:~$ map mse mas get_msg_listing noerror
         uart:~$ map mse mas get_msg_listing noerror
         uart:~$

Get Message
-----------

RFCOMM Transport
~~~~~~~~~~~~~~~~

.. tabs::

   .. group-tab:: MCE

      .. code-block:: console

         uart:~$ map mce mas get_msg 0000000000000001
         Adding default application parameters:
           Attachment: OFF (0x00)
           Charset: UTF-8 (0x01)
         MCE MAS 0x20005a00 get_msg rsp, rsp_code Continue, data len 252
         HI 48 Len 249
         00000000: 42 45 47 49 4e 3a 42 4d  53 47 0d 0a 56 45 52 53 |BEGIN:BM SG..VERS|
         00000010: 49 4f 4e 3a 31 2e 30 0d  0a 53 54 41 54 55 53 3a |ION:1.0. .STATUS:|
         ...
         uart:~$ map mce mas get_msg 0000000000000001
         MCE MAS 0x20005a00 get_msg rsp, rsp_code Success, data len 87
         HI 49 Len 84
         00000000: 3c 3e 3f 2f 0a 2d 20 4e  75 6d 62 65 72 73 3a 20 |<>?/.- N umbers: |
         00000010: 30 31 32 33 34 35 36 37  38 39 0a 0a 45 6e 64 20 |01234567 89..End |
         ...
         uart:~$

   .. group-tab:: MSE

      .. code-block:: console

         MSE MAS 0x20005c00 get_msg req, final true, data len 67
         HI cb Len 4
         Conn ID: 00000001
         HI 42 Len 13
         00000000: 78 2d 62 74 2f 6d 65 73  73 61 67 65 00          |x-bt/mes sage.   |
         HI 01 Len 34
         00000000: 00 30 00 30 00 30 00 30  00 30 00 30 00 30 00 30 |.0.0.0.0 .0.0.0.0|
         00000010: 00 30 00 30 00 30 00 30  00 30 00 30 00 30 00 31 |.0.0.0.0 .0.0.0.1|
         00000020: 00 00                                            |..               |
         HI 4c Len 6
         T 0a L 1
         00000000: 00                                               |.                |
         T 14 L 1
         00000000: 01                                               |.                |
         uart:~$ map mse mas get_msg noerror
         MSE MAS 0x20005c00 get_msg req, final true, data len 0
         uart:~$ map mse mas get_msg noerror
         uart:~$

L2CAP Transport with SRM
~~~~~~~~~~~~~~~~~~~~~~~~

.. tabs::

   .. group-tab:: MCE

      .. code-block:: console

         uart:~$ map mce mas get_msg 0000000000000001
         Adding default application parameters:
           Attachment: OFF (0x00)
           Charset: UTF-8 (0x01)
         MCE MAS 0x20005a00 get_msg rsp, rsp_code Continue, data len 252
         HI 97 Len 1
         OBEX SRM: 01
         HI 48 Len 247
         00000000: 42 45 47 49 4e 3a 42 4d  53 47 0d 0a 56 45 52 53 |BEGIN:BM SG..VERS|
         00000010: 49 4f 4e 3a 31 2e 30 0d  0a 53 54 41 54 55 53 3a |ION:1.0. .STATUS:|
         ...
         MCE MAS 0x20005a00 get_msg rsp, rsp_code Success, data len 89
         HI 49 Len 86
         00000000: 2c 2e 3c 3e 3f 2f 0a 2d  20 4e 75 6d 62 65 72 73 |,.<>?/.-  Numbers|
         uart:~$

   .. group-tab:: MSE

      .. code-block:: console

         MSE MAS 0x20005c00 get_msg req, final true, data len 69
         HI cb Len 4
         Conn ID: 00000001
         HI 97 Len 1
         OBEX SRM: 01
         HI 42 Len 13
         00000000: 78 2d 62 74 2f 6d 65 73  73 61 67 65 00          |x-bt/mes sage.   |
         HI 01 Len 34
         00000000: 00 30 00 30 00 30 00 30  00 30 00 30 00 30 00 30 |.0.0.0.0 .0.0.0.0|
         00000010: 00 30 00 30 00 30 00 30  00 30 00 30 00 30 00 31 |.0.0.0.0 .0.0.0.1|
         00000020: 00 00                                            |..               |
         HI 4c Len 6
         T 0a L 1
         00000000: 00                                               |.                |
         T 14 L 1
         00000000: 01                                               |.                |
         uart:~$ map mse mas get_msg noerror
         uart:~$ map mse mas get_msg noerror
         uart:~$

L2CAP Transport with SRM and SRMP
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. tabs::

   .. group-tab:: MCE

      .. code-block:: console

         uart:~$ map mce mas get_msg 0000000000000001 srmp
         Adding default application parameters:
           Attachment: OFF (0x00)
           Charset: UTF-8 (0x01)
         MCE MAS 0x20005a00 get_msg rsp, rsp_code Continue, data len 252
         HI 97 Len 1
         OBEX SRM: 01
         HI 48 Len 247
         00000000: 42 45 47 49 4e 3a 42 4d  53 47 0d 0a 56 45 52 53 |BEGIN:BM SG..VERS|
         ...
         uart:~$ map mce mas get_msg 0000000000000001
         MCE MAS 0x20005a00 get_msg rsp, rsp_code Continue, data len 252
         HI 48 Len 249
         00000000: 37 38 39 0d 0a 45 4e 44  3a 56 43 41 52 44 0d 0a |789..END :VCARD..|
         00000010: 42 45 47 49 4e 3a 42 42  4f 44 59 0d 0a 43 48 41 |BEGIN:BB ODY..CHA|
         ...
         MCE MAS 0x20005a00 get_msg rsp, rsp_code Success, data len 89
         HI 49 Len 86
         00000000: 2c 2e 3c 3e 3f 2f 0a 2d  20 4e 75 6d 62 65 72 73 |,.<>?/.-  Numbers|
         ...
         uart:~$

   .. group-tab:: MSE

      .. code-block:: console

         MSE MAS 0x20005c00 get_msg req, final true, data len 71
         HI cb Len 4
         Conn ID: 00000001
         HI 97 Len 1
         OBEX SRM: 01
         HI 98 Len 1
         OBEX SRMP: 01
         HI 42 Len 13
         00000000: 78 2d 62 74 2f 6d 65 73  73 61 67 65 00          |x-bt/mes sage.   |
         HI 01 Len 34
         00000000: 00 30 00 30 00 30 00 30  00 30 00 30 00 30 00 30 |.0.0.0.0 .0.0.0.0|
         00000010: 00 30 00 30 00 30 00 30  00 30 00 30 00 30 00 31 |.0.0.0.0 .0.0.0.1|
         00000020: 00 00                                            |..               |
         HI 4c Len 6
         T 0a L 1
         00000000: 00                                               |.                |
         T 14 L 1
         00000000: 01                                               |.                |
         uart:~$ map mse mas get_msg noerror
         MSE MAS 0x20005c00 get_msg req, final true, data len 0
         uart:~$ map mse mas get_msg noerror
         uart:~$ map mse mas get_msg noerror
         uart:~$

Set Message Status
------------------

.. tabs::

   .. group-tab:: MCE

      .. code-block:: console

         uart:~$ map mce mas set_msg_status 0000000000000001
         Adding default application parameters:
           Status indicator: 0x00
           Status value: 0x00
         MCE MAS 0x20005a00 set_msg_status rsp, rsp_code Success
         uart:~$

   .. group-tab:: MSE

      .. code-block:: console

         MSE MAS 0x20005c00 set_msg_status req, final true, data len 77
         HI cb Len 4
         Conn ID: 00000001
         HI 01 Len 34
         00000000: 00 30 00 30 00 30 00 30  00 30 00 30 00 30 00 30 |.0.0.0.0 .0.0.0.0|
         00000010: 00 30 00 30 00 30 00 30  00 30 00 30 00 30 00 31 |.0.0.0.0 .0.0.0.1|
         00000020: 00 00                                            |..               |
         HI 42 Len 19
         00000000: 78 2d 62 74 2f 6d 65 73  73 61 67 65 53 74 61 74 |x-bt/mes sageStat|
         00000010: 75 73 00                                         |us.              |
         HI 4c Len 6
         T 17 L 1
         00000000: 00                                               |.                |
         T 18 L 1
         00000000: 00                                               |.                |
         HI 49 Len 1
         00000000: 30                                               |0                |
         uart:~$ map mse mas set_msg_status noerror
         uart:~$

Push Message
------------

RFCOMM Transport
~~~~~~~~~~~~~~~~

.. tabs::

   .. group-tab:: MCE

      .. code-block:: console

         uart:~$ map mce mas push_msg
         Adding default application parameters:
           Charset: UTF-8 (0x01)
         MCE MAS 0x20005a00 push_msg rsp, rsp_code Continue, data len 0
         ...
         uart:~$ map mce mas push_msg
         MCE MAS 0x20005a00 push_msg rsp, rsp_code Success, data len 37
         HI 01 Len 34
         00000000: 00 30 00 30 00 30 00 30  00 30 00 30 00 30 00 30 |.0.0.0.0 .0.0.0.0|
         00000010: 00 30 00 30 00 30 00 30  00 30 00 30 00 30 00 31 |.0.0.0.0 .0.0.0.1|
         00000020: 00 00                                            |..               |
         ...
         uart:~$

   .. group-tab:: MSE

      .. code-block:: console

         MSE MAS 0x20005c00 push_msg req, final false, data len 252
         HI cb Len 4
         Conn ID: 00000001
         HI 42 Len 13
         00000000: 78 2d 62 74 2f 6d 65 73  73 61 67 65 00          |x-bt/mes sage.   |
         HI 01 Len 0
         HI 4c Len 3
         T 14 L 1
         00000000: 01                                               |.                |
         HI 48 Len 219
         00000000: 42 45 47 49 4e 3a 42 4d  53 47 0d 0a 56 45 52 53 |BEGIN:BM SG..VERS|
         00000010: 49 4f 4e 3a 31 2e 30 0d  0a 53 54 41 54 55 53 3a |ION:1.0. .STATUS:|
         ...
         uart:~$ map mse mas push_msg noerror
         MSE MAS 0x20005c00 push_msg req, final true, data len 117
         HI 49 Len 114
         00000000: 61 63 74 65 72 73 3a 20  40 23 24 25 26 2a 28 29 |acters:  @#$%&*()|
         ...
         uart:~$ map mse mas push_msg noerror
         uart:~$

L2CAP Transport with SRM
~~~~~~~~~~~~~~~~~~~~~~~~

.. tabs::

   .. group-tab:: MCE

      .. code-block:: console

         uart:~$ map mce mas push_msg
         Adding default application parameters:
           Charset: UTF-8 (0x01)
         MCE MAS 0x20005a00 push_msg rsp, rsp_code Continue, data len 2
         HI 97 Len 1
         OBEX SRM: 01
         ...
         uart:~$ map mce mas push_msg
         uart:~$ map mce mas push_msg
         MCE MAS 0x20005a00 push_msg rsp, rsp_code Success, data len 37
         HI 01 Len 34
         00000000: 00 30 00 30 00 30 00 30  00 30 00 30 00 30 00 30 |.0.0.0.0 .0.0.0.0|
         00000010: 00 30 00 30 00 30 00 30  00 30 00 30 00 30 00 31 |.0.0.0.0 .0.0.0.1|
         00000020: 00 00                                            |..               |
         ...
         uart:~$

   .. group-tab:: MSE

      .. code-block:: console

         MSE MAS 0x20005c00 push_msg req, final false, data len 252
         HI cb Len 4
         Conn ID: 00000001
         HI 97 Len 1
         OBEX SRM: 01
         HI 42 Len 13
         00000000: 78 2d 62 74 2f 6d 65 73  73 61 67 65 00          |x-bt/mes sage.   |
         HI 01 Len 0
         HI 4c Len 3
         T 14 L 1
         00000000: 01                                               |.                |
         HI 48 Len 217
         00000000: 42 45 47 49 4e 3a 42 4d  53 47 0d 0a 56 45 52 53 |BEGIN:BM SG..VERS|
         00000010: 49 4f 4e 3a 31 2e 30 0d  0a 53 54 41 54 55 53 3a |ION:1.0. .STATUS:|
         ...
         uart:~$ map mse mas push_msg noerror
         MSE MAS 0x20005c00 push_msg req, final false, data len 252
         HI 48 Len 249
         00000000: 54 46 2d 38 3a 2b 30 31  32 33 34 35 36 37 38 39 |TF-8:+01 23456789|
         ...
         MSE MAS 0x20005c00 push_msg req, final true, data len 119
         HI 49 Len 116
         00000000: 61 72 61 63 74 65 72 73  3a 20 40 23 24 25 26 2a |aracters : @#$%&*|
         ...
         uart:~$ map mse mas push_msg noerror
         uart:~$

L2CAP Transport with SRM and SRMP
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. tabs::

   .. group-tab:: MCE

      .. code-block:: console

         uart:~$ map mce mas push_msg
         Adding default application parameters:
           Charset: UTF-8 (0x01)
         MCE MAS 0x20005a00 push_msg rsp, rsp_code Continue, data len 4
         HI 97 Len 1
         OBEX SRM: 01
         HI 98 Len 1
         OBEX SRMP: 01
         ...
         uart:~$ map mce mas push_msg
         MCE MAS 0x20005c98 push_msg rsp, rsp_code Continue, data len 0
         uart:~$ map mce mas push_msg
         MCE MAS 0x20005a00 push_msg rsp, rsp_code Success, data len 37
         HI 01 Len 34
         00000000: 00 30 00 30 00 30 00 30  00 30 00 30 00 30 00 30 |.0.0.0.0 .0.0.0.0|
         00000010: 00 30 00 30 00 30 00 30  00 30 00 30 00 30 00 31 |.0.0.0.0 .0.0.0.1|
         00000020: 00 00                                            |..               |
         ...
         uart:~$

   .. group-tab:: MSE

      .. code-block:: console

         MSE MAS 0x20005c00 push_msg req, final false, data len 252
         HI cb Len 4
         Conn ID: 00000001
         HI 97 Len 1
         OBEX SRM: 01
         HI 42 Len 13
         00000000: 78 2d 62 74 2f 6d 65 73  73 61 67 65 00          |x-bt/mes sage.   |
         HI 01 Len 0
         HI 4c Len 3
         T 14 L 1
         00000000: 01                                               |.                |
         HI 48 Len 217
         00000000: 42 45 47 49 4e 3a 42 4d  53 47 0d 0a 56 45 52 53 |BEGIN:BM SG..VERS|
         00000010: 49 4f 4e 3a 31 2e 30 0d  0a 53 54 41 54 55 53 3a |ION:1.0. .STATUS:|
         ...
         uart:~$ map mse mas push_msg noerror srmp
         MSE MAS 0x20005c00 push_msg req, final false, data len 252
         HI 48 Len 249
         00000000: 54 46 2d 38 3a 2b 30 31  32 33 34 35 36 37 38 39 |TF-8:+01 23456789|
         ...
         uart:~$ map mse mas push_msg noerror
         MSE MAS 0x20005c00 push_msg req, final true, data len 119
         HI 49 Len 116
         00000000: 61 72 61 63 74 65 72 73  3a 20 40 23 24 25 26 2a |aracters : @#$%&*|
         ...
         uart:~$ map mse mas push_msg noerror
         uart:~$

Update Inbox
------------

.. tabs::

   .. group-tab:: MCE

      .. code-block:: console

         uart:~$ map mce mas update_inbox
         MCE MAS 0x20005a00 update_inbox rsp, rsp_code Success
         uart:~$

   .. group-tab:: MSE

      .. code-block:: console

         MSE MAS 0x20005c00 update_inbox req, final true, data len 35
         HI cb Len 4
         Conn ID: 00000001
         HI 42 Len 23
         00000000: 78 2d 62 74 2f 4d 41 50  2d 6d 65 73 73 61 67 65 |x-bt/MAP -message|
         00000010: 55 70 64 61 74 65 00                             |Update.          |
         HI 49 Len 1
         00000000: 30                                               |0                |
         uart:~$ map mse mas update_inbox noerror
         uart:~$

Get MAS Instance Information
----------------------------

RFCOMM Transport
~~~~~~~~~~~~~~~~

.. tabs::

   .. group-tab:: MCE

      .. code-block:: console

         uart:~$ map mce mas get_mas_inst_info
         Adding default application parameters:
           Instance ID: 0
         MCE MAS 0x20005a00 get_mas_inst_info rsp, rsp_code Success, data len 21
         HI 49 Len 18
         00000000: 4d 41 53 20 49 6e 73 74  61 6e 63 65 20 49 6e 66 |MAS Inst ance Inf|
         00000010: 6f 00                                            |o.               |
         uart:~$

   .. group-tab:: MSE

      .. code-block:: console

         MSE MAS 0x20005c00 get_mas_inst_info req, final true, data len 42
         HI cb Len 4
         Conn ID: 00000001
         HI 42 Len 28
         00000000: 78 2d 62 74 2f 4d 41 53  49 6e 73 74 61 6e 63 65 |x-bt/MAS Instance|
         00000010: 49 6e 66 6f 72 6d 61 74  69 6f 6e 00             |Informat ion.    |
         HI 4c Len 3
         T 0f L 1
         00000000: 00                                               |.                |
         uart:~$ map mse mas get_mas_inst_info noerror
         uart:~$

L2CAP Transport with SRM
~~~~~~~~~~~~~~~~~~~~~~~~

.. tabs::

   .. group-tab:: MCE

      .. code-block:: console

         uart:~$ map mce mas get_mas_inst_info
         Adding default application parameters:
           Instance ID: 0
         MCE MAS 0x20005a00 get_mas_inst_info rsp, rsp_code Success, data len 23
         HI 97 Len 1
         OBEX SRM: 01
         HI 49 Len 18
         00000000: 4d 41 53 20 49 6e 73 74  61 6e 63 65 20 49 6e 66 |MAS Inst ance Inf|
         00000010: 6f 00                                            |o.               |
         uart:~$

   .. group-tab:: MSE

      .. code-block:: console

         MSE MAS 0x20005c00 get_mas_inst_info req, final true, data len 44
         HI cb Len 4
         Conn ID: 00000001
         HI 97 Len 1
         OBEX SRM: 01
         HI 42 Len 28
         00000000: 78 2d 62 74 2f 4d 41 53  49 6e 73 74 61 6e 63 65 |x-bt/MAS Instance|
         00000010: 49 6e 66 6f 72 6d 61 74  69 6f 6e 00             |Informat ion.    |
         HI 4c Len 3
         T 0f L 1
         00000000: 00                                               |.                |
         uart:~$ map mse mas get_mas_inst_info noerror
         uart:~$

L2CAP Transport with SRM and SRMP
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. tabs::

   .. group-tab:: MCE

      .. code-block:: console

         uart:~$ map mce mas get_mas_inst_info srmp
         Adding default application parameters:
           Instance ID: 0
         MCE MAS 0x20005a00 get_mas_inst_info rsp, rsp_code Success, data len 23
         HI 97 Len 1
         OBEX SRM: 01
         HI 49 Len 18
         00000000: 4d 41 53 20 49 6e 73 74  61 6e 63 65 20 49 6e 66 |MAS Inst ance Inf|
         00000010: 6f 00                                            |o.               |
         uart:~$

   .. group-tab:: MSE

      .. code-block:: console

         MSE MAS 0x20005c00 get_mas_inst_info req, final true, data len 46
         HI cb Len 4
         Conn ID: 00000001
         HI 97 Len 1
         OBEX SRM: 01
         HI 98 Len 1
         OBEX SRMP: 01
         HI 42 Len 28
         00000000: 78 2d 62 74 2f 4d 41 53  49 6e 73 74 61 6e 63 65 |x-bt/MAS Instance|
         00000010: 49 6e 66 6f 72 6d 61 74  69 6f 6e 00             |Informat ion.    |
         HI 4c Len 3
         T 0f L 1
         00000000: 00                                               |.                |
         uart:~$ map mse mas get_mas_inst_info noerror
         uart:~$

Set Owner Status
----------------

.. tabs::

   .. group-tab:: MCE

      .. code-block:: console

         uart:~$ map mce mas set_owner_status
         Adding default application parameters:
           Chat state: 0x00
         MCE MAS 0x20005a00 set_owner_status rsp, rsp_code Success
         uart:~$

   .. group-tab:: MSE

      .. code-block:: console

         MSE MAS 0x20005c00 set_owner_status req, final true, data len 35
         HI cb Len 4
         Conn ID: 00000001
         HI 42 Len 17
         00000000: 78 2d 62 74 2f 6f 77 6e  65 72 53 74 61 74 75 73 |x-bt/own erStatus|
         00000010: 00                                               |.                |
         HI 4c Len 3
         T 21 L 1
         00000000: 00                                               |.                |
         HI 49 Len 1
         00000000: 30                                               |0                |
         uart:~$ map mse mas set_owner_status noerror
         uart:~$

Get Owner Status
----------------

RFCOMM Transport
~~~~~~~~~~~~~~~~

.. tabs::

   .. group-tab:: MCE

      .. code-block:: console

         uart:~$ map mce mas get_owner_status
         MCE MAS 0x20005a00 get_owner_status rsp, rsp_code Success, data len 18
         HI 4c Len 12
         T 1c L 1
         00000000: 00                                               |.                |
         T 1d L 7
         00000000: 55 6e 6b 6e 6f 77 6e                             |Unknown          |
         uart:~$

   .. group-tab:: MSE

      .. code-block:: console

         MSE MAS 0x20005c00 get_owner_status req, final true, data len 25
         HI cb Len 4
         Conn ID: 00000001
         HI 42 Len 17
         00000000: 78 2d 62 74 2f 6f 77 6e  65 72 53 74 61 74 75 73 |x-bt/own erStatus|
         00000010: 00                                               |.                |
         uart:~$ map mse mas get_owner_status noerror
         Adding default application parameters:
           Presence:  0x00
           Presence text: Unknown
         uart:~$

L2CAP Transport with SRM
~~~~~~~~~~~~~~~~~~~~~~~~

.. tabs::

   .. group-tab:: MCE

      .. code-block:: console

         uart:~$ map mce mas get_owner_status
         MCE MAS 0x20005a00 get_owner_status rsp, rsp_code Success, data len 20
         HI 97 Len 1
         OBEX SRM: 01
         HI 4c Len 12
         T 1c L 1
         00000000: 00                                               |.                |
         T 1d L 7
         00000000: 55 6e 6b 6e 6f 77 6e                             |Unknown          |
         HI 49 Len 0
         uart:~$

   .. group-tab:: MSE

      .. code-block:: console

         MSE MAS 0x20005c00 get_owner_status req, final true, data len 27
         HI cb Len 4
         Conn ID: 00000001
         HI 97 Len 1
         OBEX SRM: 01
         HI 42 Len 17
         00000000: 78 2d 62 74 2f 6f 77 6e  65 72 53 74 61 74 75 73 |x-bt/own erStatus|
         00000010: 00                                               |.                |
         uart:~$ map mse mas get_owner_status noerror
         Adding default application parameters:
           Presence:  0x00
           Presence text: Unknown
         uart:~$

L2CAP Transport with SRM and SRMP
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. tabs::

   .. group-tab:: MCE

      .. code-block:: console

         uart:~$ map mce mas get_owner_status srmp
         MCE MAS 0x20005a00 get_owner_status rsp, rsp_code Success, data len 20
         HI 97 Len 1
         OBEX SRM: 01
         HI 4c Len 12
         T 1c L 1
         00000000: 00                                               |.                |
         T 1d L 7
         00000000: 55 6e 6b 6e 6f 77 6e                             |Unknown          |
         HI 49 Len 0
         uart:~$

   .. group-tab:: MSE

      .. code-block:: console

         MSE MAS 0x20005c00 get_owner_status req, final true, data len 29
         HI cb Len 4
         Conn ID: 00000001
         HI 97 Len 1
         OBEX SRM: 01
         HI 98 Len 1
         OBEX SRMP: 01
         HI 42 Len 17
         00000000: 78 2d 62 74 2f 6f 77 6e  65 72 53 74 61 74 75 73 |x-bt/own erStatus|
         00000010: 00                                               |.                |
         uart:~$ map mse mas get_owner_status noerror
         Adding default application parameters:
           Presence:  0x00
           Presence text: Unknown
         uart:~$

Get Conversation Listing
------------------------

RFCOMM Transport
~~~~~~~~~~~~~~~~

.. tabs::

   .. group-tab:: MCE

      .. code-block:: console

         uart:~$ map mce mas get_convo_listing
         MCE MAS 0x20005a00 get_convo_listing rsp, rsp_code Continue, data len 252
         HI 4c Len 72
         T 12 L 2
         00000000: 00 02                                            |..               |
         T 1a L 32
         00000000: 30 30 30 30 30 30 30 30  30 30 30 30 30 30 30 31 |00000000 00000001|
         00000010: 30 30 30 30 30 30 30 30  30 30 30 30 30 30 30 31 |00000000 00000001|
         T 1b L 32
         00000000: 30 30 30 30 30 30 30 30  30 30 30 30 30 30 30 31 |00000000 00000001|
         00000010: 30 30 30 30 30 30 30 30  30 30 30 30 30 30 30 31 |00000000 00000001|
         HI 48 Len 174
         00000000: 3c 3f 78 6d 6c 20 76 65  72 73 69 6f 6e 3d 27 31 |<?xml ve rsion='1|
         00000010: 2e 30 27 20 65 6e 63 6f  64 69 6e 67 3d 27 75 74 |.0' enco ding='ut|
         ...
         uart:~$ map mce mas get_convo_listing
         MCE MAS 0x20005a00 get_convo_listing rsp, rsp_code Success, data len 117
         HI 49 Len 114
         00000000: 22 20 6e 61 6d 65 3d 22  42 6f 62 22 20 70 72 65 |" name=" Bob" pre|
         ...
         uart:~$

   .. group-tab:: MSE

      .. code-block:: console

         MSE MAS 0x20005c00 get_convo_listing req, final true, data len 31
         HI cb Len 4
         Conn ID: 00000001
         HI 42 Len 23
         00000000: 78 2d 62 74 2f 4d 41 50  2d 63 6f 6e 76 6f 2d 6c |x-bt/MAP -convo-l|
         00000010: 69 73 74 69 6e 67 00                             |isting.          |
         uart:~$ map mse mas get_convo_listing noerror
         Adding default application parameters:
           Listing size: 2
           Database ID: 00000000000000010000000000000001
           Conversation version counter: 00000000000000010000000000000001
         MSE MAS 0x20005c00 get_convo_listing req, final true, data len 0
         uart:~$ map mse mas get_convo_listing noerror
         uart:~$

L2CAP Transport with SRM
~~~~~~~~~~~~~~~~~~~~~~~~

.. tabs::

   .. group-tab:: MCE

      .. code-block:: console

         uart:~$ map mce mas get_convo_listing
         MCE MAS 0x20005a00 get_convo_listing rsp, rsp_code Continue, data len 252
         HI 97 Len 1
         OBEX SRM: 01
         HI 4c Len 72
         T 12 L 2
         00000000: 00 02                                            |..               |
         T 1a L 32
         00000000: 30 30 30 30 30 30 30 30  30 30 30 30 30 30 30 31 |00000000 00000001|
         00000010: 30 30 30 30 30 30 30 30  30 30 30 30 30 30 30 31 |00000000 00000001|
         T 1b L 32
         00000000: 30 30 30 30 30 30 30 30  30 30 30 30 30 30 30 31 |00000000 00000001|
         00000010: 30 30 30 30 30 30 30 30  30 30 30 30 30 30 30 31 |00000000 00000001|
         HI 48 Len 172
         00000000: 3c 3f 78 6d 6c 20 76 65  72 73 69 6f 6e 3d 27 31 |<?xml ve rsion='1|
         00000010: 2e 30 27 20 65 6e 63 6f  64 69 6e 67 3d 27 75 74 |.0' enco ding='ut|
         ...
         MCE MAS 0x20005a00 get_convo_listing rsp, rsp_code Success, data len 119
         HI 49 Len 116
         00000000: 32 31 22 20 6e 61 6d 65  3d 22 42 6f 62 22 20 70 |21" name ="Bob" p|
         ...
         uart:~$

   .. group-tab:: MSE

      .. code-block:: console

         MSE MAS 0x20005c00 get_convo_listing req, final true, data len 33
         HI cb Len 4
         Conn ID: 00000001
         HI 97 Len 1
         OBEX SRM: 01
         HI 42 Len 23
         00000000: 78 2d 62 74 2f 4d 41 50  2d 63 6f 6e 76 6f 2d 6c |x-bt/MAP -convo-l|
         00000010: 69 73 74 69 6e 67 00                             |isting.          |
         uart:~$ map mse mas get_convo_listing noerror
         Adding default application parameters:
           Listing size: 2
           Database ID: 00000000000000010000000000000001
           Conversation version counter: 00000000000000010000000000000001
         uart:~$ map mse mas get_convo_listing noerror
         uart:~$

L2CAP Transport with SRM and SRMP
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. tabs::

   .. group-tab:: MCE

      .. code-block:: console

         uart:~$ map mce mas get_convo_listing srmp
         MCE MAS 0x20005a00 get_convo_listing rsp, rsp_code Continue, data len 252
         HI 97 Len 1
         OBEX SRM: 01
         HI 4c Len 72
         T 12 L 2
         00000000: 00 02                                            |..               |
         T 1a L 32
         00000000: 30 30 30 30 30 30 30 30  30 30 30 30 30 30 30 31 |00000000 00000001|
         00000010: 30 30 30 30 30 30 30 30  30 30 30 30 30 30 30 31 |00000000 00000001|
         T 1b L 32
         00000000: 30 30 30 30 30 30 30 30  30 30 30 30 30 30 30 31 |00000000 00000001|
         00000010: 30 30 30 30 30 30 30 30  30 30 30 30 30 30 30 31 |00000000 00000001|
         HI 48 Len 172
         00000000: 3c 3f 78 6d 6c 20 76 65  72 73 69 6f 6e 3d 27 31 |<?xml ve rsion='1|
         00000010: 2e 30 27 20 65 6e 63 6f  64 69 6e 67 3d 27 75 74 |.0' enco ding='ut|
         ...
         uart:~$ map mce mas get_convo_listing
         MCE MAS 0x20005a00 get_convo_listing rsp, rsp_code Continue, data len 252
         HI 48 Len 249
         00000000: 79 65 73 22 20 76 65 72  73 69 6f 6e 5f 63 6f 75 |yes" ver sion_cou|
         ...
         MCE MAS 0x20005a00 get_convo_listing rsp, rsp_code Success, data len 119
         HI 49 Len 116
         00000000: 32 31 22 20 6e 61 6d 65  3d 22 42 6f 62 22 20 70 |21" name ="Bob" p|
         ...
         uart:~$

   .. group-tab:: MSE

      .. code-block:: console

         MSE MAS 0x20005c00 get_convo_listing req, final true, data len 35
         HI cb Len 4
         Conn ID: 00000001
         HI 97 Len 1
         OBEX SRM: 01
         HI 98 Len 1
         OBEX SRMP: 01
         HI 42 Len 23
         00000000: 78 2d 62 74 2f 4d 41 50  2d 63 6f 6e 76 6f 2d 6c |x-bt/MAP -convo-l|
         00000010: 69 73 74 69 6e 67 00                             |isting.          |
         uart:~$ map mse mas get_convo_listing noerror
         Adding default application parameters:
           Listing size: 2
           Database ID: 00000000000000010000000000000001
           Conversation version counter: 00000000000000010000000000000001
         MSE MAS 0x20005c00 get_convo_listing req, final true, data len 0
         uart:~$ map mse mas get_convo_listing noerror
         uart:~$ map mse mas get_convo_listing noerror
         uart:~$

Set Notification Filter
-----------------------

.. tabs::

   .. group-tab:: MCE

      .. code-block:: console

         uart:~$ map mce mas set_ntf_filter
         Adding default application parameters:
           Notification filter mask: 0x00007fff
         MCE MAS 0x20005a00 set_ntf_filter rsp, rsp_code Success
         uart:~$

   .. group-tab:: MSE

      .. code-block:: console

         MSE MAS 0x20005c00 set_ntf_filter req, final true, data len 50
         HI cb Len 4
         Conn ID: 00000001
         HI 42 Len 29
         00000000: 78 2d 62 74 2f 4d 41 50  2d 6e 6f 74 69 66 69 63 |x-bt/MAP -notific|
         00000010: 61 74 69 6f 6e 2d 66 69  6c 74 65 72 00          |ation-fi lter.   |
         HI 4c Len 6
         T 25 L 4
         00000000: 00 00 7f ff                                      |....             |
         HI 49 Len 1
         00000000: 30                                               |0                |
         uart:~$ map mse mas set_ntf_filter noerror
         uart:~$

Send Event
----------

RFCOMM Transport
~~~~~~~~~~~~~~~~

.. tabs::

   .. group-tab:: MSE

      .. code-block:: console

         uart:~$ map mse mns send_event
         Adding default application parameters:
           Instance ID: 0
         MSE MNS 0x20005d00 send_event rsp, rsp_code Continue, data len 0
         uart:~$ map mse mns send_event
         MSE MNS 0x20005d00 send_event rsp, rsp_code Success, data len 0
         uart:~$

   .. group-tab:: MCE

      .. code-block:: console

         MCE MNS 0x20005b00 send_event req, final false, data len 252
         HI cb Len 4
         Conn ID: 00000001
         HI 42 Len 22
         00000000: 78 2d 62 74 2f 4d 41 50  2d 65 76 65 6e 74 2d 72 |x-bt/MAP -event-r|
         00000010: 65 70 6f 72 74 00                                |eport.           |
         HI 4c Len 3
         T 0f L 1
         00000000: 00                                               |.                |
         HI 48 Len 213
         00000000: 3c 3f 78 6d 6c 20 76 65  72 73 69 6f 6e 3d 27 31 |<?xml ve rsion='1|
         00000010: 2e 30 27 20 65 6e 63 6f  64 69 6e 67 3d 27 75 74 |.0' enco ding='ut|
         ...
         uart:~$ map mce mns send_event noerror
         MCE MNS 0x20005b00 send_event req, final true, data len 8
         HI 49 Len 5
         00000000: 6f 72 74 3e 00                                   |ort>.            |
         uart:~$ map mce mns send_event noerror
         uart:~$

L2CAP Transport with SRM
~~~~~~~~~~~~~~~~~~~~~~~~

.. tabs::

   .. group-tab:: MSE

      .. code-block:: console

         uart:~$ map mse mns send_event
         Adding default application parameters:
           Instance ID: 0
         MSE MNS 0x20005d00 send_event rsp, rsp_code Continue, data len 2
         HI 97 Len 1
         OBEX SRM: 01
         uart:~$ map mse mns send_event
         MSE MNS 0x20005d00 send_event rsp, rsp_code Success, data len 0
         uart:~$

   .. group-tab:: MCE

      .. code-block:: console

         MCE MNS 0x20005b00 send_event req, final false, data len 252
         HI cb Len 4
         Conn ID: 00000001
         HI 97 Len 1
         OBEX SRM: 01
         HI 42 Len 22
         00000000: 78 2d 62 74 2f 4d 41 50  2d 65 76 65 6e 74 2d 72 |x-bt/MAP -event-r|
         00000010: 65 70 6f 72 74 00                                |eport.           |
         HI 4c Len 3
         T 0f L 1
         00000000: 00                                               |.                |
         HI 48 Len 211
         00000000: 3c 3f 78 6d 6c 20 76 65  72 73 69 6f 6e 3d 27 31 |<?xml ve rsion='1|
         00000010: 2e 30 27 20 65 6e 63 6f  64 69 6e 67 3d 27 75 74 |.0' enco ding='ut|
         ...
         uart:~$ map mce mns send_event noerror
         MCE MNS 0x20005b00 send_event req, final true, data len 10
         HI 49 Len 7
         00000000: 65 70 6f 72 74 3e 00                             |eport>.          |
         uart:~$ map mce mns send_event noerror
         uart:~$

L2CAP Transport with SRM and SRMP
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. tabs::

   .. group-tab:: MSE

      .. code-block:: console

         uart:~$ map mse mns send_event
         Adding default application parameters:
           Instance ID: 0
         MSE MNS 0x20005d00 send_event rsp, rsp_code Continue, data len 4
         HI 97 Len 1
         OBEX SRM: 01
         HI 98 Len 1
         OBEX SRMP: 01
         uart:~$ map mse mns send_event
         MSE MNS 0x20005d00 send_event rsp, rsp_code Success, data len 0
         uart:~$

   .. group-tab:: MCE

      .. code-block:: console

         MCE MNS 0x20005b00 send_event req, final false, data len 252
         HI cb Len 4
         Conn ID: 00000001
         HI 97 Len 1
         OBEX SRM: 01
         HI 42 Len 22
         00000000: 78 2d 62 74 2f 4d 41 50  2d 65 76 65 6e 74 2d 72 |x-bt/MAP -event-r|
         00000010: 65 70 6f 72 74 00                                |eport.           |
         HI 4c Len 3
         T 0f L 1
         00000000: 00                                               |.                |
         HI 48 Len 211
         00000000: 3c 3f 78 6d 6c 20 76 65  72 73 69 6f 6e 3d 27 31 |<?xml ve rsion='1|
         00000010: 2e 30 27 20 65 6e 63 6f  64 69 6e 67 3d 27 75 74 |.0' enco ding='ut|
         ...
         uart:~$ map mce mns send_event noerror srmp
         MCE MNS 0x20005b00 send_event req, final true, data len 10
         HI 49 Len 7
         00000000: 65 70 6f 72 74 3e 00                             |eport>.          |
         uart:~$ map mce mns send_event noerror
         uart:~$

Application Parameters
======================

The MAP shell provides commands to add application parameters to requests. These
parameters must be added before sending the corresponding operation command.

Adding Application Parameters
-----------------------------

Application parameters can be added using the :code:`map add_header app_param` commands:

.. code-block:: console

   uart:~$ map add_header app_param max_list_count 10
   Added MaxListCount: 10
   uart:~$ map add_header app_param list_start_offset 0
   Added ListStartOffset: 0
   uart:~$ map add_header app_param charset utf8
   Added Charset: utf8 (1)
   uart:~$

Listing Application Parameters
------------------------------

To view all added application parameters:

.. code-block:: console

   uart:~$ map add_header app_param list
   Added application parameters (3):
     [0] Tag=0x01, Len=2
   00000000: 00 0a                                            |..               |
     [1] Tag=0x02, Len=2
   00000000: 00 00                                            |..               |
     [2] Tag=0x14, Len=1
   00000000: 01                                               |.                |
   uart:~$

Clearing Application Parameters
-------------------------------

To clear all added application parameters:

.. code-block:: console

   uart:~$ map add_header app_param clear
   Cleared all application parameters
   uart:~$

Example: Get Message Listing with Filters
-----------------------------------------

.. code-block:: console

   uart:~$ map add_header app_param max_list_count 10
   Added MaxListCount: 10
   uart:~$ map add_header app_param filter_msg_type 1f
   Added FilterMessageType: 0x1f
     EMAIL=Y, SMS_GSM=Y, SMS_CDMA=Y, MMS=Y, IM=Y
   uart:~$ map add_header app_param filter_read_status unread
   Added FilterReadStatus: unread (1)
   uart:~$ map mce mas get_msg_listing
   MCE MAS 0x20005a00 get_msg_listing rsp, rsp_code Continue, data len 252
   ...
   uart:~$ map add_header app_param clear
   Cleared all application parameters
   uart:~$

Additional Information
**********************

RFCOMM vs L2CAP Transport
=========================

RFCOMM Transport
----------------

For GET or PUT operations, the same shell command must be called multiple times
until the complete message is delivered.

L2CAP Transport with SRM
------------------------

When SRM (Single Response Mode) is enabled, GET requests do not need to be
repeated, and the server sends all response packets. For PUT operations,
the client sends all request packets without waiting for intermediate
responses, and the server sends a single response upon receiving
the final PUT request.

L2CAP Transport with SRM and SRMP
---------------------------------

When both SRM and SRMP (Single Response Mode Parameter) are enabled, GET requests
and PUT responses need to continue sending until SRMP is disabled.

SRM (Single Response Mode)
==========================

SRM is automatically enabled on L2CAP transport (GOEP v2.0).

GET Operations
--------------

When SRM is enabled for GET operations:

* The client sends a single GET request.
* The server sends multiple response packets without waiting for additional requests.
* The :code:`srmp` parameter can be used to control the flow (wait for next request).

PUT Operations
--------------

When SRM is enabled for PUT operations:

* The client sends multiple PUT requests without waiting for additional responses.
* The server sends a single PUT response when receiving the final PUT request.
* The :code:`srmp` parameter can be used to control the flow (wait for next response).

Multiple Instance Support
=========================

The MAP shell supports multiple MAS instances:

* MCE can connect to multiple MSE MAS instances simultaneously
* MSE can register multiple MAS instances with different instance IDs
* Use the :code:`select` command to switch between instances

.. code-block:: console

   uart:~$ map mce mas select 0
   Selected MCE MAS instance_id 0 as default
   uart:~$
