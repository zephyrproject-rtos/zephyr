.. _zephyr_1.5:

Zephyr Kernel 1.5.0
####################

The Zephyr Engineering team is glad to announce the release of Zephyr Kernel
1.5.0. This is the first release to follow the 3-month release cadence.
This release includes numerous fixes and support for major features.
Additional changes include support for new drivers, sensors, and boards.

Major enhancements included with the release:

- TCP Support
- Integration of the Paho MQTT Library support with QoS
- Flash Filesystem Support
- Integration of the mbedTLS library for encryption
- Improved BR/EDR support (for L2CAP, in particular).
- Support for the Altera Nios II/f soft CPU architecture

A detailed list of changes since v1.4.0 by component follows:

Kernel
******

- Added nano_fifo_put_list() APIs, which allows queuing  a list of elements
  on a nanokernel FIFO.
- Removed unused memory pool structure field.
- Enhanced memory pool code.

Architectures
*************

- ARM: Updated to include floating point registers.
- Altera Nios II/f soft CPU architecture support
   -  Internal Interrupt Controller
   -  Avalon Timer
   -  Avalon JTAG UART (polling mode) as default for qemu-system-nios2,
      and 16550 UART as default for Altera MAX10.

Boards
******

- Added Nios II QEMU board.
- Added configuration for Altera MAX10 FPGA.

Drivers and Sensors
*******************

- Sensors: Added driver for I2C HMC5883L magnetometer.
- Sensors: Added driver for I2C TMP112 temperature sensor.
- Sensors: Added driver for MAX44009 light sensor.
- Sensors: Added driver for LPS25HB.
- HAL: Updated QMSI drivers to 1.1
- Added DMA QMSI shim driver.
- Added Quark SE USB device controller driver.
- Added suspend/resume to QMSI drivers.
- Added Guard for critical sections of the QMSI drivers.
- Added Zephyr File System API.
- Added driver for ENC28J60 Ethernet SPI module.

Networking
**********

- TCP Support
- Connection handling fixes in IP stack.
- Allow sending zero length user data IP packet.

Network Buffers

- New net_buf_simple API for light-weight on-stack (or static) buffers where a
  net_buf (and its associated pool) is overkill. The net_buf API now uses as
  an internal implementation detail net_buf_simple.
- Add support for network buffer fragmentation.
- Add more net_buf big endian helpers.

Bluetooth
*********

- Multiple fixes & improvements to the nble driver.
- New API for dealing with Out of Band data (like the local address).
- Various smaller fixes & improvements in many places.

Build and Infrastructure
************************

- Added "qemugdb" target to start a local GDB on port 1234.
- Added script to filter known issues in the build output.
- Sanity: Added "-R" option to build all test with assertions.

Libraries
*********

- File system: Imported Open Source FAT FS 0.12a code.
- Encryption: Imported mbedTLS library.
- Encryption: Updated TinyCrypt library to 2.0.

Documentation
*************

- Fixed all the documentations warnings during build.
- Fixed several typos, trademarks and grammar.
- Moved all the boards documentation to the wiki.
- Moved Code Contribution documentation to the wiki.
- Added package "ncurses" to the list of requirements.
- Updated macOS instructions.

Test and Samples
****************

- Samples: Replaced old debug macro to use new SYS_LOG macro.
- Added TMP112 sensor application.
- Added Quark SE power management sample application.
- Added DMA memory to memory transfer sample.
- Added sample for MAX44009 light sensor.
- Added MQTT publisher and subscriber samples.
- Added mbedTLS sample client.

JIRA Related Items
******************


Stories
========

