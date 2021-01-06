.. _auth_bluetooth-sample:

Authentication over Bluetooth
#############################

Overview
********

There are two Bluetooth firmware applications, central and peripheral.  The Central acts as
the client, the peripheral acts as the server.  The Central initiates the authentication
messages.

The authentication method, DTLS or Challenge-Response, is configurable via KConfig menu.

Building and Running
--------------------
This sample was developed and tested with two Nordic nRF52840 dev
kits (see: https://www.nordicsemi.com/Software-and-Tools/Development-Kits/nRF52840-DK).  Two Ubuntu
VMs were used, one running the Central the other VM running the Peripheral.

To build the peripheral with DTLS:|br|
cmake -Bbuild_auth_peripheral -DBOARD=nrf52840dk_nrf52840  -DCONF_FILE=dtls.prj.conf samples/authentication/bluetooth/peripheral_auth

To build the central with DLTS using West:|br|
west build -d build_auth_central -b nrf52840dk_nrf52840 -- -DCONF_FILE=dtls.prj.conf samples/authentication/bluetooth/central_auth |br|
(note: Two dashes '-' after 'west build -b build_auth_central -b nrf52840dk_nrf52840 --', necessary for -DCONF_FILE define)


