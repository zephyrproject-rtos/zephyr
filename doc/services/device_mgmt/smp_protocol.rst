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
    |                   |                                                   |
    +-------------------+---------------------------------------------------+
    | ``OP``            | :ref:`mcumgr_smp_protocol_op_code`                |
    +-------------------+---------------------------------------------------+
    | ``Flags``         | Reserved for flags; there are no flags defined    |
    |                   | yet, the field should be set to 0                 |
    +-------------------+---------------------------------------------------+
    | ``Data Length``   | Length of the ``Data`` field                      |
    +-------------------+---------------------------------------------------+
    | ``Group ID``      | :ref:`mcumgr_smp_protocol_group_ids`              |
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

.. _mcumgr_smp_protocol_op_code:

Operation code
==============

The operation code determines whether an information is written to a device or
requested from it and whether a packet contains request to a SMP server or
response from it.

Following operation codes are defined.

.. table::
    :align: center

    +---------------+-----------------------------------------------+
    | Decimal ID    | Operation                                     |
    +===============+===============================================+
    | ``0``         | read request                                  |
    +---------------+-----------------------------------------------+
    | ``1``         | read response                                 |
    +---------------+-----------------------------------------------+
    | ``2``         | write request                                 |
    +---------------+-----------------------------------------------+
    | ``3``         | write response                                |
    +---------------+-----------------------------------------------+


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

    +-----------------------+---------------------------------------------------+
    | "rc"                  | :ref:`mcumgr_smp_protocol_status_codes`           |
    +-----------------------+---------------------------------------------------+

.. _mcumgr_smp_protocol_status_codes:

Status/error codes in responses
===============================

.. table::
    :align: center

    +---------------+-----------------------------------------------+
    | Decimal ID    | Meaning                                       |
    +===============+===============================================+
    | ``0``         | No error, OK.                                 |
    +---------------+-----------------------------------------------+
    | ``1``         | Unknown error.                                |
    +---------------+-----------------------------------------------+
    | ``2``         | Not enough memory; this error is reported     |
    |               | when there is not enough memory to complete   |
    |               | response.                                     |
    +---------------+-----------------------------------------------+
    | ``3``         | Invalid value; a request contains an invalid  |
    |               | value.                                        |
    +---------------+-----------------------------------------------+
    | ``4``         | Timeout; the operation for some reason could  |
    |               | not be completed in assumed time.             |
    +---------------+-----------------------------------------------+
    | ``5``         | No entry; the error means that request frame  |
    |               | has been missing some information that is     |
    |               | required to perform action.                   |
    |               | It may also mean that requested information   |
    |               | is not available.                             |
    +---------------+-----------------------------------------------+
    | ``6``         | Bad state; the error means that application   |
    |               | or device is in a state that would not allow  |
    |               | it to perform or complete a requested action. |
    +---------------+-----------------------------------------------+
    | ``7``         | Response too long; this error is issued when  |
    |               | buffer assigned for gathering response is     |
    |               | not big enough.                               |
    +---------------+-----------------------------------------------+
    | ``8``         | Not supported; usually issued when requested  |
    |               | ``Group ID`` or ``Command ID`` is not         |
    |               | supported by application.                     |
    +---------------+-----------------------------------------------+
    | ``9``         | Corrupted payload received.                   |
    +---------------+-----------------------------------------------+
    | ``10``        | Device is busy with processing previous SMP   |
    |               | request and may not process incoming one.     |
    |               | Client should re-try later.                   |
    +---------------+-----------------------------------------------+
    | ``256``       | This is base error number of user defined     |
    |               | error codes.                                  |
    +---------------+-----------------------------------------------+


Zephyr uses ``MGMT_ERR_`` prefixed definitions gathered in this header file
:zephyr_file:`subsys/mgmt/mcumgr/lib/mgmt/include/mgmt/mgmt.h`

Specifications of management groups supported by Zephyr
*******************************************************

.. toctree::
    :maxdepth: 1

    smp_groups/smp_group_0.rst
    smp_groups/smp_group_1.rst
    smp_groups/smp_group_2.rst
    smp_groups/smp_group_8.rst
    smp_groups/smp_group_9.rst
