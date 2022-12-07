.. _mcumgr_smp_group_9:

Shell management
################

Shell management allows to pass commands to shell subsystem with use of SMP
protocol.

Shell management group defines following commands:

.. table::
    :align: center

    +-------------------+-----------------------------------------------+
    | ``Command ID``    | Command description                           |
    +===================+===============================================+
    | ``0``             | Shell command line execute                    |
    +-------------------+-----------------------------------------------+

Shell command line execute
**************************

The command allows to execute command line in a similar way to typing it into
a shell, but both a request and a response are transported with use of SMP.

Shell command line execute request
==================================

Execute command request header:

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``2``  | ``9``        |  ``0``         |
    +--------+--------------+----------------+

CBOR data of request:

.. code-block:: none

    {
        (str)"argv"     : [
            (str)<cmd>
            (str,opt)<arg>
            ...
        ]
    }

where:

.. table::
    :align: center

    +-----------------------+---------------------------------------------------+
    | "argv"                | is array consisting of strings representing       |
    |                       | command and its arguments                         |
    +-----------------------+---------------------------------------------------+
    | <cmd>                 | command to be executed                            |
    +-----------------------+---------------------------------------------------+
    | <arg>                 | optional arguments to command                     |
    +-----------------------+---------------------------------------------------+

Shell command line execute response
===================================

Command line execute response header fields:

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``3``  | ``9``        |  ``0``         |
    +--------+--------------+----------------+

CBOR data of successful response:

.. code-block:: none

    {
        (str)"o"            : (str)
        (str)"ret"          : (int)
    }

In case of error the CBOR data takes form:

.. code-block:: none

    {
        (str)"rc"           : (int)
    }

where:

.. table::
    :align: center

    +-----------------------+---------------------------------------------------+
    | "rc"                  | :ref:`mcumgr_smp_protocol_status_codes` (only     |
    |                       | present if an error occurred)                     |
    +-----------------------+---------------------------------------------------+
    | "o"                   | command output                                    |
    +-----------------------+---------------------------------------------------+
    | "ret"                 | return code from shell command execution          |
    +-----------------------+---------------------------------------------------+

In case when "rc" is not 0, success, the other fields will not appear.

.. note::
    In older versions of Zephyr, "rc" was used for both the mcumgr status code
    and shell command execution return code, this legacy behaviour can be
    restored by enabling :kconfig:option:`CONFIG_MCUMGR_CMD_SHELL_MGMT_LEGACY_RC_RETURN_CODE`
