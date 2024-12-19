# MCTP Endpoint Sample

MCTP provides a message serialization and endpoint muxing protocol. This can
be used to build networks of embedded devices representing various services
across multiple transport mediums (e.g. uart, i2c, spi, pcie, etc).

The Endpoint sample uses the arduino_serial UART device tree label to bind
MCTP to a UART. The sample then listens for a message on a particular endpoint and
responds back with a "world" message.
