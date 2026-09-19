Bluetooth: Classic: FTP Shell
#############################

This document describes how to run the Bluetooth Classic FTP (File Transfer Profile) shell
functionality. The :code:`ftp` command exposes the Bluetooth Classic FTP shell commands.

FTP runs on top of OBEX over GOEP. The :code:`ftp client` commands implement the FTP client
role and are built when the ``BT_FTP_CLIENT`` Kconfig option is enabled. The
:code:`ftp server` commands implement the FTP server role and are built when the
``BT_FTP_SERVER`` Kconfig option is enabled.

All commands except :code:`ftp server register`, :code:`ftp server rfcomm_register` and
:code:`ftp server l2cap_register` operate on the default ACL connection, so the ACL connection
must be established first.

The shell module carries a built-in sample file body and a built-in sample folder listing. The
file body is sent by :code:`ftp client push_file` and by :code:`ftp server pull_file`, and the
folder listing is sent by :code:`ftp server pull_folder_listing`. No file system is involved.

Commands
********

Main Commands
=============

The :code:`ftp` commands:

.. code-block:: console

   uart:~$ ftp
   ftp - Bluetooth FTP shell commands
   Subcommands:
     client  : FTP client commands
     server  : FTP server commands

Client Commands
===============

The :code:`ftp client` commands:

.. code-block:: console

   uart:~$ ftp client
   client - FTP client commands
   Subcommands:
     rfcomm_connect       : <channel>
     rfcomm_disconnect    : [none]
     l2cap_connect        : <psm>
     l2cap_disconnect     : [none]
     connect              : [password]
     disconnect           : [none]
     abort                : [none]
     set_folder           : <path: "/" | ".." | "folder">
     create_folder        : <folder_name>
     pull_folder_listing  : [name <folder_name>] [srm] [srmp]
     push_file            : <filename> [srm]
     pull_file            : <filename> [srm] [srmp]
     delete               : <filename>
     rename               : <src_name> <dst_name>
     copy                 : <src_name> <dst_name>
     set_permission       : <filename> <permission_mask>
   Permission mask format (octet): [User][Group][Other]
   Each octet: bit0=Read, bit1=Write, bit2=Delete, bit7=Modify
   Example: 0x070505 = rwx for user, rx for group/other

The :code:`<channel>`, :code:`<psm>` and :code:`<permission_mask>` arguments are parsed as
hexadecimal values.

Server Commands
===============

The :code:`ftp server` commands:

.. code-block:: console

   uart:~$ ftp server
   server - FTP server commands
   Subcommands:
     register             : [none]
     rfcomm_register      : [none]
     rfcomm_disconnect    : [none]
     l2cap_register       : [none]
     l2cap_disconnect     : [none]
     connect              : <rsp: unauth, success, error> [rsp_code] [password]
     disconnect           : <rsp: success, error> [rsp_code]
     abort                : <rsp: success, error> [rsp_code]
     set_folder           : <rsp: success, error> [rsp_code]
     create_folder        : <rsp: success, error> [rsp_code]
     pull_folder_listing  : <rsp: noerror, error> [rsp_code] [srm] [srmp]
     push_file            : <rsp: noerror, error> [rsp_code] [srm] [srmp]
     pull_file            : <rsp: noerror, error> [rsp_code] [srm] [srmp]
     delete               : <rsp: success, error> [rsp_code]
     rename               : <rsp: success, error> [rsp_code]
     copy                 : <rsp: success, error> [rsp_code]
     set_permission       : <rsp: success, error> [rsp_code]

Every server command sends one response. The :code:`success` and :code:`noerror` keywords send
the OBEX response code ``Success``, and :code:`error` sends the response code given by the
:code:`[rsp_code]` argument, which is parsed as a hexadecimal value. For the operations that can
be fragmented, :code:`noerror` lets the shell decide between ``Continue`` and ``Success``
depending on how much data is left to transfer.

Usage Examples
**************

Connect FTP Transport
=====================

The ACL connection must be established before creating the FTP transport connection. The server
side registers its FTP instances with :code:`ftp server register` before registering a transport,
so that an incoming transport connection can be accepted.

RFCOMM Transport
----------------

.. tabs::

   .. group-tab:: Client

      .. code-block:: console

         uart:~$ ftp client rfcomm_connect 6
         Security changed: XX:XX:XX:XX:XX:XX level 2
         FTP client RFCOMM connected: 0x20005a00, addr: XX:XX:XX:XX:XX:XX
         uart:~$

   .. group-tab:: Server

      .. code-block:: console

         uart:~$ ftp server register
         FTP server(s) registered
         uart:~$ ftp server rfcomm_register
         FTP RFCOMM server registered, channel 6
         Security changed: XX:XX:XX:XX:XX:XX level 2
         FTP server RFCOMM connected: 0x20005c00, addr: XX:XX:XX:XX:XX:XX
         uart:~$

L2CAP Transport
---------------

