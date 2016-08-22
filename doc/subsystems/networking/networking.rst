.. _networking:

Networking
###########

The networking section contains information regarding the network stack
of the Zephyr kernel. Use the information to understand the
principles behind the operation of the stacks and how they were implemented.

The networking stack supports the following features:

* IPv6

  * IPv6 header compresson, which is part of the 6LoWPAN support

* UDP
* IPv4

  * In this version of the IP stack, IPv6 and IPv4 cannot be utilized at the
    same time.
  * DHCP client support for IPv4

* TCP

  * Both client and server roles are supported

* RPL (Ripple) IPv6 mesh routing
* CoAP
* MQTT
* Highly configurable

  * Features, buffer sizes/counts, stack sizes, etc.

Additionally these network technologies are supported:

* IEEE 802.15.4
* Bluetooth
* Ethernet
* SLIP (for testing with Qemu)

Source tree layout
==================

The IP stack source code tree is organized as follows:

``net/ip/``
  The core stack itself. This is where the Contiki uIP stack code
  is located.

``include/net/``
  Public API header files. These are the header files applications need
  to include to use IP networking functionality.

``samples/net/``
  Sample networking code. This is a good reference to get started with
  network application development.

``tests/net/``
  Test applications. These applications are used to verify the
  functionality of the IP stack, but are not the best
  source for sample code (see ``samples/net`` instead).

.. toctree::
   :maxdepth: 1

   ip-stack-architecture.rst
   buffers.rst
