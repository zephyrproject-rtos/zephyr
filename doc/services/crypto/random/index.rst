.. _random_api:

Random Number Generation
########################

Overview
********

The random API subsystem provides random number generation APIs in both
cryptographically and non-cryptographically secure instances. Which
random API to use is based on the cryptographic requirements of the
random number. The non-cryptographic APIs will return random values
much faster if non-cryptographic values are needed.

- Use :c:func:`sys_rand_get` and related functions for non-cryptographic
  purposes such as randomizing delays, shuffling data, or general-purpose
  randomization. These functions are faster but not suitable for security
  applications.

- Use :c:func:`sys_csrand_get` for cryptographic purposes such as generating
  encryption keys, nonces, initialization vectors, or any data that must be
  unpredictable to an attacker.

.. warning::

   Never use the non-cryptographic random functions (``sys_rand_get``,
   ``sys_rand8_get``, ``sys_rand16_get``, ``sys_rand32_get``, ``sys_rand64_get``)
   for security-sensitive operations. These functions do not provide
   cryptographically secure random numbers.

API Usage
*********

Non-Cryptographic Random Numbers
================================

The following functions provide non-cryptographically secure random numbers:

- :c:func:`sys_rand_get` - Fill a buffer with random bytes
- :c:func:`sys_rand8_get` - Get a random 8-bit value
- :c:func:`sys_rand16_get` - Get a random 16-bit value
- :c:func:`sys_rand32_get` - Get a random 32-bit value
- :c:func:`sys_rand64_get` - Get a random 64-bit value

Example usage:

.. code-block:: c

   #include <zephyr/random/random.h>

   void example_random_usage(void)
   {
       uint8_t buffer[16];
       uint32_t random_value;

       /* Fill buffer with random bytes */
       sys_rand_get(buffer, sizeof(buffer));

       /* Get a single random 32-bit value */
       random_value = sys_rand32_get();
   }

Cryptographically Secure Random Numbers
=======================================

For cryptographic purposes, use :c:func:`sys_csrand_get`:

.. code-block:: c

   #include <zephyr/random/random.h>

   int generate_encryption_key(uint8_t *key, size_t key_len)
   {
       int ret;

       ret = sys_csrand_get(key, key_len);
       if (ret != 0) {
           /* Handle error - entropy source may have failed */
           return ret;
       }

       return 0;
   }

.. note::

   :c:func:`sys_csrand_get` returns 0 on success, or ``-EIO`` if the entropy
   source fails. Always check the return value when generating cryptographic
   material.

.. _random_kconfig:

Kconfig Options
***************

All configuration options can be found in :zephyr_file:`subsys/random/Kconfig`.

General Options
===============

:kconfig:option:`CONFIG_TEST_RANDOM_GENERATOR`
 For testing, this option allows a non-random number generator to be used and
 permits random number APIs to return values that are not truly random.

 .. warning::

    This option is intended only for testing on platforms that lack hardware
    entropy support. The random numbers generated when this option is enabled
    are predictable and not suitable for any security-sensitive use.

Non-Cryptographic Random Generator Selection
============================================

:kconfig:option:`CONFIG_ENTROPY_DEVICE_RANDOM_GENERATOR`
   Uses the hardware entropy driver directly to generate random numbers.
   This provides high-quality random numbers but may be slower than
   pseudo-random alternatives. Select this option when your hardware entropy
   source has sufficient performance for your application's needs.

:kconfig:option:`CONFIG_XOSHIRO_RANDOM_GENERATOR`
   Uses the Xoshiro128++ pseudo-random number generator, seeded from the
   hardware entropy source. This is a fast, general-purpose PRNG with a
   128-bit state that passes all standard randomness tests.

   This is the recommended choice for most applications that need fast
   non-cryptographic random numbers.

:kconfig:option:`CONFIG_TIMER_RANDOM_GENERATOR`
   Uses the system timer to generate pseudo-random numbers. This generator
   produces predictable values and is intended **only for testing** on
   platforms without hardware entropy support.

   Requires :kconfig:option:`CONFIG_TEST_RANDOM_GENERATOR` to be enabled.

Cryptographically Secure Random Generator Selection
===================================================

The ``CSPRNG_GENERATOR_CHOICE`` Kconfig choice group selects the source for
cryptographically secure random number generation.

To override the default in a board or SoC configuration file:

.. code-block:: kconfig

   choice CSPRNG_GENERATOR_CHOICE
       default CTR_DRBG_CSPRNG_GENERATOR
   endchoice

Available generators:

:kconfig:option:`CONFIG_HARDWARE_DEVICE_CS_GENERATOR`
 Uses the hardware entropy driver directly as the source for cryptographically
 secure random numbers. Select this when your hardware random number generator
 is certified or validated for cryptographic use.

:kconfig:option:`CONFIG_CTR_DRBG_CSPRNG_GENERATOR`
 Uses the CTR-DRBG (Counter mode Deterministic Random Bit Generator)
 algorithm as specified in `NIST SP 800-90A
 <https://nvlpubs.nist.gov/nistpubs/SpecialPublications/NIST.SP.800-90Ar1.pdf>`_.
 This is a FIPS 140-2 recommended cryptographically secure random number
 generator.

:kconfig:option:`CONFIG_TEST_CSPRNG_GENERATOR`
 Routes calls to :c:func:`sys_csrand_get` through :c:func:`sys_rand_get`.
 This allows testing libraries that require CSPRNG on platforms without
 hardware entropy support.

 Requires :kconfig:option:`CONFIG_TEST_RANDOM_GENERATOR` to be enabled.

 .. warning::

    This option provides **no cryptographic security**. Use only for testing.

CTR-DRBG Personalization
------------------------

:kconfig:option:`CONFIG_CS_CTR_DRBG_PERSONALIZATION`
 A personalization string used during CTR-DRBG initialization. This string
 is mixed with the entropy source during seeding to make the DRBG state
 as unique as possible.

 Consider customizing this value for your application to add additional
 uniqueness to the DRBG initialization.

Helper Configuration Options
============================

Devicetree Configuration
************************

The random subsystem uses the ``zephyr,entropy`` chosen node to identify
the hardware entropy device:

.. code-block:: devicetree

   / {
       chosen {
           zephyr,entropy = &rng;
       };
   };

Ensure your board's devicetree correctly specifies this chosen node if your
platform has a hardware random number generator.

API Reference
*************

.. doxygengroup:: random_api
