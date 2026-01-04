.. _external_module_edge_impulse:

Edge Impulse SDK for Zephyr
############################



Overview
************

Edge Impulse is a development platform for machine learning on edge devices. The Edge Impulse SDK for Zephyr
packages the Edge Impulse inferencing SDK as a Zephyr module, enabling easy integration with Zephyr's build system.

The module can also provide west extension commands for working with Edge Impulse
model artifacts.



Adding the Module to Your Project
*****************

To use the module, add the following entry:

.. code-block:: yaml

   manifest:
     projects:
       - name: edge-impulse-sdk-zephyr
         url: https://github.com/edgeimpulse/edge-impulse-sdk-zephyr
         revision: vX.Y.Z
         path: modules/edge-impulse-sdk-zephyr
         west-commands: scripts/west-commands.yml

to a Zephyr submanifest and run ``west update``, or add it to your project's
``west.yml`` manifest.

Usage With Zephyr
*****************

Building and deploying Edge Impulse models can be done using the provided west extension commands, or manually. See
`Edge Impulse Zephyr Module Deployment`_ for detailed instructions.

**West Extension Commands:**

- `west ei-build`: Triggers Studio build with optional parameters (`-e tflite-eon`, `-t int8`, `-i 1`)
- `west ei-deploy`: Downloads pre-built deployment artifacts
- Both require `-k` (API key) and `-p` (project ID) flags

**Note:** Commands must be run from the manifest repository directory and require `west-commands` registration in `west.yml`.

For step-by-step tutorials and guides on integrating Edge Impulse with Zephyr, see the
`_Edge Impulse Zephyr Module Deployment`. 

References
**********

.. target-notes::

.. _Sign up here:
   https://www.edgeimpulse.com/signup

.. _Edge Impulse Zephyr Module Series:
   https://docs.edgeimpulse.com/tutorials/topics/zephyr/zephyr-module-series

.. _Edge Impulse Zephyr Module Deployment:
   https://docs.edgeimpulse.com/hardware/deployments/run-zephyr-module

.. _edge-impulse-sdk-zephyr:
   https://github.com/edgeimpulse/edge-impulse-sdk-zephyr

.. _Example project:
   https://github.com/edgeimpulse/example-standalone-inferencing-zephyr-module