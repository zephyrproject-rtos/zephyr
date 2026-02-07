.. _uuid_api:

UUID
####

Overview
********

Universally Unique Identifiers (UUID), also known as Globally Unique IDentifiers (GUIDs), are
128-bit identifiers intended to guarantee uniqueness across space and time. They are defined by
:rfc:`9562`.

The UUID subsystem provides utility functions for the generation, parsing, and manipulation of
UUIDs. It supports generating version 4 (random) and version 5 (name-based) UUIDs.

Usage
=====

To use the UUID API, include the header file:

.. code-block:: c

   #include <zephyr/sys/uuid.h>

Generating a UUIDv4
-------------------

UUIDv4 is based on random number (hence requires an entropy generator), and can be generated using
the :c:func:`uuid_generate_v4` function. :kconfig:option:`CONFIG_UUID_V4` must also be enabled.

.. code-block:: c

   struct uuid my_uuid;
   char uuid_str[UUID_STR_LEN];
   int ret = uuid_generate_v4(&my_uuid);
   if (ret == 0) {
       uuid_to_string(&my_uuid, uuid_str);
       printk("UUID: %s\n", uuid_str);
   }

Generating a UUIDv5
-------------------

UUIDv5 is generated from a namespace UUID and a name (binary data), and can be generated using the
:c:func:`uuid_generate_v5` function. :kconfig:option:`CONFIG_UUID_V5` must also be enabled.

UUIDv5 is deterministic: the same namespace and name will always result in the same UUID.

.. code-block:: c

   struct uuid ns_uuid; /* Initialize with a namespace UUID */
   struct uuid my_uuid;
   const char *name = "zephyrproject.org";

   /* Initialize ns_uuid with a valid UUID, here the well-known namespace ID for URLs */
   uuid_from_str("6ba7b811-9dad-11d1-80b4-00c04fd430c8", &ns_uuid);

   int ret = uuid_generate_v5(&ns_uuid, name, strlen(name), &my_uuid);
   if (ret == 0) {
       uuid_to_string(&my_uuid, uuid_str);
       printk("UUID: %s\n", uuid_str); /* outputs: "UUID: 16ff8549-ea80-53be-a6d0-96b064c39bc6" */
   }

Configuration
*************

Related configuration options:

* :kconfig:option:`CONFIG_UUID`
* :kconfig:option:`CONFIG_UUID_V4`
* :kconfig:option:`CONFIG_UUID_V5`
* :kconfig:option:`CONFIG_UUID_BASE64`

API Reference
*************

.. doxygengroup:: uuid
