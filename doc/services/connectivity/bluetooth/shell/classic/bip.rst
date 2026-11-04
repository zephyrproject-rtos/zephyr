Bluetooth: BIP Shell
####################

This document describes how to run the Bluetooth Classic BIP (Basic Imaging
Profile) shell functionality. The :code:`bip` command exposes the Bluetooth
Classic BIP shell commands.

BIP is a Bluetooth Classic profile that enables imaging devices, such as
cameras, printers and displays, to exchange and manage images. The BIP shell
implements both the client (initiator) and the server (responder) side of every
BIP operation, on top of either an RFCOMM or an L2CAP (GOEP) transport. A
single :code:`bip` instance can act as client, as server, or as both at the
same time; the role is chosen per OBEX connection.

Commands
********

Main Commands
=============

The :code:`bip` command:

.. code-block:: console

   uart:~$ bip
   bip - Bluetooth BIP shell commands
   Subcommands:
     register-rfcomm   : <channel>
     connect-rfcomm    : <channel>
     disconnect-rfcomm :
     register-l2cap    : <psm>
     connect-l2cap     : <psm>
     disconnect-l2cap  :
     alloc-buf         : Alloc tx buffer
     release-buf       : Free allocated tx buffer
     add-header        : Adding header sets
     client            : Client sets
     server            : Server sets
     sdp               : SDP sets

The top-level commands manage the transport and route to the three command
sets that perform the actual work:

* :code:`add-header` builds the OBEX headers that are attached to an outgoing
  request or response.
* :code:`client` sends BIP requests (initiator role).
* :code:`server` sends BIP responses (responder role).
* :code:`sdp` registers the BIP SDP record and discovers a peer.

Header Commands
===============

The :code:`bip add-header` commands:

.. code-block:: console

   uart:~$ bip add-header
   add-header - Adding header sets
   Subcommands:
     count              : <number of objects (used by Connect)>
     name               : [name of the object (often a file name)]
     type               : <type of object - e.g. text, html, binary, manufacturer specific>
     len                : <length of the object in bytes>
     time_iso_8601      : <date/time stamp - ISO 8601 version - preferred>
     time               : <date/time stamp - 4 byte version (for compatibility only)>
     description        : <text description of the object>
     target             : <name of service that operation is targeted to>
     http               : <an HTTP 1.x header>
     body               : <a chunk of the object body>
     end_body           : <the final chunk of the object body>
     who                : <identifies the BIP application, used to tell if talking to a peer>
     conn_id            : <an identifier used for BIP connection multiplexing>
     app_param          : application parameter: <tag> <value> [last]
     auth_challenge     : authentication digest-challenge: <tag> <value> [last]
     auth_rsp           : authentication digest-response: <tag> <value> [last]
     creator_id         : <indicates the creator of an object>
     wan_uuid           : <uniquely identifies the network client (BIP server)>
     obj_class          : <BIP Object class of object>
     session_param      : <parameters used in session commands/responses>
     session_seq_number : <sequence number used in each BIP packet for reliability>
     action_id          : <specifies the action to be performed (used in ACTION operation)>
     dest_name          : <the destination object name (used in certain ACTION operations)>
     perm               : <4-byte bit mask for setting permissions>
     srm                : <1-byte value to setup Single Response Mode (SRM)>
     srm_param          : <Single Response Mode (SRM) Parameter>

Header values are entered in hexadecimal. The :code:`name`, :code:`type`,
:code:`time_iso_8601`, :code:`description`, :code:`target`, :code:`http`,
:code:`body`, :code:`end_body`, :code:`who`, :code:`wan_uuid`,
:code:`obj_class`, :code:`session_param` and :code:`dest_name` commands take
their payload as a hexadecimal string. The :code:`count`, :code:`len`,
:code:`time`, :code:`conn_id`, :code:`creator_id`, :code:`session_seq_number`,
:code:`action_id`, :code:`perm`, :code:`srm` and :code:`srm_param` commands
take an integer value.

