.. _jwt_api:

JSON Web Token (JWT)
####################

Overview
********

JSON Web Tokens (JWT) are an open, industry standard (:rfc:`7519`) method for representing claims
securely between two parties. Although JWT is fairly flexible, this API is limited to creating the
simplistic tokens needed to authenticate with the Google Core IoT infrastructure.

At a high level, a JWT is just a signed blob of JSON that a client can present as a token instead
of sending, for example, their username/password every time.

Usage
=====

To use the JWT API, include the header file:

.. code-block:: c

   #include <zephyr/data/jwt.h>

Generating a JWT
----------------

The JWT subsystem provides a lightweight, builder-based API for constructing JSON Web Tokens (JWT).
It allows for the creation of tokens where the payload (claims) contains: expiration time, issued-at
time, and audience. The token is then signed using a provided private key.

.. code-block:: c

   #include <zephyr/data/jwt.h>

   struct jwt_builder builder;
   char buffer[1024];
   int ret;

   /* Initialize the builder */
   ret = jwt_init_builder(&builder, buffer, sizeof(buffer));
   if (ret < 0) {
       /* Handle error */
   }

   /* Add payload: expiration, issued at, audience */
   ret = jwt_add_payload(&builder, 1767221999, 1764605987, "project-id");
   if (ret < 0) {
       /* Handle error */
   }

   /* Sign the token using a DER-encoded private key */
   ret = jwt_sign(&builder, private_key_der, private_key_der_len);
   if (ret < 0) {
       /* Handle error */
   }

   /*
    * buffer now contains the JWT; it can be passed to a third-party service that will be able
    * to validate it against the public key associated with `private_key_der`.
    */

Configuration
*************

Related configuration options:

* :kconfig:option:`CONFIG_JWT`
* :kconfig:option-regex:`CONFIG_JWT_.*`


API Reference
*************

.. doxygengroup:: jwt
