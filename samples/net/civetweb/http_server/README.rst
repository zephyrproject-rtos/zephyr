.. _civetweb-http_server-sample:

Civetweb sample
###############

Overview
********

This sample application uses the HTTP APIs provided by the external `Civetweb <https://github.com/civetweb/civetweb>`_ module to create an HTTP server demonstrating selected Civetweb features.
The Civetweb module is available as a west :ref:`module <modules>`.

The source code for this sample application can be found at:
:zephyr_file:`samples/net/civetweb/http_server`.

Requirements
************

- A board with hardware networking
- The Civetweb module (made available via west)

Building and Running
********************

This sample was tested on the Atmel SAM E70 Xplained board, so this is the recommended target.

Build it with:

.. zephyr-app-commands::
   :zephyr-app: samples/net/civetweb/http_server
   :board: sam_e70_xplained
   :goals: build
   :compact:

The sample application uses a static IP configuration.

After flashing the board, the server can be accessed with the web browser of your choice at ``192.0.2.1:8080``.

The sample does not serve any files like HTTP (it does not use any filesystem).
Instead it serves the following three URLs:

- ``/`` - a basic hello world handler
- ``/info`` - shows OS information, uses the JSON format to achieve that
- ``/history`` - demonstrates the usage of cookies

A regular 404 status code is returned when trying to access any other URL.

The IP configuration can be changed in Zephyr config.
The default port can be changed in the sources of the sample.
