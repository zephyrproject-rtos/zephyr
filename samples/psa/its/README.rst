.. zephyr:code-sample:: psa_its
   :name: PSA Internal Trusted Storage API
   :relevant-api: psa_its

   Use the PSA ITS API.

Overview
********

This sample demonstrates usage of the
`PSA Internal Trusted Storage (ITS) API <https://arm-software.github.io/psa-api/storage/1.0/overview/architecture.html#the-internal-trusted-storage-api>`_,
which is part of the `PSA Secure Storage API <https://arm-software.github.io/psa-api/storage/>`_,
for storing and retrieving persistent data.

Requirements
************

An implementation of the PSA ITS API must be present for this sample to build.
It can be provided by:

* :ref:`tfm`, for platforms supporting it.
* The :ref:`secure storage subsystem <secure_storage>`, for the other platforms.

Building
********

This sample is located in :zephyr_file:`samples/psa/its`.

Different configurations are defined in the :file:`sample.yaml` file.
You can use them to build the sample, depending on the platform to be built for, as follows:

.. tabs::

   .. tab:: TF-M

     For platforms with TF-M:

      .. zephyr-app-commands::
         :zephyr-app: samples/psa/its
         :tool: west
         :goals: build
         :board: <ns_platform>
         :west-args: -T sample.psa.its.tfm

   .. tab:: secure storage subsystem

      If the platform to be compiled for has an entropy driver (preferable):

      .. zephyr-app-commands::
         :zephyr-app: samples/psa/its
         :tool: west
         :goals: build
         :board: <platform>
         :west-args: -T sample.psa.its.secure_storage.entropy_driver

      Or, to use timer-based entropy (not secure):

      .. zephyr-app-commands::
         :zephyr-app: samples/psa/its
         :tool: west
         :goals: build
         :board: <platform>
         :west-args: -T sample.psa.its.secure_storage.entropy_not_secure

To flash it, see :ref:`west-flashing`.

API reference
*************

.. doxygengroup:: psa_its
