.. _modbus_api:

MODBUS over serial line (RTU)
#############################

Modbus is an industrial messaging protocol. The protocol is specified
for different types of networks or buses. Zephyr RTOS implementation
is currently only available for serial line and may be used
with different physical interfaces, like RS485 or RS232.

Modbus communication via serial line is based on client/server model.
Only one client may be present on the bus. Client can communicate with several
server devices. Server devices themselves are passive and must not send
requests or unsolicited responses.
Services requested by the client are specified by function codes (FCxx),
and can be found in the specification or documentation of the API below.

Zephyr RTOS implementation supports both client and server roles.

More information about Modbus and Modbus RTU can be found on the website
`MODBUS Protocol Specifications`_.

Samples
*******

`modbus-rtu-server-sample` and `modbus-rtu-client-sample` give
the possibility to try out RTU server and RTU client implementation with
an evaluation board.

API Reference
*************

.. doxygengroup:: modbus
   :project: Zephyr

.. _`MODBUS Protocol Specifications`: https://www.modbus.org/specs.php
