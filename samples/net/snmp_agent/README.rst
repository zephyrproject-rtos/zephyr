.. zephyr:code-sample:: snmp-agent
   :name: SNMP Agent
   :relevant-api: snmp

   Run an SNMPv1/v2c agent responding to GET, GETNEXT, GETBULK, SET, and sending traps.

Overview
********

This sample provides a modular reference implementation of an SNMP agent for
Zephyr RTOS supporting **SNMPv1** (:rfc:`1157`) and **SNMPv2c** (:rfc:`3416`).

It demonstrates:

* Standard MIB-II (:rfc:`1213` / :rfc:`3418`) system group and SNMP statistics.
* Custom enterprise MIB scalars (integers, strings, timestamps) and tables with dynamic indexing.
* Two-phase commit SET handler for actuators (e.g. LED control).
* Autonomous notification dispatch (SNMP Traps / Informs) on hardware events (e.g. push button).

Requirements
************

* Network interface (Ethernet or native_sim networking)
* UDP socket support

Building and Running
********************

Build and run for ``native_sim``:

.. zephyr-app-commands::
   :zephyr-app: samples/net/snmp_agent
   :board: native_sim
   :goals: build run

Testing with Net-SNMP
*********************

Walk MIB-II system group:

.. code-block:: console

   $ snmpwalk -v2c -c public 192.168.68.101 1.3.6.1.2.1.1

Get enterprise sensor data:

.. code-block:: console

   $ snmpget -v2c -c public 192.168.68.101 1.3.6.1.4.1.54321.1.2.1.0

Toggle LED via SNMP SET:

.. code-block:: console

   $ snmpset -v2c -c private 192.168.68.101 1.3.6.1.4.1.54321.1.1.1.0 i 1