The :code:`app_param`, :code:`auth_challenge` and :code:`auth_rsp` commands
take a TLV triplet of the form :code:`<tag> <value> [last]`, where both the tag
and the value are hexadecimal. Multiple triplets can be queued; appending the
:code:`last` keyword to the final triplet flushes the accumulated TLV list into
the header.

Client Commands
===============

The :code:`bip client` commands:

.. code-block:: console

   uart:~$ bip client
   client - Client sets
   Subcommands:
     set_feats_funcs      :
     conn                 : <type> [UUID 128]
     disconn              :
     abort                :
     get_caps             :
     get_image_list       :
     get_image_properties :
     get_image            :
     get_linked_thumbnail :
     get_linked_attachment:
     get_partial_image    :
     get_monitoring_image :
     get_status           :
     put_image            :
     put_linked_thumbnail :
     put_linked_attachment:
     remote_display       :
     delete_image         :
     start_print          :
     start_archive        :

The :code:`conn` command takes the connection type as a decimal integer. The
following connection types are defined:

.. list-table:: BIP connection types
   :header-rows: 1

   * - Value
     - Connection type
   * - 0
     - Image Push
   * - 1
     - Image Pull
   * - 2
     - Advanced Image Printing
   * - 3
     - Auto Archive
   * - 4
     - Remote Camera
   * - 5
     - Remote Display
   * - 10
     - Referenced Objects (secondary)
   * - 11
     - Archived Objects (secondary)

Each operation maps to one connection type. The following table lists which
client command is available for which connection type:

.. list-table:: BIP operations per connection type
   :header-rows: 1

   * - Connection type
     - Value
     - Client commands
   * - Image Push
     - 0
     - get_caps, put_image, put_linked_thumbnail, put_linked_attachment
   * - Image Pull
     - 1
     - get_caps, get_image_list, get_image_properties, get_image,
       get_linked_thumbnail, get_linked_attachment, delete_image
   * - Advanced Image Printing
     - 2
     - get_caps, get_status, start_print
   * - Auto Archive
     - 3
     - get_status, start_archive
   * - Remote Camera
     - 4
     - get_image_properties, get_image, get_linked_thumbnail,
       get_monitoring_image
   * - Remote Display
     - 5
     - get_caps, get_image_list, put_image, put_linked_thumbnail,
       remote_display
   * - Referenced Objects
     - 10
     - get_partial_image

Server Commands
===============

The :code:`bip server` commands:

.. code-block:: console

   uart:~$ bip server
   server - Server sets
   Subcommands:
     reg                    : <type> [UUID 128]
     unreg                  :
     conn                   : <rsp: continue, success, error> [rsp_code]
     disconn                : <rsp: continue, success, error> [rsp_code]
     abort                  : <rsp: continue, success, error> [rsp_code]
     get_caps               : <rsp: noerror, error> [rsp_code]
     get_image_list         : <rsp: noerror, error> [rsp_code]
     get_image_properties   : <rsp: noerror, error> [rsp_code]
     get_image              : <rsp: noerror, error> [rsp_code]
     get_linked_thumbnail   : <rsp: noerror, error> [rsp_code]
     get_linked_attachment  : <rsp: noerror, error> [rsp_code]
     get_partial_image      : <rsp: noerror, error> [rsp_code]
     get_monitoring_image   : <rsp: noerror, error> [rsp_code]
     get_status             : <rsp: continue, success, error> [rsp_code]
     put_image              : <rsp: noerror, error> [rsp_code]
     put_linked_thumbnail   : <rsp: noerror, error> [rsp_code]
     put_linked_attachment  : <rsp: noerror, error> [rsp_code]
     remote_display         : <rsp: noerror, error> [rsp_code]
     delete_image           : <rsp: noerror, error> [rsp_code]
     start_print            : <rsp: noerror, error> [rsp_code]
     start_archive          : <rsp: noerror, error> [rsp_code]

The :code:`reg` command registers the BIP server for a given connection type
and reuses the same numeric type values as :code:`bip client conn`.

The response keywords accepted by the server commands are:

