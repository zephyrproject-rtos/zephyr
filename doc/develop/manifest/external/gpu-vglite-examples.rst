.. _external_module_gpu_vglite_examples:

VGLite GPU Examples
###################

Introduction
************

The VGLite GPU Examples module provides a comprehensive collection of sample
applications demonstrating hardware-accelerated 2D graphics rendering capabilities
using the VGLite GPU library in Zephyr. VGLite is a lightweight 2D vector graphics
library optimized for embedded systems with GPU acceleration support.

This module is specifically designed for NXP i.MX RT series microcontrollers
featuring integrated VGLite GPU hardware, enabling high-performance graphics
rendering for embedded applications with minimal CPU overhead.

The examples showcase various VGLite features including vector graphics rendering,
transformations, texture compression, and advanced rendering techniques suitable
for GUI applications, industrial displays, and consumer electronics.

Relation to Zephyr
==================

The VGLite GPU Examples integrate with Zephyr's display subsystem and demonstrate
the use of VGLite GPU acceleration within the Zephyr RTOS environment. These
examples leverage:

- Zephyr's display driver interface
- VGLite GPU hardware abstraction
- Zephyr's devicetree configuration
- Board-specific display shield overlays
- Zephyr's build system and west tool integration

The module provides practical reference implementations for developers building
graphics-intensive applications on Zephyr-supported NXP i.MX RT platforms.

Usage with Zephyr
*****************

To pull in gpu-vglite-examples as a Zephyr module, add it using a submanifest
(for example, ``zephyr/submanifests/gpu-vglite-examples.yaml``) with the
following content, and then run ``west update``:

.. code-block:: yaml

   manifest:
     projects:
       - name: gpu-vglite-examples
         url: https://github.com/nxp-mcuxpresso/gpu-vglite-examples.git
         revision: zephyr
         path: modules/gpu-vglite-examples

Similarly, to pull in the gpu-vglite driver module, add it using a submanifest
(for example, ``zephyr/submanifests/gpu-vglite.yaml``) with the following
content, and then run ``west update``:

.. code-block:: yaml

   manifest:
     projects:
       - name: gpu-vglite
         url: https://github.com/nxp-mcuxpresso/gpu-vglite.git
         revision: main
         path: modules/driver/gpu-vglite

For more detailed instructions and vglite samples, refer to the
`gpu-vglite-examples documentation <https://github.com/nxp-mcuxpresso/gpu-vglite-examples>`_
and `gpu-vglite documentation <https://github.com/nxp-mcuxpresso/gpu-vglite>`_.

Reference
*********

- `NXP i.MX RT Series <https://www.nxp.com/products/processors-and-microcontrollers/arm-microcontrollers/i-mx-rt-crossover-mcus:IMX-RT-SERIES>`_
- `VGLite API Documentation <https://www.nxp.com/docs/en/reference-manual/IMXRTVGLITEAPIRM.pdf>`_
- `VGLite GitHub Repository <https://github.com/nxp-mcuxpresso/gpu-vglite>`_

