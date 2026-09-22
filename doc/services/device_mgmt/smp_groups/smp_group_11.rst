.. _mcumgr_smp_group_11:

Transport Management Group
##########################

Transport management group defines the following commands:

.. table::
    :align: center

    +----------------+------------------------------------+
    | ``Command ID`` | Command description                |
    +================+====================================+
    | ``0``          | Connect (bridge) transport         |
    +----------------+------------------------------------+
    | ``1``          | Disconnect bridged transport       |
    +----------------+------------------------------------+
    | ``2``          | Fetch bridge/transport status      |
    +----------------+------------------------------------+
    | ``6``          | List transports                    |
    +----------------+------------------------------------+
    | ``7``          | Details on transport modes         |
    +----------------+------------------------------------+
    | ``8``          | Details on transport configuration |
    +----------------+------------------------------------+

.. note::
    Transport management is experimental and subject to change without notice.

Connect (bridge) transport command
**********************************

Bridge the transport which received the MCUmgr packet to another MCUmgr transport.

Connect (bridge) transport request
==================================

Connect (bridge) transport request header fields:

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``2``  | ``11``       | ``0``          |
    +--------+--------------+----------------+

CBOR data of request:

.. code-block:: none

    {
        (str)"transport" : (uint)
        (str,opt)"mode"  : (uint)
        ...
    }

where:

.. table::
    :align: center

    +-------------+--------------------------------------------------------------+
    | "transport" | :c:enum:`smp_transport_type` contains the tranport type for  |
    |             | which to bridge (connect) from the transport to.             |
    +-------------+--------------------------------------------------------------+
    | "mode"      | contains the configuration mode of the transport to use, may |
    |             | be omitted to use the default value of 0.                    |
    +-------------+--------------------------------------------------------------+
    | ...         | there might be additional fields that the transport requires |
    |             | in order to make a connection, these are not described here  |
    |             | as are transport-specific.                                   |
    +-------------+--------------------------------------------------------------+

Connect (bridge) transport response
===================================

Connect (bridge) transport response header fields:

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``3``  | ``11``       | ``0``          |
    +--------+--------------+----------------+

The command sends an empty CBOR map as data if successful.
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

Disconnect bridged transport command
************************************

Disconnect the current transport's bridge, or disconnect all transport bridges.

Disconnect bridged transport request
====================================

Disconnect bridged transport request header fields:

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``2``  | ``11``       | ``1``          |
    +--------+--------------+----------------+

CBOR data of request:

.. tabs::

   .. group-tab:: Disconnect bridge of current transport

      The command sends an empty CBOR map as data.

   .. group-tab:: Disconnect an active bridge

      .. code-block:: none

          {
              (str)"transport" : (uint)
          }

   .. group-tab:: Disconnect all bridges

      .. code-block:: none

          {
              (str)"all" :       (bool)
          }

where:

.. table::
    :align: center

    +-------------+----------------------------------------------------------------+
    | "transport" | :c:enum:`smp_transport_type` contains the tranport type for    |
    |             | which to disconnect the bridge from, this must not be provided |
    |             | if ``all`` is provided.                                        |
    +-------------+----------------------------------------------------------------+
    | "all"       | set to true to disconnect all active bridged transports, this  |
    |             | must not be provided if ``transport`` is provided.             |
    +-------------+----------------------------------------------------------------+

Disconnect bridged transport response
=====================================

Disconnect bridged transport response header fields:

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``3``  | ``11``       | ``1``          |
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

Fetch bridge/transport status command
*************************************

Return information on active bridges and what the device supports.

Fetch bridge/transport status request
=====================================

Fetch bridge/transport status request header fields:

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``0``  | ``11``       | ``2``          |
    +--------+--------------+----------------+

The command sends an empty CBOR map as data.

Fetch bridge/transport status response
======================================

Fetch bridge/transport status response header fields:

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``1``  | ``11``       | ``2``          |
    +--------+--------------+----------------+

CBOR data of successful response:

