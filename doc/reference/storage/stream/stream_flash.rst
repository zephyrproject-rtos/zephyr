.. _stream_flash:

Stream Flash
############
The Stream Flash module takes contiguous fragments of a stream of data (e.g.
from radio packets), aggregates them into a user-provided buffer, then when the
buffer fills (or stream ends) writes it to a raw flash partition.  It supports
providing the read-back buffer to the client to use in validating the persisted
stream content.

One typical use of a stream write operation is when receiving a new firmware
image to be used in a DFU operation.

There are several reasons why one might want to use buffered writes instead of
writing the data directly as it is made available. Some devices have hardware
limitations which does not allow flash writes to be performed in parallell with
other operations, such as radio RX and TX. Also, fewer write operations result
in faster response times seen from the application.

API Reference
*************

.. doxygengroup:: stream_flash
   :project: Zephyr
