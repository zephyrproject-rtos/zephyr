.. zephyr:code-sample:: mctp_i2c_bus_endpoint
   :name: MCTP I2C GPIO Endpoint Node Sample

   Create an MCTP endpoint node controlling I2C with GPIO signaling.

Overview
********
Sets up an MCTP endpoint node that may talk to a top node and peer to other MCTP endpoints.

Requirements
************
Board with an I2C and a GPIO used for signaling the top node to read the target device.

Wiring
******
A common I2C bus SDA/SCL pair connected to any number of boards (setup in devicetree)
along with a GPIO for signaling endpoint write requests.

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/pmci/mctp/i2c_gpio_bus_endpoint
   :host-os: unix
   :board: frdm_mcxn947_mcxn947_cpu0
   :goals: run
   :compact:

References
**********

`MCTP Base Specification 2019 <https://www.dmtf.org/sites/default/files/standards/documents/DSP0236_1.3.1.pdf>`_
