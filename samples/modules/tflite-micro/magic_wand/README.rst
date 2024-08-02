.. _tensorflow_magic_wand:

TensorFlow Lite Micro Magic Wand sample
#######################################

Overview
********

This sample application shows how to use TensorFlow Lite Micro
to run a 20 kilobyte neural network model that recognizes gestures
from an accelerometer.

.. Note::
    This README and sample have been modified from
    `the TensorFlow Magic Wand sample for Zephyr`_ and
    `the Antmicro tutorial on Renode emulation for TensorFlow`_.

.. _the TensorFlow Magic Wand sample for Zephyr:
    https://github.com/tensorflow/tflite-micro-arduino-examples/tree/main/examples/magic_wand

.. _the Antmicro tutorial on Renode emulation for TensorFlow:
    https://github.com/antmicro/litex-vexriscv-tensorflow-lite-demo

Building and Running
********************

Add the tflite-micro module to your West manifest and pull it:

.. code-block:: console

    west config manifest.project-filter -- +tflite-micro
    west update

The application can be built for the :ref:`litex-vexriscv` for
emulation in Renode as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/modules/tflite-micro/magic_wand
   :host-os: unix
   :board: litex_vexriscv
   :goals: build
   :compact:

Once the application is built, `download and install Renode 1.12 or higher as a package`_
following the instructions in the `Renode GitHub README`_ and
start the emulator:

.. code-block:: console

    renode -e "set zephyr_elf @./build/zephyr/zephyr.elf; s @./samples/modules/tflite-micro/magic_wand/renode/litex-vexriscv-tflite.resc"

.. _download and install Renode 1.12 or higher as a package:
    https://github.com/renode/renode/releases/

.. _Renode GitHub README:
    https://github.com/renode/renode/blob/master/README.rst

Sample Output
=============

The Renode-emulated LiteX/VexRiscv board is fed data that the
application recognizes as a series of alternating ring and slope
gestures.

.. code-block:: console

    Got accelerometer, label: accel-0

    RING:
              *
           *     *
         *         *
        *           *
         *         *
           *     *
              *

    SLOPE:
            *
           *
          *
         *
        *
       *
      *
     * * * * * * * *

    RING:
              *
           *     *
         *         *
        *           *
         *         *
           *     *
              *

    SLOPE:
            *
           *
          *
         *
        *
       *
      *
     * * * * * * * *

Modifying Sample for Your Own Project
*************************************

It is recommended that you copy and modify one of the two TensorFlow
samples when creating your own TensorFlow project. To build with
TensorFlow, you must enable the below Kconfig options in your :file:`prj.conf`:

.. code-block:: cfg

    CONFIG_CPP=y
    CONFIG_REQUIRES_FULL_LIBC=y
    CONFIG_TENSORFLOW_LITE_MICRO=y

Training
********
Follow the instructions in the :file:`train/` directory to train your
own model for use in the sample.