* :code:`continue` - send ``Continue`` (0x90). The operation stays in progress.
* :code:`success` - send ``Success`` (0xA0).
* :code:`error` - send an error response. Requires a hexadecimal
  ``rsp_code`` argument, for example :code:`error c3` sends ``Forbidden``.
* :code:`noerror` - used by the GET operations that carry a body (such as
  :code:`get_caps`, :code:`get_image`): the shell sends the body and derives
  the response code automatically. It sends ``Continue`` while the body is
  fragmented and ``Success`` once the last chunk is sent.

SDP Commands
============

The :code:`bip sdp` commands:

.. code-block:: console

   uart:~$ bip sdp
   sdp - SDP sets
   Subcommands:
     reg           :
     set_caps      : <capabilities>
     set_features  : <features>
     set_functions : <functions>
     discover      :

The SDP record advertises the responder's capabilities, features, functions and
total imaging data capacity. The :code:`set_caps`, :code:`set_features` and
:code:`set_functions` commands change the values that the :code:`bip sdp reg`
command later publishes; they take a hexadecimal bit mask.

.. list-table:: BIP supported features bit map
   :header-rows: 1

   * - Bit
     - Feature
   * - 0
     - Image Push
   * - 1
     - Image Push Store
   * - 2
     - Image Push Print
   * - 3
     - Image Push Display
   * - 4
     - Image Pull
   * - 5
     - Advanced Image Printing
   * - 6
     - Auto Archive
   * - 7
     - Remote Camera
   * - 8
     - Remote Display

Usage Examples
**************

The examples below assume two devices, one running the BIP client (initiator)
and the other running the BIP server (responder). The ACL connection must be
established before any BIP transport connection, for example with
:code:`br connect <peer-address>`.

SDP Discovery
=============

The client discovers the server's BIP service before connecting:

.. code-block:: console

   uart:~$ bip sdp discover
   Found RFCOMM channel 9
   Found GOEP L2CAP PSM 4105
   Found BIP features 0x000001ff
   Found BIP functions 0x0001ffff
   uart:~$

The discovered features and functions are stored automatically and later
applied by :code:`bip client set_feats_funcs`.

Transport Connection
====================

The BIP transport can be either RFCOMM or L2CAP. In the examples the server
registers the transport and the client connects to it.

RFCOMM Transport
----------------

.. tabs::

   .. group-tab:: Server

      .. code-block:: console

         uart:~$ bip register-rfcomm 9
         RFCOMM server (channel 09) is registered
         uart:~$

   .. group-tab:: Client

      .. code-block:: console

         uart:~$ bip connect-rfcomm 9
         BIP RFCOMM connection pending
         BIP 0x20003420 transport connected on 0x20002870
         uart:~$

The channel must be registered first on the server side with a fixed value, so
the client can connect to it. Registering channel 0 lets the stack allocate one
dynamically; the resulting channel is printed by :code:`bip sdp discover`.

L2CAP Transport
---------------

.. tabs::

   .. group-tab:: Server

      .. code-block:: console

         uart:~$ bip register-l2cap 1009
         L2CAP server (psm 1009) is registered
         uart:~$

   .. group-tab:: Client

      .. code-block:: console

         uart:~$ bip connect-l2cap 1009
         BIP L2CAP connection pending
         BIP 0x20003420 transport connected on 0x20002870
         uart:~$

Transport Disconnection
=======================

Either side may tear down the transport. The side that issued the command sees
a "pending" message followed by the disconnect notification; the peer only
sees the notification.

.. tabs::

   .. group-tab:: Initiating side

      .. code-block:: console

         uart:~$ bip disconnect-rfcomm
         BIP RFCOMM disconnection pending
         BIP 0x20003420 transport disconnected
         uart:~$

   .. group-tab:: Peer side

      .. code-block:: console

         BIP 0x20003420 transport disconnected
         uart:~$

The L2CAP transport is disconnected with :code:`bip disconnect-l2cap`.

OBEX Connection
===============

After the transport is up, an OBEX connection must be established before any
BIP operation. The client initiates with :code:`bip client conn`, and the
server replies with :code:`bip server conn`.

Connect to BIP Server
---------------------

