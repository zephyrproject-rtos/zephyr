:orphan:

.. _zephyr_1.9:

Zephyr Kernel 1.9.0 (WIP)
#########################

We are pleased to announce the release of Zephyr kernel version 1.9.0
(planned for release in August 2017).

Major enhancements planned with this release include:

* POSIX API Layer
* BSD Socket Support
* Expand Device Tree support to more architectures
* BLE Mesh
* Bluetooth 5.0 Support (all features except Advertising Extensions)
* Expand LLVM Support to more architectures
* Revamp Testsuite, Increase Coverage
* Zephyr SDK NG
* Eco System: Tracing, debugging support through 3rd party tools

These enhancements are planned, but may move out to a future release:

* LWM2M
* Thread Protocol (initial drop)
* MMU/MPU (Cont.): Thread Isolation, Paging
* Build and Configuration System (CMake)


The following sections provide detailed lists of changes by component.

Kernel
******

* change description

Architectures
*************

* change description

Boards
******

* change description

Drivers and Sensors
*******************

* KW40Z IEEE 802.15.4 radio driver support added

Networking
**********

* LWM2M support added
* net-app API support added. This is higher level API that can be used
  by applications to create client/server applications with transparent
  TLS (for TCP) or DTLS (for UDP) support.
* MQTT TLS support added
* Add support to automatically setup IEEE 802.15.4 and Bluetooth IPSP networks
* TCP receive window support added
* Network sample application configuration file unification, where most of the
  similar configuration files were merged together
* Added Bluetooth support to HTTP(S) server sample application
* BSD socket layer fixes and enhancements
* Networking API documentation fixes
* Network shell enhancements
* Trickle algorithm fixes
* CoAP API fixes
* IPv6 fixes
* RPL fixes

Bluetooth
*********

* Bluetooth Mesh support (all mandatory features and most optional ones)
* GATT Service Changed Characteristic support
* IPSP net-app support: a simplified networking API reducing duplication
  of common tasks an application writer has to go through to connect
  to the network.
* BLE controller qualification-ready, with all required tests passing
* Controller-based privacy (including all optional features)
* Extended Scanner Filter Policies support in the controller
* Controller roles (Advertiser, Scanner, Master and Slave) separation in
  source code, conditionally includable
* Flash access cooperation with BLE radio activity

Build and Infrastructure
************************

* change description

Libraries
*********

* change description

HALs
****

* change description

Documentation
*************

* CONTRIBUTING.rst and Contribution Guide material added
* Configuration options doc reorganized for easier access
* Navigation sidebar issues fixed for supported boards section
* Completed migration of wiki.zephyrproject.org content into docs and
  GitHub wiki. All links to old wiki updated.
* Broken link and spelling check scans through .rst, Kconfig (used for
  auto-generated configuration docs), and source code doxygen comments
  (used for API documentation).
* API documentation added for new interfaces and improved for existing
  ones.
* Documentation added for new boards supported with this release.
* Python packages needed for document generation added to new python
  pip requirements.txt


Tests and Samples
*****************

* change description

JIRA Related Items
******************

.. comment  List derived from Jira query: ...

* :jira:`ZEP-000` - Title
