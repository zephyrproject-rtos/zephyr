.. _bbram_api:

Battery Backed RAM (BBRAM)
##########################

The BBRAM APIs allow interfacing with the unique properties of this memory region. The following
common types of BBRAM properties are easily accessed via this API:

- IBBR (invalid) state - check that the BBRAM is not corrupt.
- VSBY (voltage standby) state - check if the BBRAM is using standby voltage.
- VCC (active power) state - check if the BBRAM is on normal power.
- Size - get the size (in bytes) of the BBRAM region.

Along with these, the API provides a means for reading and writing to the memory region via
:c:func:`bbram_read` and :c:func:`bbram_write` respectively. Both functions are expected to only
succeed if the BBRAM is in a valid state and the operation is bounded to the memory region.

API Reference
*************

.. doxygengroup:: bbram_interface
