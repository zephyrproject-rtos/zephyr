.. zephyr:code-sample:: psa_its
   :name: PSA Internal Trusted Storage API
   :relevant-api: psa_secure_storage

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

* :ref:`tfm`, for ``*/ns`` :term:`board targets<board target>`.
* The :ref:`secure storage subsystem <secure_storage>`, for the other board targets.

Building
********

This sample is located in :zephyr_file:`samples/psa/its`.

Different configurations are defined in the :file:`sample.yaml` file.
You can use them to build the sample, depending on the PSA ITS provider, as follows:

.. tabs::

   .. tab:: TF-M

     For board targets with TF-M:

      .. zephyr-app-commands::
         :zephyr-app: samples/psa/its
         :tool: west
         :goals: build
         :board: <ns_board_target>
         :west-args: -T sample.psa.its.tfm

   .. tab:: secure storage subsystem

      For board targets without TF-M.

      If the board target to compile for has an entropy driver (preferable):

      .. zephyr-app-commands::
         :zephyr-app: samples/psa/its
         :tool: west
         :goals: build
         :board: <board_target>
         :west-args: -T sample.psa.its.secure_storage.entropy_driver

      Or, to use an insecure entropy source (only for testing):

      .. zephyr-app-commands::
         :zephyr-app: samples/psa/its
         :tool: west
         :goals: build
         :board: <board_target>
         :west-args: -T sample.psa.its.secure_storage.entropy_not_secure

To flash it, see :ref:`west-flashing`.
