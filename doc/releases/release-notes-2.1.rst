:orphan:

.. _zephyr_2.1:

Zephyr 2.1.0 (Working Draft)
############################

We are pleased to announce the release of Zephyr kernel version 2.1.0.

Major enhancements with this release include:

* TBD

The following sections provide detailed lists of changes by component.

Kernel
******

* TBD

Architectures
*************

* TBD

Boards & SoC Support
********************

* TBD

Drivers and Sensors
*******************

* TBD

Networking
**********

* Added new TCP stack implementation. The new TCP stack is still experimental
  and is turned off by default. Users wanting to experiment with it can set
  :option:`CONFIG_NET_TCP2` Kconfig option.
* Added support for running MQTT protocol on top of a Websocket connection.
* Added infrastructure for testing network samples in automatic way.
* Added support for enabling DNS in LWM2M.
* Added support for resetting network statistics in net-shell.
* Added support for getting statistics about the time it took to receive or send
  a network packet.
* Added support for sending a PPP Echo-Reply packet when a Echo-Request packet
  is received.
* Added CC13xx / CC26xx device drivers for IEEE 802.15.4.
* Added TCP/UDP socket offload with TLS for eswifi network driver.
* Added support for sending multiple SNTP requests to increase reliability.
* Added support for choosing a default network protocol in socket() call.
* Added support for selecting either native IP stack, which is the default, or
  offloaded IP stack. This can save ROM and RAM as we do not need to enable
  network functionality that is not going to be used in the network device.
* Added support for LWM2M client initiated de-register.
* Updated the supported version of OpenThread.
* Updated OpenThread configuration to use mbedTLS provided by Zephyr.
* Various fixes to TCP connection establishment.
* Fixed delivery of multicast packets to all listening sockets.
* Fixed network interface initialization when using socket offloading.
* Fixed initial message id seed value for sent CoAP messages.
* Fixed selection of network interface when using "net ping" command to send
  ICMPv4 echo-request packet.
* Networking sample changes for:

  .. rst-class:: rst-columns

     - http_client
     - dumb_http_server_mt
     - dumb_http_server
     - echo_server
     - mqtt_publisher
     - zperf

* Network device driver changes for:

  .. rst-class:: rst-columns

     - Ethernet enc424j600 (new driver)
     - Ethernet enc28j60
     - Ethernet stm32
     - WiFi simplelink
     - Ethernet DesignWare (removed)

Bluetooth
*********

* TBD

Build and Infrastructure
************************

* Deprecated kconfig functions dt_int_val, dt_hex_val, and dt_str_val.
  Use new functions that utilize eDTS info such as dt_node_reg_addr.
  See :zephyr_file:`scripts/kconfig/kconfigfunctions.py` for details.

* Deprecated direct use of the ``DT_`` Kconfig symbols from the generated
  ``generated_dts_board.conf``.  This was done to have a single source of
  Kconfig symbols coming from only Kconfig (additionally the build should
  be slightly faster).  For Kconfig files we should utilize functions from
  :zephyr_file:`scripts/kconfig/kconfigfunctions.py`.  See
  :ref:`kconfig-functions` for usage details.  For sanitycheck yaml usage
  we should utilize functions from
  :zephyr_file:`scripts/sanity_chk/expr_parser.py`.  Its possible that a
  new function might be required for a particular use pattern that isn't
  currently supported.

* Various parts of the binding format have been simplified. The format is
  better documented now too.

  See :ref:`legacy_binding_syntax` for more information.

Libraries / Subsystems
***********************

* TBD

HALs
****

* TBD

Documentation
*************

* TBD

Tests and Samples
*****************

* TBD

Issue Related Items
*******************

These GitHub issues were addressed since the previous 2.0.0 tagged
release:

.. comment  List derived from GitHub Issue query: ...
   * :github:`issuenumber` - issue title

* :github:`99999` - issue title
