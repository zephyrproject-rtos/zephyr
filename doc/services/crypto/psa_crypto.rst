.. _psa_crypto:

PSA Crypto
##########

Overview
********

The PSA (Platform Security Architecture) Crypto API offers a portable
programming interface for cryptographic operations and key storage
across a wide range of hardware. It is designed to be user-friendly
while still providing access to the low-level primitives essential for
modern cryptography.

It is created and maintained by Arm. Arm developed the PSA as a
comprehensive security framework to address the increasing security
needs of connected devices.

In Zephyr, the PSA Crypto API is implemented using Mbed TLS, an
open-source cryptographic library that provides the underlying
cryptographic functions.

Design Goals
************

The interface is suitable for a vast range of devices: from
special-purpose cryptographic processors that process data with a
built-in key, to constrained devices running custom application code,
such as microcontrollers, and multi-application devices, such as
servers. It follows the principle of cryptographic agility.

Algorithm Flexibility
  The PSA Crypto API supports a wide range of cryptographic algorithms,
  allowing developers to switch between different cryptographic
  methods as needed. This flexibility is crucial for maintaining
  security as new algorithms emerge and existing ones become obsolete.

Key Management
  The PSA Crypto API includes robust key management features that
  support the creation, storage, and use of cryptographic keys in a
  secure and flexible manner. It uses opaque key identifiers, which
  allows for easy key replacement and updates without exposing key
  material.

Implementation Independence
  The PSA Crypto API abstracts the underlying cryptographic library,
  meaning that the specific implementation can be changed without
  affecting the application code. This abstraction supports
  cryptographic agility by enabling the use of different cryptographic
  libraries or hardware accelerators as needed.

Future-Proofing
  By adhering to cryptographic agility, PSA Crypto ensures that
  applications can quickly adapt to new cryptographic standards and
  practices, enhancing long-term security and compliance.

Examples of Applications
************************

Network Security (TLS)
  The API provides all of the cryptographic primitives needed to establish TLS connections.

Secure Storage
  The API provides all primitives related to storage encryption, block
  or file-based, with master encryption keys stored inside a key store.

Network Credentials
  The API provides network credential management inside a key store,
  for example, for X.509-based authentication or pre-shared keys on
  enterprise networks.

Device Pairing
  The API provides support for key agreement protocols that are often
  used for secure pairing of devices over wireless channels. For
  example, the pairing of an NFC token or a Bluetooth device might use
  key agreement protocols upon first use.

Secure Boot
  The API provides primitives for use during firmware integrity and
  authenticity validation, during a secure or trusted boot process.

Attestation
  The API provides primitives used in attestation
  activities. Attestation is the ability for a device to sign an array
  of bytes with a device private key and return the result to the
  caller. There are several use cases; ranging from attestation of the
  device state, to the ability to generate a key pair and prove that it
  has been generated inside a secure key store. The API provides access
  to the algorithms commonly used for attestation.

Factory Provisioning
  Most IoT devices receive a unique identity during the factory
  provisioning process, or once they have been deployed to the
  field. This API provides the APIs necessary for populating a device
  with keys that represent that identity.

Usage considerations
********************

Always check for errors
  Most functions in the PSA Crypto API can return errors. All functions
  that can fail have the return type ``psa_status_t``. A few functions
  cannot fail, and thus, return void or some other type.

  If an error occurs, unless otherwise specified, the content of the
  output parameters is undefined and must not be used.

  Some common causes of errors include:

  * In implementations where the keys are stored and processed in a
    separate environment from the application, all functions that need
    to access the cryptography processing environment might fail due
    to an error in the communication between the two environments.

  * If an algorithm is implemented with a hardware accelerator, which
    is logically separate from the application processor, the
    accelerator might fail, even when the application processor keeps
    running normally.

  * Most functions might fail due to a lack of resources. However,
    some implementations guarantee that certain functions always have
    sufficient memory.

  * All functions that access persistent keys might fail due to a
    storage failure.

  * All functions that require randomness might fail due to a lack of
    entropy. Implementations are encouraged to seed the random
    generator with sufficient entropy during the execution of
    ``psa_crypto_init()``. However, some security standards require
    periodic reseeding from a hardware random generator, which can
    fail.

Shared memory and concurrency
  Some environments allow applications to be multithreaded, while
  others do not. In some environments, applications can share memory
  with a different security context. In environments with
  multithreaded applications or shared memory, applications must be
  written carefully to avoid data corruption or leakage. This
  specification requires the application to obey certain constraints.

  In general, the PSA Crypto API allows either one writer or any number of
  simultaneous readers, on any given object. In other words, if two or
  more calls access the same object concurrently, then the behavior is
  only well-defined if all the calls are only reading from the object
  and do not modify it. Read accesses include reading memory by input
  parameters and reading keystore content by using a key. For more
  details, refer to `Concurrent calls
  <https://arm-software.github.io/psa-api/crypto/1.2/overview/conventions.html#concurrent-calls>`_

  If an application shares memory with another security context, it
  can pass shared memory blocks as input buffers or output buffers,
  but not as non-buffer parameters. For more details, refer to
  `Stability of parameters <https://arm-software.github.io/psa-api/crypto/1.2/overview/conventions.html#stability-of-parameters>`_.

Cleaning up after use
  To minimize impact if the system is compromised, it is recommended
  that applications wipe all sensitive data from memory when it is no
  longer used. That way, only data that is currently in use can be
  leaked, and past data is not compromised.

  Wiping sensitive data includes:

  * Clearing temporary buffers in the stack or on the heap.

  * Aborting operations if they will not be finished.

  * Destroying keys that are no longer used.

References
**********

* `PSA Crypto`_

.. _PSA Crypto:
   https://arm-software.github.io/psa-api/crypto/

* `Mbed TLS`_

.. _Mbed TLS:
   https://www.trustedfirmware.org/projects/mbed-tls/
