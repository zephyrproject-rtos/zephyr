.. _arm_scmi_shell:

ARM SCMI shell sample
#####################

Overview
********

This sample demonstrates the usage of ARM SCMI. It provides access to the
ARM SCMI shell interface for demo and testing purposes of different
ARM SCMI protocols.

* Base protocol
* Reset domain management protocol

Caveats
*******

Zephyr ARM SCMI relies on the firmware running in EL3 layer to be compatible
with Arm System Control and Management Interface Platform Design Document
and ARM SCMI driver used by the sample.

Building and Running
********************

For building the application:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/firmware/arm_scmi
   :board: rpi_5
   :goals: build

Running on RPI5
^^^^^^^^^^^^^^^

* The SCMI compatible Trusted Firmware-A (TF-A) binary should be placed
   at RPI5 rootfs as `armstub8-2712.bin`.
* The Zephyr application binary should be placed at RPI5 rootfs
  as `zephyr.bin`.
* Modify config.txt parameter `kernel=zephyr.bin`

Sample Output
*************

Base protocol
^^^^^^^^^^^^^

.. code-block:: console

	*** Booting Zephyr OS build v3.6.0-126-g9a3343971f49 ***
	ARM SCMI shell sample

	uart:~$ arm_scmi base
	base - SCMI Base proto commands.
	Subcommands:
	  revision           : SCMI Base get revision info
	                      Usage: arm_scmi base revision

	  discover_agent     : SCMI Base discover an agent
	                      Usage: arm_scmi base discover_agent [agent_id]

	  device_permission  : SCMI Base set an agent permissions to access device
	                      Usage: arm_scmi base discover_agent <agent_id> <device_id>
	                      <allow := 0|1>

	  reset_agent_cfg    : SCMI Base reset an agent configuration
	                      Usage: arm_scmi base reset_agent_cfg <agent_id>
	                      <reset_perm := 0|1>

	uart:~$ arm_scmi base revision
	ARM SCMI base protocol v0002.0000
	  vendor        :EPAM
	  subvendor     :
	  fw version    :0x40
	  protocols     :2
	  num_agents    :8

	uart:~$ arm_scmi base discover_agent
	  agent id      :0
	  agent name    :agent-0

	uart:~$ arm_scmi base device_permission 1 1 1
	agent:1 device:1 permission set to allow

	uart:~$ arm_scmi base device_permission 2 1 1
	E: failed to read reply
	E: base agent:2 device:1 permission allow:1 failed (-13)
	base proto device permission failed (-13)

	uart:~$ arm_scmi base reset_agent_cfg 1 1
	agent:1 reset cfg reset_perm:reset

	uart:~$ arm_scmi base device_permission 2 1 1
	agent:2 device:1 permission set to allow
