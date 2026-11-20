.. _ipc_service_backend_rpmsg_lite:

RPMsg-Lite backend
##################

The RPMsg-Lite backend implements the IPC service on top of the
`RPMsg-Lite <https://github.com/nxp-mcuxpresso/rpmsg-lite>`_ library. It is a
lightweight alternative to the RPMsg static vrings backend that does not
depend on OpenAMP, and it is
available on any platform that provides a :ref:`MBOX <mbox_api>` driver and a
memory region reachable by both cores.

The RPMsg-Lite library is provided by the ``rpmsg-lite`` west project. Enable
the backend with :kconfig:option:`CONFIG_IPC_SERVICE_BACKEND_RPMSG_LITE`.

Overview
========

Each IPC service instance is one RPMsg-Lite *link*. A link owns a dedicated
shared memory region that carries the virtqueues and the message buffers, and
it is bound to the other core by a matching ``link-id``. Endpoints are
announced through the RPMsg name service, so endpoint registration on the host
and on the remote side can happen in any order.

The backend supports:

* Several instances (links) per core pair, sharing a single MBOX channel.
* Several endpoints per instance.
* The no-copy API (:c:func:`ipc_service_get_tx_buffer`,
  :c:func:`ipc_service_send_nocopy`, :c:func:`ipc_service_hold_rx_buffer`).
* The run-time deregistration of an endpoint and the closing of an instance.

Shared memory layout
====================

The ``memory-region`` property of an instance points to the memory used by the
link. Its base address and size are used as-is, and the same region must be
described on both cores. The region is laid out by RPMsg-Lite as follows:

.. code-block:: none

   memory-region base
   +---------------------------+
   | Virtqueue 0 (host TX)     |  2 x vring size, aligned
   | Virtqueue 1 (remote TX)   |
   +---------------------------+
   | Message buffers           |  2 x buffer-count buffers of
   |                           |  buffer-size bytes each
   +---------------------------+

The size of the vrings is derived from ``buffer-count``. Every message buffer
starts with a 16-byte RPMsg header, so the payload available to the
application is ``buffer-size`` minus 16 bytes. The region must be large enough
to contain the vrings and all the buffers, otherwise the initialization of the
instance fails.

The following properties shape the layout and must have the same value on both
cores:

* ``buffer-size``: total size of one message buffer, including the header
  (default: 512, that is 496 bytes of payload).
* ``buffer-count``: number of buffers per direction. Must be a power of two
  (default: 2).

.. note::

   The shared memory must be accessible by both cores. If a core uses a data
   cache, mark the region as non-cacheable, for example with
   ``zephyr,memory-attr``, because the backend does not perform any cache
   maintenance.

Links and notification topology
===============================

Every instance has a mandatory ``link-id``. The instance with the same
``link-id`` on the other core is its peer, so the values must match on both
sides. The ``link-id`` must be smaller than the number of enabled
``nxp,ipc-rpmsg-lite`` instances, and each value must be used only once on a
core.

The ``role`` property selects the RPMsg-Lite role of the instance: ``host``
(virtio driver) or ``remote`` (virtio device). The two peers of a link must
have opposite roles. On the remote side :c:func:`ipc_service_open_instance`
waits for the host to bring the link up for at most
:kconfig:option:`CONFIG_IPC_SERVICE_BACKEND_RPMSG_LITE_LINK_UP_TIMEOUT_MS`.

All links between two cores share one pair of MBOX channels. The signal sent
over the channel carries the ``link-id`` and the virtqueue index, which the
receiver uses to route the notification to the right instance. The topology is
described in devicetree:

* The instance with ``link-id = <0>`` is the *notification owner*. It declares
  the ``mboxes`` and ``mbox-names`` properties (``"tx"`` and ``"rx"``).
* Every other instance declares ``notification-parent`` pointing to the
  notification owner and does not declare ``mboxes``.

.. code-block:: devicetree

   reserved-memory {
      shm_ipc0: memory@202c0000 {
         reg = <0x202c0000 0x8000>;
      };

      shm_ipc1: memory@202c8000 {
         reg = <0x202c8000 0x8000>;
      };
   };

   ipc {
      ipc0: ipc0 {
         compatible = "nxp,ipc-rpmsg-lite";
         memory-region = <&shm_ipc0>;
         mboxes = <&mbox 2>, <&mbox 3>;
         mbox-names = "tx", "rx";
         link-id = <0>;
         role = "host";
         status = "okay";
      };

      ipc1: ipc1 {
         compatible = "nxp,ipc-rpmsg-lite";
         memory-region = <&shm_ipc1>;
         notification-parent = <&ipc0>;
         link-id = <1>;
         role = "host";
         buffer-size = <128>;
         status = "okay";
      };
   };

The other core uses the same ``link-id`` values and the same memory regions,
the ``remote`` role, and the swapped MBOX channels (the ``"tx"`` channel of
one core is the ``"rx"`` channel of the other).

Each instance runs its own RX work queue, which delivers the received data to
the endpoint callbacks. Its priority is set with the ``priority`` property
(default: ``<0 PRIO_PREEMPT>``) and its stack size with
:kconfig:option:`CONFIG_IPC_SERVICE_BACKEND_RPMSG_LITE_WQ_STACK_SIZE`.

MBOX notification handling is selected with a Kconfig choice:

* :kconfig:option:`CONFIG_IPC_SERVICE_BACKEND_RPMSG_LITE_NOTIFY_DIRECT`
  (default) keeps only the latest notification. It has the lowest latency and
  RAM usage, but notifications can be coalesced under heavy load.
* :kconfig:option:`CONFIG_IPC_SERVICE_BACKEND_RPMSG_LITE_NOTIFY_QUEUE` queues
  pending notifications to survive bursts, at the cost of some latency and
  additional RAM per instance.

Endpoints
=========

An instance can hold at most
:kconfig:option:`CONFIG_IPC_SERVICE_BACKEND_RPMSG_LITE_NUM_ENDPOINTS_PER_INSTANCE`
endpoints (default: 2). The limit is the same for all instances. Registering
more endpoints than this limit fails with ``-EINVAL``.

The following rules apply to endpoints:

* The endpoint name (:c:member:`ipc_ept_cfg.name`) is mandatory and is used
  for the name service announcement. It cannot be empty and is limited to 32
  characters including the terminator.
* An endpoint is bound when the endpoint with the same name is registered on
  the other core. The :c:member:`ipc_service_cb.bound` callback is called then.
* Data cannot be sent before the endpoint is bound, and an empty message is
  rejected with ``-EBADMSG``.
* The maximum message size is ``buffer-size`` minus 16 bytes. When no buffer is
  available, :c:func:`ipc_service_send` returns ``-ENOMEM`` without blocking.

Usage
=====

See :zephyr:code-sample:`rpmsg-lite` for an application that uses
the backend on several NXP platforms.
