Bluetooth: Telephone and Media Audio Profile Shell
##################################################

This document describes how to run the Telephone and Media Audio Profile functionality.
Unlike most other low-layer profiles, TMAP is a profile that exists and has a service (TMAS) on all
devices. Thus both the initiator and acceptor (or central and peripheral) should do a discovery of
the remote device's TMAS to see what TMAP roles they support.

Using the TMAP Shell
********************

When the Bluetooth stack has been initialized (:code:`bt init`), the TMAS can be registered by
by calling :code:`tmap init`.

.. code-block:: console


   tmap --help
   tmap - Bluetooth TMAP shell commands
   Subcommands:
     init          :Initialize and register the TMAS
     discover      :Discover TMAS on remote device
