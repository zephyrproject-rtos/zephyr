.. _secure_storage:

Secure Storage
##############

| The secure storage subsystem provides an implementation of the functions defined in the
  `Platform Security Architecture (PSA) Secure Storage API <https://arm-software.github.io/psa-api/storage/>`_.
| It can be enabled on :term:`board targets<board target>`
  that don't already have an implementation of the API.

Overview
********

The secure storage subsystem makes the PSA Secure Storage API available on all board targets with
non-volatile memory support.
As such, it provides an implementation of the API on those that don't already have one, ensuring
functional support for the API.
Board targets with :ref:`tfm` enabled (ending in ``/ns``), for instance,
cannot enable the subsystem because TF-M already provides an implementation of the API.

| In addition to providing functional support for the API, depending on
  device-specific security features and the configuration, the subsystem
  may secure the data stored via the PSA Secure Storage API at rest.
| Keep in mind, however, that it's preferable to use a secure processing environment like TF-M when
  possible because it's able to provide more security due to isolation guarantees.

Limitations
***********

The secure storage subsystem's implementation of the PSA Secure Storage API:

* does not aim at full compliance with the specification.

  | Its foremost goal is functional support for the API on all board targets.
  | See below for important ways the implementation deviates from the specification.

* does not guarantee that the data it stores will be secure at rest in all cases.

  This depends on device-specific security features and the configuration.

Below are some ways the implementation purposefully deviates from the specification
and an explanation why. This is not an exhaustive list.

* The UID type is only 30 bits by default. (Against `2.5 UIDs <https://arm-software.github.io/psa-api/storage/1.0/overview/architecture.html#uids>`_.)

  | This is an optimization done to make it more convenient to directly use the UIDs as
    storage entry IDs (e.g., with :ref:`ZMS <zms_api>` when
    :kconfig:option:`CONFIG_SECURE_STORAGE_ITS_STORE_IMPLEMENTATION_ZMS` or
    :kconfig:option:`CONFIG_SECURE_STORAGE_PS_STORE_IMPLEMENTATION_ZMS` is enabled).
  | Zephyr defines numerical ranges to be used by different users of the API which guarantees that
    there are no collisions and that they all fit within 30 bits.
    See the header files in :zephyr_file:`include/zephyr/psa` for more information.

* The data stored in the ITS is by default encrypted and authenticated (Against ``1.`` in
  `3.2. Internal Trusted Storage requirements <https://arm-software.github.io/psa-api/storage/1.0/overview/requirements.html#internal-trusted-storage-requirements>`_.)

  | The specification considers the storage underlying the ITS to be
    ``implicitly confidential and protected from replay``
    (`2.4. The Internal Trusted Storage API <https://arm-software.github.io/psa-api/storage/1.0/overview/architecture.html#the-internal-trusted-storage-api>`_)
    because ``most embedded microprocessors (MCU) have on-chip flash storage that can be made
    inaccessible except to software running on the MCU``
    (`2.2. Technical Background <https://arm-software.github.io/psa-api/storage/1.0/overview/architecture.html#technical-background>`_).
  | This is not the case on all MCUs. Thus, additional protection is provided to the stored data.

  However, this does not guarantee that the data stored will be secure at rest in all cases,
  because this depends on device-specific security features and the configuration.
  It requires a random entropy source and especially a secure encryption key provider
  (:kconfig:option:`CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_KEY_PROVIDER`).

  In addition, the data stored in the ITS is not protected against replay attacks,
  because this requires storage that is protected by hardware.
  This also limits the replay protection of the data stored via Zephyr's implementation of the
  PS API, whose replay protection values are stored in the ITS (see `PS API`_).

* The data stored via the PSA Secure Storage API is not protected from direct
  read/write by software or debugging. (Against ``2.`` and ``10.`` in
  `3.2. Internal Trusted Storage requirements <https://arm-software.github.io/psa-api/storage/1.0/overview/requirements.html#internal-trusted-storage-requirements>`_.)

  It is only secured at rest. Protecting it at runtime as well
  requires specific hardware mechanisms to support this.

