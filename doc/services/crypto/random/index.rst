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

   Never use the non-cryptographic random functions (:c:func:`sys_rand_get`,
   :c:func:`sys_rand8_get`, :c:func:`sys_rand16_get`, :c:func:`sys_rand32_get`,
   :c:func:`sys_rand64_get`) for security-sensitive operations. These functions
   do not provide cryptographically secure random numbers.




.. _random_kconfig:

Kconfig Options
***************

These options can be found in the following path :zephyr_file:`subsys/random/Kconfig`.

:kconfig:option:`CONFIG_TEST_RANDOM_GENERATOR`
 For testing, this option allows a non-random number generator to be used and
 permits random number APIs to return values that are not truly random.

The random number generator choice group allows selection of the RNG
source function for the system via the RNG_GENERATOR_CHOICE choice group.
An override of the default value can be specified in the SOC or board
.defconfig file by using:

.. code-block:: none

   choice RNG_GENERATOR_CHOICE
	   default XOSHIRO_RANDOM_GENERATOR
   endchoice

The random number generators available include:

:kconfig:option:`CONFIG_TIMER_RANDOM_GENERATOR`
 enables number generator based on system timer clock. This number
 generator is not random and used for testing only.

:kconfig:option:`CONFIG_ENTROPY_DEVICE_RANDOM_GENERATOR`
 enables a random number generator that uses the enabled hardware
 entropy gathering driver to generate random numbers.

:kconfig:option:`CONFIG_XOSHIRO_RANDOM_GENERATOR`
 enables the Xoshiro128++ pseudo-random number generator, that uses the
 entropy driver as a seed source.

The CSPRNG_GENERATOR_CHOICE choice group provides selection of the
cryptographically secure random number generator source function. An
override of the default value can be specified in the SOC or board
.defconfig file by using:

.. code-block:: none

   choice CSPRNG_GENERATOR_CHOICE
	   default CTR_DRBG_CSPRNG_GENERATOR
   endchoice

The cryptographically secure random number generators available include:

:kconfig:option:`CONFIG_HARDWARE_DEVICE_CS_GENERATOR`
 enables a cryptographically secure random number generator using the
 hardware random generator driver

:kconfig:option:`CONFIG_CTR_DRBG_CSPRNG_GENERATOR`
 enables the CTR-DRBG pseudo-random number generator. The CTR-DRBG is
 a FIPS140-2 recommended cryptographically secure random number generator.

Personalization data can be provided in addition to the entropy source
to make the initialization of the CTR-DRBG as unique as possible.

:kconfig:option:`CONFIG_CS_CTR_DRBG_PERSONALIZATION`
 CTR-DRBG Initialization Personalization string

API Reference
*************

.. doxygengroup:: random_api
