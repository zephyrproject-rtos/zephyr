.. _snippet-nxp-ot:

NXP OpenThread (nxp-ot)
#######################

Overview
********

This snippet enables NXP OpenThread platform support including:

- PSA crypto for OpenThread security
- Board-specific radio interface configuration

Supported Boards
****************

- ``frdm_rw612`` / ``rd_rw612_bga`` (HDLC RCP interface for 802.15.4)
- ``frdm_mcxw72`` (native 802.15.4 radio)

Usage
*****

.. code-block:: console

   west build -b rd_rw612_bga <sample> -S nxp-ot
