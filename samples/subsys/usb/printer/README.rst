.. _usb_printer:

USB Printer Sample
################

Overview
********

This sample implements a USB printer device that supports:

* PCL (Printer Command Language) commands
* PJL (Printer Job Language) job control
* Print job queuing and management
* Test page generation
* Error simulation and handling

The printer emulates a basic PCL printer with job management capabilities.

Building and Running
******************

Build and flash the sample for your board. For example:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/usb/printer
   :board: nrf52840dk_nrf52840
   :goals: build flash
   :compact:

Testing
*******

The sample includes comprehensive testing support:

1. Basic test using any printer driver:
   - Install as a generic/custom PCL printer
   - Print test documents

2. Automated testing using provided Python script:
   - Install dependencies: ``pip install -r requirements.txt``
   - Run tests: ``python test_printer.py --all``

See TESTING.md for detailed test procedures and examples.

Supported Features
****************

PCL Support
==========

* Basic PCL commands (reset, orientation, paper source)
* Font selection commands
* Page formatting

PJL Support
==========

* Job control (start/end job)
* Job naming and tracking
* Printer status reporting
* Device information

Job Management
============

* Multi-job queue
* Job status tracking
* Error recovery
* Job cancellation

Configuration
************

The sample can be configured using Kconfig options:

* :kconfig:option:`CONFIG_USB_PRINTER_SAMPLE_PCL_SUPPORT` - Enable PCL support
* :kconfig:option:`CONFIG_USB_PRINTER_SAMPLE_PJL_SUPPORT` - Enable PJL support
* :kconfig:option:`CONFIG_USB_PRINTER_SAMPLE_TEST_PAGE` - Enable test page support
* :kconfig:option:`CONFIG_USB_PRINTER_SAMPLE_RING_BUF_SIZE` - Set receive buffer size

Implementation Details
*******************

The implementation consists of several key components:

1. USB Printer Class Implementation
   - Standard printer class requests
   - Bulk endpoints for data transfer
   - Status reporting

2. Command Processing
   - PCL command parser
   - PJL command handler
   - Print data buffering

3. Job Management
   - Job queuing
   - Status tracking
   - Error handling

4. Test Support
   - Test page generation
   - Error simulation
   - Automated testing

Board Support
***********

The sample includes configurations for:

1. Nordic nRF52840 DK
2. STM32 Nucleo boards
3. Raspberry Pi Pico
4. ESP32 DevKit

See board overlay files for specific configurations.

Known Issues
**********

1. Some PCL commands are not fully implemented
2. Large print jobs may require buffer size adjustment
3. USB endpoints may need tuning for specific boards

API Documentation
**************

.. doxygengroup:: usb_printer
   :project: Zephyr