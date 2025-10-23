.. zephyr:code-sample:: ot-coap
   :name: OpenThread CoAP client and server application
   :relevant-api: openthread

   Build a Full Thread Device (FTD) CoAP server and client.

Overview
********

This sample demonstrates how to use OpenThread CoAP API.
It can be built to work as a server or as a client.

By running a client and server on two boards, a local Thread network can be created.
To create the network, OpenThread uses the network key provided with Kconfig.
Once the boards have been flashed, the network will be
automatically created and configured.

Once the network is operational, then the client could start interacting with
the server.
Every time the user presses the button, the LED on server should toggle.

The source code for this sample application can be found at:
:zephyr_file:`samples/net/openthread/coap`.

.. note::
   This sample uses the OpenThread CoAP API whereas Zephyr has its own CoAP API.
   So, why are we using the OpenThread CoAP API here ?

   * OpenThread uses it internally to implement many of its services.
   * OpenThread CoAP API has a more direct access to radio.

   So by using OpenThread CoAP API instead of Zephyr one,
   we could expect less overhead although this makes the application less portable.

Building and Running
********************

Build the OpenThread FTD CoAP server sample application like this:

.. zephyr-app-commands::
   :zephyr-app: samples/net/openthread/coap
   :board: <board to use>
   :west-args: -T sample.net.openthread.ftd.coap.server
   :goals: build
   :compact:

Build the OpenThread FTD CoAP client sample application like this:

.. zephyr-app-commands::
   :zephyr-app: samples/net/openthread/coap
   :board: <board to use>
   :west-args: -T sample.net.openthread.ftd.coap.client
   :goals: build
   :compact:

Example building CoAP server for the cc1352p7 launchpad:

.. zephyr-app-commands::
   :zephyr-app: samples/net/openthread/coap
   :host-os: unix
   :board: cc1352p7_lp
   :west-args: -T sample.net.openthread.ftd.coap.server
   :goals: build flash
   :compact:

Example building CoAP client for the cc1352p7 launchpad:

.. zephyr-app-commands::
   :zephyr-app: samples/net/openthread/coap
   :host-os: unix
   :board: cc1352p7_lp
   :west-args: -T sample.net.openthread.ftd.coap.client
   :goals: build flash
   :compact:

Checking Thread network state
*****************************

Open a console on both server and client boards then check the sate:

.. code-block:: console

   server:~$ ot state
   router
   Done

A valid state could be child, router or leader.

Once Thread network is operational, you could start using client.

Controlling server board's LED using a button on client board
*************************************************************

There is nothing to do once Thread network is operational.
Just press the button sw0 on the client and you should see led0 toggling.

The client uses a broadcast address to request CoAP server to toggle the LED.
It does not know the address of the server so if there is a second server
on the network, then the LED of the second board will toggle too.

Controlling server board's LED from a computer
**********************************************

Although we use OpenThread CoAP API, we could interact with any CoAP client
or server available on network. In this example, we are going to control the
LED from a computer that is not in the Thread network.
This requires an `OpenThread Border Router`_ with NAT64 support enabled on the same network.

First, check that the server (or the client) is connected to the otbr and
can use NAT64:

.. code-block:: console

   server:~$ ot netdata show
   router
   Done
   Prefixes:
   fd6f:cb3a:802:1::/64 paos low dc00
   Routes:
   fc00::/7 sa med dc00
   fd6f:cb3a:802:2:0:0::/96 sn low dc00
   Services:
   44970 01 14000500000e10 s dc00 0
   44970 5d fd78b9ce54779c6eb5484d062c3b5b22d120 s dc00 1
   Contexts:
   fd6f:cb3a:802:1::/64 1 c
   Commissioning:
   11426 - - -
   Done

Prefixes show the IPv6 prefies that could be used by device outside the
Thread network to contact devices on Thread network.

We should have an IPv6 address using the prefix:

.. code-block:: console

   server:~$ ot ipaddr
   fd78:b9ce:5477:9c6e:0:ff:fe00:a800
   fd6f:cb3a:802:1:f0ec:c1e2:c1bb:744
   fd78:b9ce:5477:9c6e:75b8:386c:1f79:1013
   fe80:0:0:0:50d1:bed5:6e6e:ad75
   Done

fd6f:cb3a:802:1:f0ec:c1e2:c1bb:744 is the IPv6 address that could be used
to contact the CoAP server outside of the Thread network.

We could also check that we could access internet from Thread network:

.. code-block:: console

   server:~$ ot ping 8.8.8.8
   Pinging synthesized IPv6 address: fd6f:cb3a:802:2:0:0:808:808
   16 bytes from fd6f:cb3a:802:2:0:0:808:808: icmp_seq=1 hlim=114 time=36ms
   1 packets transmitted, 1 packets received. Packet loss = 0.0%. Round-trip min/avg/max = 36/36.0/36 ms.
   Done

If everything is working, then, we could start controlling the LED from a computer.
To do that, let's use aiocoap-client, a tool written in python.
First, install it:

.. code-block:: shell

   pip install aiocoap


Then, send a request to the server to toggle the LED:

.. code-block:: shell

    aiocoap-client -m PUT --payload '{"led_id":0,"state":2}'  coap://[fd6f:cb3a:802:1:f0ec:c1e2:c1bb:744]/led

The LED state should have changed.


.. _OpenThread Border Router: https://openthread.io/codelabs/openthread-border-router-nat64

Controlling server board's LED using shell command
**************************************************

The example also provides a shell command to control the LED on the server from the client.

To toggle the LED:

.. code-block::

   $client:~$ ot_coap led set 0 toggle

The LED state should have changed.

Same as for the button, this uses the broadcast address by default.
To control the LED of a specific server, we can use it IPv6 address:

.. code-block::

   $client:~$ ot_coap led set 0 toggle fd6f:cb3a:802:1:f0ec:c1e2:c1bb:744
