.. _smbus_emul_tests:

SMBUS unit tests
################

Unit tests emulate SMBus hardware controller and connected SMBus peripheral
device by redirecting I/O read / write to a memory buffer. This allows to
cover all SMBus protocol commands.
