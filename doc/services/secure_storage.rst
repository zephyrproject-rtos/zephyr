.. _secure_storage:

Secure storage
##############

The secure storage subsystem implements the
`Platform Security Architecture (PSA) Secure Storage API <https://arm-software.github.io/psa-api/storage/>`_.
It can be enabled on platforms that don't already have an implementation of the API.

Overview
********

The secure storage subsystem makes the PSA Secure Storage API available on all platforms.
As such, it provides an implementation of the API on platforms that don't already have one, ensuring functional support for the API.
Platforms with :ref:`tfm` enabled (ending in ``/ns``), for instance,
cannot enable the subsystem because TF-M already provides an implementation of the API.

In addition to providing functional support for the API, depending on device-specific security features
and the configuration, the subsystem may secure the data stored via the PSA Secure Storage API at rest.
However, it is recommended to use a secure processing environment like TF-M when possible
because they are able to provide better security due to better isolation and protection guarantees.

.. toctree::
   :caption: Subpages
   :glob:

   secure_storage/*

Samples
*******

* :zephyr:code-sample:`persistent_key`
* :zephyr:code-sample:`psa_its`

PSA Secure Storage API reference
********************************

.. doxygengroup:: psa_secure_storage
