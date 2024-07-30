.. zephyr:code-sample:: edac
   :name: EDAC shell
   :relevant-api: edac

   Test error detection and correction (EDAC) using shell commands.

Overview
********

This sample demonstrates the :ref:`EDAC driver API <edac_api>` in a simple EDAC shell sample.

Building and Running
********************

This sample can be found under :zephyr_file:`samples/subsys/edac` in the
Zephyr tree.
The sample can be built as follows for the :ref:`intel_ehl_crb` board:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/edac
   :host-os: unix
   :board: intel_ehl_crb
   :goals: build
   :compact:

The Zephyr image that's created can be run on the :ref:`intel_ehl_crb` board
as per the instructions in the board documentation. Check out the
:ref:`intel_ehl_crb` for details.

Sample output
*************

Getting help
============

After the application has started help can be read with the following
command:

.. code-block:: console

   uart:~$ edac -h
   edac - EDAC information
   Subcommands:
     info    :Show EDAC information
              edac info <subcommands>
     inject  :Inject ECC error commands
              edac inject <subcommands>

Help for subcommand info can be read with:

.. code-block:: console

   uart:~$ edac info -h
   info - Show EDAC information
          edac info <subcommands>
   Subcommands:
     ecc_error     :ECC Error Show / Clear commands
     parity_error  :Parity Error Show / Clear commands

Injection help can be received with:

.. code-block:: console

   uart:~$ edac inject -h
   inject - Inject ECC error commands
            edac inject <subcommands>
   Subcommands:
     addr          :Get / Set physical address
     mask          :Get / Set address mask
     trigger       :Trigger injection
     error_type    :Get / Set injection error type
     disable_nmi   :Disable NMI
     enable_nmi    :Enable NMI
     test_default  :Test default injection parameters

Testing Error Injection
=======================

Set Error Injection parameters with:

.. code-block:: console

   uart:~$ edac inject addr 0x1000
   Set injection address base to: 0x1000

   uart:~$ edac inject mask 0x7fffffffc0
   Set injection address mask to 7fffffffc0

   uart:~$ edac inject error_type correctable
   Set injection error type: correctable

Trigger injection with:

.. code-block:: console

   uart:~$ edac inject trigger
   Triggering injection

Now Read / Write to the injection address to trigger Error Injection with
following devmem commands:

.. code-block:: console

   uart:~$ devmem 0x1000 32 0xabcd
   Mapped 0x1000 to 0x2ffcf000

   Using data width 32
   Writing value 0xabcd

   uart:~$ devmem 0x1000
   Mapped 0x1000 to 0x2ffce000

   Using data width 32
   Read value 0xabcd

We should get the following message on screen indicating an IBECC event:

.. code-block:: none

   Got notification about IBECC event
