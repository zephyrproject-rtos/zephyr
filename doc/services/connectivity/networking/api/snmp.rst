.. _snmp_interface:

SNMP (Simple Network Management Protocol)
#########################################

.. contents::
    :local:
    :depth: 2

Overview
********

The SNMP library provides an embedded, heap-free SNMP agent supporting:

* **SNMPv1** (:rfc:`1157`) and **SNMPv2c** (:rfc:`3416`)
* **SNMPv3** (:rfc:`3412`, :rfc:`3414`, :rfc:`3415`) with User-based Security Model (USM) and View-based Access Control Model (VACM)
* Standard MIB-II (:rfc:`1213`, :rfc:`3418`) groups (system, ifTable, and snmp counters)
* Custom enterprise MIB tree registration with scalar and tabular objects
* Two-phase commit (2PC) SET handling with test/commit semantics
* Asynchronous Traps and Inform notifications

The agent is multi-instance capable: each agent instance is encapsulated in a
:c:struct:`snmp_agent` structure with no global mutable state, allowing multiple
independent agents to listen on different ports or network interfaces.

Configuration
*************

Enable SNMP by setting :kconfig:option:`CONFIG_SNMP` in your project configuration.

Key configuration options include:

* :kconfig:option:`CONFIG_SNMP_VERSION_1`: Enable SNMPv1 support.
* :kconfig:option:`CONFIG_SNMP_VERSION_2C`: Enable SNMPv2c support (default).
* :kconfig:option:`CONFIG_SNMP_VERSION_3`: Enable SNMPv3 USM/VACM support.
* :kconfig:option:`CONFIG_SNMP_TRAP_ENABLED`: Enable Trap & Inform notification engine.
* :kconfig:option:`CONFIG_SNMP_MAX_INSTANCES`: Maximum number of running agent instances.

API Reference
*************

.. doxygengroup:: snmp
