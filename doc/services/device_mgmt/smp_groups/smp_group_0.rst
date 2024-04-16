.. _mcumgr_smp_group_0:

Default/OS Management Group
###########################

OS management group defines following commands:

.. table::
    :align: center

    +-------------------+-----------------------------------------------+
    | ``Command ID``    | Command description                           |
    +===================+===============================================+
    | ``0``             | Echo                                          |
    +-------------------+-----------------------------------------------+
    | ``1``             | Console/Terminal echo control;                |
    |                   | unimplemented by Zephyr                       |
    +-------------------+-----------------------------------------------+
    | ``2``             | Task Statistics                               |
    +-------------------+-----------------------------------------------+
    | ``3``             | Memory pool statistics                        |
    +-------------------+-----------------------------------------------+
    | ``4``             | Date-time string; unimplemented by Zephyr     |
    +-------------------+-----------------------------------------------+
    | ``5``             | System reset                                  |
    +-------------------+-----------------------------------------------+
    | ``6``             | MCUMGR parameters                             |
    +-------------------+-----------------------------------------------+
    | ``7``             | OS/Application info                           |
    +-------------------+-----------------------------------------------+
    | ``8``             | Bootloader information                        |
    +-------------------+-----------------------------------------------+

Echo command
************

Echo command responses by sending back string that it has received.

Echo request
============

Echo request header fields:

.. table::
    :align: center

    +--------------------+--------------+----------------+
    | ``OP``             | ``Group ID`` | ``Command ID`` |
    +====================+==============+================+
    | ``0`` or ``2``     | ``0``        |  ``0``         |
    +--------------------+--------------+----------------+

CBOR data of request:

.. code-block:: none

    {
        (str)"d" : (str)
    }

where:

.. table::
    :align: center

    +-----------------------+---------------------------------------------------+
    | "d"                   | string to be replied by echo service.             |
    +-----------------------+---------------------------------------------------+

Echo response
=============

Echo response header fields:

