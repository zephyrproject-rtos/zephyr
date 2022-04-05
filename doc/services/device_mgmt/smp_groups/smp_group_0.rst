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
    | ``2``             | Statistics                                    |
    +-------------------+-----------------------------------------------+
    | ``3``             | Memory pool statistics                        |
    +-------------------+-----------------------------------------------+
    | ``4``             | Date-time string; unimplemented by Zephyr     |
    +-------------------+-----------------------------------------------+
    | ``5``             | System reset                                  |
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
    | "d"                   | string to be replied by echo service              |
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

In case of error the CBOR data takes form:

.. code-block:: none

    {
        (str)"rc"       : (int)
    }

where:

.. table::
    :align: center

    +-----------------------+---------------------------------------------------+
    | "r"                   | Replying echo string                              |
    +-----------------------+---------------------------------------------------+
    | "rc"                  | :ref:`mcumgr_smp_protocol_status_codes`           |
    +-----------------------+---------------------------------------------------+

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

The command sends empty CBOR map as data.


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

CBOR data of response:

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
        (str)"rc" : (int)
    }


where:

.. table::
    :align: center

    +-----------------------+---------------------------------------------------+
    | <task_name>           | string identifying task                           |
    +-----------------------+---------------------------------------------------+
    | "prio"                | task priority                                     |
    +-----------------------+---------------------------------------------------+
    | "tid"                 | numeric task ID                                   |
    +-----------------------+---------------------------------------------------+
    | "state"               | numeric task state                                |
    +-----------------------+---------------------------------------------------+
    | "stkuse"              | task's/thread's stack usage                       |
    +-----------------------+---------------------------------------------------+
    | "stksiz"              | task's/thread's stack size                        |
    +-----------------------+---------------------------------------------------+
    | "cswcnt"              | task's/thread's context switches                  |
    +-----------------------+---------------------------------------------------+
    | "runtime"             | task's/thread's runtime in "ticks"                |
    +-----------------------+---------------------------------------------------+
    | "last_checkin"        | set to 0 by Zephyr                                |
    +-----------------------+---------------------------------------------------+
    | "next_checkin"        | set to 0 by Zephyr                                |
    +-----------------------+---------------------------------------------------+
    | "rc"                  | :ref:`mcumgr_smp_protocol_status_codes`           |
    +-----------------------+---------------------------------------------------+

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

The command sends empty CBOR map as data.

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

CBOR data of response:

.. code-block:: none

    {
        (str)<pool_name> {
            (str)"blksiz"   : (int)
            (str)"nblks"    : (int)
            (str)"nfree"    : (int)
            (str)"min'      : (int)
        }
        ...
        (str)"rc" : (int)
    }

where:

.. table::
    :align: center

    +-----------------------+---------------------------------------------------+
    | <pool_name>           | string representing the pool name, used as a key  |
    |                       | for dictionary with pool statistics data          |
    +-----------------------+---------------------------------------------------+
    | "blksiz"              | size of the memory block in the pool              |
    +-----------------------+---------------------------------------------------+
    | "nblks"               | number of blocks in the pool                      |
    +-----------------------+---------------------------------------------------+
    | "nrfree"              | number of free blocks                             |
    +-----------------------+---------------------------------------------------+
    | "min"                 | lowest number of free blocks the pool reached     |
    |                       | during run-time                                   |
    +-----------------------+---------------------------------------------------+
    | "rc"                  | :ref:`mcumgr_smp_protocol_status_codes`           |
    +-----------------------+---------------------------------------------------+

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

The command sends empty CBOR map as data.

Data-time get response
----------------------

Date-time get response header fields:

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``1``  | ``0``        |  ``4``         |
    +--------+--------------+----------------+

CBOR data of response:

.. code-block:: none

    {
        (str)"datetime" : (str)
        (opt,str)"rc"   : (int)
    }

where:

.. table::
    :align: center

    +-----------------------+---------------------------------------------------+
    | "datetime"            | String in format                                  |
    |                       | yyyy-MM-dd'T'HH:mm:ss.SSSSSSZZZZZ                 |
    +-----------------------+---------------------------------------------------+
    | "rc"                  | :ref:`mcumgr_smp_protocol_status_codes`;          |
    |                       | may not appear if 0                               |
    +-----------------------+---------------------------------------------------+


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

    +-----------------------+---------------------------------------------------+
    | "datetime"            | String in format                                  |
    |                       | yyyy-MM-dd'T'HH:mm:ss.SSSSSSZZZZZ                 |
    +-----------------------+---------------------------------------------------+

Data-time set response
----------------------

Date-time set response header fields:

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``1``  | ``0``        |  ``4``         |
    +--------+--------------+----------------+

CBOR data of response:

.. code-block:: none

    {
        (str)"rc"       : (int)
    }

where:

.. table::
    :align: center

    +-----------------------+---------------------------------------------------+
    | "rc"                  | :ref:`mcumgr_smp_protocol_status_codes`           |
    +-----------------------+---------------------------------------------------+

System reset
************

Performs reset of system. The device should issue response before resetting so that
the SMP client could receive information that the command has been accepted.

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

The command sends empty CBOR map as data.

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

CBOR data of response:

.. code-block:: none

    {
        (opt,str)"rc"       : (int)
    }

where:

.. table::
    :align: center

    +-----------------------+---------------------------------------------------+
    | "rc"                  | :ref:`mcumgr_smp_protocol_status_codes`;          |
    |                       | may not appear if 0                               |
    +-----------------------+---------------------------------------------------+
