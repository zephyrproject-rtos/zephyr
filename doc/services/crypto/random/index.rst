.. _random_api:

Random Number Generation
########################

Overview
********

The random API subsystem provides random number generation APIs in both
cryptographically and non-cryptographically secure instances. Which
random API to use is based on the cryptographic requirements of the
random number. The non-cryptographic APIs return random values much
faster, so they should be preferred whenever cryptographically secure
values are not needed.

- Use :c:func:`sys_rand_get` and related functions for non-cryptographic
  purposes such as randomizing delays, shuffling data, or general-purpose
  randomization. These functions are faster but not suitable for security
  applications.

- Use :c:func:`sys_csrand_get` for cryptographic purposes such as generating
  encryption keys, nonces, initialization vectors, or any data that must be
  unpredictable to an attacker.

.. warning::

   Never use the non-cryptographic random functions (:c:func:`sys_rand_get`,
   :c:func:`sys_rand8_get`, :c:func:`sys_rand16_get`, :c:func:`sys_rand32_get`,
   :c:func:`sys_rand64_get`) for security-sensitive operations. These functions
   do not provide cryptographically secure random numbers.

The random API is declared in :zephyr_file:`include/zephyr/random/random.h`.
:c:func:`sys_rand_get` and :c:func:`sys_csrand_get` are system calls and can
be invoked from user mode threads when :kconfig:option:`CONFIG_USERSPACE` is
enabled.

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
 permits random number APIs to return values that are not truly random. It is
 intended to be selected by application-level configurations such as tests and
 samples, never in production.

 .. warning::

    This option is intended only for testing on platforms that lack hardware
    entropy support. The random numbers generated when this option is enabled
    are predictable and not suitable for any security-sensitive use.

Non-Cryptographic Random Generator Selection
============================================

The ``RNG_GENERATOR_CHOICE`` kconfig choice selects the generator backing
:c:func:`sys_rand_get`. To override the default in a board or SoC
configuration file:

.. code-block:: kconfig

   choice RNG_GENERATOR_CHOICE
	   default XOSHIRO_RANDOM_GENERATOR
   endchoice

Available generators:

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

:kconfig:option:`CONFIG_ARM64_RANDOM_GENERATOR`
   Uses the ``RNDR`` instruction from the ARM64 ``FEAT_RNG`` extension, on
   platforms that support it. The retry behavior on instruction failure can
   be tuned with :kconfig:option:`CONFIG_ARM64_RANDOM_GENERATOR_MAX_RETRIES`
   and :kconfig:option:`CONFIG_ARM64_RANDOM_GENERATOR_RETRY_WAIT_USEC`.

:kconfig:option:`CONFIG_TIMER_RANDOM_GENERATOR`
   Uses the system timer to generate pseudo-random numbers. This generator
   produces predictable values and is intended **only for testing** on
   platforms without hardware entropy support. Its initial state can be set
   with :kconfig:option:`CONFIG_TIMER_RANDOM_INITIAL_STATE`.

   Requires :kconfig:option:`CONFIG_TEST_RANDOM_GENERATOR` to be enabled.

Cryptographically Secure Random Generator Selection
===================================================

The ``CSPRNG_GENERATOR_CHOICE`` kconfig choice selects the source
for cryptographically secure random number generation.

To override the default in a board or SoC configuration file:

.. code-block:: kconfig

   choice CSPRNG_GENERATOR_CHOICE
	   default PSA_CSPRNG_GENERATOR
   endchoice

Available generators:

:kconfig:option:`CONFIG_HARDWARE_DEVICE_CS_GENERATOR`
 Uses the hardware entropy driver directly as the source for cryptographically
 secure random numbers. Select this when your hardware random number generator
 is certified or validated for cryptographic use.

:kconfig:option:`CONFIG_PSA_CSPRNG_GENERATOR`
 Uses the PSA Crypto API's secure random number generation, which may in turn
 use hardware entropy sources when available. It requires
 :kconfig:option:`CONFIG_PSA_CRYPTO`, i.e. a PSA Crypto provider such as
 Mbed TLS or TF-M must be part of the build.

:kconfig:option:`CONFIG_TEST_CSPRNG_GENERATOR`
 Routes calls to :c:func:`sys_csrand_get` through :c:func:`sys_rand_get`.
 This allows testing libraries that require CSPRNG on platforms without
 hardware entropy support.

 Requires :kconfig:option:`CONFIG_TEST_RANDOM_GENERATOR` to be enabled.

 .. warning::

    This option provides **no cryptographic security**. Use only for testing.

Applications that need cryptographically secure random numbers only when the
platform can actually provide them can enable
:kconfig:option:`CONFIG_CSPRNG_NEEDED` and then check
:kconfig:option:`CONFIG_CSPRNG_ENABLED`, which is set only when an entropy
driver suitable for cryptographically secure generation is part of the build.

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
platform has a hardware random number generator. See :ref:`entropy_api` for
the entropy driver API. If the platform has no entropy driver, only the test
generators are available, gated behind
:kconfig:option:`CONFIG_TEST_RANDOM_GENERATOR`.

API Reference
*************

.. doxygengroup:: random_api
