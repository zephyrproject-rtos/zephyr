.. _mcumgr_smp_protocol_specification:

SMP Protocol Specification
##########################

This is description of Simple Management Protocol, SMP, that is used by
MCUmgr to pass requests to devices and receive responses from them.

SMP is an application layer protocol. The underlying transport layer is not
in scope of this documentation.

.. note::
    SMP in this context refers to SMP for MCUmgr (Simple Management Protocol),
    it is unrelated to SMP in Bluetooth (Security Manager Protocol), but there
    is an MCUmgr SMP transport for Bluetooth.

Frame: The envelope
*******************

Each frame consists of a header and data. The ``Data Length`` field in the
header may be used for reassembly purposes if underlying transport layer supports
fragmentation.
Frames are encoded in "Big Endian" (Network endianness) when fields are more than
one byte long, and takes the following form:

.. _mcumgr_smp_protocol_frame:

.. table::
    :align: center

    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |3              |2              |1              |0              |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |7|6|5|4|3|2|1|0|7|6|5|4|3|2|1|0|7|6|5|4|3|2|1|0|7|6|5|4|3|2|1|0|
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    | Res |Ver| OP  |      Flags    |          Data Length          |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |            Group ID           | Sequence Num  |   Command ID  |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                             Data                              |
    |                             ...                               |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

.. note::
    The original specification states that SMP should support receiving
    both the "Little-endian" and "Big-endian" frames but in reality the
    MCUmgr library is hardcoded to always treat "Network" side as
    "Big-endian".


Data is optional and is not present when ``Data Length`` is zero.
The encoding of data depends on the target of group/ID.

A description of the various fields and their meaning:

.. table::
    :align: center

    +-------------------+---------------------------------------------------+
    | Field             | Description                                       |
    +===================+===================================================+
    | ``Res``           | This is reserved, not-used field and must be      |
    |                   | always set to 0.                                  |
    +-------------------+---------------------------------------------------+
    | ``Ver`` (Version) | This indicates the version of the protocol being  |
    |                   | used, this should be set to 0b01 to use the newer |
    |                   | SMP transport where error codes are more detailed |
    |                   | and returned in the map, otherwise left as 0b00   |
    |                   | to use the legacy SMP protocol. Versions 0b10 and |
    |                   | 0b11 are reserved for future use and should not   |
    |                   | be used.                                          |
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
    Contents of ``Data`` depends on a value of an ``OP``, a ``Group ID``,
    and a ``Command ID``.

.. _mcumgr_smp_protocol_group_ids:

Management ``Group ID``'s
=========================

The SMP protocol supports predefined common groups and allows user defined
groups. The following table presents a list of common groups:


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
    | ``3``         | :ref:`mcumgr_smp_group_3`                     |
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
    | ``63``        | :ref:`mcumgr_smp_group_63`                    |
    +---------------+-----------------------------------------------+
    | ``64``        | This is the base group for defining           |
    |               | an application specific management groups.    |
    +---------------+-----------------------------------------------+

The payload for above groups, except for user groups (``64`` and above) is
always CBOR encoded. The group ``64``, and above can define their own scheme
for data communication.

Minimal response
****************

Regardless of a command issued, as long as there is SMP client on the
other side of a request, a response should be issued containing the header
followed by CBOR map container.
Lack of response is only allowed when there is no SMP service or device is
non-responsive.

Minimal response SMP data
=========================

Minimal response is:

.. tabs::

   .. group-tab:: SMP version 2

      .. code-block:: none

          {
              (str)"err" : {
                  (str)"group"    : (uint)
                  (str)"rc"       : (uint)
              }
          }

   .. group-tab:: SMP version 1 (and non-group SMP version 2)

      .. code-block:: none

          {
              (str)"rc"       : (int)
          }

where:

.. table::
    :align: center

    +------------------+-------------------------------------------------------------------------+
    | "err" -> "group" | :c:enum:`mcumgr_group_t` group of the group-based error code. Only      |
    |                  | appears if an error is returned when using SMP version 2.               |
    +------------------+-------------------------------------------------------------------------+
    | "err" -> "rc"    | contains the index of the group-based error code. Only appears if       |
    |                  | non-zero (error condition) when using SMP version 2.                    |
    +------------------+-------------------------------------------------------------------------+
    | "rc"             | :c:enum:`mcumgr_err_t` only appears if non-zero (error condition) when  |
    |                  | using SMP version 1 or for SMP errors when using SMP version 2.         |
    +------------------+-------------------------------------------------------------------------+

Note that in the case of a successful command, an empty map will be returned (``rc``/``err`` is
only returned if there is an error condition, therefore if only an empty map is returned or a
response lacks these, the request can be considered as being successful. For SMP version 2,
errors relating to SMP itself that are not group specific will still be returned as ``rc``
errors, SMP version 2 clients must therefore be able to handle both types of errors.

Specifications of management groups supported by Zephyr
*******************************************************

.. toctree::
    :maxdepth: 1

    smp_groups/smp_group_0.rst
    smp_groups/smp_group_1.rst
    smp_groups/smp_group_2.rst
    smp_groups/smp_group_3.rst
    smp_groups/smp_group_8.rst
    smp_groups/smp_group_9.rst
