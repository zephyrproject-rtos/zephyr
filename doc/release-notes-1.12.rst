:orphan:

.. _zephyr_1.12:

Zephyr Kernel 1.12.0 (DRAFT)
############################

We are pleased to announce the release of Zephyr kernel version 1.12.0.

Major enhancements with this release include:

- 802.1Q - Virtual Local Area Network (VLAN) traffic on an Ethernet network
- Support multiple concurrent filesystem devices, partitions, and FS types
- Support for TAP net device on the the native POSIX port
- SPI slave support
- Runtime non-volatile configuration data storage system


The following sections provide detailed lists of changes by component.

Kernel
******

* Added k_thread_foreach API

Architectures
*************

* nxp_imx/mcimx7_m4: Added support for i.MX7 Cortex M4 core

Boards
******

* nios2: Added device tree support for qemu_nios2 and altera_max10
* colibri_imx7d_m4: Added support for Toradex Colibri i.MX7 board

Drivers and Sensors
*******************

* serial: Added support for i.MX UART interface
* gpio: Added support for i.MX GPIO

Networking
**********

* Minimal server side websocket support.
* Add network support to syslog.
* Reducing net_pkt RAM usage.
* TCP code refactoring. TCP code is now in one place in tcp.c
* Support MSG_DONTWAIT and MSG_PEEK in recvfrom() socket call.
* Support MSG_DONTWAIT in sendto() socket call.
* Add support for freeaddrinfo() API.
* Allow empty service in getaddrinfo() API.
* Add PRIORITY support to net_context. This is working same way as SO_PRIORITY
  in BSD sockets API.
* Add network traffic classification support to Rx and Tx paths. This allows
  priorization of incoming or outgoing network traffic. Both Rx and Tx can
  have max 8 network queues.
* Add network interface up/down command to net-shell.
* Create ethernet driver for native_posix board. The driver is enabled
  automatically if networking is active when compiling for native_posix board.
* Support network packet checksum calculation offloading. This is available for
  ethernet based boards.
* Add support for ethernet virtual LANs (VLAN). Following ethernet drivers
  support VLANs: frdm_k64f, sam_e70_explained, native_posix and qemu.
* Allow network statistics collection / network interface.
* Add network management support to ethernet sub-system.
* Add network capabilities support to ethernet network drivers. This is used
  for management purposes.
* Allow collection of ethernet statistics. Currently only native_posix ethernet
  driver supports this.
* Add OpenThread support for KW41Z driver.
* Add initial WiFi management API definitions.
* Add a shell module for controlling WiFi devices.
* Add dedicated net mgmt hooks for WiFi offload devices.
* Use proper IPv4 source address when sending IPv4 packets.
* Add support for energy detection scan on IEEE 802.15.4 driver API.
* Add support for filtering source short IEEE 802.15.4 addresses.
* Add RPL border router sample application.
* LWM2M code refactoring.
* LWM2M OPTIONAL resource fixes.
* LWM2M source port fixes.
* LWM2M resource usage enhancements.
* Fixing network management event ordering.
* Fix ENC28J70 ethernet driver.
* CoAP sample application fixes.
* Network timeout fixes.
* ICMPv6 error check fixes.
* Net-app API port number fixes.
* WPAN USB driver and sample application fixes.
* BSD socket sample application fixes.
* Fix IPv4 echo-request (ping) in net-shell when having multiple network
  interfaces.
* Fixing IPv6 compile error in certain configuration.

Bluetooth
*********


Build and Infrastructure
************************


Libraries / Subsystems
***********************

* subsys/disk: Added support for multiple disk interfaces
* subsys/fs: Added support for multiple instances of filesystem
* subsys/fs: Added Virtual File system Switch (VFS) support

HALs
****

* nxp/imx: imported i.MX7 FreeRTOS HAL

Documentation
*************


Tests and Samples
*****************


Issue Related Items
*******************

These GitHub issues were addressed since the previous 1.11.0 tagged
release:

.. comment  List derived from GitHub Issue query: ...
   * :github:`issuenumber` - issue title
