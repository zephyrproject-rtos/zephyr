.. _osdp-sample:

Open Supervised Device Protocol (OSDP)
######################################

Open Supervised Device Protocol (OSDP) is an IEC standard (IEC 60839-11-5)
intended to improve interoperability among access control and security products.
It supports Secure Channel (SC) for encrypted and authenticated communication
between configured devices.

OSDP describes the communication protocol for interfacing one or more Peripheral
Devices (PD) to a Control Panel (CP) over a two-wire RS-485 multi-drop serial
communication channel. Nevertheless, this protocol can be used to transfer
secure data over any stream based physical channel. Read more about `OSDP here
<https://github.com/goToMain/libosdp/>`_..

Although OSDP is steered towards the Access and Security industries, it can be
used as a general communication protocol for devices in a secure way without
too much resource requirements. The security is not top-notch (AES-128) but it
is reasonable enough, given that the alternative is no security at all.

OSDP Supports the control of the following components on a PD:

- LED
- Buzzer
- Keypad
- Output (GPIOs)
- Input Control (GPIOs)
- Displays
- Device status (tamper, power, etc.,)
- Card Reader
- Fingerprint Reader
