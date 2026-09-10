.. _json_api:

JSON
####

Zephyr provides a JSON library that can be used to encode and decode JSON data.

Usage
*****

Defining the Data Structure
===========================

First, define the C structure that corresponds to the JSON object and the descriptor that maps the
structure fields to JSON tokens.

.. code-block:: c

   #include <zephyr/data/json.h>

   struct foo {
       int bar;
       const char *baz;
   };

   static const struct json_obj_descr foo_descr[] = {
       JSON_OBJ_DESCR_PRIM(struct foo, bar, JSON_TOK_NUMBER),
       JSON_OBJ_DESCR_PRIM(struct foo, baz, JSON_TOK_STRING),
   };

Encoding
========

To encode a C structure into a JSON string, use :c:func:`json_obj_encode_buf`.

.. code-block:: c

   void encode_example(void)
   {
       struct foo data = { .bar = 42, .baz = "hello" };
       char buffer[128];
       int ret;

       ret = json_obj_encode_buf(foo_descr, ARRAY_SIZE(foo_descr),
                                 &data,
                                 buffer, sizeof(buffer));
       if (ret < 0) {
           /* handle error */
       }
   }

Decoding
========

To decode a JSON string into a C structure, use :c:func:`json_obj_parse`.

.. code-block:: c

   void decode_example(void)
   {
       struct foo data;
       const char *json_text = "{\"bar\": 42, \"baz\": \"hello\"}";
       int ret;

       ret = json_obj_parse(json_text, strlen(json_text),
                            foo_descr, ARRAY_SIZE(foo_descr),
                            &data);
       if (ret < 0) {
           /* handle error */
       }
   }

Configuration
*************

To enable JSON support, enable the :kconfig:option:`CONFIG_JSON_LIBRARY` Kconfig option.

API Reference
*************

.. doxygengroup:: json
