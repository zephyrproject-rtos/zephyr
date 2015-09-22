.. _crypto:

TinyCrypt Cryptographic Library v1.0
####################################
Copyright (C) 2015 by Intel Corporation, All Rights Reserved.

Overview
********
The TinyCrypt Library provides an implementation for constrained devices of a
minimal set of standard cryptography primitives, as listed below. TinyCrypt's
implementation differs in some aspects from the standard specifications for
better serving applications targeting constrained devices. See the Limitations
section for these differences. Note that some primitives depend on the
availability of other primitives.

* SHA-256:

  * Type of primitive: Hash function.
  * Standard Specification: NIST FIPS PUB 180-4.
  * Requires: --

* HMAC-SHA256:

  * Type of primitive: Message authentication code.
  * Standard Specification: RFC 2104.
  * Requires: SHA-256

* HMAC-PRNG:

  * Type of primitive: Pseudo-random number generator.
  * Standard Specification: NIST SP 800-90A.
  * Requires: SHA-256 and HMAC-SHA256.

* AES-128:

  * Type of primitive: Block cipher.
  * Standard Specification: NIST FIPS PUB 197.
  * Requires: --

* AES-CBC mode:

  * Type of primitive: Mode of operation.
  * Standard Specification: NIST SP 800-38A.
  * Requires: AES-128.

* AES-CTR mode:

  * Type of primitive: Mode of operation.
  * Standard Specification: NIST SP 800-38A.
  * Requires: AES-128.


Design Goals
************

* Minimize the code size of each primitive. This means minimize the size of
 the generic code. Various usages may require further features, optimizations
 and treatments for specific threats that would increase the overall code size.

* Minimize the dependencies among primitive implementations. This means that
 it is unnecessary to build and allocate object code for more primitives
 than the ones strictly required by the usage. In other words,
 in the Makefile you can select only the primitives that your application requires.


Limitations
***********

The TinyCrypt library has some known limitations. Some are inherent to
the cryptographic primitives; others are specific to TinyCrypt, to
meet the design goals (in special, minimal code size) and better serving
applications targeting constrained devices in general.

General Limitations
===================

* TinyCrypt does **not** intend to be fully side-channel resistant. There is a huge
  variety of side-channel attacks, many of them only relevant to certain
  platforms. In this sense, instead of penalizing all library users with
  side-channel countermeasures such as increasing the overall code size,
  TinyCrypt only implements certain generic timing-attack countermeasures.

Specific Limitations
====================

* SHA-256:

  * The state buffer 'leftover' stays in memory after processing. If your
    application intends to have sensitive data in this buffer, remember to
    erase it after the data has been processed.

  * The number of bits_hashed in the state is not checked for overflow.
    This will only be a problem if you intend to hash more than
    2^64 bits, which is an extremely large window.

* HMAC:

  * The HMAC state stays in memory after processing. If your application
    intends to have sensitive data in this buffer, remind to erase it after
    the data is processed.

  * The HMAC verification process is assumed to be performed by the application.
    In essence, this process compares the computed tag with some given tag.
    Note that memcmp methods might be vulnerable to timing attacks; be
    sure to use a safe memory comparison function for this purpose.

* HMAC-PRNG:

  * Before using HMAC-PRNG, you **must** find an entropy source to produce a seed.
    PRNGs only stretch the seed into a seemingly random output of fairly
    arbitrary length. The security of the output is exactly equal to the
    unpredictability of the seed.

  * During the initialization step, NIST SP 800-90A requires two items as seed
    material: entropy material and personalization material. A nonce material is optional.
    For achieving small code size, TinyCrypt only requires the personalization,
    which is always available to the user, and indirectly requires the entropy seed,
    which requires a mandatory call of the reseed function.

* AES-128:

  * The state stays in memory after processing. If your application intends to
    have sensitive data in this buffer, remember to erase it after the data is
    processed.

  * The current implementation does not support other key-lengths (such as 256 bits).
    If you need AES-256, it is likely that your application is running in a
    constrained environment. AES-256 requires keys twice the size as for AES-128,
    and the key schedule is 40% larger.

* CTR mode:

  * The AES-CTR mode limits the size of a data message they encrypt to 2^32 blocks.
    If you need to encrypt larger data sets, your application would
    need to replace the key after 2^32 block encryptions.

* CBC mode:

  * TinyCrypt CBC decryption assumes that the iv and the ciphertext are
    contiguous (as produced by TinyCrypt CBC encryption). This allows for a
    very efficient decryption algorithm that would not otherwise be possible.


Examples of Applications
************************
It is possible to do useful cryptography with only the given small set of primitives.
With this list of primitives it becomes feasible to support a range of cryptography usages:

 * Measurement of code, data structures, and other digital artifacts (SHA256)

 * Generate commitments (SHA256)

 * Construct keys (HMAC-SHA256)

 * Extract entropy from strings containing some randomness (HMAC-SHA256)

 * Construct random mappings (HMAC-SHA256)

 * Construct nonces and challenges (HMAC-PRNG)

 * Authenticate using a shared secret (HMAC-SHA256)

 * Create an authenticated, replay-protected session (HMAC-SHA256 + HMAC-PRNG)

 * Encrypt data and keys (AES-128 encrypt + AES-CTR + HMAC-SHA256)

 * Decrypt data and keys (AES-128 encrypt + AES-CTR + HMAC-SHA256)


Test Vectors
************

The library includes a test program for each primitive. The tests are available
in the :file:`samples/crypto/` folder. Each test illustrates how to use the corresponding
TinyCrypt primitives and also evaluates its correct behavior according to
well-known test-vectors (except for HMAC-PRNG). To evaluate the unpredictability
of the HMAC-PRNG, we suggest the NIST Statistical Test Suite. See References below.

References
**********

* `NIST FIPS PUB 180-4 (SHA-256)`_

.. _NIST FIPS PUB 180-4 (SHA-256):
   http://csrc.nist.gov/publications/fips/fips180-4/fips-180-4.pdf

* `NIST FIPS PUB 197 (AES-128)`_

.. _NIST FIPS PUB 197 (AES-128):
   http://csrc.nist.gov/publications/fips/fips197/fips-197.pdf

* `NIST SP800-90A (HMAC-PRNG)`_

.. _NIST SP800-90A (HMAC-PRNG):
   http://csrc.nist.gov/publications/nistpubs/800-90A/SP800-90A.pdf

* `NIST SP 800-38A (AES-CBC and AES-CTR)`_

.. _NIST SP 800-38A (AES-CBC and AES-CTR):
   http://csrc.nist.gov/publications/nistpubs/800-38a/sp800-38a.pdf

* `NIST Statistical Test Suite`_

.. _NIST Statistical Test Suite:
   http://csrc.nist.gov/groups/ST/toolkit/rng/documentation_software.html

* `RFC 2104 (HMAC-SHA256)`_

.. _RFC 2104 (HMAC-SHA256):
   https://www.ietf.org/rfc/rfc2104.txt
