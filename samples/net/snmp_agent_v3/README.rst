.. zephyr:code-sample:: snmp-agent-v3
   :name: SNMPv3 Agent
   :relevant-api: snmp

   Run an SNMP agent with SNMPv3 USM authentication and encryption, and VACM access control.

Overview
********

This sample demonstrates an SNMPv3 agent for Zephyr RTOS with:

* User-based Security Model (USM, :rfc:`3414`): HMAC-SHA1/SHA256 authentication and AES-128 privacy.
* View-based Access Control Model (VACM, :rfc:`3415`): Role-based access control and subtree views.
* Authoritative engine ID discovery and boots counter synchronization.

Building and Running
********************

Build and run for ``native_sim``:

.. zephyr-app-commands::
   :zephyr-app: samples/net/snmp_agent_v3
   :board: native_sim
   :goals: build run

Testing with Net-SNMP
*********************

Authenticated and encrypted walk:

.. code-block:: console

   $ snmpwalk -v3 -l authPriv -u admin -a SHA -A authpass128 -x AES -X privpass128 192.168.68.101 1.3.6.1.2.1.1
