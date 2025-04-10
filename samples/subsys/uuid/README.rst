.. zephyr:code-sample:: uuid
   :name: UUID

   Manipulate UUID v4 and v5 compliant with IETF RFC 9562.

Overview
********

This sample app demonstrates the use of the UUID utilities to generate and manipulate UUIDs
accordingly to IETF RFC 9562.

The following functionality is demonstrated:
- UUIDv4 generation
- UUIDv5 generation from namespace and data
- UUID conversion from/to string and to base64 and base64 URL safe formats

Requirements
************

This sample relies on the following modules:
- MbedTLS for the UUIDv5 hash functions
- Base64 for the base64 encoding of UUIDs
- Entropy source for the pseudo-random generation of UUIDv4

Building and Running
********************

Use the standard ``west`` commands to build and flash this application.
For example for ``native_sim`` build with:
```
west build -p -b native_sim samples/subsys/uuid
```
Then run with:
```
west build -t run
```