.. table::
    :align: center

    +--------+--------------+----------------+----------------------------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` | Note                             |
    +========+==============+================+==================================+
    | ``1``  | ``0``        |  ``0``         | When request ``OP`` was ``0``    |
    +--------+--------------+----------------+----------------------------------+
    | ``3``  | ``0``        |  ``0``         | When request ``OP`` was ``2``    |
    +--------+--------------+----------------+----------------------------------+

CBOR data of successful response:

.. code-block:: none

    {
        (str)"r"        : (str)
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
    | "r"              | replying echo string.                                                   |
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

Task statistics command
***********************

The command responds with some system statistics.

Task statistics request
=======================

Task statistics request header fields:

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``0``  | ``0``        |  ``2``         |
    +--------+--------------+----------------+

The command sends an empty CBOR map as data.


Task statistics response
========================

Task statistics response header fields:

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``1``  | ``0``        |  ``2``         |
    +--------+--------------+----------------+

CBOR data of successful response:

.. code-block:: none

    {
        (str)"tasks" : {
            (str)<task_name> : {
                (str)"prio"         : (uint)
                (str)"tid"          : (uint)
                (str)"state"        : (uint)
                (str)"stkuse"       : (uint)
                (str)"stksiz"       : (uint)
                (str)"cswcnt"       : (uint)
                (str)"runtime"      : (uint)
                (str)"last_checkin" : (uint)
                (str)"next_checkin" : (uint)
            }
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
    | <task_name>      | string identifying task.                                                |
    +------------------+-------------------------------------------------------------------------+
    | "prio"           | task priority.                                                          |
    +------------------+-------------------------------------------------------------------------+
    | "tid"            | numeric task ID.                                                        |
    +------------------+-------------------------------------------------------------------------+
    | "state"          | numeric task state.                                                     |
    +------------------+-------------------------------------------------------------------------+
    | "stkuse"         | task's/thread's stack usage.                                            |
    +------------------+-------------------------------------------------------------------------+
    | "stksiz"         | task's/thread's stack size.                                             |
    +------------------+-------------------------------------------------------------------------+
    | "cswcnt"         | task's/thread's context switches.                                       |
    +------------------+-------------------------------------------------------------------------+
    | "runtime"        | task's/thread's runtime in "ticks".                                     |
    +------------------+-------------------------------------------------------------------------+
    | "last_checkin"   | set to 0 by Zephyr.                                                     |
    +------------------+-------------------------------------------------------------------------+
    | "next_checkin"   | set to 0 by Zephyr.                                                     |
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

.. note::
    The unit for "stkuse" and "stksiz" is system dependent and in case of Zephyr
    this is number of 4 byte words.

Memory pool statistics
**********************

The command is used to obtain information on memory pools active in running
system.

Memory pool statistic request
=============================

Memory pool statistics request header fields:

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``0``  | ``0``        |  ``3``         |
    +--------+--------------+----------------+

The command sends an empty CBOR map as data.

Memory pool statistics response
===============================

Memory pool statistics response header fields:

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``1``  | ``0``        |  ``3``         |
    +--------+--------------+----------------+

CBOR data of successful response:

.. code-block:: none

    {
        (str)<pool_name> {
            (str)"blksiz"   : (int)
            (str)"nblks"    : (int)
            (str)"nfree"    : (int)
            (str)"min'      : (int)
        }
        ...
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
    | <pool_name>      | string representing the pool name, used as a key for dictionary with    |
    |                  | pool statistics data.                                                   |
    +------------------+-------------------------------------------------------------------------+
    | "blksiz"         | size of the memory block in the pool.                                   |
    +------------------+-------------------------------------------------------------------------+
    | "nblks"          | number of blocks in the pool.                                           |
    +------------------+-------------------------------------------------------------------------+
    | "nfree"          | number of free blocks.                                                  |
    +------------------+-------------------------------------------------------------------------+
    | "min"            | lowest number of free blocks the pool reached during run-time.          |
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

Date-time command
*****************

The command allows to obtain string representing current time-date on a device
or set a new time to a device.
The time format used, by both set and get operations, is:

    "yyyy-MM-dd'T'HH:mm:ss.SSSSSSZZZZZ"

Date-time get
=============

The command allows to obtain date-time from a device.

Date-time get request
---------------------

Date-time request header fields:

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``0``  | ``0``        |  ``4``         |
    +--------+--------------+----------------+

The command sends an empty CBOR map as data.

Date-time get response
----------------------

Date-time get response header fields:

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``1``  | ``0``        |  ``4``         |
    +--------+--------------+----------------+

CBOR data of successful response:

.. code-block:: none

    {
        (str)"datetime" : (str)
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
    | "datetime"       | String in format: ``yyyy-MM-dd'T'HH:mm:ss.SSSSSSZZZZZ``.                |
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


Date-time set
=============

The command allows to set date-time to a device.

Date-time set request
---------------------

Date-time set request header fields:

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``2``  | ``0``        |  ``4``         |
    +--------+--------------+----------------+

CBOR data of response:

.. code-block:: none

    {
        (str)"datetime" : (str)
    }

where:

.. table::
    :align: center

    +---------------+----------------------------------------------------------+
    | "datetime"    | String in format: ``yyyy-MM-dd'T'HH:mm:ss.SSSSSSZZZZZ``. |
    +---------------+----------------------------------------------------------+

Date-time set response
----------------------

Date-time set response header fields:

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``3``  | ``0``        |  ``4``         |
    +--------+--------------+----------------+

The command sends an empty CBOR map as data if successful. In case of error the
CBOR data takes the form:

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

System reset
************

Performs reset of system. The device should issue response before resetting so
that the SMP client could receive information that the command has been
accepted. By default, this command is accepted in all conditions, however if
the :kconfig:option:`CONFIG_MCUMGR_GRP_OS_RESET_HOOK` is enabled and an
application registers a callback, the callback will be called when this command
is issued and can be used to perform any necessary tidy operations prior to the
module rebooting, or to reject the reset request outright altogether with an
error response. For details on this functionality, see `ref:`mcumgr_callbacks`.

