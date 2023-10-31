.. _mcumgr_smp_group_63:

Zephyr Management Group
#######################

Zephyr management group defines the following commands:

.. table::
    :align: center

    +----------------+------------------------------+
    | ``Command ID`` | Command description          |
    +================+==============================+
    | ``0``          | Erase storage                |
    +----------------+------------------------------+

Erase storage command
*********************

Erase storage command allows clearing the ``storage_partition`` flash partition on a device,
generally this is used when switching to a new application build if the application uses storage
that should be cleared (application dependent).

Erase storage request
=====================

Erase storage request header fields:

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``2``  | ``63``       | ``0``          |
    +--------+--------------+----------------+

The command sends sends empty CBOR map as data.

Erase storage response
======================

Read setting response header fields:

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``3``  | ``63``       | ``0``          |
    +--------+--------------+----------------+

The command sends an empty CBOR map as data if successful. In case of error the CBOR data takes
the form:

.. tabs::

   .. group-tab:: SMP version 2

      .. code-block:: none

          {
              (str)"err" : {
                  (str)"group"    : (uint)
                  (str)"rc"       : (uint)
              }
          }

   .. group-tab:: SMP version 1

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
