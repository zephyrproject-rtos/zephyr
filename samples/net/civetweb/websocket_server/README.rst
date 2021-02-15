.. _civetweb-websocket-server-sample:

Civetweb WebSocket Server sample
################################

Overview
********

This sample application uses the HTTP APIs provided by the external
`Civetweb <https://github.com/civetweb/civetweb>`_ module to create an WebSocket
server demonstrating selected Civetweb features.
The Civetweb module is available as a west :ref:`module <modules>`.

The source code for this sample application can be found at:
:zephyr_file:`samples/net/civetweb/websocket_server`.

Requirements
************

- A board with hardware networking
- The Civetweb module (made available via west)

Building and Running
********************

This sample was tested on the NUCLEO H745ZI-Q board, so this is the recommended target.

Build it with:

.. zephyr-app-commands::
   :zephyr-app: samples/net/civetweb/websocket_server
   :board: nucleo_h745zi_q_m7
   :goals: build
   :compact:

The sample application uses a static IP configuration.

After flashing the board, the server can be accessed with the web browser
of your choice (preferably Chrome) under ``192.0.2.1`` IPv4 address.
The IP address can be changed in :zephyr_file:`samples/net/civetweb/websocket_server/prj.conf`
The port number can be changed in :zephyr_file:`samples/net/civetweb/websocket_server/main.c`

This sample application consists of two main parts:

- **HTTP Server** - ``http://192.0.2.1:8080`` or ``http://192.0.2.1:8080/index.html`` It is needed to serve application's main page and its dependencies.
- **WebSocket Server** - ``ws://192.0.2.1:8080/ws``. It is an echo server, which sends recived data enclosed in **board name** and a string **too!** back.

The **HTTP Server*** serves following statically allocated files
(*no file system is present*):

- ``/`` - main application page (redirects requests to ``/index.html``)
- ``/index.html`` - main application page
- ``/index.css`` - main application page style sheet
- ``/ws.js`` - WebSocket client JavaScript

A regular 404 status code is returned when trying to access other links.

The **WebSocket Server** works as follows:

Calling the ``http://192.0.2.1:8080`` in your browser provides WebSocket
client JavaScript load. This script establishes the connection to the WebSocket
server running on your board.
Putting some message in ```Message Text``` window and pressing *Send* button generates
predefined answer from WebSocket server printed in log window.
