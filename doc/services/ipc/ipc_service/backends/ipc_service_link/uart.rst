.. _ipc_service_backend_link_uart:

IPC link UART transport
#######################

The UART transport of the :ref:`IPC link backend <ipc_service_backend_link>` runs an IPC service
instance over a UART, between two processors that run the same transport: the UART TX of one is
connected to the UART RX of the other. It works over any ``uart`` device, the host
pseudo-terminals of ``native_sim`` included.

Configuration
=============

Devicetree
----------

:dtcompatible:`zephyr,ipc-uart` is a child node of the UART controller connected to the peer.

.. code-block:: devicetree

   &uart1 {
      ipc0: ipc-uart {
         compatible = "zephyr,ipc-uart";
         mtu = <256>;
         retransmit-timeout-ms = <100>;
      };
   };

The node has the properties common to every transport, ``mtu`` and ``retransmit-timeout-ms``,
described with the :ref:`backend <ipc_service_backend_link>`. The default frame size is 256
bytes. At 115200 baud a frame of that size takes about 25 ms on the wire, and the default
timeout of 100 ms leaves the peer time to answer; a slower UART or a larger frame needs a longer
timeout.

Kconfig
-------

:kconfig:option:`CONFIG_IPC_SERVICE_BACKEND_LINK_UART` is enabled by default when the devicetree
has such a node. The transport uses the asynchronous UART API when
:kconfig:option:`CONFIG_UART_ASYNC_API` is enabled, and the interrupt-driven API otherwise.

Framing
=======

.. mermaid::
   :caption: The UART transport
   :alt: The IPC link backend hands frames to the UART transport, which encodes them with COBS
         from a work item and sends them with the UART driver; bytes received by the driver
         are decoded straight into the link's receive buffer and handed back as frames. The
         UART line leads to the peer, which runs the same transport.

   ---
   config:
     flowchart:
       useMaxWidth: false
   ---
   flowchart TB
     link["IPC link backend"]
     subgraph transport["UART transport"]
       direction LR
       enc["COBS encoder<br/>work item"]
       dec["COBS decoder<br/>into the receive buffer"]
     end
     uart["UART driver<br/>asynchronous or interrupt-driven API"]
     peer["Peer: the same transport"]

     link -- "frame to send" --> enc
     dec -- "frame received" --> link
     enc -- "encoded frame, 0x00" --> uart
     uart -- "received bytes" --> dec
     uart <-- "TX, RX" --> peer

Each frame is encoded with :abbr:`COBS (Consistent Overhead Byte Stuffing)` and ends with a
``0x00`` delimiter: the encoding adds at most one byte per 254, and the receiver finds the next
frame after any corruption at the next delimiter. A frame that does not decode, or that does not
fit the frame size, counts as a frame without a valid header, which the backend recovers from by
asking the peer for its header again.

Frames are encoded and started from a work item; the interrupt handlers feed the decoder, which
hands a complete frame to the backend, and complete transmissions. Hardware flow control is
recommended but not required: bytes lost to an overrun take a frame with them, and the frame is
sent again. With the asynchronous API, a transmission that flow control holds for longer than
``retransmit-timeout-ms`` is given up, so that the retransmissions go on and a peer that stops
taking bytes still ends the session.
