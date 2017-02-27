.. _ip_stack_overview:

Overview
########

Supported Features
******************

The networking stack supports the following features:

* IPv6

  * IPv6 header compression, which is part of the 6LoWPAN support

* UDP
* IPv4

  * DHCP client support for IPv4

* IPv6 and IPv4 are supported at the same time.
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

Source Tree Layout
******************

The IP stack source code tree is organized as follows:

``subsys/net/ip/``
  This is where the IP stack code is located.

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
