.. _mcumgr_smp_group_9:

Shell management
################

Shell management allows passing commands to the shell subsystem over the SMP
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
a shell, but both a request and a response are transported over SMP.

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
    | "argv"                | array consisting of strings representing command  |
    |                       | and its arguments.                                |
    +-----------------------+---------------------------------------------------+
    | <cmd>                 | command to be executed.                           |
    +-----------------------+---------------------------------------------------+
    | <arg>                 | optional arguments to command.                    |
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
    | "o"              | command output.                                                         |
    +------------------+-------------------------------------------------------------------------+
    | "ret"            | return code from shell command execution.                               |
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
    In older versions of Zephyr, "rc" was used for both the mcumgr status code
    and shell command execution return code, this legacy behaviour can be
    restored by enabling :kconfig:option:`CONFIG_MCUMGR_GRP_SHELL_LEGACY_RC_RETURN_CODE`
