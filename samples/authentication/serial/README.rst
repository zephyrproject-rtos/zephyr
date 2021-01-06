.. _auth_serial-sample:

Authentication over Serial
##########################

Overview
********

There are two Serial firmware applications, client and server.  Pin P.06 and P.08 were used
for TX and RX lines on the Nordic nRF52840-DK board, one board acting as the server, the other as
the client.

The authentication method, DTLS or Challenge-Response, is configurable via KConfig menu.

Building and Running
--------------------
This sample was developed and tested with two Nordic nRF52840 dev
kits (see: https://www.nordicsemi.com/Software-and-Tools/Development-Kits/nRF52840-DK).  Two Ubuntu
VMs were used, one running the Client the other VM running the Server.

There are two project config files, proj.conf for use with the Challenge-Response authentication method and dtls.prj.conf for
use with the DTLS authentication method.  Use the -DCONFIG_FILE=dtls.prj.conf to use DLTS.

To build the client with DTLS:|br|
cmake -Bbuild_auth_client -DBOARD=nrf52840dk_nrf52840 -DCONF_FILE=dtls.prj.conf samples/authentication/serial/auth_client

To build the server with DLTS using West:|br|
west build -d build_auth_server -b nrf52840dk_nrf52840  -- -DCONF_FILE=dtls.prj.conf samples/authentication/serial/auth_server |br|
(note: Two dashes '-' after 'west build -b build_auth_server -b nrf52840dk_nrf52840 --', necessary for -DCONF_FILE define)


Note:  To avoid problems, ensure debug output is done via Jlink RTT, not the UART backend.





