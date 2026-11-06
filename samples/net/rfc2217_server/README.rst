.. zephyr:code-sample:: rfc2217-server
   :name: RFC 2217 server
   :relevant-api: rfc2217

   Serve a UART over TCP to RFC 2217 clients such as pyserial.

Overview
********

This sample serves a UART over TCP with the Telnet COM port control option (:rfc:`2217`). A
client such as pyserial's ``rfc2217://`` exchanges data with the UART and changes its baud rate,
data size, parity and stop bits. The sample only logs the BREAK, DTR and RTS requests: a real
application would act on the hardware there, for example on the reset and boot pins of the board
at the other end of the UART.

The server is defined in the devicetree, by a ``zephyr,rfc2217-server`` node, and starts at boot.
Built with :file:`api.overlay`, which has no such node, the application starts the server itself,
on the UART with the ``rfc2217-uart`` alias and on port
:kconfig:option:`CONFIG_SAMPLE_RFC2217_PORT`.

The sample gets on the network as the other networking samples do: with the static addresses
192.0.2.1 and 2001:db8::1, which :file:`prj.conf` sets.

Requirements
************

- A board with networking and a spare UART to serve, or :zephyr:board:`native_sim`. On
  :zephyr:board:`native_sim`, the sample serves ``uart_1``, which the host sees as a
  pseudoterminal.
- For :zephyr:board:`native_sim`, a network interface on the host: see
  :ref:`networking_with_host`. Alternatively, the sample can use the host's network stack, see
  below.
- An RFC 2217 client, for example pyserial (``pip install pyserial``).

Building and Running
********************

native_sim
==========

Set up the ``zeth`` interface on the host as :ref:`networking_with_host` describes, then build
and run the sample:

.. zephyr-app-commands::
   :zephyr-app: samples/net/rfc2217_server
   :host-os: unix
   :board: native_sim
   :goals: run
   :compact:

Without the ``zeth`` interface, :file:`overlay-nsos.conf` makes the sample use the host's network
stack (offloaded sockets), and clients connect to 127.0.0.1 instead of 192.0.2.1:

.. zephyr-app-commands::
   :zephyr-app: samples/net/rfc2217_server
   :host-os: unix
   :board: native_sim
   :gen-args: -DEXTRA_CONF_FILE=overlay-nsos.conf
   :goals: run
   :compact:

To have the application start the server instead of the devicetree, add
``-DDTC_OVERLAY_FILE=api.overlay``.

The output gives the pseudoterminal of ``uart_1`` and the server's port:

.. code-block:: console

   uart_1 connected to pseudotty: /dev/pts/5
   uart connected to pseudotty: /dev/pts/6
   *** Booting Zephyr OS build v4.5.0 ***
   [00:00:00.010,000] <inf> rfc2217_server_sample: Serving uart_1 on TCP port 2217

The pseudoterminal is the other end of ``uart_1``: what a client sends to the server comes out
there, and what is typed there reaches the client. Open it, for example with
``screen /dev/pts/5`` or ``socat - /dev/pts/5,raw,echo=0``, then connect a client.

With pyserial
-------------

pyserial's terminal speaks the whole protocol:

.. code-block:: console

   python -m serial.tools.miniterm rfc2217://192.0.2.1:2217 115200

What one side types appears on the other. In the terminal, :kbd:`Ctrl+T` followed by
:kbd:`Ctrl+D`, :kbd:`Ctrl+R` or :kbd:`Ctrl+B` toggles DTR, RTS or BREAK, and the sample logs the
request.

With socat
----------

socat connects as a plain TCP client, without Telnet. The line settings, DTR, RTS and BREAK are
out of its reach, the server's opening negotiation arrives as 12 bytes before the data, and 0xff
bytes do not go through unchanged. That is enough for a text console:

.. code-block:: console

   socat - TCP:192.0.2.1:2217

With the same limits, socat can also create a local serial device linked to the server, for a
tool that only opens a device:

.. code-block:: console

   socat PTY,link=/tmp/ttyRFC2217,raw,echo=0 TCP:192.0.2.1:2217

Other boards
============

On other boards, a devicetree overlay adds the ``zephyr,rfc2217-server`` node. The sample has one
for :zephyr:board:`nucleo_f767zi`: it serves the serial port of the Arduino header
(``arduino_serial``) on port 2217.

.. zephyr-app-commands::
   :zephyr-app: samples/net/rfc2217_server
   :board: nucleo_f767zi
   :goals: build flash
   :compact:

The board has the same addresses as on :zephyr:board:`native_sim`. From a host on its network,
connect a client to it:

.. code-block:: console

   python -m serial.tools.miniterm rfc2217://192.0.2.1:2217 115200
