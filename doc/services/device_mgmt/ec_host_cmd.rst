.. _ec_host_cmd_backend_api:

EC Host Command
###############

Overview
********
The host command protocol defines the interface for a host, or application processor, to
communicate with a target embedded controller (EC). The EC Host command subsystem implements the
target side of the protocol, generating responses to commands sent by the host. The host command
protocol interface supports multiple versions, but this subsystem implementation only support
protocol version 3.

Architecture
************
The Host Command subsystem contains a few components:
  * Backend
  * General handler
  * Command handler

The backend is a layer between a peripheral driver and the general handler. It is responsible for
sending and receiving commands via chosen peripheral.

The general handler validates data from the backend e.g. check sizes, checksum, etc. If the command
is valid and the user has provided a handler for a received command id, the command handler is
called.

.. image:: ec_host_cmd.png
   :align: center

SHI (Serial Host Interface) is different to this because it is used olny for communication with a
host. SHI does not have API itself, thus the backend and peripheral driver layers are combined into
one backend layer.

.. image:: ec_host_cmd_shi.png
   :align: center

The supported backend and peripheral drivers:
  * Simulator
  * SHI - ITE and NPCX
  * eSPI - any eSPI slave driver that support :kconfig:option:`CONFIG_ESPI_PERIPHERAL_EC_HOST_CMD` and
    :kconfig:option:`CONFIG_ESPI_PERIPHERAL_CUSTOM_OPCODE`
  * UART - any UART driver that supports the asynchronous API

Initialization
**************

If the application configures the ``zephyr,host-cmd-backend`` chosen node, then the backend
automatically initializes the host command subsystem by calling :c:func:`ec_host_cmd_init`.

If ``zephyr,host-cmd-backend`` is not chosen, the :c:func:`ec_host_cmd_init` function should be
called by application code. This way of initialization is useful if a backend is chosen in runtime
based on e.g. GPIO state.

Buffers
*******

The host command communication requires buffers for rx and tx. The buffers are be provided by the
general handler if :kconfig:option:`CONFIG_EC_HOST_CMD_HANDLER_RX_BUFFER_SIZE` > 0 for rx buffer and
:kconfig:option:`CONFIG_EC_HOST_CMD_HANDLER_TX_BUFFER_SIZE` > 0 for the tx buffer. The shared buffers are
useful for applications that use multiple backends. Defining separate buffers by every backend would
increase the memory usage. However, some buffers can be defined by a peripheral driver e.g. eSPI.
These ones should be reused as much as possible.

API Reference
*************

.. doxygengroup:: ec_host_cmd_interface