.. tabs::

   .. group-tab:: Client

      .. code-block:: console

         uart:~$ ftp client l2cap_connect 1001
         Security changed: XX:XX:XX:XX:XX:XX level 2
         FTP client L2CAP connected: 0x20005a00, addr: XX:XX:XX:XX:XX:XX
         uart:~$

   .. group-tab:: Server

      .. code-block:: console

         uart:~$ ftp server register
         FTP server(s) registered
         uart:~$ ftp server l2cap_register
         FTP L2CAP server registered, psm 0x1001
         Security changed: XX:XX:XX:XX:XX:XX level 2
         FTP server L2CAP connected: 0x20005c00, addr: XX:XX:XX:XX:XX:XX
         uart:~$

The channel and the PSM are allocated dynamically, so use the value printed by
:code:`rfcomm_register` or :code:`l2cap_register` as the argument of the corresponding client
connect command, or use :code:`br sdp-find` to discover the values.

Disconnect FTP Transport
========================

RFCOMM Transport (client initiates)
-----------------------------------

.. tabs::

   .. group-tab:: Client

      .. code-block:: console

         uart:~$ ftp client rfcomm_disconnect
         FTP client RFCOMM disconnected: 0x20005a00
         uart:~$

   .. group-tab:: Server

      .. code-block:: console

         FTP server RFCOMM disconnected: 0x20005c00
         uart:~$

RFCOMM Transport (server initiates)
-----------------------------------

.. tabs::

   .. group-tab:: Client

      .. code-block:: console

         FTP client RFCOMM disconnected: 0x20005a00
         uart:~$

   .. group-tab:: Server

      .. code-block:: console

         uart:~$ ftp server rfcomm_disconnect
         FTP server RFCOMM disconnected: 0x20005c00
         uart:~$

L2CAP Transport (client initiates)
----------------------------------

.. tabs::

   .. group-tab:: Client

      .. code-block:: console

         uart:~$ ftp client l2cap_disconnect
         FTP client L2CAP disconnected: 0x20005a00
         uart:~$

   .. group-tab:: Server

      .. code-block:: console

         FTP server L2CAP disconnected: 0x20005c00
         uart:~$

L2CAP Transport (server initiates)
----------------------------------

.. tabs::

   .. group-tab:: Client

      .. code-block:: console

         FTP client L2CAP disconnected: 0x20005a00
         uart:~$

   .. group-tab:: Server

      .. code-block:: console

         uart:~$ ftp server l2cap_disconnect
         FTP server L2CAP disconnected: 0x20005c00
         uart:~$

Connect to FTP Server
=====================

The client adds the FTP target UUID (``F9EC7BC4-953C-11D2-984E-525400DC9E09``) to the OBEX
connect request. The server adds the connection ID and the ``Who`` header to a successful
connect response.

Without Authentication
----------------------