System reset request
====================

System reset request header fields:

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``2``  | ``0``        |  ``5``         |
    +--------+--------------+----------------+

Normally the command sends an empty CBOR map as data, but if a previous reset
attempt has responded with "rc" equal to :c:enum:`MGMT_ERR_EBUSY` then the
following map may be sent to force a reset:

.. code-block:: none

    {
        (opt)"force"       : (int)
    }

where:

.. table::
    :align: center

    +-----------------------+---------------------------------------------------+
    | "force"               | Force reset if value > 0, optional if 0.          |
    +-----------------------+---------------------------------------------------+


System reset response
=====================

System reset response header fields

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``3``  | ``0``        |  ``5``         |
    +--------+--------------+----------------+

The command sends an empty CBOR map as data if successful. In case of error the
CBOR data takes the form:

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

MCUmgr Parameters
*****************

Used to obtain parameters of mcumgr library.

MCUmgr Parameters Request
=========================

MCUmgr parameters request header fields:

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``0``  | ``0``        |  ``6``         |
    +--------+--------------+----------------+

The command sends an empty CBOR map as data.

MCUmgr Parameters Response
==========================

MCUmgr parameters response header fields

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``1``  | ``0``        |  ``6``         |
    +--------+--------------+----------------+

CBOR data of successful response:

