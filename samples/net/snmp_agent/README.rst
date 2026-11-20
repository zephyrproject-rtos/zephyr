.. zephyr:code-sample:: snmp-agent
   :name: SNMP agent
   :relevant-api: snmp

   Answer SNMPv1 and SNMPv2c requests and send notifications.

Overview
********

This sample runs the :ref:`SNMP agent <snmp_interface>` with the MIB-2 system and snmp
groups and two objects of its own: ``sampleUpTime``, the seconds since boot, and
``sampleLevel``, a writable integer between 0 and 100. Changing ``sampleLevel`` with a SET
request sends the ``sampleLevelChanged`` notification, carrying the new value of
``sampleLevel``, to the configured receiver. The sample also lists itself in ``sysORTable`` and
sets ``sysObjectID`` to its own subtree.

The sample's objects live under the experimental subtree ``1.3.6.1.3.9``. A product places
its objects under its own enterprise number.

The source code can be found at :zephyr_file:`samples/net/snmp_agent`.

Requirements
************

- :ref:`networking_with_host`, which gives the host the address ``192.0.2.2`` and the sample
  ``192.0.2.1``
- The `Net-SNMP <http://www.net-snmp.org/>`_ command line tools on the host

Building and Running
********************

Build and run the sample for :zephyr:board:`qemu_x86` or :zephyr:board:`native_sim` with the
host networking set up:

.. zephyr-app-commands::
   :zephyr-app: samples/net/snmp_agent
   :board: qemu_x86
   :goals: run
   :compact:

Walk the agent from the host:

.. code-block:: console

   snmpwalk -v2c -c public 192.0.2.1

The output lists the system group, the sample's objects and the snmp group, in that order:

.. code-block:: console

   SNMPv2-MIB::sysDescr.0 = STRING: Zephyr 4.5.99 qemu_x86
   SNMPv2-MIB::sysObjectID.0 = OID: SNMPv2-SMI::experimental.9
   DISMAN-EVENT-MIB::sysUpTimeInstance = Timeticks: (1234) 0:00:12.34
   SNMPv2-MIB::sysContact.0 = STRING: admin@example.com
   SNMPv2-MIB::sysName.0 = STRING: zephyr
   ...
   SNMPv2-SMI::experimental.9.1.1.0 = Counter32: 12
   SNMPv2-SMI::experimental.9.1.2.0 = INTEGER: 50
   SNMPv2-MIB::snmpInPkts.0 = Counter32: 3
   ...

Change a value with the write community, and read it back:

.. code-block:: console

   snmpset -v2c -c private 192.0.2.1 1.3.6.1.3.9.1.2.0 i 75
   snmpget -v2c -c private 192.0.2.1 sysContact.0
   snmpset -v2c -c private 192.0.2.1 sysContact.0 s "ops@example.com"

A value outside the range is refused with ``wrongValue``, and a SET with the read community
with ``authorizationError``. SNMPv1 requests work the same way with ``-v1``.

Notifications
=============

To receive notifications, start a receiver on the host and point the sample at it:

.. code-block:: console

   sudo snmptrapd -f -Lo --authCommunity="log public"

.. zephyr-app-commands::
   :zephyr-app: samples/net/snmp_agent
   :board: qemu_x86
   :gen-args: -DCONFIG_NET_SAMPLE_SNMP_TRAP_RECEIVER=\"192.0.2.2\"
   :goals: run
   :compact:

The receiver logs a ``coldStart`` when the agent starts, an ``authenticationFailure`` for a
request with a wrong community, and ``experimental.9.0.1`` whenever ``sampleLevel`` is set,
with the new value as its binding:

.. code-block:: console

   DISMAN-EVENT-MIB::sysUpTimeInstance = Timeticks: (2345) 0:00:23.45
   SNMPv2-MIB::snmpTrapOID.0 = OID: SNMPv2-SMI::experimental.9.0.1
   SNMPv2-SMI::experimental.9.1.2.0 = INTEGER: 75

Set :kconfig:option:`CONFIG_NET_SAMPLE_SNMP_TRAP_V1` to send SNMPv1 traps instead.

Configuration
*************

The community strings and the notification receiver are options of the sample, see
:file:`Kconfig` in the sample directory. The system group's strings come from
:kconfig:option:`CONFIG_SNMP_SYS_NAME`, :kconfig:option:`CONFIG_SNMP_SYS_CONTACT` and
:kconfig:option:`CONFIG_SNMP_SYS_LOCATION`; a manager can change them, and an application that
wants such changes to survive a reset registers a :c:struct:`snmp_storage`.