* The ``PSA_STORAGE_FLAG_WRITE_ONCE`` flag only protects an entry against modification through
  the API, not against modification of the storage medium itself.

  | Upholding the flag requires knowing that an entry was created in the first place, which is
    state that has to survive the storage medium being rewritten. Like replay protection, that
    requires storage that is protected by hardware.
  | An attacker who can write to the storage medium can therefore have a write-once entry
    overwritten or removed: either by tampering with it, after which the subsystem treats it as
    corrupted and allows it to be replaced, or simply by erasing it, after which it appears never
    to have existed. Neither can be prevented by encrypting and authenticating the entry.
  | With Zephyr's implementation of the PS API, the create flags of PS entries are stored in the
    ITS, so this requires writing to the ITS storage medium, not only to the PS one.

* The ``psa_its_get*()`` functions can return ``PSA_ERROR_INVALID_SIGNATURE`` and
  ``PSA_ERROR_DATA_CORRUPT``.

  The specification doesn't define these for the ITS API because it assumes that the storage
  underlying it is protected by hardware, and thus that data read back from it is always intact.
  As it's not the case here, these error codes are passed on to let callers tell an entry that has
  been tampered with apart from an internal failure.

* With Zephyr's implementation of the PS API, an interrupted ``psa_ps_set()`` does not always
  retain the previous content of the entry. (Against ``11.`` in
  `3.1. Protected Storage requirements <https://arm-software.github.io/psa-api/storage/1.0/overview/requirements.html#protected-storage-requirements>`_.)

  | When an existing entry is overwritten, its new replay protection value is written to the ITS
    before its data is written to PS. A power loss between the two writes makes the entry
    unreadable until it is overwritten or removed: reading it returns
    ``PSA_ERROR_INVALID_SIGNATURE``.
  | ``PSA_STORAGE_FLAG_WRITE_ONCE`` is only added to the ITS data of an existing entry once its PS
    data has been written, so that an interrupted write never leaves an entry that can be neither
    read, overwritten nor removed. A power loss right after the PS data is written leaves the
    entry readable but without the flag.
  | If writing the PS data fails without a power loss, the previous ITS data is restored and the
    previous content of the entry is retained.
  | When an entry is created, or its ITS data cannot be read back, its data is written to PS
    before its ITS data, which includes the create flags. A power loss between the two writes, or
    a failure to write the ITS data, leaves the entry in its previous state: a new entry does not
    exist and reading it returns ``PSA_ERROR_DOES_NOT_EXIST``. The PS data left behind is replaced
    when the entry is created again, or deleted by ``psa_ps_remove()``, which returns
    ``PSA_ERROR_DOES_NOT_EXIST`` in that case.

Configuration
*************

To configure the implementation of the PSA Secure Storage API provided by Zephyr, have a look at the
available :kconfig:option-regex:`Kconfig options <CONFIG_SECURE_STORAGE_.*>`.
They are defined in the various Kconfig files found under :zephyr_file:`subsys/secure_storage/`.

Customization
*************

Custom implementations can also replace those of Zephyr at different levels
if the functionality provided by the existing implementations isn't enough.

Whole API
=========

If you already have an implementation of the whole ITS or PS API and want to make use of it, you
can do so by enabling the following Kconfig option and implementing the relevant functions:

* :kconfig:option:`CONFIG_SECURE_STORAGE_ITS_IMPLEMENTATION_CUSTOM`, for the ITS API.
* :kconfig:option:`CONFIG_SECURE_STORAGE_PS_IMPLEMENTATION_CUSTOM`, for the PS API.

ITS API
=======

Zephyr's implementation of the ITS API
(:kconfig:option:`CONFIG_SECURE_STORAGE_ITS_IMPLEMENTATION_ZEPHYR`)
makes use of the ITS transform and store modules, which can be configured and customized separately.
Have a look at the :kconfig:option-regex:`ITS transform and store Kconfig options
<CONFIG_SECURE_STORAGE_ITS_(TRANSFORM|STORE)_.*>` to see the different configuration possibilities.

