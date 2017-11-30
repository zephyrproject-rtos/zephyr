:orphan:

.. _zephyr_1.10:

Zephyr Kernel 1.10.0
#####################

DRAFT: We are pleased to announce the release of Zephyr kernel version 1.10.0.

Major enhancements with this release include:

* Integration with MCUBOOT Bootloader
* Additional implementation of MMU/MPU support
* Build and Configuration System (CMake)
* Newtron Flash Filesystem (NFFS) Support
* Increased testsuite coverage

The following sections provide detailed lists of changes by component.

Kernel
******

* details ...

Architectures
*************

* nrf52: Add support for LOW_POWER state and SYSTEM_OFF

Boards
******

* details ...

Drivers and Sensors
*******************

* timer: Add Support for TICKLESS KERNEL in xtensa_sys_timer

Networking
**********

* HTTP API changed to use net-app API. Old HTTP API is deprecated.
* Loopback network interface support added. This is used in testing only.
* LWM2M multi-fragment network packet support added.
* New CoAP library implementation which supports longer network packets.
* Old ZoAP library deprecated.
* mDNS (multicast DNS) support added.
* SNTP (Simple Network Time Protocol) client library added.
* TCP fixes.
* RPL fixes.
* ARP fixes.
* DNS fixes.
* LWM2M fixes.
* Ethernet fixes.
* net-app API fixes.
* Network shell fixes.
* BSD socket API fixes.
* Network management API fixes.
* Networking sample application fixes.
* 6lo IPv6 header compression fixes.
* IEEE 802.15.4 generic fixes.
* IEEE 802.15.4 mcr20a driver fixes.
* IEEE 802.15.4 kw41z driver fixes.
* IEEE 802.15.4 nrf5 driver fixes.

Bluetooth
*********

* Multiple qualification-related fixes for Bluetooth Mesh
* Support for Bluetooth Mesh Friend Node role
* Support for Bluetooth Mesh Foundation Client Models
* New Bluetooth Mesh shell module and test application
* Support for PA/LNA amplifiers in the BLE Controller
* Support for additional VS commands in the BLE Controller
* Multiple stability fixes for the BLE Controller

Build and Infrastructure
************************

* The Zephyr project has migrated to CMake, an important step in a
  larger effort to make Zephyr easier to use for application developers
  working on different platforms with different development environment
  needs.  This change retains Kconfig as-is, and replaces all Makefiles
  with corresponding CMakeLists.txt.  The DSL-like Make language that
  KBuild offers is replaced by a set of CMake extensions that provide
  either simple one-to-one translations of KBuild features or introduce
  new concepts that replace KBuild concepts. Please re-read the Getting
  Started guide
  (http://docs.zephyrproject.org/getting_started/getting_started.html)
  with updated instructions for setting up and developing on your host-OS.
  You *will* need to port your own out-of-tree scripts and Makefiles to
  CMake.

Libraries
*********

* details ...

HALs
****

* details ...

Documentation
*************

* Missing API documentation caused by doxygen subgroups and missing
  Sphinx directives now included.
* Note added to release doc pages mentioning more current content could
  be available from the master branch version of the documentation.
* Documentation updated to use CMake (vs. Make) in all examples.
* Getting Started Guide material updated to include CMake dependencies
  and build instructions.
* Instead of hiding all expected warnings from the document build
  process (there are some known doxygen/sphinx issues), the build
  now outputs all warnings, and then reports
  if any new/unexpected warnings or errors were detected.
* Obsolete V1 to V2 porting material removed.
* Continued updates to documentation for new board support, new samples,
  and new features.
* Integration of documentation with new zephyrproject.org website

Tests and Samples
*****************

* details ...

Issue Related Items
*******************

.. comment  List derived from Jira/GitHub Issue query: ...

* :jira:`ZEP-248` - reference a (public) Jira issue
* :github:`1234` - reference a GitHub issue
