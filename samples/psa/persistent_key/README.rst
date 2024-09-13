.. zephyr:code-sample:: persistent_key
   :name: PSA Crypto persistent key

   Manage and use persistent keys via the PSA Crypto API.

Overview
********

This sample demonstrates usage of persistent keys in the :ref:`PSA Crypto API <psa_crypto>`.

Requirements
************

In addition to the PSA Crypto API, an implementation of the
`PSA Internal Trusted Storage (ITS) API <https://arm-software.github.io/psa-api/storage/1.0/overview/architecture.html#the-internal-trusted-storage-api>`_
(for storage of the persistent keys) must be present for this sample to work.
It can be provided by:

* :ref:`tfm`, for ``*/ns`` :term:`board targets<board target>`.
* The :ref:`secure storage subsystem <secure_storage>`, for the other board targets.

Building
********

This sample is located in :zephyr_file:`samples/psa/persistent_key`.

Different configurations are defined in the :file:`sample.yaml` file.
You can use them to build the sample, depending on the PSA ITS provider, as follows:

.. tabs::

   .. tab:: TF-M

     For board targets with TF-M:

      .. zephyr-app-commands::
         :zephyr-app: samples/psa/persistent_key
         :tool: west
         :goals: build
         :board: <ns_board_target>
         :west-args: -T sample.psa.persistent_key.tfm

   .. tab:: secure storage subsystem

      For board targets without TF-M.

      If the board target to compile for has an entropy driver (preferable):

      .. zephyr-app-commands::
         :zephyr-app: samples/psa/persistent_key
         :tool: west
         :goals: build
         :board: <board_target>
         :west-args: -T sample.psa.persistent_key.secure_storage.entropy_driver

      Or, to use an insecure entropy source (only for testing):

      .. zephyr-app-commands::
         :zephyr-app: samples/psa/persistent_key
         :tool: west
         :goals: build
         :board: <board_target>
         :west-args: -T sample.psa.persistent_key.secure_storage.entropy_not_secure

To flash it, see :ref:`west-flashing`.

API reference
*************

`PSA Crypto key management API reference <https://arm-software.github.io/psa-api/crypto/1.2/api/keys/index.html>`_
