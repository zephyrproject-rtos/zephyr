:orphan:

.. _zephyr_1.11:

Zephyr Kernel 1.11.0
#####################

We are pleased to announce the release of Zephyr kernel version 1.11.0.

Major enhancements with this release include:

* Thread-level memory protection on x86, ARC and ARM, userspace and memory
  domains
* Symmetric Multi Processing (SMP) support on the Xtensa architecture.
* Initial Armv8-M architecture support.
* Native development environment on Microsoft Windows.
* Native build target on POSIX platforms.
* POSIX PSE52 partial support.
* Thread support via integration with OpenThread.
* Over the Air Firmware Updates over BLE using MCUmgr.
* Lightweight flash storage layer for constrained devices.

* Additional SoC, platform and driver support for many of the already supported
  platforms.

The following sections provide detailed lists of changes by component.

Kernel
******

* Initial Symmetric Multi Processing (SMP) support added:
  * SMP-aware scheduler
  * SMP timer and idling support
  * Available on the Xtensa architecture
* POSIX PSE52 support:
  * Timer, clock, scheduler and pthread APIs

Architectures
*************

* User space and system call related changes:
  * Added ARC user space implementation
  * Added ARM user space implementation
  * Fixed a few MPU related issues with stack guards
* Armv8-M iniital architecture support, including the following cores:
  * Arm Cortex-M23
  * Arm Cortex-M33
* New "posix" architecture for native GNU/Linux and macOS build targets:
  * Targets native executables that can be run on the operating system

Boards
******

* New native_posix board for the posix architecture:
  * Includes a template for hardware models
  * Adds support for console and logging
  * Interrupts and timers are simulated in several different configurations

Drivers and Sensors
*******************

* New LED PWM driver for ESP32 SoC
* Fixed ESP32 I2C driver

Networking
**********

* Generic OpenThread support added
* OpenThread support to nrf5 IEEE 802.15.4 driver added
* NXP MCUX ethernet driver IPv6 multicast join/leave enhancements
* Ethernet stm32 fixes
* IEEE 802.15.4 Sub-GHz TI CC1200 chip support added
* IEEE 802.15.4 test driver (upipe) hw filtering support added
* IEEE 802.15.4 radio API enhancements
* Net loopback driver fixes
* Net management API event enhancements
* IPv6 neighbor addition and removal can be monitored
* Static IPv4 and DHCPv4 configuration enhancements
* Bluetooth IPSP disconnect fix
* Network buffer enhancements
* ICMPv4 and ICMPv6 error checking fixes
* Network interface address handling enhancements
* Add routing support between network interfaces
* LWM2M fixes and enhancements
* Old legacy HTTP API removed
* Old legacy ZoAP API removed
* CoAP fixes
* TCP fixes
* HTTP fixes
* RPL fixes
* Net-app API fixes
* Net-shell fixes
* BSD socket API fixes

Bluetooth
*********

* Multiple fixes to the controller
* Fixed potential connection transmission deadlock issue with the help
  of a dedicated fragment pool
* Multiple fixes to Mesh support
* Added test automation for Mesh (for tests/bluetooth/tester)

Build and Infrastructure
************************

* Native development enviroment on Microsoft Windows:
  * Uses CMake and Kconfiglib to avoid requiring an emulation layer
  * Package management support with Chocolatey for simple setup
  * Build time now comparable to Linux and macOS using Ninja

Libraries / Subsystems
***********************

* New management subystem based on the cross-RTOS MCUmgr:
  * Secure Firmware Updates over BLE and serial
  * Support for file system access and statistics
  * mcumgr cross-platform command-line tool

* FCB (File Circular Buffer) lightweight storage layer:
  * Wear-levelling support for NOR flashes
  * Suitable for memory constrained devices

HALs
****

Documentation
*************

* Added MPU specific stack and userspace documentation
* Improved docs for Native (POSIX) support
* Docs for new samples and supported board
* General documentation clarifications and improvements
* Addressed Sphinx-generated intra-page link issues
* Updated doc generation tools (Doxygen, Sphinx, Breathe, Docutils)

Tests and Samples
*****************

* Added additional tests and test improvements for user space testing

Issue Related Items
*******************

These GitHub issues were addressed since the previous 1.10.0 tagged
release:

.. comment  List derived from Jira/GitHub Issue query: ...