.. tabs::

   .. group-tab:: Client

      .. code-block:: console

         uart:~$ ftp client connect
         FTP client 0x20005a00 OBEX connect rsp, rsp_code Success, version 10, mopl 00ff
         HI 0xcb Len 4
         Conn ID: 0x00000001
         HI 0x4a Len 16
         00000000: f9 ec 7b c4 95 3c 11 d2  98 4e 52 54 00 dc 9e 09 |..{..<.. .NRT....|
         Connection established (no auth required)
         uart:~$

   .. group-tab:: Server

      .. code-block:: console

         FTP server 0x20005c00 OBEX connect req, version 10, mopl 00ff
         HI 0x46 Len 16
         00000000: f9 ec 7b c4 95 3c 11 d2  98 4e 52 54 00 dc 9e 09 |..{..<.. .NRT....|
         uart:~$ ftp server connect success
         uart:~$

With Authentication
-------------------

The server answers the first connect request with ``Unauthorized`` and an authentication
challenge. The client then repeats the connect command with the password. The client also adds
its own challenge, so both sides authenticate each other.

.. tabs::

   .. group-tab:: Client

      .. code-block:: console

         uart:~$ ftp client connect
         FTP client 0x20005a00 OBEX connect rsp, rsp_code Unauthorized, version 10, mopl 00ff
         HI 0x4d Len 18
         00000000: 00 10 3a 7f 1c 92 b4 5e  08 d6 f1 a2 c3 54 6e 9b |..:....^ .....Tn.|
         00000010: 07 d8                                            |..               |
         Server requires authentication
         Re-send connect with password: ftp client connect <password>
         uart:~$ ftp client connect password123
         FTP client 0x20005a00 OBEX connect rsp, rsp_code Success, version 10, mopl 00ff
         HI 0x4e Len 18
         00000000: 00 10 c4 1b 6e 37 a9 f0  52 8d 1e 7c 46 b3 09 d2 |....n7.. R..|F...|
         00000010: fa 85                                            |..               |
         HI 0xcb Len 4
         Conn ID: 0x00000001
         HI 0x4a Len 16
         00000000: f9 ec 7b c4 95 3c 11 d2  98 4e 52 54 00 dc 9e 09 |..{..<.. .NRT....|
         Connection established (authentication succeeded)
         uart:~$

   .. group-tab:: Server

      .. code-block:: console

         FTP server 0x20005c00 OBEX connect req, version 10, mopl 00ff
         HI 0x46 Len 16
         00000000: f9 ec 7b c4 95 3c 11 d2  98 4e 52 54 00 dc 9e 09 |..{..<.. .NRT....|
         uart:~$ ftp server connect unauth password123
         FTP server 0x20005c00 OBEX connect req, version 10, mopl 00ff
         HI 0x46 Len 16
         00000000: f9 ec 7b c4 95 3c 11 d2  98 4e 52 54 00 dc 9e 09 |..{..<.. .NRT....|
         HI 0x4d Len 18
         00000000: 00 10 c4 1b 6e 37 a9 f0  52 8d 1e 7c 46 b3 09 d2 |....n7.. R..|F...|
         00000010: fa 85                                            |..               |
         HI 0x4e Len 18
         00000000: 00 10 3a 7f 1c 92 b4 5e  08 d6 f1 a2 c3 54 6e 9b |..:....^ .....Tn.|
         00000010: 07 d8                                            |..               |
         Authentication succeeded
         Client requires authentication
         uart:~$ ftp server connect success
         uart:~$

The password is limited to :kconfig:option:`CONFIG_BT_OBEX_AUTH_PWD_LEN` characters. A password
passed to :code:`ftp client connect` outside of an authentication flow is ignored.

Disconnect from FTP Server
==========================

.. tabs::

   .. group-tab:: Client

      .. code-block:: console

         uart:~$ ftp client disconnect
         FTP client 0x20005a00 OBEX disconnect rsp, rsp_code Success
         uart:~$

   .. group-tab:: Server

      .. code-block:: console

         FTP server 0x20005c00 OBEX disconnect req
         HI 0xcb Len 4
         Conn ID: 0x00000001
         uart:~$ ftp server disconnect success
         uart:~$

FTP Operations
==============

Set Folder
----------

The :code:`<path>` argument selects the navigation direction: :code:`/` goes to the root folder,
a path starting with :code:`..` goes to the parent folder and any other value goes down into the
named child folder. A :code:`./folder` form is accepted as well and is equivalent to
:code:`folder`; a bare :code:`./` prints a warning and the command help instead of sending a
request.

.. tabs::

   .. group-tab:: Client

      .. code-block:: console

         uart:~$ ftp client set_folder /
         FTP client 0x20005a00 set_folder rsp, rsp_code Success
         uart:~$ ftp client set_folder docs
         FTP client 0x20005a00 set_folder rsp, rsp_code Success
         uart:~$ ftp client set_folder ..
         FTP client 0x20005a00 set_folder rsp, rsp_code Success
         uart:~$

   .. group-tab:: Server

      .. code-block:: console

         FTP server 0x20005c00 set_folder req, flags 02
         HI 0xcb Len 4
         Conn ID: 0x00000001
         HI 0x01 Len 0
         uart:~$ ftp server set_folder success
         FTP server 0x20005c00 set_folder req, flags 02
         HI 0xcb Len 4
         Conn ID: 0x00000001
         HI 0x01 Len 10
         00000000: 00 64 00 6f 00 63 00 73  00 00                   |.d.o.c.s ..      |
         uart:~$ ftp server set_folder success
         FTP server 0x20005c00 set_folder req, flags 03
         HI 0xcb Len 4
         Conn ID: 0x00000001
         HI 0x01 Len 0
         uart:~$ ftp server set_folder success
         uart:~$

Create Folder
-------------

.. tabs::

   .. group-tab:: Client

      .. code-block:: console

         uart:~$ ftp client create_folder photos
         FTP client 0x20005a00 create_folder rsp, rsp_code Success
         uart:~$

   .. group-tab:: Server

      .. code-block:: console

         FTP server 0x20005c00 create_folder req
         HI 0xcb Len 4
         Conn ID: 0x00000001
         HI 0x01 Len 14
         00000000: 00 70 00 68 00 6f 00 74  00 6f 00 73 00 00       |.p.h.o.t .o.s..  |
         uart:~$ ftp server create_folder success
         uart:~$

Pull Folder Listing
-------------------

The client sends a GET request with the type header ``x-obex/folder-listing``. Without the
:code:`name` argument the listing of the current folder is requested.

RFCOMM Transport
~~~~~~~~~~~~~~~~

When using RFCOMM transport, the same shell command must be called on both sides until the
complete listing is delivered.

.. tabs::

   .. group-tab:: Client

      .. code-block:: console

         uart:~$ ftp client pull_folder_listing
         FTP client 0x20005a00 pull_folder_listing rsp, rsp_code Continue, data len 252
         HI 0x48 Len 249
         00000000: 3c 3f 78 6d 6c 20 76 65  72 73 69 6f 6e 3d 22 31 |<?xml ve rsion="1|
         00000010: 2e 30 22 3f 3e 0d 0a 3c  21 44 4f 43 54 59 50 45 |.0"?>..< !DOCTYPE|
         ...
         uart:~$ ftp client pull_folder_listing
         FTP client 0x20005a00 pull_folder_listing rsp, rsp_code Continue, data len 252
         HI 0x48 Len 249
         00000000: 32 36 30 31 30 33 54 30  30 30 30 30 30 5a 22 2f |260103T0 00000Z"/|
         00000010: 3e 0d 0a 3c 66 6f 6c 64  65 72 20 6e 61 6d 65 3d |>..<fold er name=|
         ...
         uart:~$ ftp client pull_folder_listing
         FTP client 0x20005a00 pull_folder_listing rsp, rsp_code Success, data len 170
         HI 0x49 Len 167
         00000000: 32 36 30 31 30 32 54 30  31 30 30 30 30 5a 22 2f |260102T0 10000Z"/|
         00000010: 3e 0d 0a 3c 66 69 6c 65  20 6e 61 6d 65 3d 22 73 |>..<file  name="s|
         ...
         uart:~$

   .. group-tab:: Server

      .. code-block:: console

         FTP server 0x20005c00 pull_folder_listing req, final true
         HI 0xcb Len 4
         Conn ID: 0x00000001
         HI 0x42 Len 22
         00000000: 78 2d 6f 62 65 78 2f 66  6f 6c 64 65 72 2d 6c 69 |x-obex/f older-li|
         00000010: 73 74 69 6e 67 00                                |sting.           |
         HI 0x01 Len 0
         uart:~$ ftp server pull_folder_listing noerror
         FTP server 0x20005c00 pull_folder_listing req, final true
         uart:~$ ftp server pull_folder_listing noerror
         FTP server 0x20005c00 pull_folder_listing req, final true
         uart:~$ ftp server pull_folder_listing noerror
         uart:~$

L2CAP Transport with SRM
~~~~~~~~~~~~~~~~~~~~~~~~

With SRM enabled, the client sends a single GET request and the server sends all response
packets without waiting for another request.

.. tabs::

   .. group-tab:: Client

      .. code-block:: console

         uart:~$ ftp client pull_folder_listing srm
         FTP client 0x20005a00 pull_folder_listing rsp, rsp_code Continue, data len 252
         HI 0x97 Len 1
         OBEX SRM: 0x01
         HI 0x48 Len 247
         00000000: 3c 3f 78 6d 6c 20 76 65  72 73 69 6f 6e 3d 22 31 |<?xml ve rsion="1|
         00000010: 2e 30 22 3f 3e 0d 0a 3c  21 44 4f 43 54 59 50 45 |.0"?>..< !DOCTYPE|
         ...
         FTP client 0x20005a00 pull_folder_listing rsp, rsp_code Continue, data len 252
         HI 0x48 Len 249
         ...
         FTP client 0x20005a00 pull_folder_listing rsp, rsp_code Success, data len 172
         HI 0x49 Len 169
         ...
         uart:~$

   .. group-tab:: Server

      .. code-block:: console

         FTP server 0x20005c00 pull_folder_listing req, final true
         HI 0xcb Len 4
         Conn ID: 0x00000001
         HI 0x97 Len 1
         OBEX SRM: 0x01
         HI 0x42 Len 22
         00000000: 78 2d 6f 62 65 78 2f 66  6f 6c 64 65 72 2d 6c 69 |x-obex/f older-li|
         00000010: 73 74 69 6e 67 00                                |sting.           |
         HI 0x01 Len 0
         uart:~$ ftp server pull_folder_listing noerror srm
         uart:~$ ftp server pull_folder_listing noerror
         uart:~$ ftp server pull_folder_listing noerror
         uart:~$

L2CAP Transport with SRM and SRMP
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

With SRM and SRMP enabled, the client keeps sending GET requests until SRMP is no longer added.

.. tabs::

   .. group-tab:: Client

      .. code-block:: console

         uart:~$ ftp client pull_folder_listing srm srmp
         FTP client 0x20005a00 pull_folder_listing rsp, rsp_code Continue, data len 252
         HI 0x97 Len 1
         OBEX SRM: 0x01
         HI 0x48 Len 247
         00000000: 3c 3f 78 6d 6c 20 76 65  72 73 69 6f 6e 3d 22 31 |<?xml ve rsion="1|
         00000010: 2e 30 22 3f 3e 0d 0a 3c  21 44 4f 43 54 59 50 45 |.0"?>..< !DOCTYPE|
         ...
         uart:~$ ftp client pull_folder_listing
         FTP client 0x20005a00 pull_folder_listing rsp, rsp_code Continue, data len 252
         HI 0x48 Len 249
         ...
         FTP client 0x20005a00 pull_folder_listing rsp, rsp_code Success, data len 172
         HI 0x49 Len 169
         ...
         uart:~$

   .. group-tab:: Server

      .. code-block:: console

         FTP server 0x20005c00 pull_folder_listing req, final true
         HI 0xcb Len 4
         Conn ID: 0x00000001
         HI 0x97 Len 1
         OBEX SRM: 0x01
         HI 0x98 Len 1
         OBEX SRMP: 0x01
         HI 0x42 Len 22
         00000000: 78 2d 6f 62 65 78 2f 66  6f 6c 64 65 72 2d 6c 69 |x-obex/f older-li|
         00000010: 73 74 69 6e 67 00                                |sting.           |
         HI 0x01 Len 0
         uart:~$ ftp server pull_folder_listing noerror srm
         FTP server 0x20005c00 pull_folder_listing req, final true
         uart:~$ ftp server pull_folder_listing noerror
         uart:~$ ftp server pull_folder_listing noerror
         uart:~$

Requesting a Named Folder Listing
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. tabs::

   .. group-tab:: Client

      .. code-block:: console

         uart:~$ ftp client pull_folder_listing name docs
         FTP client 0x20005a00 pull_folder_listing rsp, rsp_code Continue, data len 252
         ...
         uart:~$

   .. group-tab:: Server

      .. code-block:: console

         FTP server 0x20005c00 pull_folder_listing req, final true
         HI 0xcb Len 4
         Conn ID: 0x00000001
         HI 0x42 Len 22
         00000000: 78 2d 6f 62 65 78 2f 66  6f 6c 64 65 72 2d 6c 69 |x-obex/f older-li|
         00000010: 73 74 69 6e 67 00                                |sting.           |
         HI 0x01 Len 10
         00000000: 00 64 00 6f 00 63 00 73  00 00                   |.d.o.c.s ..      |
         uart:~$ ftp server pull_folder_listing noerror
         uart:~$

Push File
---------

The client sends the built-in sample file body with the given file name.

RFCOMM Transport
~~~~~~~~~~~~~~~~

.. tabs::

   .. group-tab:: Client

      .. code-block:: console

         uart:~$ ftp client push_file test.txt
         FTP client 0x20005a00 push_file rsp, rsp_code Continue
         uart:~$ ftp client push_file test.txt
         FTP client 0x20005a00 push_file rsp, rsp_code Continue
         uart:~$ ftp client push_file test.txt
         FTP client 0x20005a00 push_file rsp, rsp_code Continue
         uart:~$ ftp client push_file test.txt
         FTP client 0x20005a00 push_file rsp, rsp_code Success
         uart:~$

   .. group-tab:: Server

      .. code-block:: console

         FTP server 0x20005c00 push_file req, final false, data len 252
         HI 0xcb Len 4
         Conn ID: 0x00000001
         HI 0x01 Len 18
         00000000: 00 74 00 65 00 73 00 74  00 2e 00 74 00 78 00 74 |.t.e.s.t ...t.x.t|
         00000010: 00 00                                            |..               |
         HI 0x48 Len 223
         00000000: 54 68 69 73 20 69 73 20  61 20 73 61 6d 70 6c 65 |This is  a sample|
         00000010: 20 46 54 50 20 66 69 6c  65 20 66 6f 72 20 42 6c | FTP fil e for Bl|
         ...
         uart:~$ ftp server push_file noerror
         FTP server 0x20005c00 push_file req, final false, data len 252
         HI 0x48 Len 249
         00000000: 6c 6d 6e 6f 70 71 72 73  74 75 76 77 78 79 7a 0d |lmnopqrs tuvwxyz.|
         00000010: 0a 4c 69 6e 65 20 36 3a  20 54 68 69 73 20 6d 6f |.Line 6:  This mo|
         ...
         uart:~$ ftp server push_file noerror
         FTP server 0x20005c00 push_file req, final false, data len 252
         HI 0x48 Len 249
         00000000: 76 69 67 61 74 69 6f 6e  2e 0d 0a 4c 69 6e 65 20 |vigation ...Line |
         00000010: 39 3a 20 61 6e 64 20 61  64 76 61 6e 63 65 64 20 |9: and a dvanced |
         ...
         uart:~$ ftp server push_file noerror
         FTP server 0x20005c00 push_file req, final true, data len 213
         HI 0x49 Len 210
         00000000: 3a 20 2d 20 46 6f 6c 64  65 72 20 6f 70 65 72 61 |: - Fold er opera|
         00000010: 74 69 6f 6e 73 3a 20 6e  61 76 69 67 61 74 65 2c |tions: n avigate,|
         ...
         uart:~$ ftp server push_file noerror
         uart:~$

L2CAP Transport with SRM
~~~~~~~~~~~~~~~~~~~~~~~~

With SRM enabled, the client sends all PUT requests without waiting for an intermediate
response, and the server answers once, when it receives the final request.

.. tabs::

   .. group-tab:: Client

      .. code-block:: console

         uart:~$ ftp client push_file test.txt srm
         uart:~$ ftp client push_file test.txt
         uart:~$ ftp client push_file test.txt
         uart:~$ ftp client push_file test.txt
         FTP client 0x20005a00 push_file rsp, rsp_code Success
         uart:~$

   .. group-tab:: Server

      .. code-block:: console

         FTP server 0x20005c00 push_file req, final false, data len 252
         HI 0xcb Len 4
         Conn ID: 0x00000001
         HI 0x97 Len 1
         OBEX SRM: 0x01
         HI 0x01 Len 18
         00000000: 00 74 00 65 00 73 00 74  00 2e 00 74 00 78 00 74 |.t.e.s.t ...t.x.t|
         00000010: 00 00                                            |..               |
         HI 0x48 Len 221
         00000000: 54 68 69 73 20 69 73 20  61 20 73 61 6d 70 6c 65 |This is  a sample|
         00000010: 20 46 54 50 20 66 69 6c  65 20 66 6f 72 20 42 6c | FTP fil e for Bl|
         ...
         FTP server 0x20005c00 push_file req, final false, data len 252
         HI 0x48 Len 249
         ...
         FTP server 0x20005c00 push_file req, final false, data len 252
         HI 0x48 Len 249
         ...
         FTP server 0x20005c00 push_file req, final true, data len 215
         HI 0x49 Len 212
         ...
         uart:~$ ftp server push_file noerror srm
         uart:~$

Pull File
---------

The server sends the built-in sample file body.

RFCOMM Transport
~~~~~~~~~~~~~~~~

.. tabs::

   .. group-tab:: Client

      .. code-block:: console

         uart:~$ ftp client pull_file test.txt
         FTP client 0x20005a00 pull_file rsp, rsp_code Continue, data len 252
         HI 0x48 Len 249
         00000000: 54 68 69 73 20 69 73 20  61 20 73 61 6d 70 6c 65 |This is  a sample|
         00000010: 20 46 54 50 20 66 69 6c  65 20 66 6f 72 20 42 6c | FTP fil e for Bl|
         ...
         uart:~$ ftp client pull_file test.txt
         FTP client 0x20005a00 pull_file rsp, rsp_code Continue, data len 252
         HI 0x48 Len 249
         00000000: 68 69 73 20 6d 6f 64 75  6c 65 20 70 72 6f 76 69 |his modu le provi|
         00000010: 64 65 73 20 69 6e 74 65  72 61 63 74 69 76 65 20 |des inte ractive |
         ...
         uart:~$ ftp client pull_file test.txt
         FTP client 0x20005a00 pull_file rsp, rsp_code Continue, data len 252
         HI 0x48 Len 249
         00000000: 61 6e 63 65 64 20 66 65  61 74 75 72 65 73 20 6c |anced fe atures l|
         00000010: 69 6b 65 20 53 69 6e 67  6c 65 20 52 65 73 70 6f |ike Sing le Respo|
         ...
         uart:~$ ftp client pull_file test.txt
         FTP client 0x20005a00 pull_file rsp, rsp_code Success, data len 187
         HI 0x49 Len 184
         00000000: 69 67 61 74 65 2c 20 63  72 65 61 74 65 2c 20 6c |igate, c reate, l|
         00000010: 69 73 74 0d 0a 4c 69 6e  65 20 31 34 3a 20 2d 20 |ist..Lin e 14: - |
         ...
         uart:~$

   .. group-tab:: Server

      .. code-block:: console

         FTP server 0x20005c00 pull_file req, final true
         HI 0xcb Len 4
         Conn ID: 0x00000001
         HI 0x01 Len 18
         00000000: 00 74 00 65 00 73 00 74  00 2e 00 74 00 78 00 74 |.t.e.s.t ...t.x.t|
         00000010: 00 00                                            |..               |
         uart:~$ ftp server pull_file noerror
         FTP server 0x20005c00 pull_file req, final true
         uart:~$ ftp server pull_file noerror
         FTP server 0x20005c00 pull_file req, final true
         uart:~$ ftp server pull_file noerror
         FTP server 0x20005c00 pull_file req, final true
         uart:~$ ftp server pull_file noerror
         uart:~$

L2CAP Transport with SRM
~~~~~~~~~~~~~~~~~~~~~~~~

.. tabs::

   .. group-tab:: Client

      .. code-block:: console

         uart:~$ ftp client pull_file test.txt srm
         FTP client 0x20005a00 pull_file rsp, rsp_code Continue, data len 252
         HI 0x97 Len 1
         OBEX SRM: 0x01
         HI 0x48 Len 247
         00000000: 54 68 69 73 20 69 73 20  61 20 73 61 6d 70 6c 65 |This is  a sample|
         00000010: 20 46 54 50 20 66 69 6c  65 20 66 6f 72 20 42 6c | FTP fil e for Bl|
         ...
         FTP client 0x20005a00 pull_file rsp, rsp_code Continue, data len 252
         HI 0x48 Len 249
         ...
         FTP client 0x20005a00 pull_file rsp, rsp_code Continue, data len 252
         HI 0x48 Len 249
         ...
         FTP client 0x20005a00 pull_file rsp, rsp_code Success, data len 189
         HI 0x49 Len 186
         ...
         uart:~$

   .. group-tab:: Server

      .. code-block:: console

         FTP server 0x20005c00 pull_file req, final true
         HI 0xcb Len 4
         Conn ID: 0x00000001
         HI 0x97 Len 1
         OBEX SRM: 0x01
         HI 0x01 Len 18
         00000000: 00 74 00 65 00 73 00 74  00 2e 00 74 00 78 00 74 |.t.e.s.t ...t.x.t|
         00000010: 00 00                                            |..               |
         uart:~$ ftp server pull_file noerror srm
         uart:~$ ftp server pull_file noerror
         uart:~$ ftp server pull_file noerror
         uart:~$ ftp server pull_file noerror
         uart:~$

L2CAP Transport with SRM and SRMP
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. tabs::

   .. group-tab:: Client

      .. code-block:: console

         uart:~$ ftp client pull_file test.txt srm srmp
         FTP client 0x20005a00 pull_file rsp, rsp_code Continue, data len 252
         HI 0x97 Len 1
         OBEX SRM: 0x01
         HI 0x48 Len 247
         00000000: 54 68 69 73 20 69 73 20  61 20 73 61 6d 70 6c 65 |This is  a sample|
         00000010: 20 46 54 50 20 66 69 6c  65 20 66 6f 72 20 42 6c | FTP fil e for Bl|
         ...
         uart:~$ ftp client pull_file test.txt
         FTP client 0x20005a00 pull_file rsp, rsp_code Continue, data len 252
         HI 0x48 Len 249
         ...
         FTP client 0x20005a00 pull_file rsp, rsp_code Continue, data len 252
         HI 0x48 Len 249
         ...
         FTP client 0x20005a00 pull_file rsp, rsp_code Success, data len 189
         HI 0x49 Len 186
         ...
         uart:~$

   .. group-tab:: Server

      .. code-block:: console

         FTP server 0x20005c00 pull_file req, final true
         HI 0xcb Len 4
         Conn ID: 0x00000001
         HI 0x97 Len 1
         OBEX SRM: 0x01
         HI 0x98 Len 1
         OBEX SRMP: 0x01
         HI 0x01 Len 18
         00000000: 00 74 00 65 00 73 00 74  00 2e 00 74 00 78 00 74 |.t.e.s.t ...t.x.t|
         00000010: 00 00                                            |..               |
         uart:~$ ftp server pull_file noerror srm
         FTP server 0x20005c00 pull_file req, final true
         uart:~$ ftp server pull_file noerror
         uart:~$ ftp server pull_file noerror
         uart:~$ ftp server pull_file noerror
         uart:~$

Delete
------

.. tabs::

   .. group-tab:: Client

      .. code-block:: console

         uart:~$ ftp client delete old.txt
         FTP client 0x20005a00 delete rsp, rsp_code Success
         uart:~$

   .. group-tab:: Server

      .. code-block:: console

         FTP server 0x20005c00 delete req, final true
         HI 0xcb Len 4
         Conn ID: 0x00000001
         HI 0x01 Len 16
         00000000: 00 6f 00 6c 00 64 00 2e  00 74 00 78 00 74 00 00 |.o.l.d.. .t.x.t..|
         uart:~$ ftp server delete success
         uart:~$

Rename
------

The rename operation uses the OBEX action operation with the action ID ``Move/Rename`` (0x01),
the ``Name`` header for the source and the ``Dest Name`` header for the destination. The same
operation moves an object when the destination name contains a path.

.. tabs::

   .. group-tab:: Client

      .. code-block:: console

         uart:~$ ftp client rename test.txt renamed.txt
         FTP client 0x20005a00 rename rsp, rsp_code Success
         uart:~$

   .. group-tab:: Server

      .. code-block:: console

         FTP server 0x20005c00 rename req, final true
         HI 0xcb Len 4
         Conn ID: 0x00000001
         HI 0x94 Len 1
         00000000: 01                                               |.                |
         HI 0x01 Len 18
         00000000: 00 74 00 65 00 73 00 74  00 2e 00 74 00 78 00 74 |.t.e.s.t ...t.x.t|
         00000010: 00 00                                            |..               |
         HI 0x15 Len 24
         00000000: 00 72 00 65 00 6e 00 61  00 6d 00 65 00 64 00 2e |.r.e.n.a .m.e.d..|
         00000010: 00 74 00 78 00 74 00 00                          |.t.x.t..         |
         uart:~$ ftp server rename success
         uart:~$

Copy
----

The copy operation uses the OBEX action operation with the action ID ``Copy`` (0x00).

.. tabs::

   .. group-tab:: Client

      .. code-block:: console

         uart:~$ ftp client copy test.txt copy.txt
         FTP client 0x20005a00 copy rsp, rsp_code Success
         uart:~$

   .. group-tab:: Server

      .. code-block:: console

         FTP server 0x20005c00 copy req, final true
         HI 0xcb Len 4
         Conn ID: 0x00000001
         HI 0x94 Len 1
         00000000: 00                                               |.                |
         HI 0x01 Len 18
         00000000: 00 74 00 65 00 73 00 74  00 2e 00 74 00 78 00 74 |.t.e.s.t ...t.x.t|
         00000010: 00 00                                            |..               |
         HI 0x15 Len 18
         00000000: 00 63 00 6f 00 70 00 79  00 2e 00 74 00 78 00 74 |.c.o.p.y ...t.x.t|
         00000010: 00 00                                            |..               |
         uart:~$ ftp server copy success
         uart:~$

Set Permission
--------------

The permission mask has one octet per group, in the order user, group and other. In each octet,
bit 0 is read, bit 1 is write, bit 2 is delete and bit 7 is modify. The example below grants
read, write and delete to the user and read and delete to the group and to others.

.. tabs::

   .. group-tab:: Client

      .. code-block:: console

         uart:~$ ftp client set_permission test.txt 070505
         FTP client 0x20005a00 set_permission rsp, rsp_code Success
         uart:~$

   .. group-tab:: Server

      .. code-block:: console

         FTP server 0x20005c00 set_permission req, final true
         HI 0xcb Len 4
         Conn ID: 0x00000001
         HI 0x94 Len 1
         00000000: 02                                               |.                |
         HI 0x01 Len 18
         00000000: 00 74 00 65 00 73 00 74  00 2e 00 74 00 78 00 74 |.t.e.s.t ...t.x.t|
         00000010: 00 00                                            |..               |
         HI 0xd6 Len 4
         00000000: 00 07 05 05                                      |....             |
         uart:~$ ftp server set_permission success
         uart:~$

Abort
-----

An ongoing multi-packet operation can be aborted by the client. The abort response resets the
fragmentation state on both sides.

.. tabs::

   .. group-tab:: Client

      .. code-block:: console

         uart:~$ ftp client pull_file test.txt
         FTP client 0x20005a00 pull_file rsp, rsp_code Continue, data len 252
         HI 0x48 Len 249
         ...
         uart:~$ ftp client abort
         FTP client 0x20005a00 abort rsp, rsp_code Success
         uart:~$

   .. group-tab:: Server

      .. code-block:: console

         FTP server 0x20005c00 pull_file req, final true
         HI 0xcb Len 4
         Conn ID: 0x00000001
         HI 0x01 Len 18
         00000000: 00 74 00 65 00 73 00 74  00 2e 00 74 00 78 00 74 |.t.e.s.t ...t.x.t|
         00000010: 00 00                                            |..               |
         uart:~$ ftp server pull_file noerror
         FTP server 0x20005c00 abort req
         HI 0xcb Len 4
         Conn ID: 0x00000001
         uart:~$ ftp server abort success
         uart:~$

Error Responses
---------------

Any server command can send an error response instead of a successful one. The
:code:`[rsp_code]` argument is the OBEX response code in hexadecimal, for example ``c1`` for
``Unauthorized``, ``c3`` for ``Forbidden``, ``c4`` for ``Not Found`` and ``d1`` for
``Not Implemented``.

.. tabs::

   .. group-tab:: Client

      .. code-block:: console

         uart:~$ ftp client delete old.txt
         FTP client 0x20005a00 delete rsp, rsp_code Not Found
         uart:~$

   .. group-tab:: Server

      .. code-block:: console

         FTP server 0x20005c00 delete req, final true
         HI 0xcb Len 4
         Conn ID: 0x00000001
         HI 0x01 Len 16
         00000000: 00 6f 00 6c 00 64 00 2e  00 74 00 78 00 74 00 00 |.o.l.d.. .t.x.t..|
         uart:~$ ftp server delete error c4
         uart:~$

Additional Information
**********************

RFCOMM vs L2CAP Transport
=========================

RFCOMM Transport
----------------

RFCOMM transport is GOEP v1.1. SRM is not available, so for a GET or a PUT operation the same
shell command must be called repeatedly on both sides until the complete object is delivered.
The :code:`srm` argument is ignored on RFCOMM transport.

L2CAP Transport with SRM
------------------------

L2CAP transport is GOEP v2.0 and supports SRM. When SRM is enabled, GET requests do not need to
be repeated and the server sends all response packets. For PUT operations, the client sends all
request packets without waiting for an intermediate response, and the server sends a single
response when it receives the final request.

L2CAP Transport with SRM and SRMP
---------------------------------

When both SRM and SRMP are enabled, GET requests and PUT responses need to continue being sent
until SRMP is no longer added.

SRM (Single Response Mode)
==========================

The :code:`srm` argument adds the OBEX ``SRM`` header with the value ``Enable`` to the first
packet of an operation. It only takes effect on L2CAP transport. The :code:`srmp` argument adds
the OBEX ``SRMP`` header with the value ``Wait``, which suspends single response mode for one
packet.

GET Operations
--------------

When SRM is enabled for GET operations:

* The client sends a single GET request.
* The server sends multiple response packets without waiting for additional requests.
* The :code:`srmp` argument can be used to control the flow (wait for the next request).

PUT Operations
--------------

When SRM is enabled for PUT operations:

* The client sends multiple PUT requests without waiting for additional responses.
* The server sends a single PUT response when it receives the final PUT request.
* The :code:`srmp` argument can be used to control the flow (wait for the next response).

OBEX Authentication
===================

The FTP shell supports mutual OBEX authentication on the OBEX connect operation:

* :code:`ftp server connect unauth <password>` answers with ``Unauthorized`` and an
  authentication challenge that is derived from the password.
* :code:`ftp client connect <password>` answers the challenge with an authentication response
  and adds its own challenge, so that the server authenticates the client as well.
* :code:`ftp server connect success` answers the client challenge with an authentication
  response.
* Both sides verify the digest they receive. On failure the client prints
  ``Authentication failed: ...`` and sends an OBEX disconnect request; the server only prints the
  failure.

The same flow can be triggered with :code:`ftp server connect error c1 <password>`, which is
equivalent to :code:`ftp server connect unauth <password>`.

Object Names
============

File and folder names are converted from ASCII to big-endian UTF-16 before they are added to the
``Name`` and ``Dest Name`` headers, which is why the header dumps above show every character
preceded by a null byte. A name is limited to 63 characters.

Multiple Connection Support
===========================

The shell keeps one client instance and one server instance per ACL connection, indexed by the
connection index. All commands operate on the default connection, which is selected with the
:code:`br select` command.