* :jira:`ZEP-49` - x86: unify separate SysV and IAMCU code
* :jira:`ZEP-55` - enable nanokernel test_context on ARC
* :jira:`ZEP-58` - investigate use of -fomit-frame-pointer
* :jira:`ZEP-60` - irq priorities should be rebased to safe values
* :jira:`ZEP-69` - Extend PWM API to use arbitrary unit of time
* :jira:`ZEP-203` - clean up APIs for static exceptions
* :jira:`ZEP-225` - Add kernel API to put SoC to Deep Sleep (DS) State
* :jira:`ZEP-226` - Update sample PMA to support device suspend/resume
* :jira:`ZEP-227` - Add kernel API to put SoC to Low Power State (LPS)
* :jira:`ZEP-228` - File system interface designed after POSIX
* :jira:`ZEP-232` - Support for USB communications device class ACM
* :jira:`ZEP-234` - provide a direct memory access (DMA) interface
* :jira:`ZEP-243` - Create Wiki Structure for Boards
* :jira:`ZEP-249` - nios2: Enable altera_max10 board in sanitycheck runs for nanokernel
* :jira:`ZEP-254` - nios2: define NANO_ESF struct and populate _default_esf
* :jira:`ZEP-270` - nios2: determine optimal value for PERFOPT_ALIGN
* :jira:`ZEP-271` - nios2: enable microkernel & test cases
* :jira:`ZEP-272` - nios2: add global pointer support
* :jira:`ZEP-273` - nios2: implement flashing scripts
* :jira:`ZEP-274` - nios2: document GDB debugging procedure
* :jira:`ZEP-275` - nios2: scope support for instruction/data caches
* :jira:`ZEP-279` - nios2: demonstrate nanokernel hello world
* :jira:`ZEP-285` - FAT filesystem support on top of SPI Flash
* :jira:`ZEP-289` - nios2: implement kernel_event_logger
* :jira:`ZEP-291` - Driver for the ENC28J60 ethernet device
* :jira:`ZEP-304` - Investigate QEMU support for Nios II
* :jira:`ZEP-327` - Encryption Libraries needed for Thread support
* :jira:`ZEP-340` - TLS/SSL
* :jira:`ZEP-354` - Provide a DMA driver for Quark SE core
* :jira:`ZEP-356` - DMA device support
* :jira:`ZEP-357` - Support for the MAX44009 sensor
* :jira:`ZEP-358` - Add support for TMP112 sensor
* :jira:`ZEP-412` - Add driver API reentrancy support to RTC driver for LMT
* :jira:`ZEP-414` - Add driver API reentrancy support to flash driver
* :jira:`ZEP-415` - aaU, I want to use the NATS messaging protocol to send sensor data to the cloud
* :jira:`ZEP-416` - MQTT client capability: QoS1, QoS2
* :jira:`ZEP-424` - AON counter driver needs to add driver API reentrancy support
* :jira:`ZEP-430` - Add driver API reentrancy support to PWM shim driver
* :jira:`ZEP-434` - Driver for HMC5883L magnetometer
* :jira:`ZEP-440` - Add driver API reentrancy support to WDT shim driver
* :jira:`ZEP-441` - Add driver API reentrancy support to GPIO shim drivers
* :jira:`ZEP-489` - nios2: handle unimplemented multiply/divide instructions
* :jira:`ZEP-500` - Domain Name System client library
* :jira:`ZEP-506` - nios2: support bare metal boot and XIP on Altera MAX10
* :jira:`ZEP-511` - Add Deep Sleep support in PMA
* :jira:`ZEP-512` - Add suspend/resume support for some core devices to enable Deep Sleep support in PMA
* :jira:`ZEP-541` - Integrate QMSI releases to Zephyr
* :jira:`ZEP-567` - netz sample code
* :jira:`ZEP-568` - MQTT QoS sample app
* :jira:`ZEP-573` - IoT applications must use netz API
* :jira:`ZEP-590` - Update Zephyr's TinyCrypt to version 2.0
* :jira:`ZEP-643` - Add file system API documentation
* :jira:`ZEP-650` - Quark SE: Implement PM reference application
* :jira:`ZEP-652` - QMSI shim driver: RTC: Implement suspend and resume callbacks
* :jira:`ZEP-655` - QMSI shim driver: PWM: Implement suspend and resume callbacks
* :jira:`ZEP-658` - QMSI shim driver: GPIO: Implement suspend and resume callbacks
* :jira:`ZEP-659` - QMSI shim driver: UART: Implement suspend and resume callbacks
* :jira:`ZEP-662` - QMSI shim driver: Pinmux: Implement suspend and resume callbacks

