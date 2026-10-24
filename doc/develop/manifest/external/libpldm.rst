.. _external_module_libpldm:

libpldm
#######

Introduction
************

The `libpldm`_ library provides a library for encoding and decoding of Platform
Level Data Model (`PLDM`_) messages. PLDM is a `DMTF`_ protocol "designed to be
an effective interface and data model that provides efficient access to
low-level platform inventory, monitoring, control, event, and data/parameters
transfer functions".
libpldm is developed by the `OpenBMC`_ project, but it can also be used in other
contexts.


Usage with Zephyr
*****************

To pull in libpldm as a Zephyr module, either add it as a West project in the
``west.yml`` file or pull it in by adding a submanifest (e.g.
``zephyr/submanifests/libpldm.yaml``) file with the following content and run
``west update``:

.. code-block:: yaml

   manifest:
     projects:
       - name: libpldm
         url: https://github.com/openbmc/libpldm.git
         revision: main
         path: modules/lib/libpldm

Then enable the library in the application's ``prj.conf``:

.. code-block:: cfg

   CONFIG_PLDM=y

The libpldm headers can then be included by the application, e.g.
``#include <libpldm/base.h>``.

Reference
*********

.. target-notes::

.. _libpldm:
   https://github.com/openbmc/libpldm

.. _PLDM:
    https://dmtf.org/sites/default/files/standards/documents/DSP0240_1.2.1.pdf

.. _DMTF:
   https://www.dmtf.org/

.. _OpenBMC:
   https://github.com/openbmc
