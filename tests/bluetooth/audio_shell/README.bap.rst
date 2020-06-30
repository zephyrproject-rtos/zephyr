Bluetooth: Basic Audio Profile
##############################

This document describes how to run basic audio profile functionality which
includes:

  - Capabilities and Endpoint discovery
  - Audio Stream Endpoint procedures
  - Managing, linking and unlinking audio stream endpoints

Commands
********

.. code-block:: console

   bap --help
   ase - ASE related commands
   Subcommands:
     init     :
     discover :
     config   :<ase> <direction: sink, source> [codec] [preset]
     qos      :[preset] [interval] [framing] [latency] [pd] [sdu] [phy] [rtn]
     enable   :
     start    :
     disable  :
     release  :
     list     :
     select   :<ase>
     link     :<ase1> <ase2>
     unlink   :<ase1> <ase2>

1. Discover PACs and ASEs:

.. code-block:: console

   uart:~$ bap discover <dir>

2. Configure Codec:

.. code-block:: console

   uart:~$ bap config <ase> <dir>

3. Configure QoS:

.. code-block:: console

   uart:~$ bap qos [preset] [interval] [framing] [latency] [pd] [sdu] [phy] [rtn]

4. Enable ASE:

.. code-block:: console

   uart:~$ bap enable

5. [Sink] Start ASE:

.. code-block:: console

   uart:~$ bap enable

6. Disable ASE:

.. code-block:: console

   uart:~$ bap disable

7. Release ASE:

.. code-block:: console

   uart:~$ bap release

8. List ASEs:

.. code-block:: console

   uart:~$ bap list

9. Select ASE:

.. code-block:: console

   uart:~$ bap select <ase>

10. Link ASEs:

.. code-block:: console

   uart:~$ bap link <ase1> <ase2>

11. Unlink ASEs:

.. code-block:: console

   uart:~$ bap unlink <ase1> <ase2>
