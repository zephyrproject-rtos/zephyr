.. _cs_trace_defmt:

ARM Coresight Trace Deformatter
###############################

Formatter is a method of wrapping multiple trace streams (specified by 7 bit ID) into a
single output stream. Formatter is using 16 byte frames which wraps up to 15 bytes of
data. It is used, for example, by ETR (Embedded Trace Router) which is a circular RAM
buffer where data from various trace streams can be saved. Typically tracing data is
decoded offline by the host but deformatter can be used on-chip to decode the data during
application runtime.

Usage
*****

Deformatter is initialized with a user callback. Data is decoded using
:c:func:`cs_trace_defmt_process` in 16 bytes chunks. Callback is called whenever stream changes or
end of chunk is reached. Callback contains stream ID and the data.

API documentation
*****************

.. doxygengroup:: cs_trace_defmt