.. code-block:: none

    {
        (str)"supported"      : (uint)
        (str)"active"         : (uint)
        (str,opt)"bridged"    : (bool)
        (str,opt)"transport"  : (uint)
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

   .. group-tab:: SMP version 1

      .. code-block:: none

          {
              (str)"rc"       : (int)
          }

where:

.. table::
    :align: center

    +------------------+----------------------------------------------------------------------------+
    | "supported"      | contains how many bridges can be active at a given time.                   |
    +------------------+----------------------------------------------------------------------------+
    | "active"         | contains how many bridges are currently active.                            |
    +------------------+----------------------------------------------------------------------------+
    | "bridged"        | will be present and true if the current transport is bridged, otherwise    |
    |                  | will be omitted.                                                           |
    +------------------+----------------------------------------------------------------------------+
    | "transport"      | the transport ID of the MCUmgr transport that the transport is bridged to. |
    |                  | Only appears if the transport is bridged.                                  |
    +------------------+----------------------------------------------------------------------------+
    | "err" -> "group" | :c:enum:`mcumgr_group_t` group of the group-based error code. Only appears |
    |                  | if an error is returned when using SMP version 2.                          |
    +------------------+----------------------------------------------------------------------------+
    | "err" -> "rc"    | contains the index of the group-based error code. Only appears if non-zero |
    |                  | (error condition) when using SMP version 2.                                |
    +------------------+----------------------------------------------------------------------------+
    | "rc"             | :c:enum:`mcumgr_err_t` only appears if non-zero (error condition) when     |
    |                  | using SMP version 1 or for SMP errors when using SMP version 2.            |
    +------------------+----------------------------------------------------------------------------+

List transports command
***********************

Return information on transports that the device supports.

List transports request
=======================

List transports request header fields:

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``0``  | ``11``       | ``6``          |
    +--------+--------------+----------------+

The command sends an empty CBOR map as data.

List transports response
========================

List transports response header fields:

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``1``  | ``11``       | ``6``          |
    +--------+--------------+----------------+

CBOR data of successful response:

.. code-block:: none

    {
        (str)"transports" : [
            {
                (str)"id"         : (uint)
                (str,opt)"name"   : (str)
            }
            ...
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

   .. group-tab:: SMP version 1

      .. code-block:: none

          {
              (str)"rc"       : (int)
          }

where:

.. table::
    :align: center

    +------------------+-------------------------------------------------------------------------+
    | "id"             | the transport ID of the MCUmgr transport that supports bridging.        |
    +------------------+-------------------------------------------------------------------------+
    | "name"           | optional name of the MCUmgr transport (if available).                   |
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

Details on transport modes command
**********************************

Return information on modes of a transport that the device supports.

Details on transport mode request
=================================

Details on transport modes request header fields:

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``0``  | ``11``       | ``7``          |
    +--------+--------------+----------------+

CBOR data of request:

.. code-block:: none

    {
        (str)"transport" : (uint)
    }

where:

.. table::
    :align: center

    +-------------+-------------------------------------------------------------+
    | "transport" | :c:enum:`smp_transport_type` contains the tranport type for |
    |             | which to get details on.                                    |
    +-------------+-------------------------------------------------------------+


Details on transport modes response
===================================

Details on transport modes response header fields:

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``1``  | ``11``       | ``7``          |
    +--------+--------------+----------------+

CBOR data of successful response:

.. code-block:: none

    {
        (str)"modes" : [
            {
                (str)"id"            : (uint)
                (str)"description"   : (str)
                (str,opt)"incoming"  : (bool)
                (str,opt)"outgoing"  : (bool)
            }
            ...
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

   .. group-tab:: SMP version 1

      .. code-block:: none

          {
              (str)"rc"       : (int)
          }

where:

.. table::
    :align: center

    +------------------+-----------------------------------------------------------------------------+
    | "id"             | the ID of the transport mode.                                               |
    +------------------+-----------------------------------------------------------------------------+
    | "description"    | description of the transport mode.                                          |
    +------------------+-----------------------------------------------------------------------------+
    | "incoming"       | will be set to true if transport mode supports incoming bridge connections. |
    +------------------+-----------------------------------------------------------------------------+
    | "outgoing"       | will be set to true if transport mode supports outgoing bridge connections. |
    +------------------+-----------------------------------------------------------------------------+
    | "err" -> "group" | :c:enum:`mcumgr_group_t` group of the group-based error code. Only appears  |
    |                  | if an error is returned when using SMP version 2.                           |
    +------------------+-----------------------------------------------------------------------------+
    | "err" -> "rc"    | contains the index of the group-based error code. Only appears if non-zero  |
    |                  | (error condition) when using SMP version 2.                                 |
    +------------------+-----------------------------------------------------------------------------+
    | "rc"             | :c:enum:`mcumgr_err_t` only appears if non-zero (error condition) when      |
    |                  | using SMP version 1 or for SMP errors when using SMP version 2.             |
    +------------------+-----------------------------------------------------------------------------+


Details on transport configuration command
******************************************

Return information on transport configuration that is supported.

Details on transport configuration request
==========================================

Details on transport configuration request header fields:

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``0``  | ``11``       | ``8``          |
    +--------+--------------+----------------+

CBOR data of request:

.. code-block:: none

    {
        (str)"transport" : (uint)
        (str)"mode"      : (uint)
    }

where:

.. table::
    :align: center

    +-------------+-------------------------------------------------------------+
    | "transport" | :c:enum:`smp_transport_type` contains the tranport type for |
    |             | which to get configuration information for.                 |
    +-------------+-------------------------------------------------------------+
    | "mode"      | contains the configuration mode of the tranport type which  |
    |             | to get configuration information for.                       |
    +-------------+-------------------------------------------------------------+

Details on transport configuration response
===========================================

Details on transport configuration response header fields:

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``1``  | ``11``       | ``8``          |
    +--------+--------------+----------------+

CBOR data of successful response:

.. code-block:: none

    {
        (str)"configs" : [
            {
                (str)"name"          : (str)
                (str)"type"          : (uint)
                (str,opt)"required"  : (bool)
            }
            ...
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

   .. group-tab:: SMP version 1

      .. code-block:: none

          {
              (str)"rc"       : (int)
          }

where:

.. table::
    :align: center

    +------------------+------------------------------------------------------------------------+
    | "name"           | name of the configuration item.                                        |
    +------------------+------------------------------------------------------------------------+
    | "type"           | the type of the configuration item, using the following mapping:       |
    |                  |  - 0: uint                                                             |
    |                  |  - 1: int                                                              |
    |                  |  - 2: bool                                                             |
    |                  |  - 3: string                                                           |
    |                  |  - 4: byte string                                                      |
    +------------------+------------------------------------------------------------------------+
    | "required"       | will be present and set to true if the parameter is required.          |
    +------------------+------------------------------------------------------------------------+
    | "err" -> "group" | :c:enum:`mcumgr_group_t` group of the group-based error code. Only     |
    |                  | appears if an error is returned when using SMP version 2.              |
    +------------------+------------------------------------------------------------------------+
    | "err" -> "rc"    | contains the index of the group-based error code. Only appears if      |
    |                  | non-zero (error condition) when using SMP version 2.                   |
    +------------------+------------------------------------------------------------------------+
    | "rc"             | :c:enum:`mcumgr_err_t` only appears if non-zero (error condition) when |
    |                  | using SMP version 1 or for SMP errors when using SMP version 2.        |
    +------------------+------------------------------------------------------------------------+
