.. _bluetooth_mesh_srpl_srv:

Solicitation PDU RPL Configuration Server
#########################################

The Solicitation PDU RPL Configuration Server model is a foundation model defined by the Bluetooth
mesh specification. The model is enabled if the node has the :ref:`bluetooth_mesh_od_srv` enabled.

The Solicitation PDU RPL Configuration Server model was introduced in the Bluetooth Mesh Protocol
Specification version 1.1, and manages the Solicitation Replay Protection List (SRPL) saved on the
device. The SRPL is used to reject Solicitation PDUs that are already processed by a node. When a
valid Solicitation PDU message is successfully processed by a node, the SSRC field and SSEQ field
of the message are stored in the node's SRPL.

The Solicitation PDU RPL Configuration Server does not have an API of its own, and relies on a
:ref:`bluetooth_mesh_srpl_cli` to control it. The model only accepts messages encrypted with an
application key as configured by the Configuration Client.

If present, the Solicitation PDU RPL Configuration Server model must only be instantiated on the
primary element.

Configurations
**************

For the Solicitation PDU RPL Configuration Server model, the
:kconfig:option:`CONFIG_BT_MESH_PROXY_SRPL_SIZE` option can be configured to set the size of the
SRPL.

API reference
*************

.. doxygengroup:: bt_mesh_sol_pdu_rpl_srv
