.. _external_module_emlearn:

emlearn
#######

Introduction
************

`emlearn`_ is an open source library for deploying machine learning models on micro-controllers
and embedded systems. It provides portable C code generation from models trained with
scikit-learn or Keras.

A Python library allows converting complex machine learning models to a minimal C code
representation, which enables running ML inference on resource-constrained embedded devices.

emlearn is licensed under the MIT license.

Usage with Zephyr
*****************

The emlearn repository is a Zephyr :ref:`module <modules>` which provides TinyML capabilities to
Zephyr applications, allowing machine learning models to be run directly on Zephyr-powered devices.

To pull in emlearn as a Zephyr module, either add it as a West project in the ``west.yaml``
file or pull it in by adding a submanifest (e.g. ``zephyr/submanifests/emlearn.yaml``) file
with the following content and run ``west update``:

.. code-block:: yaml

   manifest:
     projects:
       - name: emlearn
         url: https://github.com/emlearn/emlearn.git
         revision: master
         path: modules/lib/emlearn # adjust the path as needed

For more detailed instructions and API documentation, refer to the `emlearn documentation`_, and in
particular the `Getting Started on Zephyr RTOS`_ section.

References
**********

.. target-notes::

.. _emlearn:
   https://github.com/emlearn/emlearn

.. _emlearn documentation:
   https://emlearn.readthedocs.io/en/latest/

.. _Getting Started on Zephyr RTOS:
   https://emlearn.readthedocs.io/en/latest/getting_started_zephyr.html
