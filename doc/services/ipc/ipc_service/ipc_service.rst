.. _ipc_service:

IPC service
###########

.. contents::
    :local:
    :depth: 2

The IPC service API provides an interface to exchange data between two domains
or CPUs.

Overview
========

An IPC service communication channel consists of one instance and one or
several endpoints associated with the instance.

An instance is the external representation of a physical communication channel
between two domains or CPUs. The actual implementation and internal
representation of the instance is peculiar to each backend.

An individual instance is not used to send data between domains/CPUs. To send
and receive the data, the user must create (register) an endpoint in the
instance. This allows for the connection of the two domains of interest.

It is possible to have zero or multiple endpoints for one single instance,
possibly with different priorities, and to use each to exchange data.
Endpoint prioritization and multi-instance ability highly depend on the backend
used.

The endpoint is an entity the user must use to send and receive data between
two domains (connected by the instance). An endpoint is always associated to an
instance.

The creation of the instances is left to the backend, usually at init time.
The registration of the endpoints is left to the user, usually at run time.

The API does not mandate a way for the backend to create instances but it is
strongly recommended to use the devicetree to retrieve the configuration
parameters for an instance. Currently, each backend defines its own
DT-compatible configuration that is used to configure the interface at boot
time.

The following usage scenarios are supported:

* Simple data exchange.
* Data exchange using the no-copy API.

Simple data exchange
====================

To send data between domains or CPUs, an endpoint must be registered onto
an instance.

See the following example:

.. note::

   Before registering an endpoint, the instance must be opened using the
   :c:func:`ipc_service_open_instance` function.


.. code-block:: c

   #include <zephyr/ipc/ipc_service.h>

   static void bound_cb(void *priv)
   {
      /* Endpoint bounded */
   }

   static void recv_cb(const void *data, size_t len, void *priv)
   {
      /* Data received */
   }

   static struct ipc_ept_cfg ept0_cfg = {
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
      int ret;

      inst0 = DEVICE_DT_GET(DT_NODELABEL(ipc0));
      ret = ipc_service_open_instance(inst0);
      ret = ipc_service_register_endpoint(inst0, &ept0, &ept0_cfg);

      /* Wait for endpoint bound (bound_cb called) */

      unsigned char message[] = "hello world";
      ret = ipc_service_send(&ept0, &message, sizeof(message));
   }

Data exchange using the no-copy API
===================================

If the backend supports the no-copy API you can use it to directly write and
read to and from shared memory regions.

See the following example:

.. code-block:: c

   #include <zephyr/ipc/ipc_service.h>
   #include <stdint.h>
   #include <string.h>

   static struct ipc_ept ept0;

   static void bound_cb(void *priv)
   {
      /* Endpoint bounded */
   }

   static void recv_cb_nocopy(const void *data, size_t len, void *priv)
   {
      int ret;

      ret = ipc_service_hold_rx_buffer(&ept0, (void *)data);
      /* Process directly or put the buffer somewhere else and release. */
      ret = ipc_service_release_rx_buffer(&ept0, (void *)data);
   }

   static struct ipc_ept_cfg ept0_cfg = {
      .name = "ept0",
      .cb = {
         .bound    = bound_cb,
         .received = recv_cb,
      },
   };

   int main(void)
   {
      const struct device *inst0;
      int ret;

      inst0 = DEVICE_DT_GET(DT_NODELABEL(ipc0));
      ret = ipc_service_open_instance(inst0);
      ret = ipc_service_register_endpoint(inst0, &ept0, &ept0_cfg);

      /* Wait for endpoint bound (bound_cb called) */
      void *data;
      unsigned char message[] = "hello world";
      uint32_t len = sizeof(message);

      ret = ipc_service_get_tx_buffer(&ept0, &data, &len, K_FOREVER);

      memcpy(data, message, len);

      ret = ipc_service_send_nocopy(&ept0, data, sizeof(message));
   }

Backends
========

The requirements needed for implementing backends give flexibility to the IPC
service. These allow for the addition of dedicated backends having only a
subsets of features for specific use cases.

The backend must support at least the following:

* The init-time creation of instances.
* The run-time registration of an endpoint in an instance.

Additionally, the backend can also support the following:

* The run-time deregistration of an endpoint from the instance.
* The run-time closing of an instance.
* The no-copy API.

Each backend can have its own limitations and features that make the backend
unique and dedicated to a specific use case. The IPC service API can be used
with multiple backends simultaneously, combining the pros and cons of each
backend.

.. toctree::
   :maxdepth: 1

   backends/ipc_service_icmsg.rst
   backends/ipc_service_icbmsg.rst

API Reference
=============

IPC service API
***************

.. doxygengroup:: ipc_service_api

IPC service backend API
***********************

.. doxygengroup:: ipc_service_backend
