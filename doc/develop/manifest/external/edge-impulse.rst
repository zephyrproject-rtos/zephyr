.. _external_module_edge_impulse:

Edge Impulse SDK for Zephyr
############################

Overview
************

Edge Impulse is a leading edge AI design and development platform for deploying machine learning
to edge devices. The Edge Impulse SDK for Zephyr packages the Edge Impulse inferencing SDK as a
Zephyr module, enabling easy integration with Zephyr's build system. Sign up for a free account
`here <https://www.edgeimpulse.com/signup>`_.

The module packages the Edge Impulse inferencing SDK as a Zephyr module, enabling easy
integration with Zephyr's build system. It can also provide west extension commands to build (on
the Edge Impulse platform) and download model deployment artifacts into your workspace for
integration into a Zephyr application.

The Edge Impulse SDK is provided under the `BSD-3-Clause-Clear license
<https://github.com/edgeimpulse/edge-impulse-sdk-zephyr?tab=BSD-3-Clause-Clear-1-ov-file>`_.
Note that Edge Impulse cloud services require at least a free tier account or api-key to a
public project; see the `Edge Impulse website <https://edgeimpulse.com>`_ for pricing and terms
of service.

Adding the Module to Your Project
**********************************

To use the module, add the following entry (replace ``v1.82.3`` with the desired version):

.. code-block:: yaml

   manifest:
     projects:
       - name: edge-impulse-sdk-zephyr
         url: https://github.com/edgeimpulse/edge-impulse-sdk-zephyr
         revision: v1.82.3
         path: modules/edge-impulse-sdk-zephyr
         west-commands: west/west-commands.yml

to a Zephyr submanifest and run ``west update``, or add it to your project's ``west.yml`` manifest.

Usage With Zephyr
******************

Building and deploying Edge Impulse models can be done using the provided west extension commands,
or manually. See `Edge Impulse Zephyr Module Deployment`_ for detailed instructions.

**West Extension Commands:**

- ``west ei-build``: Triggers Studio build with optional parameters (``-e tflite-eon``,
  ``-t int8``, ``-i 1``)
- ``west ei-deploy``: Downloads pre-built deployment artifacts
- Both require ``-k`` (API key) and ``-p`` (project ID) options

For step-by-step tutorials and guides on integrating Edge Impulse with Zephyr, see the
`Edge Impulse Zephyr Module Deployment`_.

References
**********

.. target-notes::

.. _Edge Impulse Zephyr Module Deployment:
   https://docs.edgeimpulse.com/hardware/deployments/run-zephyr-module
