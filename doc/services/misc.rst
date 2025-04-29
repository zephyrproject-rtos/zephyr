.. _misc_api:

Miscellaneous
#############

.. comment
   not documenting
   .. doxygengroup:: checksum
   .. doxygengroup:: structured_data

Checksum APIs
*************

CRC
=====

.. doxygengroup:: crc

Structured Data APIs
********************

JSON
====

.. doxygengroup:: json

JWT
===

JSON Web Tokens (JWT) are an open, industry standard [RFC
7519](https://tools.ietf.org/html/rfc7519) method for representing
claims securely between two parties.  Although JWT is fairly flexible,
this API is limited to creating the simplistic tokens needed to
authenticate with the Google Core IoT infrastructure.

.. doxygengroup:: jwt

Identifier APIs
***************

UUID
====

Universally Unique Identifiers (UUID), also known as Globally Unique
IDentifiers (GUIDs) are an open, industry standard [RFC
9562](https://tools.ietf.org/html/rfc9562) 128 bits long identifiers
intended to guarantee uniqueness across space and time.

.. doxygengroup:: uuid
