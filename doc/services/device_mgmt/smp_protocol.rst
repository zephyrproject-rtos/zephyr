.. _mcumgr_smp_protocol_specification:

SMP Protocol Specification
##########################

This is description of Simple Management Protocol, SMP, that is used by
mcumgr to pass requests to devices and receive responses from them.

SMP is an application layer protocol. The underlying transport layer is not
in scope of this documentation.

Frame: The envelope
*******************

Each frame consists of header and following it data. The ``Data Length``" field in
the header may be used for reassembly purposes if underlying transport layer supports
fragmentation.
Frame is encoded in "Big Endian" (Network endianness), where field is more than
one byte lone, and takes the following form:

.. _mcumgr_smp_protocol_frame:

.. table::
    :align: center

    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |3              |2              |1              |0              |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |7|6|5|4|3|2|1|0|7|6|5|4|3|2|1|0|7|6|5|4|3|2|1|0|7|6|5|4|3|2|1|0|
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |   Res   | OP  |      Flags    |          Data Length          |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |            Group ID           | Sequence Num  |   Command ID  |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                             Data                              |
    |                             ...                               |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

.. note::
    The original specification states that SMP should support receiving
    both the "Little-endian" and "Big-endian" frames but in reality the
    mcumgr library is hardcoded to always treat "Network" side as
    "Big-endian".


The Data is optional and is not present when ``Data Length`` is zero.
The encoding of data depends on the target of group/ID.

Where meaning of fields is:

.. table::
    :align: center

    +-------------------+---------------------------------------------------+
    | Field             | Description                                       |
    +===================+===================================================+
    | ``Res``           | This is reserved, not-used field and should be    |
    |                   | always set to 0.                                  |
    +-------------------+---------------------------------------------------+
    | ``OP``            | :c:enum:`mcumgr_op_t`, determines whether         |
    |                   | information is written to a device or requested   |
    |                   | from it and whether a packet contains request to  |
    |                   | an SMP server or response from it.                |
    +-------------------+---------------------------------------------------+
    | ``Flags``         | Reserved for flags; there are no flags defined    |
    |                   | yet, the field should be set to 0                 |
    +-------------------+---------------------------------------------------+
    | ``Data Length``   | Length of the ``Data`` field                      |
    +-------------------+---------------------------------------------------+
    | ``Group ID``      | :c:enum:`mcumgr_group_t`, see                     |
    |                   | :ref:`mcumgr_smp_protocol_group_ids` for further  |
    |                   | details.                                          |
    +-------------------+---------------------------------------------------+
    | ``Sequence Num``  | This is a frame sequence number.                  |
    |                   | The number is increased by one with each request  |
    |                   | frame.                                            |
    |                   | The Sequence Num of a response should match       |
    |                   | the one in the request.                           |
    +-------------------+---------------------------------------------------+
    | ``Command ID``    | This is a command, within ``Group``.              |
    +-------------------+---------------------------------------------------+
    | ``Data``          | This is data payload of the ``Data Length``       |
    |                   | size. It is optional as ``Data Length`` may be    |
    |                   | set to zero, which means that no data follows     |
    |                   | the header.                                       |
    +-------------------+---------------------------------------------------+

.. note::
    Contents of a ``Data`` depends on a value of an ``OP``, a ``Group ID``,
    and a ``Command ID``.

.. note::
    The ``Res`` field may be repurposed by Zephyr for protocol version
    in the future.

.. _mcumgr_smp_protocol_group_ids:

Management ``Group ID``'s
=========================

The SMP protocol supports predefined common groups and allows user defined
groups. Below table presents list of common groups:


.. table::
    :align: center

    +---------------+-----------------------------------------------+
    | Decimal ID    | Group description                             |
    +===============+===============================================+
    | ``0``         | :ref:`mcumgr_smp_group_0`                     |
    +---------------+-----------------------------------------------+
    | ``1``         | :ref:`mcumgr_smp_group_1`                     |
    +---------------+-----------------------------------------------+
    | ``2``         | :ref:`mcumgr_smp_group_2`                     |
    +---------------+-----------------------------------------------+
    | ``3``         | Application/system configuration              |
    |               | (currently not used by Zephyr)                |
    +---------------+-----------------------------------------------+
    | ``4``         | Application/system log management             |
    |               | (currently not used by Zephyr)                |
    +---------------+-----------------------------------------------+
    | ``5``         | Run-time tests                                |
    |               | (unused by Zephyr)                            |
    +---------------+-----------------------------------------------+
    | ``6``         | Split image management                        |
    |               | (unused by Zephyr)                            |
    +---------------+-----------------------------------------------+
    | ``7``         | Test crashing application                     |
    |               | (unused by Zephyr)                            |
    +---------------+-----------------------------------------------+
    | ``8``         | :ref:`mcumgr_smp_group_8`                     |
    +---------------+-----------------------------------------------+
    | ``9``         | :ref:`mcumgr_smp_group_9`                     |
    +---------------+-----------------------------------------------+
    | ``63``        | Zephyr specific basic commands group          |
    +---------------+-----------------------------------------------+
    | ``64``        | This is the base group for defining           |
    |               | an application specific management groups.    |
    +---------------+-----------------------------------------------+

The payload for above groups, except for ``64`` which is not defined,
is always CBOR encoded. The group ``64``, and above, are free to be defined
by application developers and are not defined within this documentation.

Minimal response
****************

Regardless of a command issued, as long as there is SMP client on the
other side of a request, a response should be issued containing header
followed by CBOR map container.
Lack of response is only allowed when there is no SMP service or device is
non-responsive.

Minimal response SMP data
=========================

Minimal response is CBOR directory:

.. code-block:: none

    {
        (str)"rc" : (int)
    }

where:

.. table::
    :align: center

    +-----------------------+--------------------------+
    | "rc"                  | :c:enum:`mcumgr_err_t`   |
    +-----------------------+--------------------------+

Specifications of management groups supported by Zephyr
*******************************************************

.. toctree::
    :maxdepth: 1

    smp_groups/smp_group_0.rst
    smp_groups/smp_group_1.rst
    smp_groups/smp_group_2.rst
    smp_groups/smp_group_8.rst
    smp_groups/smp_group_9.rst
