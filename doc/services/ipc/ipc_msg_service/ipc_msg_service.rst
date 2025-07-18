.. _ipc_msg_service:

IPC Message Service
###################

.. contents::
    :local:
    :depth: 2

The IPC message service API provides an interface to facilitate the following
functions between two domains or CPUs:

* Exchanging messages, where these messages are strongly typed,
* Query status for associated domains or CPUs, and
* Receiving events pertaining to associated domains or CPUs.

Overview
========

A communication channel consists of one instance and one or several endpoints
associated with the instance.

An instance is the external representation of a physical communication channel
between two domains or CPUs. The actual implementation and internal
representation of the instance is peculiar to each backend.

An individual instance is not used to send messages between domains/CPUs.
To send and receive messages, the user must create (register) an endpoint in
the instance. This allows for the connection of the two domains of interest.

It is possible to have zero or multiple endpoints for one single instance,
possibly with different priorities, and to use each to exchange messages.
Endpoint prioritization and multi-instance ability highly depend on the backend
used and its implementation.

The endpoint is an entity the user must use to send and receive messages between
two domains (connected by the instance). An endpoint is always associated to an
instance.

The creation of the instances is left to the backend, usually at init time.
The registration of the endpoints is left to the user, usually at run time.

The API does not mandate a way for the backend to create instances but it is
strongly recommended to use the devicetree to retrieve the configuration
parameters for an instance. Currently, each backend defines its own
DT-compatible configuration that is used to configure the interface at boot
time.

Exchanging messages
===================

To send messages between domains or CPUs, an endpoint must be registered onto
an instance.

See the following example:

.. note::

   Before registering an endpoint, the instance must be opened using the
   :c:func:`ipc_msg_service_open_instance` function.


.. code-block:: c

   #include <zephyr/ipc/ipc_msg_service.h>

   static void bound_cb(void *priv)
   {
      /* Endpoint bounded */
   }

   static int recv_cb(uint16_t msg_type, const void *msg_data, void *priv)
   {
      /* Data received */

      return 0;
   }

   static struct ipc_msg_ept_cfg ept0_cfg = {
      .name = "ept0",
      .cb = {
         .bound    = bound_cb,
         .received = recv_cb,
      },
   };

   int main(void)
   {
      const struct device *inst0;
      struct ipc_ept ept0;
      struct ipc_msg_type_cmd message;
      int ret;

      inst0 = DEVICE_DT_GET(DT_NODELABEL(ipc0));
      ret = ipc_msg_service_open_instance(inst0);
      ret = ipc_msg_service_register_endpoint(inst0, &ept0, &ept0_cfg);

      /* Wait for endpoint bound (bound_cb called) */

      message.cmd = 0x01;
      ret = ipc_msg_service_send(&ept0, IPC_MSG_TYPE_CMD, &message);
   }

Querying status
===============

The API provides a way to perform query on the backend regarding instance
and endpoint.

See the following example for querying if the endpoint is ready for
exchanging messages:

.. code-block:: c

   #include <zephyr/ipc/ipc_msg_service.h>

   static void bound_cb(void *priv)
   {
      /* Endpoint bounded */
   }

   static int recv_cb(uint16_t msg_type, const void *msg_data, void *priv)
   {
      /* Data received */

      return 0;
   }

   static struct ipc_msg_ept_cfg ept0_cfg = {
      .name = "ept0",
      .cb = {
         .bound    = bound_cb,
         .received = recv_cb,
      },
   };

   int main(void)
   {
      const struct device *inst0;
      struct ipc_ept ept0;
      struct ipc_msg_type_cmd message;
      int ret;

      inst0 = DEVICE_DT_GET(DT_NODELABEL(ipc0));
      ret = ipc_msg_service_open_instance(inst0);
      ret = ipc_msg_service_register_endpoint(inst0, &ept0, &ept0_cfg);

      /* Wait for endpoint bound (bound_cb called) */

      /* Check if endpoint is ready. */
      ret = ipc_msg_service_query(&ept0, IPC_MSG_QUERY_IS_READY, NULL, NULL);
      if (ret != 0) {
         /* Endpoint is not ready */
      }

      message.cmd = 0x01;
      ret = ipc_msg_service_send(&ept0, IPC_MSG_TYPE_CMD, &message);
   }

Events
======

The backend can also do a callback when certain events come in through
the instance or endpoint.

See the following example on adding an event callback:

.. code-block:: c

   #include <zephyr/ipc/ipc_msg_service.h>

   static void bound_cb(void *priv)
   {
      /* Endpoint bounded */
   }

   static int recv_cb(uint16_t msg_type, const void *msg_data, void *priv)
   {
      /* Data received */

      return 0;
   }

   static int evt_cb(uint16_t evt_type, const void *evt_data, void *priv)
   {
      /* Event received */

      return 0;
   }

   static struct ipc_msg_ept_cfg ept0_cfg = {
      .name = "ept0",
      .cb = {
         .bound    = bound_cb,
         .event    = evt_cb,
         .received = recv_cb,
      },
   };

   int main(void)
   {
      const struct device *inst0;
      struct ipc_ept ept0;
      struct ipc_msg_type_cmd message;
      int ret;

      inst0 = DEVICE_DT_GET(DT_NODELABEL(ipc0));
      ret = ipc_msg_service_open_instance(inst0);
      ret = ipc_msg_service_register_endpoint(inst0, &ept0, &ept0_cfg);

      /* Wait for endpoint bound (bound_cb called) */

      /* Check if endpoint is ready. */
      ret = ipc_msg_service_query(&ept0, IPC_MSG_QUERY_IS_READY, NULL, NULL);
      if (ret != 0) {
         /* Endpoint is not ready */
      }

      message.cmd = 0x01;
      ret = ipc_msg_service_send(&ept0, IPC_MSG_TYPE_CMD, &message);
   }

Backends
========

The requirements needed for implementing backends give flexibility to the IPC
message service. These allow for the addition of dedicated backends having only
a subsets of features for specific use cases.

The backend must support at least the following:

* The init-time creation of instances.
* The run-time registration of an endpoint in an instance.

Additionally, the backend can also support the following:

* The run-time deregistration of an endpoint from the instance.
* The run-time closing of an instance.
* The run-time querying of an endpoint or instance status.

Each backend can have its own limitations and features that make the backend
unique and dedicated to a specific use case. The IPC message service API can be
used with multiple backends simultaneously, combining the pros and cons of each
backend.

API Reference
=============

IPC Message Service API
***********************

.. doxygengroup:: ipc_msg_service_api

IPC Message Service Backend API
*******************************

.. doxygengroup:: ipc_msg_service_backend