.. tabs::

   .. group-tab:: Client

      .. code-block:: console

         uart:~$ bip client set_feats_funcs
         uart:~$ bip alloc-buf
         uart:~$ bip client conn 0
         BIP client 0x200033b0 conn rsp, rsp_code Success, version 10, mopl 00ff
         uart:~$

   .. group-tab:: Server

      .. code-block:: console

         BIP server 0x200036f0 conn req, version 10, mopl 00ff
         uart:~$ bip alloc-buf
         uart:~$ bip server conn success
         uart:~$

The server can reject the connection by replying with an error code:

.. tabs::

   .. group-tab:: Client

      .. code-block:: console

         uart:~$ bip client conn 0
         BIP client 0x200033b0 conn rsp, rsp_code Forbidden - operation is understood but refused, version 10, mopl 00ff
         uart:~$

   .. group-tab:: Server

      .. code-block:: console

         BIP server 0x200036f0 conn req, version 10, mopl 00ff
         uart:~$ bip server conn error c3
         uart:~$

Disconnect from BIP Server
--------------------------

.. tabs::

   .. group-tab:: Client

      .. code-block:: console

         uart:~$ bip alloc-buf
         uart:~$ bip client disconn
         BIP client 0x200033b0 disconn rsp, rsp_code Success
         uart:~$

   .. group-tab:: Server

      .. code-block:: console

         BIP server 0x200036f0 disconn req
         uart:~$ bip alloc-buf
         uart:~$ bip server disconn success
         uart:~$

BIP Operations
==============

Every BIP operation follows the same request/response exchange. The client
sends the request with a :code:`bip client <operation>` command; the server
replies with :code:`bip server <operation> <rsp>`. GET operations that carry a
body (for example :code:`get_caps`, :code:`get_image_list`,
:code:`get_image`) may require the exchange to be repeated until the server
sends ``Success``.

Get Capabilities
----------------

.. tabs::

   .. group-tab:: Client

      .. code-block:: console

         uart:~$ bip client get_caps
         BIP client 0x200033b0 get_caps rsp, rsp_code Success, data len 4
         uart:~$

   .. group-tab:: Server

      .. code-block:: console

         BIP server 0x200036f0 get_caps req, final true, data len 6
         uart:~$ bip server get_caps noerror
         uart:~$

Get Image List
--------------

.. tabs::

   .. group-tab:: Client

      .. code-block:: console

         uart:~$ bip client get_image_list
         BIP client 0x200033b0 get_image_list rsp, rsp_code Success, data len 4
         uart:~$

   .. group-tab:: Server

      .. code-block:: console

         BIP server 0x200036f0 get_image_list req, final true, data len 6
         uart:~$ bip server get_image_list noerror
         uart:~$

Get Image Properties
--------------------

.. tabs::

   .. group-tab:: Client

      .. code-block:: console

         uart:~$ bip client get_image_properties
         BIP client 0x200033b0 get_image_properties rsp, rsp_code Success, data len 4
         uart:~$

   .. group-tab:: Server

      .. code-block:: console

         BIP server 0x200036f0 get_image_properties req, final true, data len 6
         uart:~$ bip server get_image_properties noerror
         uart:~$

Get Image
---------

.. tabs::

   .. group-tab:: Client

      .. code-block:: console

         uart:~$ bip client get_image
         BIP client 0x200033b0 get_image rsp, rsp_code Success, data len 4
         uart:~$

   .. group-tab:: Server

      .. code-block:: console

         BIP server 0x200036f0 get_image req, final true, data len 6
         uart:~$ bip server get_image noerror
         uart:~$

Get Linked Thumbnail
--------------------

.. tabs::

   .. group-tab:: Client

      .. code-block:: console

         uart:~$ bip client get_linked_thumbnail
         BIP client 0x200033b0 get_linked_thumbnail rsp, rsp_code Success, data len 4
         uart:~$

   .. group-tab:: Server

      .. code-block:: console

         BIP server 0x200036f0 get_linked_thumbnail req, final true, data len 6
         uart:~$ bip server get_linked_thumbnail noerror
         uart:~$