It's especially recommended to use or implement a secure :kconfig:option-regex:`encryption
key provider <CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_KEY_PROVIDER_.*>`.

PS API
======

Zephyr's implementation of the PS API
(:kconfig:option:`CONFIG_SECURE_STORAGE_PS_IMPLEMENTATION_ZEPHYR`)
makes use of the PS transform, store and replay protection modules, which can be configured and
customized separately.
Have a look at the :kconfig:option-regex:`PS transform, store and replay protection Kconfig options
<CONFIG_SECURE_STORAGE_PS_(TRANSFORM|STORE|REPLAY_PROTECTION)_.*>` to see the different
configuration possibilities.
It is not enabled by default: with
:kconfig:option:`CONFIG_SECURE_STORAGE_PS_IMPLEMENTATION_ITS` (the default), the PS API directly
calls into the ITS API, and PS entries are stored like ITS entries.

.. warning::

   The two implementations store PS entries in incompatible ways, and no migration is done
   when switching from one to the other. Do not switch the PS implementation on devices that
   already have PS entries stored.

   With :kconfig:option:`CONFIG_SECURE_STORAGE_PS_IMPLEMENTATION_ITS`, a PS entry is stored in
   the ITS under the same UID that Zephyr's implementation uses for the create flags and replay
   protection value of the entry. After switching to
   :kconfig:option:`CONFIG_SECURE_STORAGE_PS_IMPLEMENTATION_ZEPHYR`, the data of a previously
   stored entry is interpreted as its create flags and replay protection value.

The create flags and the replay protection value of each entry are stored in the ITS, through the
same interface and under the same caller ID that
:kconfig:option:`CONFIG_SECURE_STORAGE_PS_IMPLEMENTATION_ITS` uses for whole PS entries.
The ITS interface is thus unchanged, and Zephyr's implementation of the PS API works on top of
both Zephyr's and a `custom <#whole-api>`_ implementation of the ITS API.
:kconfig:option:`CONFIG_SECURE_STORAGE_ITS_MAX_DATA_SIZE` must be at least one byte larger than
:kconfig:option:`CONFIG_SECURE_STORAGE_PS_REPLAY_PROTECTION_SIZE` to fit both.
The replay protection value is passed to the PS transform module, which binds it to the data
stored in PS, so that data replayed in PS fails to be read back. Thus, the PS data is only
protected against replay as much as the ITS data is.
By default (:kconfig:option:`CONFIG_SECURE_STORAGE_PS_REPLAY_PROTECTION_RANDOM`), a new random value
is generated each time the entry is written, so an old version of the entry can be replayed if its
value happens to repeat.
With :kconfig:option:`CONFIG_SECURE_STORAGE_PS_REPLAY_PROTECTION_COUNTER`, the value starts from a
random value when the entry is created and is incremented by one each time the entry is
overwritten, so that it never repeats during the lifetime of the entry.

With the AEAD transform
(:kconfig:option:`CONFIG_SECURE_STORAGE_PS_TRANSFORM_IMPLEMENTATION_AEAD`), the PS data is always
encrypted and authenticated, together with the UID and the replay protection value of the entry.
``PSA_STORAGE_FLAG_NO_CONFIDENTIALITY`` and ``PSA_STORAGE_FLAG_NO_REPLAY_PROTECTION`` are accepted
but have no effect. A custom transform
(:kconfig:option:`CONFIG_SECURE_STORAGE_PS_TRANSFORM_IMPLEMENTATION_CUSTOM`) only provides the
protection it implements, and must bind the replay protection value to the stored data for replay
protection to work.

The ZMS store module (:kconfig:option:`CONFIG_SECURE_STORAGE_PS_STORE_IMPLEMENTATION_ZMS`) needs
a ``zephyr,secure-storage-ps-partition`` devicetree chosen property that points to a partition
dedicated to the PS, separate from the one used by the ITS.

``PSA_ERROR_DATA_CORRUPT`` is returned when the PS data of an entry is missing from the storage
medium while its ITS data exists.
If the ITS data of an entry cannot be read back, the entry can be overwritten or removed even if
it was created with ``PSA_STORAGE_FLAG_WRITE_ONCE``, as its create flags are unknown.

The optional ``psa_ps_create()`` and ``psa_ps_set_extended()`` functions are not supported:
``psa_ps_get_support()`` returns 0 and they return ``PSA_ERROR_NOT_SUPPORTED``.

It's especially recommended to use or implement a secure :kconfig:option-regex:`encryption
key provider <CONFIG_SECURE_STORAGE_PS_TRANSFORM_AEAD_KEY_PROVIDER_.*>`.

Samples
*******

* :zephyr:code-sample:`persistent_key`
* :zephyr:code-sample:`psa_its`

PSA Secure Storage API reference
********************************

.. doxygengroup:: psa_secure_storage
