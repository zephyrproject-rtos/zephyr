.. _modbus:

Modbus
######

Modbus is an industrial messaging protocol. The protocol is specified
for different types of networks or buses. Zephyr OS implementation
supports communication over serial line and may be used
with different physical interfaces, like RS485 or RS232.
TCP support is not implemented directly, but there are helper functions
to realize TCP support according to the application's needs.

Modbus communication is based on client/server model.
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

:ref:`modbus-rtu-server-sample` and :ref:`modbus-rtu-client-sample` give
the possibility to try out RTU server and RTU client implementation with
an evaluation board.

:ref:`modbus-tcp-server-sample` is a simple Modbus TCP server.

:ref:`modbus-gateway-sample` is an example how to build a TCP to serial line
gateway with Zephyr OS.

API Reference
*************

.. doxygengroup:: modbus

.. _`MODBUS Protocol Specifications`: https://www.modbus.org/specs.php
