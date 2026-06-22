.. zephyr:code-sample:: mctp_i3c_bus_endpoint
   :name: MCTP I3C Endpoint Node Sample

   Create an MCTP endpoint node over I3C.

Overview
********
Sets up an MCTP endpoint node that may talk to a top node and peer to other MCTP endpoints.

Requirements
************
Board with I3C interface capable of assuming the "target" role.

Wiring
******
A common I3C bus SDA/SCL pair connected to a board acting on the "controller" role.

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/pmci/mctp/i3c_bus_endpoint
   :host-os: unix
   :board: npcx4m8f_evb
   :goals: run
   :compact:

References
**********

`MCTP Base Specification 2019 <https://www.dmtf.org/sites/default/files/standards/documents/DSP0236_1.3.1.pdf>`_