Epic
====

* :jira:`ZEP-278` - Enable Nios II CPU on Altera Max10
* :jira:`ZEP-284` - Flash Filesystem Support
* :jira:`ZEP-305` - Device Suspend / Resume infrastructure
* :jira:`ZEP-306` - PWM Enabling
* :jira:`ZEP-406` - Drivers shall be re-entrant

Bug
===

* :jira:`ZEP-68` - Final image contains duplicates of some routines
* :jira:`ZEP-156` - PWM Set Value API behaves incorrectly
* :jira:`ZEP-158` - PWM Set Duty Cycle API does not work
* :jira:`ZEP-180` - make menuconfig user provided options are ignored at building time
* :jira:`ZEP-187` - BLE APIs are not documented
* :jira:`ZEP-218` - [drivers/nble][PTS_TEST] Fix responding with the wrong error codes to the Prepare Write Request
* :jira:`ZEP-221` - [drivers/nble][PTS_TEST] Implement Execute Write Request handler
* :jira:`ZEP-369` - When building out of the tree, application object files are not placed into outdir
* :jira:`ZEP-379` - _k_command_stack may be improperly initialized when debugging
* :jira:`ZEP-384` - D2000 hangs after I2C communication with BMC150 sensor
* :jira:`ZEP-401` - PWM driver turns off pin if off time is 0 in set_values
* :jira:`ZEP-423` - Quark D2000 CRB documentation should include instructions to flash bootloader
* :jira:`ZEP-435` - Ethernet/IPv4/TCP: ip_buf_appdatalen returns wrong values
* :jira:`ZEP-456` - doc: ``IDT security`` section disappeared
* :jira:`ZEP-457` - doc: contribute/doxygen/typedefs.rst: examples files are broken
* :jira:`ZEP-459` - doc: kconfig reference entries in HTML are lacking a title
* :jira:`ZEP-460` - doc: document parameters of DEVICE* macros
* :jira:`ZEP-461` - Release 1.4.0 has broken the BMI160 sample as well as an application based on it
* :jira:`ZEP-463` - Getting started guide "next" link doesn't take you to "Checking Out the Source Code Anonymously" section
* :jira:`ZEP-469` - Ethernet/IPv4/TCP: net_receive & net_reply in server mode
* :jira:`ZEP-474` - ND: Neighbor cache is not getting cleared
* :jira:`ZEP-475` - Issue with timer callback routine: Condition checked is incorrect
* :jira:`ZEP-478` - Linux setup docs missing step to install curses development package for Fedora
* :jira:`ZEP-497` - Ethernet/IPv4/TCP: failed to get free buffer
* :jira:`ZEP-499` - TMP007 driver returns invalid values for negative temperature
* :jira:`ZEP-514` - memory corruption in microkernel memory pool defrag()
* :jira:`ZEP-516` - Ubuntu setup instructions missing 'upgrade' step
* :jira:`ZEP-518` - SPI not working on Arduino101
* :jira:`ZEP-522` - TCP/client-mode: disconnect
* :jira:`ZEP-523` - FIFOs defined by DEFINE_FIFO macro use the same memory buffer
* :jira:`ZEP-525` - srctree changes are breaking applications
* :jira:`ZEP-526` - build "kernel event logger" sample app failed for BOARD=quark_d2000_crb
* :jira:`ZEP-534` - Scan for consistent use of "platform/board/SoC" in documentation
* :jira:`ZEP-537` - doc: create external wiki page "Maintainers"
* :jira:`ZEP-545` - Wrong default value of CONFIG_ADC_QMSI_SAMPLE_WIDTH for x86 QMSI ADC
* :jira:`ZEP-547` - [nble] Failed to start encryption after reconnection
* :jira:`ZEP-554` - samples/drivers/aon_counter check README file
* :jira:`ZEP-555` - correct libgcc not getting linked for CONFIG_FLOAT=y on ARM
* :jira:`ZEP-556` - System hangs during I2C transfer
* :jira:`ZEP-565` - Ethernet/IPv4/TCP: last commits are breaking network support
* :jira:`ZEP-571` - ARC kernel BAT failed due to race in nested interrupts
* :jira:`ZEP-572` - X86 kernel BAT failed: Kernel Allocation Failure!
* :jira:`ZEP-575` - Ethernet/IPv4/UDP: ip_buf_appdatalen returns wrong values
* :jira:`ZEP-595` - UART: usb simulated uart doesn't work in poll mode
* :jira:`ZEP-598` - CoAP Link format filtering is not supported
* :jira:`ZEP-611` - Links on downloads page are not named consistently
* :jira:`ZEP-616` - OS X setup instructions not working on El Capitan
* :jira:`ZEP-617` - MQTT samples build fail because netz.h file missing.
* :jira:`ZEP-621` - samples/static_lib: fatal error: stdio.h: No such file or directory
* :jira:`ZEP-623` - MQTT sample mqtt.h missing "mqtt_unsubscribe" function
* :jira:`ZEP-632` - MQTT fail to re-connect to the broker.
* :jira:`ZEP-633` - samples/usb/cdc_acm: undefined reference to 'uart_qmsi_pm_save_config'
* :jira:`ZEP-642` - Inconsistent interpretation of pwm_pin_set_values arguments among drivers
* :jira:`ZEP-645` - ARC QMSI ADC shim driver fails to read sample data
* :jira:`ZEP-646` - I2C fail to read GY2561 sensor when GY2561 & GY271 sensor are attached to I2C bus.
* :jira:`ZEP-647` - Power management state storage should use GPS1 instead of GPS0
* :jira:`ZEP-669` - MQTT fail to pingreq if broker deliver topic to client but client doesn't read it.
* :jira:`ZEP-673` - Sanity crashes and doesn't kill qemu upon timeout
* :jira:`ZEP-679` - HMC5883L I2C Register Read Order
* :jira:`ZEP-681` - MQTT client sample throws too many warnings when build.
* :jira:`ZEP-687` - docs: Subsystems/Networking section is almost empty
* :jira:`ZEP-689` - Builds on em_starterkit fail
* :jira:`ZEP-695` - FatFs doesn't compile using Newlib
* :jira:`ZEP-697` - samples/net/test_15_4 cannot be built by sanitycheck
* :jira:`ZEP-703` - USB sample apps are broken after QMSI update
* :jira:`ZEP-704` - test_atomic does not complete on ARC
* :jira:`ZEP-708` - tests/kernel/test_ipm fails on Arduino 101
* :jira:`ZEP-739` - warnings when building samples for quark_se devboard

Known issues
============

* :jira:`ZEP-517` - build on windows failed "zephyr/Makefile:869: \*\*\* multiple target patterns"
   - No workaround, will fix in future release.

* :jira:`ZEP-711` - I2c: fails to write with mode fast plus
   - No workaround need it, there is no support for high speed mode.

* :jira:`ZEP-724` - build on windows failed: 'make: execvp: uname: File or path name too long'
   - No workaround, will fix in future release.

* :jira:`ZEP-467` - Hang using UART and console.
   - No workaround, will fix in future release.

* :jira:`ZEP-599` - Periodic call-back function for periodic REST resources is not getting invoked
   - No workaround, will fix in future release.

* :jira:`ZEP-471` - Ethernet packet with multicast address is not working
   - No workaround, will fix in future release.

* :jira:`ZEP-473` - Destination multicast address is not correct
   - No workaround, will fix in future release.
