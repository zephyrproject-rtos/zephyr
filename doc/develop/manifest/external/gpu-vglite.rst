.. _external_module_gpu_vglite:

GPU VGLite Driver
#################

Introduction
************

The VGLite GPU Driver module provides hardware-accelerated 2D graphics rendering
capabilities for embedded systems. VGLite is a lightweight, high-performance 2D
vector graphics library designed for resource-constrained embedded applications
with GPU acceleration support.

The driver enables efficient vector graphics rendering, image processing, and
advanced 2D graphics operations with minimal CPU overhead, making it ideal for
GUI applications, industrial displays, and consumer electronics.

The VGLite driver integrates with Zephyr's display subsystem and provides a
hardware abstraction layer for VGLite GPU operations within the Zephyr RTOS
environment. It supports various SoCs with integrated VGLite GPU hardware,
including NXP i.MX RT series microcontrollers.

Usage with Zephyr
*****************

To pull in gpu-vglite as a Zephyr module, add it using a submanifest
(for example, ``zephyr/submanifests/gpu-vglite.yaml``) with the
following content, and then run ``west update``:

.. code-block:: yaml

   manifest:
     projects:
       - name: gpu-vglite
         url: https://github.com/VeriSilicon/VGLite_4.x.git
         revision: master
         path: modules/gpu-vglite

For more detailed instructions and API documentation, refer to the
`gpu-vglite documentation <https://github.com/nxp-mcuxpresso/gpu-vglite>`_.

Reference
*********

- `VGLite API Documentation <https://www.nxp.com/docs/en/reference-manual/IMXRTVGLITEAPIRM.pdf>`_
- `NXP i.MX RT Series <https://www.nxp.com/products/processors-and-microcontrollers/arm-microcontrollers/i-mx-rt-crossover-mcus:IMX-RT-SERIES>`_
