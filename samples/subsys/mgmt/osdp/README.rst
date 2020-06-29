.. _osdp-sample:

OSDP
####

OSDP describes the communication interface one or more Peripheral Devices (PD)
to a Control Panel (CP). The specification describes the protocol
implementation over a two-wire RS-485 multi-drop serial communication channel.
Nevertheless, this protocol can be used to transfer secure byte stream over any
physical channel in low memory embedded devices.

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
