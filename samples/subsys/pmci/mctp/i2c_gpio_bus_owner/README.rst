.. zephyr:code-sample:: mctp_i2c_bus_owner
   :name: MCTP I2C GPIO Top Node Sample

   Create an MCTP top node controlling I2C with GPIO signaling.

Overview
********
Sets up an MCTP top node that may talk to any number of connected endpoints over I2C.

Requirements
************
Board with an I2C and number of GPIOs used for triggering by each bus connected endpoint.

Wiring
******
A common I2C bus SDA/SCL pair connected to any number of boards (setup in devicetree)
along with a GPIO per board for signaling endpoint write requests.

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/pmci/mctp/i2c_gpio_bus_owner
   :host-os: unix
   :board: frdm_mcxn947_mcxn947_cpu0
   :goals: run
   :compact:

References
**********

`MCTP Base Specification 2019 <https://www.dmtf.org/sites/default/files/standards/documents/DSP0236_1.3.1.pdf>`_
