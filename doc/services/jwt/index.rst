.. _jwt_api:

JSON Web Token (JWT)
####################

Overview
********

JSON Web Tokens (JWT) are an open, industry standard (:rfc:`7519`) method for representing claims
securely between two parties.

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
The payload is described with a :ref:`JSON object descriptor <json_api>`, so any set of
claims can be used. The token is signed with a PSA key that the caller has already imported or
generated.

The header is derived from the key and the signature algorithm, so the ``alg`` value it announces
always describes the signature that is actually produced. The pair has to map onto one of the JWS
algorithms of :rfc:`7518#section-3.1`, which are ``RS256``/``RS384``/``RS512``,
``PS256``/``PS384``/``PS512`` and ``ES256``/``ES384``/``ES512``. Any other combination is rejected
with ``-ENOTSUP``.

.. code-block:: c

   #include <zephyr/data/jwt.h>

   struct claims {
       const char *sub;
       int64_t exp;
       int64_t iat;
   };

   static const struct json_obj_descr claims_descr[] = {
       JSON_OBJ_DESCR_PRIM(struct claims, sub, JSON_TOK_STRING),
       JSON_OBJ_DESCR_PRIM(struct claims, exp, JSON_TOK_INT64),
       JSON_OBJ_DESCR_PRIM(struct claims, iat, JSON_TOK_INT64),
   };

   const struct claims claims = {
       .sub = "device-1234",
       .exp = 1767221999,
       .iat = 1764605987,
   };

   struct jwt_builder builder;
   char buffer[1024];
   int ret;

   /* Initialize the builder and write the header. This emits "ES256". */
   ret = jwt_init_builder(&builder, buffer, sizeof(buffer), key_id,
                          PSA_ALG_ECDSA(PSA_ALG_SHA_256));
   if (ret < 0) {
       /* Handle error */
   }

   /* Add the claims */
   ret = jwt_add_payload(&builder, &claims, claims_descr, ARRAY_SIZE(claims_descr));
   if (ret < 0) {
       /* Handle error */
   }

   /* Sign the token with the key given to the builder */
   ret = jwt_sign(&builder);
   if (ret < 0) {
       /* Handle error */
   }

   /*
    * buffer now contains the JWT; it can be passed to a third-party service that will be able
    * to validate it against the public key associated with `key_id`.
    */

Configuration
*************

Related configuration options:

* :kconfig:option:`CONFIG_JWT`

API Reference
*************

.. doxygengroup:: jwt
