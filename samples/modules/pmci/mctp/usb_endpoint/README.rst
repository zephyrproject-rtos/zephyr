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
A board and SoC that provide support for USB device capability (either FS or HS). Testing requires
a USB host that has the ability to enumerate the MCTP interface and interact with the board using
the MCTP protocol. An easy way to do this is a Python script.

An example script to test this interface is provided below.

.. code-block:: python

    import usb.core
    import usb.util

    class USBDevice:
        def __init__(self, vid, pid):
            self.vid = vid
            self.pid = pid
            self.device = None
            self.endpoint_in = None
            self.endpoint_out = None
            self.interface = None

        def connect(self):
            # Find the device
            self.device = usb.core.find(idVendor=self.vid, idProduct=self.pid)
            if self.device is None:
                raise ValueError("Device not found")

            # Set the active configuration
            self.device.set_configuration()
            cfg = self.device.get_active_configuration()
            self.interface = cfg[(0, 0)]

            # Claim the interface
            usb.util.claim_interface(self.device, self.interface.bInterfaceNumber)

            # Find bulk IN and OUT endpoints
            for ep in self.interface:
                if usb.util.endpoint_direction(ep.bEndpointAddress) == usb.util.ENDPOINT_IN:
                    self.endpoint_in = ep
                elif usb.util.endpoint_direction(ep.bEndpointAddress) == usb.util.ENDPOINT_OUT:
                    self.endpoint_out = ep

            if not self.endpoint_in or not self.endpoint_out:
                raise ValueError("Bulk IN/OUT endpoints not found")

        def send_data(self, data):
            if not self.endpoint_out:
                raise RuntimeError("OUT endpoint not initialized")
            self.endpoint_out.write(data)

        def receive_data(self, size=64, timeout=1000):
            if not self.endpoint_in:
                raise RuntimeError("IN endpoint not initialized")
            return self.endpoint_in.read(size, timeout)

        def disconnect(self):
            usb.util.release_interface(self.device, self.interface.bInterfaceNumber)
            usb.util.dispose_resources(self.device)

    if __name__ == "__main__":
        usb_dev = USBDevice(0x2fe3, 0x0001)
        try:
            usb_dev.connect()

            print("Sending message with size smaller than USB FS MPS (<64b)")
            usb_dev.send_data(b'\x1a\xb4\x00\x0f\x01\x0a\x14\xc0\x48\x65\x6c\x6c\x6f\x2c\x20\x4d\x43\x58\x00')
            response = usb_dev.receive_data()
            print("Received:", bytes(response))

            print("Sending message spanning two USB FS packets (>64b)")
            usb_dev.send_data(b'\x1a\xb4\x00\x4c\x01\x0a\x14\xc0\x48\x65\x6c\x6c\x6f\x2c\x20\x4d\x43\x58\x2e\x20\x4c\x65\x74\x27\x73\x20\x74\x65\x73\x74\x20\x61\x20\x6d\x75\x6c\x74\x69\x2d\x70\x61\x63\x6b\x65\x74\x20\x6d\x65\x73\x73\x61\x67\x65\x20\x74\x6f\x20\x73\x65\x65\x20\x68\x6f\x77\x20\x79\x6f\x75\x20\x68\x61\x6e\x64\x6c\x65\x20\x69\x74\x2e\x00')
            response = usb_dev.receive_data()
            print("Received:", bytes(response))

            print("Sending message with two MCTP messages in a single USB FS packet")
            usb_dev.send_data(b'\x1a\xb4\x00\x12\x01\x0a\x14\xc0\x48\x65\x6c\x6c\x6f\x2c\x20\x4d\x43\x58\x2c\x20\x31\x00\x1a\xb4\x00\x12\x01\x0a\x14\xc0\x48\x65\x6c\x6c\x6f\x2c\x20\x4d\x43\x58\x2c\x20\x32\x00')
            response = usb_dev.receive_data()
            print("Received:", bytes(response))
            print("Received:", bytes(response))
        finally:
            usb_dev.disconnect()

Wiring
******
Connect a USB cable between the host (likely a PC) and the board. The type of cable (mini, micro,
type C) depends on the host and board connectors.

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/modules/pmci/mctp/usb_endpoint
   :host-os: unix
   :board: frdm_mcxn947_mcxn947_cpu0
   :goals: run
   :compact:

References
**********

`MCTP Base Specification 2019 <https://www.dmtf.org/sites/default/files/standards/documents/DSP0236_1.3.1.pdf>`_
