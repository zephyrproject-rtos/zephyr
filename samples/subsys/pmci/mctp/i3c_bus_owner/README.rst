.. zephyr:code-sample:: mctp_i3c_bus_owner
   :name: MCTP I3C Top Node Sample

   Create an MCTP top node controlling I3C.

Overview
********
Sets up an MCTP top node that may talk to any number of connected endpoints over I3C.

Requirements
************
Board with an I3C bus acting as the bus controller.

Wiring
******
A common I3C bus SDA/SCL pair connected to any number of boards (setup in devicetree).

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/pmci/mctp/i3c_bus_owner
   :host-os: unix
   :board: frdm_mcxn947_mcxn947_cpu0
   :goals: run
   :compact:

References
**********

`MCTP Base Specification 2019 <https://www.dmtf.org/sites/default/files/standards/documents/DSP0236_1.3.1.pdf>`_