.. code-block:: none

    {
        (str)"buf_size"     : (uint)
        (str)"buf_count"    : (uint)
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
    | "buf_size"       | Single SMP buffer size, this includes SMP header and CBOR payload.      |
    +------------------+-------------------------------------------------------------------------+
    | "buf_count"      | Number of SMP buffers supported.                                        |
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

.. _mcumgr_os_application_info:

OS/Application Info
*******************

Used to obtain information on running image, similar functionality to the linux
uname command, allowing details such as kernel name, kernel version, build
date/time, processor type and application-defined details to be returned. This
functionality can be enabled with :kconfig:option:`CONFIG_MCUMGR_GRP_OS_INFO`.

OS/Application Info Request
===========================

OS/Application info request header fields:

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``0``  | ``0``        |  ``7``         |
    +--------+--------------+----------------+

CBOR data of request:

.. code-block:: none

    {
        (str,opt)"format"      : (str)
    }

where:

.. table::
    :align: center

    +----------+-------------------------------------------------------------------+
    | "format" | Format specifier of returned response, fields are appended in     |
    |          | their natural ascending index order, not the order of             |
    |          | characters that are received by the command. Format               |
    |          | specifiers: |br|                                                  |
    |          | * ``s`` Kernel name |br|                                          |
    |          | * ``n`` Node name |br|                                            |
    |          | * ``r`` Kernel release |br|                                       |
    |          | * ``v`` Kernel version |br|                                       |
    |          | * ``b`` Build date and time (requires                             |
    |          | :kconfig:option:`CONFIG_MCUMGR_GRP_OS_INFO_BUILD_DATE_TIME`) |br| |
    |          | * ``m`` Machine |br|                                              |
    |          | * ``p`` Processor |br|                                            |
    |          | * ``i`` Hardware platform |br|                                    |
    |          | * ``o`` Operating system |br|                                     |
    |          | * ``a`` All fields (shorthand for all above options) |br|         |
    |          | If this option is not provided, the ``s`` Kernel name option      |
    |          | will be used.                                                     |
    +----------+-------------------------------------------------------------------+

OS/Application Info Response
============================

OS/Application info response header fields

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``1``  | ``0``        |  ``7``         |
    +--------+--------------+----------------+

CBOR data of successful response:

.. code-block:: none

    {
        (str)"output"       : (str)
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
    | "output"         | Text response including requested parameters.                           |
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

Bootloader Information
**********************

Allows retrieving information about the on-board bootloader and its parameters.

Bootloader Information Request
==============================

Bootloader information request header:

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``0``  | ``0``        |  ``8``         |
    +--------+--------------+----------------+

CBOR data of request:

.. code-block:: none

    {
        (str,opt)"query"  : (str)
    }

where:

.. table::
    :align: center

    +--------------+-----------------------------------------------+
    | "query"      | Is string representing query for parameters,  |
    |              | with no restrictions how the query looks like |
    |              | as processing of query is left for bootloader |
    |              | backend.                                      |
    |              | If there is no query, then response will      |
    |              | return string identifying the bootloader.     |
    +--------------+-----------------------------------------------+

Bootloader Information Response
===============================

Bootloader information response header:

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``1``  | ``0``        |  ``8``         |
    +--------+--------------+----------------+

In case when no "query" has been provided in request,
CBOR data of response:

.. code-block:: none

    {
        (str)"bootloader"      : (str)
    }

where:

.. table::
    :align: center

    +--------------+-----------------------------------------------+
    | "bootloader" | String representing bootloader name           |
    +--------------+-----------------------------------------------+

In case when "query" is provided:

.. code-block:: none

    {
        (str,opt)<response>   : ()
	...
    }

where:

.. table::
    :align: center

    +------------------+-------------------------------------------------------------------------+
    | <response>       | Response to "query". This is optional and may be left out in case when  |
    |                  | query yields no response, SMP version 2 error code of                   |
    |                  | `OS_MGMT_ERR_QUERY_YIELDS_NO_ANSWER` is expected.                       |
    |                  | Response may have more than one parameter reported back or it may be    |
    |                  | a map, that is dependent on bootloader backednd and query.              |
    +------------------+-------------------------------------------------------------------------+
    | ...              | Parameter characteristic information.                                   |
    +------------------+-------------------------------------------------------------------------+

Parameter may be accompanied by additional, parameter specific, information keywords with
assigned values.

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
    | "err" -> "group" | :c:enum:`mcumgr_group_t` group of the group-based error code. Only      |
    |                  | appears if an error is returned when using SMP version 2.               |
    +------------------+-------------------------------------------------------------------------+
    | "err" -> "rc"    | contains the index of the group-based error code. Only appears if       |
    |                  | non-zero (error condition) when using SMP version 2.                    |
    +------------------+-------------------------------------------------------------------------+
    | "rc"             | :c:enum:`mcumgr_err_t` only appears if non-zero (error condition) when  |
    |                  | using SMP version 1 or for SMP errors when using SMP version 2.         |
    +------------------+-------------------------------------------------------------------------+

Bootloader Information: MCUboot
===============================

In case when MCUboot is application bootloader, empty request will
be responded with:

.. code-block:: none

    {
        (str)"bootloader"      : (str)"MCUboot"
    }

Currently "MCUboot" supports querying for mode of operation:

.. code-block:: none

    {
        (str)"query"           : (str)"mode"
    }

Response to "mode" is:

.. code-block:: none

    {
        (str)"mode"                     : (int)
        (str,opt)"no-downgrade"         : (bool)
    }

where "mode" is one of:

.. table::
    :align: center

    +-----+-----------------------------------------------------+
    | -1  | Unknown mode of MCUboot.                            |
    +-----+-----------------------------------------------------+
    |  0  | MCUboot is in single application mode.              |
    +-----+-----------------------------------------------------+
    |  1  | MCUboot is in swap using scratch partition mode.    |
    +-----+-----------------------------------------------------+
    |  2  | MCUboot is in overwrite (upgrade-only) mode.        |
    +-----+-----------------------------------------------------+
    |  3  | MCUboot is in swap without scratch mode.            |
    +-----+-----------------------------------------------------+
    |  4  | MCUboot is in DirectXIP without revert mode.        |
    +-----+-----------------------------------------------------+
    |  5  | MCUboot is in DirectXIP with revert mode.           |
    +-----+-----------------------------------------------------+
    |  6  | MCUboot is in RAM loader mode.                      |
    +-----+-----------------------------------------------------+

The ``no-downgrade`` field is a flag, which is always sent when true, indicating that MCUboot has
downgrade prevention enabled; downgrade prevention means that if the uploaded image has a lower
version than the currently running application, it will not be used for an update by MCUboot.

MCUmgr may reject images with a lower version in this configuration.
