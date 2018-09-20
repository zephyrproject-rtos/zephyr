.. _misc_api:

Miscellaneous APIs
##################

.. comment
   not documenting
   .. doxygengroup:: checksum
   .. doxygengroup:: structured_data

Checksum APIs
*************

CRC
=====

.. doxygengroup:: crc
   :project: Zephyr

Structured Data APIs
********************

JSON
====

.. doxygengroup:: json
   :project: Zephyr

JWT
===

JSON Web Tokens (JWT) are an open, industry standard [RFC
7519](https://tools.ietf.org/html/rfc7519) method for representing
claims securely between two parties.  Although JWT is fairly flexible,
this API is limited to creating the simplistic tokens needed to
authenticate with the Google Core IoT infrastructure.

.. doxygengroup:: jwt
   :project: Zephyr
