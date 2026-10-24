.. SPDX-FileCopyrightText: Copyright 2026 NXP
.. SPDX-License-Identifier: Apache-2.0

Bluetooth: Classic: OPP Shell
#############################

This document describes how to run the Bluetooth Classic OPP (Object Push Profile) functionality.
The :code:`opp` command exposes the Bluetooth Classic OPP shell commands. See :c:group:`bt_opp`
for the API.

The client commands require :kconfig:option:`CONFIG_BT_OPP_CLIENT`, and the commands starting
with :code:`server_` require :kconfig:option:`CONFIG_BT_OPP_SERVER`. When only one of them is
enabled, :code:`opp` lists only that group. :zephyr_file:`tests/bluetooth/shell/prj_br.conf`
enables both.

Commands
********

The :code:`opp` commands:

.. code-block:: console

   uart:~$ opp
   opp - Bluetooth OPP shell commands
   Subcommands:
     discover               : Discover OPP service RFCOMM channel via SDP
     transport_connect      : Connect RFCOMM using SDP-discovered channel
     transport_disconnect   : Disconnect RFCOMM
     connect                : [mopl]  Establish OBEX connection
     disconnect             : Terminate OBEX connection
     push                   : <vcard|vcal|vnote|vmsg>  Push a demo object to the
                              server
     pull                   : Pull the server's default business card
     exchange               : Business Card Exchange: push own vCard then pull
                              server's
     abort                  : Abort the current operation
     server_register        : Register OPP Push Server (RFCOMM, OPP 1.1)
     server_connect_rsp     : [success|reject] [mopl]  Send OBEX CONNECT response
     server_disconnect_rsp  : Send OBEX DISCONNECT response
     server_push_rsp        : [continue|success|reject|toobig|unsupported]  Send
                              PUT response (default: success if final, else
                              continue)
     server_pull_rsp        : [success|reject|notfound]  Send GET response for
                              business card pull
     server_abort_rsp       : Send ABORT response

:code:`opp discover` and :code:`opp transport_connect` use the default ACL connection, which has
to be established first, for example with :code:`br connect <address>`. The
:code:`server_*_rsp` commands can only be used after a client has connected the transport. The
server handles one client at a time. It does not answer the requests it receives automatically:
for each request it prints the command that answers it. Requests that the profile rejects on its
own are answered by the stack and not reported.

Connect
*******

The server has to be connectable and registered before the client connects:

.. tabs::

   .. group-tab:: Client

      .. code-block:: console

         uart:~$ br connect <address>
         uart:~$ opp discover
         uart:~$ opp transport_connect
         uart:~$ opp connect

   .. group-tab:: Server

      .. code-block:: console

         uart:~$ br pscan on
         uart:~$ opp server_register
         uart:~$ opp server_connect_rsp success

Push an Object
**************

:code:`opp push` sends a built-in object of the given type in a single PUT request with the final
bit set, which the server answers with :code:`success`. A PUT request without the final bit,
which a client sends when it splits an object over several requests, is answered with
:code:`continue`.

.. tabs::

   .. group-tab:: Client

      .. code-block:: console

         uart:~$ opp push vcard

   .. group-tab:: Server

      .. code-block:: console

         uart:~$ opp server_push_rsp success

Pull the Default Business Card
******************************

:code:`opp pull` requests the server's default business card, and
:code:`opp server_pull_rsp success` answers it with the built-in vCard of the shell:

.. tabs::

   .. group-tab:: Client

      .. code-block:: console

         uart:~$ opp pull

   .. group-tab:: Server

      .. code-block:: console

         uart:~$ opp server_pull_rsp success

Business Card Exchange
**********************

:code:`opp exchange` pushes a built-in vCard and, once the server has answered the push with
success, pulls the server's default business card:

.. tabs::

   .. group-tab:: Client

      .. code-block:: console

         uart:~$ opp exchange

   .. group-tab:: Server

      .. code-block:: console

         uart:~$ opp server_push_rsp success
         uart:~$ opp server_pull_rsp success

Disconnect
**********

The client closes the OBEX connection first and then the transport:

.. tabs::

   .. group-tab:: Client

      .. code-block:: console

         uart:~$ opp disconnect
         uart:~$ opp transport_disconnect

   .. group-tab:: Server

      .. code-block:: console

         uart:~$ opp server_disconnect_rsp
