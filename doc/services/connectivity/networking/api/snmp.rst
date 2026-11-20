.. _snmp_interface:

SNMP Agent
##########

.. contents::
    :local:
    :depth: 2

Overview
********

The SNMP agent answers SNMPv1 (:rfc:`1157`) and SNMPv2c (:rfc:`3416`) requests over UDP. It
supports the GET, GETNEXT, GETBULK and SET operations and the MIB-2 system and snmp groups of
:rfc:`3418`. It sends the coldStart and authenticationFailure notifications and notifications
of the application as traps. Informs and SNMPv3 are not supported.

The agent is enabled with :kconfig:option:`CONFIG_SNMP` and started by the application with
:c:func:`snmp_agent_start`. It listens on :kconfig:option:`CONFIG_SNMP_PORT` on every enabled
address family and answers from the socket service thread, so it needs no thread of its own.

Community strings
*****************

Two community strings control access. The read community grants GET, GETNEXT and GETBULK, the
write community additionally grants SET. Their initial values come from
:kconfig:option:`CONFIG_SNMP_COMMUNITY_READ` and :kconfig:option:`CONFIG_SNMP_COMMUNITY_WRITE`;
the application replaces them at runtime with :c:func:`snmp_agent_set_community`, for example
after loading them from the settings subsystem. An empty community string matches nothing, so
clearing the write community disables SET. SET requests are only processed when
:kconfig:option:`CONFIG_SNMP_SET_ENABLE` is set.

Requests carrying an unknown community are dropped without a response. Note that SNMPv1 and
SNMPv2c transmit community strings in clear text, so they are no protection on an untrusted
network.

Managed objects
***************

Managed objects are registered at build time, from any part of the application or the tree, and
collected in an iterable section. A scalar is one object with the single instance ``.0``:

.. code-block:: c

   static int uptime_get(const struct snmp_object *obj, void *row, struct snmp_value *val)
   {
           val->type = SNMP_TYPE_TIMETICKS;
           val->u32 = (uint32_t)(k_uptime_get() / 10);

           return 0;
   }

   SNMP_SCALAR_DEFINE(my_uptime, uptime_get, NULL, NULL, 1, 3, 6, 1, 4, 1, 99999, 1);

A writable object also provides a setter. The agent checks the community and compares the type
of the new value with the one the getter reports before invoking it. A SET request is processed
in two phases, as :rfc:`3416` requires: the setter of every binding is first called with
``SNMP_SET_CHECK`` to validate length and range, and only when all checks pass is each called
again with ``SNMP_SET_COMMIT`` to apply the value. The error codes a setter reports are
documented in :c:struct:`snmp_object`. Nothing is rolled back, so a commit that fails after
another binding was committed is reported as undoFailed.

Tables
******

A table is registered column by column. All columns of one table share a
:c:struct:`snmp_table`, the row provider, which derives the rows on demand from the data that
owns them instead of storing them:

* ``foreach_row`` invokes a callback for every existing row.
* ``row_index`` encodes the index of a row as OID sub-identifiers, for example the interface
  index for ``ifTable``.
* ``row_lookup`` resolves an exact index to a row for GET and SET.

The agent walks the rows on every request and selects the lexicographically next one itself,
so a row that appears or disappears between two requests cannot corrupt a walk. The
``sysORTable`` provider in :zephyr_file:`subsys/net/lib/snmp/mib2_system.c` shows the pattern
on top of an iterable section; a table over the network interfaces would enumerate them with
:c:func:`net_if_foreach` and resolve an index with :c:func:`net_if_get_by_index` the same way.

The registry is not sorted. Every request scans all registered objects, which is cheap for the
tens of objects a device typically exposes but grows linearly with their number.

MIB-2 system group
******************

:kconfig:option:`CONFIG_SNMP_MIB2_SYSTEM` implements the system group of :rfc:`3418`: the seven
scalars of :rfc:`1213`, plus ``sysORLastChange`` and ``sysORTable``. The table lists the
compliance and capabilities statements declared with :c:macro:`SNMP_CAPABILITY_DEFINE`, from any
part of the build. The library declares ``snmpBasicComplianceRev2`` when the snmp group and the
notifications are enabled as well, since those complete that statement; an application adds the
AGENT-CAPABILITIES statement of its own MIB the same way. ``sysName``, ``sysContact`` and ``sysLocation`` start with the values of the corresponding
Kconfig options and can be changed by managers with SET and by the application with
:c:func:`snmp_mib2_system_set_name` and its siblings; see Persistence for keeping them across
resets.
``sysObjectID`` reports ``zeroDotZero`` until the application sets its own identifier with
:c:func:`snmp_mib2_system_set_object_id`.

Notifications
*************

With :kconfig:option:`CONFIG_SNMP_NOTIFICATIONS` the application registers receivers with
:c:func:`snmp_agent_add_notification_target`, each described by a
:c:struct:`snmp_notification_target` with its own protocol version and community.
The agent sends coldStart when it starts, so receivers that should see it are added before
:c:func:`snmp_agent_start`, and authenticationFailure when it drops a request with an unknown
community, as long as ``snmpEnableAuthenTraps`` permits. The application sends its own
notifications with :c:func:`snmp_agent_notify`, passing the instances of the notification's
OBJECTS clause as :c:struct:`snmp_varbind` bindings. Version 2c receivers get an SNMPv2-Trap-PDU
with ``sysUpTime.0``, ``snmpTrapOID.0`` and the bindings; version 1 receivers get a Trap-PDU
translated as :rfc:`3584` section 3.2 describes, carrying the same bindings, except that a
notification with a Counter64 value cannot be expressed in version 1 and is not sent to them.
There are no informs.

MIB-2 snmp group
****************

:kconfig:option:`CONFIG_SNMP_MIB2_SNMP` implements the snmp group of :rfc:`3418`. The agent
counts every received message and the reasons it discards or refuses one, so the counters
are maintained whether or not the group is enabled. ``snmpEnableAuthenTraps`` is writable by
managers and through :c:func:`snmp_mib2_snmp_set_authen_traps`. The group also
provides ``snmpSetSerialNo``, the advisory lock managers use to coordinate SET requests.

Persistence
***********

The agent keeps the objects a manager can change, ``sysName``, ``sysContact``, ``sysLocation``
and ``snmpEnableAuthenTraps``, in RAM. An application that wants them to survive a reset
registers a :c:struct:`snmp_storage` with :c:func:`snmp_agent_set_storage`: two callbacks that
read and durably write one item each, backed by whatever medium the product uses. The agent
loads every item in :c:func:`snmp_agent_start`, so stored values take precedence over the
Kconfig defaults and over values set before the start, and it writes an item before a new value
takes effect, so a SET is acknowledged only once the value is durable. A failed write is
reported to the manager as commitFailed and to the application as an error. Community strings
and notification receivers are not persisted by the agent; they stay with the application.

Sample
******

The :zephyr:code-sample:`snmp-agent` sample registers objects of its own, sends a notification
when one of them changes and shows the net-snmp commands to talk to the agent.

API Reference
*************

.. doxygengroup:: snmp
