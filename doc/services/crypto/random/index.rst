.. _random_api:

Random Number Generation
########################

The random API subsystem provides random number generation APIs in both
cryptographically and non-cryptographically secure instances. Which
random API to use is based on the cryptographic requirements of the
random number. The non-cryptographic APIs will return random values
much faster if non-cryptographic values are needed.

The cryptographically secure random functions shall be compliant to the
FIPS 140-2 [NIST02]_ recommended algorithms. Hardware based random-number
generators (RNG) can be used on platforms with appropriate hardware support.
Platforms without hardware RNG support shall use cryptographically secure
random number generation through the `PSA Crypto API
<https://arm-software.github.io/psa-api/crypto/>`_.

  .. note::

    Cryptographically secure random number generators need an entropy source
    to establish and maintain the cryptographic security of the PRNG. The PSA
    Crypto API leverages the underlying entropy sources available on the platform,
    which may include hardware entropy sources when available.

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

   choice CSPRNG_GENERATOR_CHOICE
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
	   default PSA_CSPRNG_GENERATOR
   endchoice

The cryptographically secure random number generators available include:

:kconfig:option:`CONFIG_HARDWARE_DEVICE_CS_GENERATOR`
 enables a cryptographically secure random number generator using the
 hardware random generator driver

:kconfig:option:`CONFIG_CTR_DRBG_CSPRNG_GENERATOR`
 enables a cryptographically secure random number generator that uses the
 PSA Crypto API. This option is deprecated and will be removed in a future
 release. New applications should use :kconfig:option:`CONFIG_PSA_CSPRNG_GENERATOR`
 instead.

:kconfig:option:`CONFIG_PSA_CSPRNG_GENERATOR`
 enables a cryptographically secure random number generator that uses the
 PSA Crypto API. This CSPRNG implementation leverages the underlying PSA Crypto
 library's secure random number generation capabilities, which may use hardware
 entropy sources when available. The PSA CSPRNG provides cryptographically secure
 random numbers suitable for security-sensitive applications.

API Reference
*************

.. doxygengroup:: random_api
