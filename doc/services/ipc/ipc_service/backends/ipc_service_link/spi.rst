.. _ipc_service_backend_link_spi:

IPC link SPI transport
######################

The SPI transport of the :ref:`IPC link backend <ipc_service_backend_link>` runs an IPC service
instance over an SPI bus, between the processor that clocks the bus, the controller, and a
processor whose SPI controller runs in peripheral mode, the peripheral. Every transaction is one
frame each way, of the frame size, and two GPIO lines driven by the peripheral tell the controller
when to clock.

Configuration
=============

Devicetree
----------

:dtcompatible:`zephyr,ipc-spi` is a child node of the SPI controller connected to the peer, on
the controller. :dtcompatible:`zephyr,ipc-spi-peripheral` is a child node of the SPI controller
in peripheral mode, on the peripheral. Both need two GPIO lines, driven by the peripheral:

* ``rdy-gpios`` is active while the peripheral has a transfer armed: the controller waits for it
  before each transaction, so it never clocks an unarmed peripheral.
* ``irq-gpios`` is active while the peripheral has something to send.

.. code-block:: devicetree

   /* The controller */
   &spi1 {
      ipc0: ipc-spi@0 {
         compatible = "zephyr,ipc-spi";
         reg = <0>;
         spi-max-frequency = <8000000>;
         rdy-gpios = <&gpio0 10 GPIO_ACTIVE_HIGH>;
         irq-gpios = <&gpio0 11 GPIO_ACTIVE_HIGH>;
         mtu = <1600>;
      };
   };

   /* The peripheral */
   &spi2 {
      ipc0: ipc-spi@0 {
         compatible = "zephyr,ipc-spi-peripheral";
         reg = <0>;
         spi-max-frequency = <8000000>;
         rdy-gpios = <&gpio1 4 GPIO_ACTIVE_HIGH>;
         irq-gpios = <&gpio1 5 GPIO_ACTIVE_HIGH>;
         mtu = <1600>;
      };
   };

Both nodes have the properties common to every transport, ``mtu`` and ``retransmit-timeout-ms``,
described with the :ref:`backend <ipc_service_backend_link>`. Every transaction is ``mtu`` bytes
long; the default of 1600 bytes fits an Ethernet frame.

Kconfig
-------

:kconfig:option:`CONFIG_IPC_SERVICE_BACKEND_LINK_SPI` is enabled by default when the devicetree
has such a node. The peripheral role needs an SPI driver with
:kconfig:option:`CONFIG_SPI_PERIPHERAL` and the asynchronous API,
:kconfig:option:`CONFIG_SPI_ASYNC`.

:kconfig:option:`CONFIG_IPC_SERVICE_BACKEND_LINK_SPI_STACK_SIZE` and
:kconfig:option:`CONFIG_IPC_SERVICE_BACKEND_LINK_SPI_PRIORITY` - the transfer thread of each
instance, which runs the transactions.

Transactions
============

.. mermaid::
   :caption: The SPI transport, both roles
   :alt: Two mirrored stacks, read from left to right: on the controller, the IPC link backend,
         the SPI transport with its transfer thread, then the SPI controller driver; on the
         peripheral the SPI driver in peripheral mode, the transport with its transfer thread,
         and the same backend. The controller drives SCK, CS and its SDO to the peripheral, the
         peripheral drives its SDO to the controller, and the peripheral's transport drives the
         RDY and IRQ lines to the controller's transport.

   ---
   config:
     flowchart:
       useMaxWidth: false
   ---
   flowchart LR
     subgraph c["Controller"]
       clink["IPC link backend"]
       ctr["SPI transport<br/>transfer thread"]
       cdrv["SPI controller driver"]
       clink <-- "frames" --> ctr <-- "transactions" --> cdrv
     end
     subgraph p["Peripheral"]
       pdrv["SPI driver<br/>peripheral mode, asynchronous API"]
       ptr["SPI transport<br/>transfer thread"]
       plink["IPC link backend"]
       pdrv <-- "transactions" --> ptr <-- "frames" --> plink
     end
     cdrv -- "SCK, CS, SDO" --> pdrv
     pdrv -- "SDO" --> cdrv
     ptr -. "RDY, IRQ" .-> ctr

The data lines are named from each side's own perspective, as in the :ref:`SPI API <spi_api>`.
The two GPIO lines, driven by the peripheral, tell the controller when to clock:

``RDY``
   Raised once a transfer of ``mtu`` bytes is armed with the asynchronous SPI API, lowered when
   it completes. The controller never clocks while it is low.
``IRQ``
   Active while the armed frame carries something to send, or a message was queued since.

The controller clocks a transaction when ``RDY`` is active and either ``IRQ`` is active or it
has a frame of its own. No transaction runs while both sides are idle.

.. mermaid::
   :caption: SPI transactions
   :alt: Sequence diagram: the peripheral arms a transfer and raises RDY; a message is queued on
         the peripheral and it raises IRQ; the controller sees IRQ, waits for RDY and clocks a
         transaction, after which the peripheral lowers RDY, processes the frame, arms the next
         transfer and raises RDY again; the controller clocks a second transaction carrying its
         acknowledgment, after which IRQ is lowered.

   ---
   config:
     sequence:
       useMaxWidth: false
   ---
   sequenceDiagram
     participant C as Controller
     participant P as Peripheral
     Note over P: arm transfer
     P-->>C: RDY high
     Note over P: message queued
     P-->>C: IRQ high
     Note over C: wait for RDY
     C->>P: transaction 1: a frame each way
     P-->>C: RDY low
     Note over P: deliver, arm next
     P-->>C: RDY high
     C->>P: transaction 2: ack
     P-->>C: IRQ low
     P-->>C: RDY low

A frame is built before the transaction that carries it, so an acknowledgment arrives one
transaction after the payload; the acknowledging side asks for that transaction with ``IRQ``, or
clocks it. The timeouts of the :ref:`backend <ipc_service_backend_link>` only come into play when
a transaction is lost or the peer is gone: a peripheral that never raises ``RDY`` again, or a
controller that never clocks again, ends the session on the other side.

Limitations
===========

* The peripheral role needs the asynchronous SPI API: the peripheral arms a transfer and raises
  ``RDY`` only then, so the controller never clocks an unarmed peripheral.
* The controller must keep chip select asserted for a whole transaction of ``mtu`` bytes. A
  transaction cut short, or a chip select pulse without clocks, delivers no frame to the
  peripheral and counts as one lost transaction.
