.. _mcumgr_smp_group_10:

Enumeration Management Group
#############################

Enumeration management group defines the following commands:

.. table::
    :align: center

    +----------------+-----------------------------+
    | ``Command ID`` | Command description         |
    +================+=============================+
    | ``0``          | Count of supported groups   |
    +----------------+-----------------------------+
    | ``1``          | List supported groups       |
    +----------------+-----------------------------+
    | ``2``          | Fetch single group ID       |
    +----------------+-----------------------------+
    | ``3``          | Details on supported groups |
    +----------------+-----------------------------+

Count of supported groups command
*********************************

Count of supported groups returns the total number of MCUmgr command groups that a device supports.

Count of supported groups request
=================================

Read setting request header fields:

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``0``  | ``10``       | ``0``          |
    +--------+--------------+----------------+

The command sends an empty CBOR map as data.

Count of supported groups response
==================================

Count of supported groups response header fields:

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``1``  | ``10``       | ``0``          |
    +--------+--------------+----------------+

CBOR data of successful response:

.. code-block:: none

    {
        (str)"count"        : (uint)
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
    | "count"          | contains the total number of supported MCUmgr groups on the device.     |
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

List supported groups command
*****************************

List supported groups command allows listing the group IDs of supported MCUmgr groups on the
device.

List supported groups request
=============================

List supported groups request header fields:

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``0``  | ``10``       | ``1``          |
    +--------+--------------+----------------+

The command sends an empty CBOR map as data.

List supported groups response
==============================

List supported groups response header fields:

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``1``  | ``10``       | ``1``          |
    +--------+--------------+----------------+

CBOR data of successful response:

.. code-block:: none

    {
        (str)"groups" : [
            (uint)
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
    | "groups"         | contains a list of the supported MCUmgr group IDs on the device.        |
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

Fetch single group ID command
*****************************

Fetch single group ID command allows listing the group IDs of supported MCUmgr groups on the
device, one by one.

Fetch single group ID request
=============================

Fetch single group ID request header fields:

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``0``  | ``10``       | ``2``          |
    +--------+--------------+----------------+

CBOR data of request:

.. code-block:: none

    {
        (str,opt)"index" : (uint)
    }

where:

.. table::
    :align: center

    +----------+-----------------------------------------------------------------+
    | "index"  | contains the (0-based) index of the group to return information |
    |          | on, can be omitted to return the first group's details.         |
    +----------+-----------------------------------------------------------------+

Fetch single group ID response
==============================

Fetch single group ID response header fields:

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``1``  | ``10``       | ``2``          |
    +--------+--------------+----------------+

CBOR data of successful response:

.. code-block:: none

    {
        (str)"group"    : (uint)
        (str,opt)"end"  : (bool)
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
    | "group"          | contains the list of the supported MCUmgr group IDs on the device.      |
    +------------------+-------------------------------------------------------------------------+
    | "end"            | will be set to true if the listed group is the final supported group on |
    |                  | the device, otherwise will be omitted.                                  |
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

Details on supported groups command
***********************************

Details on supported groups command allows fetching details on each supported MCUmgr group, such
as the name and number of handlers. A device can specify an allow list of groups to return details
on or details on all groups can be returned.

This command is optional, it can be enabled using :kconfig:option:`CONFIG_MCUMGR_GRP_ENUM_DETAILS`.
The optional name and number of handlers can be enabled/disabled with
:kconfig:option:`CONFIG_MCUMGR_GRP_ENUM_DETAILS_NAME` and
:kconfig:option:`CONFIG_MCUMGR_GRP_ENUM_DETAILS_HANDLERS`.

Details on supported groups request
===================================

Details on supported groups request header fields:

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``0``  | ``10``       | ``3``          |
    +--------+--------------+----------------+

.. tabs::

   .. group-tab:: Details on all groups

      The command sends an empty CBOR map as data.

   .. group-tab:: Details on specified groups

      CBOR data of request:

      .. code-block:: none

          {
              (str)"groups" : [
                  (uint)
                  ...
              ]
          }

      where:

      .. table::
          :align: center

          +----------+--------------------------------------------------------------+
          | "groups" | contains a list of the MCUmgr group IDs to fetch details on. |
          +----------+--------------------------------------------------------------+

Details on supported groups response
====================================

Details on supported groups response header fields:

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``1``  | ``10``       | ``3``          |
    +--------+--------------+----------------+

CBOR data of successful response:

.. code-block:: none

    {
        (str)"groups" : [
            {
                (str)"group"          : (uint)
                (str,opt)"name"       : (str)
                (str,opt)"handlers"   : (uint)
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
    | "group"          | the group ID of the MCUmgr command group.                               |
    +------------------+-------------------------------------------------------------------------+
    | "name"           | the name of the MCUmgr command group.                                   |
    +------------------+-------------------------------------------------------------------------+
    | "handlers"       | the number of handlers that the MCUmgr command group supports.          |
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

Details on supported groups callback
************************************

There is a details on supported groups MCUmgr callback available (see :ref:`mcumgr_callbacks` for
details on callbacks) which allows for applications/modules to add additional fields to this
response. This callback can be enabled with :kconfig:option:`CONFIG_MCUMGR_GRP_ENUM_DETAILS_HOOK`,
registered with the event :c:enumerator:`MGMT_EVT_OP_ENUM_MGMT_DETAILS`, whereby the supplied
callback data is :c:struct:`enum_mgmt_detail_output`. Note that
:kconfig:option:`CONFIG_MCUMGR_GRP_ENUM_DETAILS_STATES` will need incrementing by the number of
additional extra fields that are added.
