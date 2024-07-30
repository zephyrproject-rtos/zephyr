.. _mcumgr_smp_group_2:

Statistics management
#####################

Statistics management allows to obtain data gathered by Statistics subsystem
of Zephyr, enabled with :kconfig:option:`CONFIG_STATS`.

Statistics management group defines commands:

.. table::
    :align: center

    +-------------------+-----------------------------------------------+
    | ``Command ID``    | Command description                           |
    +===================+===============================================+
    | ``0``             | Group data                                    |
    +-------------------+-----------------------------------------------+
    | ``1``             | List groups                                   |
    +-------------------+-----------------------------------------------+

Statistics: group data
**********************

The command is used to obtain data for group specified by a name.
The name is one of group names as registered, with
:c:macro:`STATS_INIT_AND_REG` macro or :c:func:`stats_init_and_reg` function
call, within module that gathers the statistics.

Statistics: group data request
==============================

Statistics group data request header:

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``0``  | ``2``        |  ``0``         |
    +--------+--------------+----------------+

CBOR data of request:

.. code-block:: none

    {
        (str)"name" :  (str)
    }

where:

.. table::
    :align: center

    +-----------------------+---------------------------------------------------+
    | "name"                | group name.                                       |
    +-----------------------+---------------------------------------------------+

Statistics: group data response
===============================

Statistics group data response header:

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``1``  | ``2``        |  ``0``         |
    +--------+--------------+----------------+

CBOR data of successful response:

.. code-block:: none

    {
        (str)"name"     : (str)
        (str)"fields"   : {
            (str)<entry_name> : (uint)
            ...
        }
    }

In case of error the CBOR data takes the form:


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
    | "name"           | this is name of group the response contains data for.                   |
    +------------------+-------------------------------------------------------------------------+
    | "fields"         | this is map of entries within groups that consists of pairs where the   |
    |                  | entry name is mapped to value it represents in statistics.              |
    +------------------+-------------------------------------------------------------------------+
    | <entry_name>     | single entry to value mapping; value is hardcoded to unsigned integer   |
    |                  | type, in a CBOR meaning.                                                |
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

Statistics: list of groups
**************************

The command is used to obtain list of groups of statistics that are gathered
on a device. This is a list of names as given to groups with
:c:macro:`STATS_INIT_AND_REG` macro or :c:func:`stats_init_and_reg` function
calls, within module that gathers the statistics; this means that this command
may be considered optional as it is known during compilation what groups will
be included into build and listing them is not needed prior to issuing a query.

Statistics: list of groups request
==================================

Statistics group list request header:

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``0``  | ``2``        |  ``1``         |
    +--------+--------------+----------------+

The command sends an empty CBOR map as data.

Statistics: list of groups response
===================================

Statistics group list request header:

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``1``  | ``2``        |  ``1``         |
    +--------+--------------+----------------+

CBOR data of successful response:

.. code-block:: none

    {
        (str)"stat_list" :  [
            (str)<stat_group_name>, ...
        ]
    }

In case of error the CBOR data takes the form:


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
    | "stat_list"      | array of strings representing group names; this array may be empty if   |
    |                  | there are no groups.                                                    |
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
