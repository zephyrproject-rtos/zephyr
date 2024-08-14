.. zephyr:code-sample:: mctp_host_sample
   :name: MCTP Host Sample
   :relevant-api: mctp

   This sample shows how to create an MCTP host (a requesting node in a network
   of MCTP nodes) over UART. MCTP is a multiplexed messaging protocol that works
   over a variety of transports (i2c, i3c, uart, pcie, etc).

Overview
********
Sets up an MCTP node that sends a request on a UART targeting a particular MCTP
endpoint id with the message "hello". Expects and waits for a response to this
"hello" message containing "world".

Requirements
************
A board and SoC that provide access to a UART and a driver that implements the
UART async API.

Wiring
******
The UART pins should be wired to a board which will run the MCTP endpoint
sample such that this boards UART tx pin connects to the endpoint boards rx
pin, and this boards UART rx pin connects to the endpoint boards tx pin. The
boards grounds should also be wired together.

Optionally a logic analyzer can be wired up and listening to the UART to inspect
the data flowing.

Building and Running
********************


.. zephyr-app-commands::
   :zephyr-app: samples/modules/mctp/mctp_endpoint
   :host-os: unix
   :board: nrf52840_nrf52840dk
   :goals: run
   :compact:

References
**********

`MCTP Base Specification 2019 <https://www.dmtf.org/sites/default/files/standards/documents/DSP0236_1.3.1.pdf>`_
