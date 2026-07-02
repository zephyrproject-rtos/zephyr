.. zephyr:code-sample:: uwb-ranging
   :name: Ultra-Wide Band Ranging controller
   :relevant-api: uwb_api

   Start a controller-initiator ranging session

.. _demo-ranging-controller:

=======================================================================
 Overview
=======================================================================

.. brief:start

This demo showcases normal ranging with one device configured as a Controller - Initiator
and another device configured as a Controlee - Responder [Another demo].

.. brief:end


Following sequence of steps are handled.

- Initialize UWBD in Mainline Firmware.
- Set the application ranging parameters
- Perform normal ranging with static STS.

Building
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Enable the required UWB shield through ``--shield`` option on command line

.. code-block:: bash

    west build --pristine -b frdm_rw612 --shield nxp_sr250 samples/uwb/demo_ranging_controller

Run demo_ranging_controlee on counterpart device for successful ranging
