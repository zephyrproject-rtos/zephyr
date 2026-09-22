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

UUIDv4 is based on random numbers (hence requires an entropy generator), and can be generated using
the :c:func:`uuid_generate_v4` function. :kconfig:option:`CONFIG_UUID_V4` must also be enabled.

.. code-block:: c

   struct uuid my_uuid;
   char uuid_str[UUID_STR_LEN];

   int ret = uuid_generate_v4(&my_uuid);

   if (ret != 0) {
       printk("Failed to generate UUID v4 (err %d)\n", ret);
       return ret;
   }

   ret = uuid_to_string(&my_uuid, uuid_str);
   if (ret != 0) {
       printk("Failed to convert UUID to string (err %d)\n", ret);
       return ret;
   }

    printk("UUID: %s\n", uuid_str);

Generating a UUIDv5
-------------------

UUIDv5 is generated from a namespace UUID and a name (binary data), and can be generated using the
:c:func:`uuid_generate_v5` function. :kconfig:option:`CONFIG_UUID_V5` must also be enabled.

UUIDv5 is deterministic: the same namespace and name will always result in the same UUID.

.. code-block:: c

   struct uuid ns_uuid;
   struct uuid my_uuid;
   char uuid_str[UUID_STR_LEN];

   /* Well-known namespace ID for URLs (RFC 9562) */
   uuid_from_string("6ba7b811-9dad-11d1-80b4-00c04fd430c8", &ns_uuid);

   const char *name = "https://zephyrproject.org/";
   int ret = uuid_generate_v5(&ns_uuid, name, strlen(name), &my_uuid);

   if (ret != 0) {
       printk("Failed to generate UUID v5 (err %d)\n", ret);
       return ret;
   }

   ret = uuid_to_string(&my_uuid, uuid_str);
   if (ret != 0) {
       printk("Failed to convert UUID to string (err %d)\n", ret);
       return ret;
   }

   /* outputs: "UUID: abdb72b5-b342-5882-925b-4ce0e3cb4790" */
   printk("UUID: %s\n", uuid_str);

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