Get Linked Attachment
---------------------

.. tabs::

   .. group-tab:: Client

      .. code-block:: console

         uart:~$ bip client get_linked_attachment
         BIP client 0x200033b0 get_linked_attachment rsp, rsp_code Success, data len 4
         uart:~$

   .. group-tab:: Server

      .. code-block:: console

         BIP server 0x200036f0 get_linked_attachment req, final true, data len 6
         uart:~$ bip server get_linked_attachment noerror
         uart:~$

Get Partial Image
-----------------

The partial image operation runs over the secondary Referenced Objects
connection (connection type 10).

.. tabs::

   .. group-tab:: Client

      .. code-block:: console

         uart:~$ bip client get_partial_image
         BIP client 0x200033b0 get_partial_image rsp, rsp_code Success, data len 4
         uart:~$

   .. group-tab:: Server

      .. code-block:: console

         BIP server 0x200036f0 get_partial_image req, final true, data len 6
         uart:~$ bip server get_partial_image noerror
         uart:~$

Get Monitoring Image
--------------------

.. tabs::

   .. group-tab:: Client

      .. code-block:: console

         uart:~$ bip client get_monitoring_image
         BIP client 0x200033b0 get_monitoring_image rsp, rsp_code Success, data len 4
         uart:~$

   .. group-tab:: Server

      .. code-block:: console

         BIP server 0x200036f0 get_monitoring_image req, final true, data len 6
         uart:~$ bip server get_monitoring_image noerror
         uart:~$

Get Status
----------

Unlike the other GET operations, :code:`get_status` accepts the same
:code:`continue`, :code:`success` and :code:`error` keywords as the connection
commands, because it carries no body.

.. tabs::

   .. group-tab:: Client

      .. code-block:: console

         uart:~$ bip client get_status
         BIP client 0x200033b0 get_status rsp, rsp_code Success, data len 0
         uart:~$

   .. group-tab:: Server

      .. code-block:: console

         BIP server 0x200036f0 get_status req, final true, data len 0
         uart:~$ bip server get_status success
         uart:~$

Put Image
---------

:code:`put_image` sends a fixed JPEG payload. The client repeats the command
until the whole payload has been transferred and the server responds with
``Success``.

.. tabs::

   .. group-tab:: Client

      .. code-block:: console

         uart:~$ bip client put_image
         BIP client 0x200033b0 put_image rsp, rsp_code Continue, data len 0
         uart:~$ bip client put_image
         BIP client 0x200033b0 put_image rsp, rsp_code Success, data len 0
         uart:~$

   .. group-tab:: Server

      .. code-block:: console

         BIP server 0x200036f0 put_image req, final false, data len 6
         uart:~$ bip server put_image noerror
         BIP server 0x200036f0 put_image req, final true, data len 6
         uart:~$ bip server put_image noerror
         uart:~$

The server rejects the operation with an error:

.. tabs::

   .. group-tab:: Client

      .. code-block:: console

         uart:~$ bip client put_image
         BIP client 0x200033b0 put_image rsp, rsp_code Unsupported media type, data len 0
         uart:~$

   .. group-tab:: Server

      .. code-block:: console

         BIP server 0x200036f0 put_image req, final false, data len 6
         uart:~$ bip server put_image error cf
         uart:~$

Put Linked Thumbnail
--------------------

.. tabs::

   .. group-tab:: Client

      .. code-block:: console

         uart:~$ bip client put_linked_thumbnail
         BIP client 0x200033b0 put_linked_thumbnail rsp, rsp_code Success, data len 0
         uart:~$

   .. group-tab:: Server

      .. code-block:: console

         BIP server 0x200036f0 put_linked_thumbnail req, final true, data len 6
         uart:~$ bip server put_linked_thumbnail noerror
         uart:~$

Put Linked Attachment
---------------------

