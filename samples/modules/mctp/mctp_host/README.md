# MCTP Host Sample

MCTP provides a message serialization and endpoint muxing protocol. This can
be used to build networks of embedded devices representing various services
across multiple transport mediums (e.g. uart, i2c, spi, pcie, etc).

The Host sample uses the arduino_serial UART device tree label to bind
MCTP to a UART. The sample then attempts to send a "hello" message to a
matching device running the MCTP Endpoint Sample. It expects to see a
response (the endpoint responsed with "world") and will print it.
