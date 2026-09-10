.. zephyr:code-sample:: mctp-usb-endpoint
   :name: MCTP USB Endpoint Node Sample
   :relevant-api: usbd_api

   Create an MCTP endpoint node using the USB device interface.

Overview
********
Sets up an MCTP endpoint node that listens for messages from the MCTP bus owner with EID 20.
Responds with "Hello, bus owner" message.

Requirements
************
A board and SoC that provide support for USB device capability (either FS or HS, though DMTF spec
specifies only HS). Testing requires a USB host that has the ability to enumerate the MCTP interface
and interact with the board using the MCTP protocol. An easy way to do this is a Python script.

An example script to test this interface is provided with this sample application. To run the
script, you must first install pyusb.

.. code-block:: console

    pip install requirements.txt
    python usb_host_tester.py

If the test is successful, you will see the following output from the script:

.. code-block:: console

    Sending message with size smaller than USB FS MPS (<64b)
    Received: b'\x1a\xb4\x00\x15\x01\x14\n\xc0Hello, bus owner\x00'
    Sending message spanning two USB FS packets (>64b)
    Received: b'\x1a\xb4\x00\x15\x01\x14\n\xc0Hello, bus owner\x00'
    Sending message with two MCTP messages in a single USB FS packet
    Received: b'\x1a\xb4\x00\x15\x01\x14\n\xc0Hello, bus owner\x00'
    Received: b'\x1a\xb4\x00\x15\x01\x14\n\xc0Hello, bus owner\x00'

Wiring
******
Connect a USB cable between the host (likely a PC) and the board. The type of cable (mini, micro,
type C) depends on the host and board connectors.

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/pmci/mctp/usb_endpoint
   :host-os: unix
   :board: frdm_mcxn947_mcxn947_cpu0
   :goals: run
   :compact:

References
**********

`MCTP Base Specification 2019 <https://www.dmtf.org/sites/default/files/standards/documents/DSP0236_1.3.1.pdf>`_