.. tabs::

   .. group-tab:: Client

      .. code-block:: console

         uart:~$ bip client put_linked_attachment
         BIP client 0x200033b0 put_linked_attachment rsp, rsp_code Success, data len 0
         uart:~$

   .. group-tab:: Server

      .. code-block:: console

         BIP server 0x200036f0 put_linked_attachment req, final true, data len 6
         uart:~$ bip server put_linked_attachment noerror
         uart:~$

Remote Display
--------------

.. tabs::

   .. group-tab:: Client

      .. code-block:: console

         uart:~$ bip client remote_display
         BIP client 0x200033b0 remote_display rsp, rsp_code Success, data len 0
         uart:~$

   .. group-tab:: Server

      .. code-block:: console

         BIP server 0x200036f0 remote_display req, final true, data len 6
         uart:~$ bip server remote_display noerror
         uart:~$

Delete Image
------------

.. tabs::

   .. group-tab:: Client

      .. code-block:: console

         uart:~$ bip client delete_image
         BIP client 0x200033b0 delete_image rsp, rsp_code Success, data len 0
         uart:~$

   .. group-tab:: Server

      .. code-block:: console

         BIP server 0x200036f0 delete_image req, final true, data len 6
         uart:~$ bip server delete_image noerror
         uart:~$

Start Print
-----------

.. tabs::

   .. group-tab:: Client

      .. code-block:: console

         uart:~$ bip client start_print
         BIP client 0x200033b0 start_print rsp, rsp_code Success, data len 0
         uart:~$

   .. group-tab:: Server

      .. code-block:: console

         BIP server 0x200036f0 start_print req, final true, data len 6
         uart:~$ bip server start_print noerror
         uart:~$

Start Archive
-------------

.. tabs::

   .. group-tab:: Client

      .. code-block:: console

         uart:~$ bip client start_archive
         BIP client 0x200033b0 start_archive rsp, rsp_code Success, data len 0
         uart:~$

   .. group-tab:: Server

      .. code-block:: console

         BIP server 0x200036f0 start_archive req, final true, data len 6
         uart:~$ bip server start_archive noerror
         uart:~$

Abort an Operation
==================

An in-progress operation can be aborted from the client side:

.. tabs::

   .. group-tab:: Client

      .. code-block:: console

         uart:~$ bip alloc-buf
         uart:~$ bip client abort
         BIP client 0x200033b0 abort rsp, rsp_code Success
         uart:~$

   .. group-tab:: Server

      .. code-block:: console

         BIP server 0x200036f0 abort req
         uart:~$ bip alloc-buf
         uart:~$ bip server abort success
         uart:~$

SDP Registration
================

Before advertising the service, configure the SDP attributes and register the
record:

.. code-block:: console

   uart:~$ bip sdp set_caps 0
   uart:~$ bip sdp set_features 1ff
   uart:~$ bip sdp set_functions 1ffff
   uart:~$ bip sdp reg
   uart:~$

Additional Information
**********************

RFCOMM vs L2CAP Transport
=========================

RFCOMM Transport
----------------

For GET or PUT operations, the same shell command must be called multiple
times until the complete message is delivered: each ``Continue`` response means
another chunk is expected.

L2CAP Transport with SRM
------------------------

SRM (Single Response Mode) is enabled automatically on L2CAP transport (GOEP
2.0). With SRM, GET requests do not need to be repeated: the server sends all
response packets, and PUT requests flow without waiting for intermediate
responses.

The :code:`bip add-header srm 1` command can be used to add the SRM header to
a request explicitly, and :code:`bip add-header srm_param 1` adds an SRM
parameter. These are only meaningful over L2CAP.

Data Buffers
============

The BIP shell uses a single transmit buffer per device. Before sending an OBEX
request or response, allocate it with :code:`bip alloc-buf`, build the headers
with :code:`bip add-header`, and then issue the :code:`bip client` or
:code:`bip server` command, which consumes and frees the buffer. A buffer that
is no longer needed can be released with :code:`bip release-buf`.

Some commands, such as :code:`bip client get_caps` and the PUT operations,
allocate their buffer internally and do not require a preceding
:code:`bip alloc-buf`. The connection, disconnection and abort commands do
require an explicit :code:`bip alloc-buf` on the side that sends the request.
