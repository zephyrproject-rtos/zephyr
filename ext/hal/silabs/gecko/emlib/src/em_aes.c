/***************************************************************************//**
 * @file em_aes.c
 * @brief Advanced Encryption Standard (AES) accelerator peripheral API.
 * @version 5.1.2
 *******************************************************************************
 * @section License
 * <b>Copyright 2016 Silicon Laboratories, Inc. http://www.silabs.com</b>
 *******************************************************************************
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 * DISCLAIMER OF WARRANTY/LIMITATION OF REMEDIES: Silicon Labs has no
 * obligation to support this Software. Silicon Labs is providing the
 * Software "AS IS", with no express or implied warranties of any kind,
 * including, but not limited to, any implied warranties of merchantability
 * or fitness for any particular purpose or warranties against infringement
 * of any proprietary rights of a third party.
 *
 * Silicon Labs will not be liable for any consequential, incidental, or
 * special damages, or any other relief, or for any claim by any third party,
 * arising from your use of this Software.
 *
 ******************************************************************************/

#include "em_aes.h"
#if defined(AES_COUNT) && (AES_COUNT > 0)

#include "em_assert.h"
/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup AES
 * @{
 ******************************************************************************/

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */

#define AES_BLOCKSIZE    16

/** @endcond */

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

/***************************************************************************//**
 * @brief
 *   Cipher-block chaining (CBC) cipher mode encryption/decryption, 128 bit key.
 *
 * @details
 *   Encryption:
 * @verbatim
 *           Plaintext                  Plaintext
 *               |                          |
 *               V                          V
 * InitVector ->XOR        +-------------->XOR
 *               |         |                |
 *               V         |                V
 *       +--------------+  |        +--------------+
 * Key ->| Block cipher |  |  Key ->| Block cipher |
 *       |  encryption  |  |        |  encryption  |
 *       +--------------+  |        +--------------+
 *               |---------+                |
 *               V                          V
 *           Ciphertext                 Ciphertext
 * @endverbatim
 *   Decryption:
 * @verbatim
 *         Ciphertext                 Ciphertext
 *              |----------+                |
 *              V          |                V
 *       +--------------+  |        +--------------+
 * Key ->| Block cipher |  |  Key ->| Block cipher |
 *       |  decryption  |  |        |  decryption  |
 *       +--------------+  |        +--------------+
 *               |         |                |
 *               V         |                V
 * InitVector ->XOR        +-------------->XOR
 *               |                          |
 *               V                          V
 *           Plaintext                  Plaintext
 * @endverbatim
 *   Please refer to general comments on layout and byte ordering of parameters.
 *
 * @param[out] out
 *   Buffer to place encrypted/decrypted data. Must be at least @p len long. It
 *   may be set equal to @p in, in which case the input buffer is overwritten.
 *
 * @param[in] in
 *   Buffer holding data to encrypt/decrypt. Must be at least @p len long.
 *
 * @param[in] len
 *   Number of bytes to encrypt/decrypt. Must be a multiple of 16.
 *
 * @param[in] key
 *   When doing encryption, this is the 128 bit encryption key. When doing
 *   decryption, this is the 128 bit decryption key. The decryption key may
 *   be generated from the encryption key with AES_DecryptKey128().
 *   On devices supporting key buffering this argument can be null, if so, the
 *   key will not be loaded, as it is assumed the key has been loaded
 *   into KEYHA previously.
 *
 * @param[in] iv
 *   128 bit initalization vector to use.
 *
 * @param[in] encrypt
 *   Set to true to encrypt, false to decrypt.
 ******************************************************************************/
void AES_CBC128(uint8_t *out,
                const uint8_t *in,
                unsigned int len,
                const uint8_t *key,
                const uint8_t *iv,
                bool encrypt)
{
  int            i;
  uint32_t       *_out = (uint32_t *)out;
  const uint32_t *_in  = (const uint32_t *)in;
  const uint32_t *_key = (const uint32_t *)key;
  const uint32_t *_iv  = (const uint32_t *)iv;
  /* Need to buffer one block when decrypting in case 'out' replaces 'in' */
  uint32_t       prev[4];

  EFM_ASSERT(!(len % AES_BLOCKSIZE));

  /* Number of blocks to process */
  len /= AES_BLOCKSIZE;

  #if defined( AES_CTRL_KEYBUFEN )
  if (key)
  {
    /* Load key into high key for key buffer usage */
    for (i = 3; i >= 0; i--)
    {
      AES->KEYHA = __REV(_key[i]);
    }
  }
  #endif

  if (encrypt)
  {
    /* Enable encryption with auto start using XOR */
    #if defined( AES_CTRL_KEYBUFEN )
    AES->CTRL = AES_CTRL_KEYBUFEN | AES_CTRL_XORSTART;
    #else
    AES->CTRL = AES_CTRL_XORSTART;
    #endif

    /* Load initialization vector, since writing to DATA, it will */
    /* not trigger encryption. */
    for (i = 3; i >= 0; i--)
    {
      AES->DATA = __REV(_iv[i]);
    }

    /* Encrypt data */
    while (len--)
    {
      #if !defined( AES_CTRL_KEYBUFEN )
      /* Load key */
      for (i = 3; i >= 0; i--)
      {
        AES->KEYLA = __REV(_key[i]);
      }
      #endif

      /* Load data and trigger encryption */
      for (i = 3; i >= 0; i--)
      {
        AES->XORDATA = __REV(_in[i]);
      }
      _in += 4;

      /* Wait for completion */
      while (AES->STATUS & AES_STATUS_RUNNING)
        ;

      /* Save encrypted data */
      for (i = 3; i >= 0; i--)
      {
        _out[i] = __REV(AES->DATA);
      }
      _out += 4;
    }
  }
  else
  {
    /* Select decryption mode */
    #if defined( AES_CTRL_KEYBUFEN )
    AES->CTRL = AES_CTRL_DECRYPT | AES_CTRL_KEYBUFEN | AES_CTRL_DATASTART;
    #else
    AES->CTRL = AES_CTRL_DECRYPT | AES_CTRL_DATASTART;
    #endif

    /* Copy init vector to previous buffer to avoid special handling */
    for (i = 0; i < 4; i++)
    {
      prev[i] = _iv[i];
    }

    /* Decrypt data */
    while (len--)
    {
      #if !defined( AES_CTRL_KEYBUFEN )
      /* Load key */
      for (i = 3; i >= 0; i--)
      {
        AES->KEYLA = __REV(_key[i]);
      }
      #endif

      /* Load data and trigger decryption */
      for (i = 3; i >= 0; i--)
      {
        AES->DATA = __REV(_in[i]);
      }

      /* Wait for completion */
      while (AES->STATUS & AES_STATUS_RUNNING)
        ;

      /* In order to avoid additional buffer, we use HW directly for XOR and buffer */
      /* (Writing to XORDATA will not trigger encoding, triggering enabled on DATA.) */
      for (i = 3; i >= 0; i--)
      {
        AES->XORDATA = __REV(prev[i]);
        prev[i]      = _in[i];
      }
      _in += 4;

      /* Then fetch decrypted data, we have to do it in a separate loop */
      /* due to internal auto-shifting of words */
      for (i = 3; i >= 0; i--)
      {
        _out[i] = __REV(AES->DATA);
      }
      _out += 4;
    }
  }
}


#if defined( AES_CTRL_AES256 )
/***************************************************************************//**
 * @brief
 *   Cipher-block chaining (CBC) cipher mode encryption/decryption, 256 bit key.
 *
 * @details
 *   Please see AES_CBC128() for CBC figure.
 *
 *   Please refer to general comments on layout and byte ordering of parameters.
 *
 * @param[out] out
 *   Buffer to place encrypted/decrypted data. Must be at least @p len long. It
 *   may be set equal to @p in, in which case the input buffer is overwritten.
 *
 * @param[in] in
 *   Buffer holding data to encrypt/decrypt. Must be at least @p len long.
 *
 * @param[in] len
 *   Number of bytes to encrypt/decrypt. Must be a multiple of 16.
 *
 * @param[in] key
 *   When doing encryption, this is the 256 bit encryption key. When doing
 *   decryption, this is the 256 bit decryption key. The decryption key may
 *   be generated from the encryption key with AES_DecryptKey256().
 *
 * @param[in] iv
 *   128 bit initalization vector to use.
 *
 * @param[in] encrypt
 *   Set to true to encrypt, false to decrypt.
 ******************************************************************************/
void AES_CBC256(uint8_t *out,
                const uint8_t *in,
                unsigned int len,
                const uint8_t *key,
                const uint8_t *iv,
                bool encrypt)
{
  int            i;
  int            j;
  uint32_t       *_out = (uint32_t *)out;
  const uint32_t *_in  = (const uint32_t *)in;
  const uint32_t *_key = (const uint32_t *)key;
  const uint32_t *_iv  = (const uint32_t *)iv;
  /* Need to buffer one block when decrypting in case output replaces input */
  uint32_t       prev[4];

  EFM_ASSERT(!(len % AES_BLOCKSIZE));

  /* Number of blocks to process */
  len /= AES_BLOCKSIZE;

  if (encrypt)
  {
    /* Enable encryption with auto start using XOR */
    AES->CTRL = AES_CTRL_AES256 | AES_CTRL_XORSTART;

    /* Load initialization vector, since writing to DATA, it will */
    /* not trigger encryption. */
    for (i = 3; i >= 0; i--)
    {
      AES->DATA = __REV(_iv[i]);
    }

    /* Encrypt data */
    while (len--)
    {
      /* Load key and data and trigger encryption */
      for (i = 3, j = 7; i >= 0; i--, j--)
      {
        AES->KEYLA = __REV(_key[j]);
        AES->KEYHA = __REV(_key[i]);
        /* Write data last, since will trigger encryption on last iteration */
        AES->XORDATA = __REV(_in[i]);
      }
      _in += 4;

      /* Wait for completion */
      while (AES->STATUS & AES_STATUS_RUNNING)
        ;

      /* Save encrypted data */
      for (i = 3; i >= 0; i--)
      {
        _out[i] = __REV(AES->DATA);
      }
      _out += 4;
    }
  }
  else
  {
    /* Select decryption mode */
    AES->CTRL = AES_CTRL_AES256 | AES_CTRL_DECRYPT | AES_CTRL_DATASTART;

    /* Copy init vector to previous buffer to avoid special handling */
    for (i = 0; i < 4; i++)
    {
      prev[i] = _iv[i];
    }

    /* Decrypt data */
    while (len--)
    {
      /* Load key and data and trigger decryption */
      for (i = 3, j = 7; i >= 0; i--, j--)
      {
        AES->KEYLA = __REV(_key[j]);
        AES->KEYHA = __REV(_key[i]);
        /* Write data last, since will trigger encryption on last iteration */
        AES->DATA = __REV(_in[i]);
      }

      /* Wait for completion */
      while (AES->STATUS & AES_STATUS_RUNNING)
        ;

      /* In order to avoid additional buffer, we use HW directly for XOR and buffer */
      for (i = 3; i >= 0; i--)
      {
        AES->XORDATA = __REV(prev[i]);
        prev[i]      = _in[i];
      }
      _in += 4;

      /* Then fetch decrypted data, we have to do it in a separate loop */
      /* due to internal auto-shifting of words */
      for (i = 3; i >= 0; i--)
      {
        _out[i] = __REV(AES->DATA);
      }
      _out += 4;
    }
  }
}
#endif


/***************************************************************************//**
 * @brief
 *   Cipher feedback (CFB) cipher mode encryption/decryption, 128 bit key.
 *
 * @details
 *   Encryption:
 * @verbatim
 *           InitVector    +----------------+
 *               |         |                |
 *               V         |                V
 *       +--------------+  |        +--------------+
 * Key ->| Block cipher |  |  Key ->| Block cipher |
 *       |  encryption  |  |        |  encryption  |
 *       +--------------+  |        +--------------+
 *               |         |                |
 *               V         |                V
 *  Plaintext ->XOR        |   Plaintext ->XOR
 *               |---------+                |
 *               V                          V
 *           Ciphertext                 Ciphertext
 * @endverbatim
 *   Decryption:
 * @verbatim
 *          InitVector     +----------------+
 *               |         |                |
 *               V         |                V
 *       +--------------+  |        +--------------+
 * Key ->| Block cipher |  |  Key ->| Block cipher |
 *       |  encryption  |  |        |  encryption  |
 *       +--------------+  |        +--------------+
 *               |         |                |
 *               V         |                V
 *              XOR<- Ciphertext           XOR<- Ciphertext
 *               |                          |
 *               V                          V
 *           Plaintext                  Plaintext
 * @endverbatim
 *   Please refer to general comments on layout and byte ordering of parameters.
 *
 * @param[out] out
 *   Buffer to place encrypted/decrypted data. Must be at least @p len long. It
 *   may be set equal to @p in, in which case the input buffer is overwritten.
 *
 * @param[in] in
 *   Buffer holding data to encrypt/decrypt. Must be at least @p len long.
 *
 * @param[in] len
 *   Number of bytes to encrypt/decrypt. Must be a multiple of 16.
 *
 * @param[in] key
 *   128 bit encryption key is used for both encryption and decryption modes.
 *
 * @param[in] iv
 *   128 bit initalization vector to use.
 *
 * @param[in] encrypt
 *   Set to true to encrypt, false to decrypt.
 ******************************************************************************/
void AES_CFB128(uint8_t *out,
                const uint8_t *in,
                unsigned int len,
                const uint8_t *key,
                const uint8_t *iv,
                bool encrypt)
{
  int            i;
  uint32_t       *_out = (uint32_t *)out;
  const uint32_t *_in  = (const uint32_t *)in;
  const uint32_t *_key = (const uint32_t *)key;
  const uint32_t *_iv  = (const uint32_t *)iv;
  const uint32_t *data;
  uint32_t       tmp[4];

  EFM_ASSERT(!(len % AES_BLOCKSIZE));

  #if defined( AES_CTRL_KEYBUFEN )
  AES->CTRL = AES_CTRL_KEYBUFEN | AES_CTRL_DATASTART;
  #else
  AES->CTRL = AES_CTRL_DATASTART;
  #endif

  #if defined( AES_CTRL_KEYBUFEN )
  /* Load key into high key for key buffer usage */
  for (i = 3; i >= 0; i--)
  {
    AES->KEYHA = __REV(_key[i]);
  }
  #endif

  /* Encrypt/decrypt data */
  data = _iv;
  len /= AES_BLOCKSIZE;
  while (len--)
  {
    #if !defined( AES_CTRL_KEYBUFEN )
    /* Load key */
    for (i = 3; i >= 0; i--)
    {
      AES->KEYLA = __REV(_key[i]);
    }
    #endif

    /* Load data and trigger encryption */
    for (i = 3; i >= 0; i--)
    {
      AES->DATA = __REV(data[i]);
    }

    /* Do some required processing before waiting for completion */
    if (encrypt)
    {
      data = _out;
    }
    else
    {
      /* Must copy current ciphertext block since it may be overwritten */
      for (i = 0; i < 4; i++)
      {
        tmp[i] = _in[i];
      }
      data = tmp;
    }

    /* Wait for completion */
    while (AES->STATUS & AES_STATUS_RUNNING)
      ;

    /* Save encrypted/decrypted data */
    for (i = 3; i >= 0; i--)
    {
      _out[i] = __REV(AES->DATA) ^ _in[i];
    }
    _out += 4;
    _in  += 4;
  }
}


#if defined( AES_CTRL_AES256 )
/***************************************************************************//**
 * @brief
 *   Cipher feedback (CFB) cipher mode encryption/decryption, 256 bit key.
 *
 * @details
 *   Please see AES_CFB128() for CFB figure.
 *
 *   Please refer to general comments on layout and byte ordering of parameters.
 *
 * @param[out] out
 *   Buffer to place encrypted/decrypted data. Must be at least @p len long. It
 *   may be set equal to @p in, in which case the input buffer is overwritten.
 *
 * @param[in] in
 *   Buffer holding data to encrypt/decrypt. Must be at least @p len long.
 *
 * @param[in] len
 *   Number of bytes to encrypt/decrypt. Must be a multiple of 16.
 *
 * @param[in] key
 *   256 bit encryption key is used for both encryption and decryption modes.
 *
 * @param[in] iv
 *   128 bit initalization vector to use.
 *
 * @param[in] encrypt
 *   Set to true to encrypt, false to decrypt.
 ******************************************************************************/
void AES_CFB256(uint8_t *out,
                const uint8_t *in,
                unsigned int len,
                const uint8_t *key,
                const uint8_t *iv,
                bool encrypt)
{
  int            i;
  int            j;
  uint32_t       *_out = (uint32_t *)out;
  const uint32_t *_in  = (const uint32_t *)in;
  const uint32_t *_key = (const uint32_t *)key;
  const uint32_t *_iv  = (const uint32_t *)iv;
  const uint32_t *data;
  uint32_t       tmp[4];

  EFM_ASSERT(!(len % AES_BLOCKSIZE));

  /* Select encryption mode */
  AES->CTRL = AES_CTRL_AES256 | AES_CTRL_DATASTART;

  /* Encrypt/decrypt data */
  data = _iv;
  len /= AES_BLOCKSIZE;
  while (len--)
  {
    /* Load key and block to be encrypted/decrypted */
    for (i = 3, j = 7; i >= 0; i--, j--)
    {
      AES->KEYLA = __REV(_key[j]);
      AES->KEYHA = __REV(_key[i]);
      /* Write data last, since will trigger encryption on last iteration */
      AES->DATA = __REV(data[i]);
    }

    /* Do some required processing before waiting for completion */
    if (encrypt)
    {
      data = _out;
    }
    else
    {
      /* Must copy current ciphertext block since it may be overwritten */
      for (i = 0; i < 4; i++)
      {
        tmp[i] = _in[i];
      }
      data = tmp;
    }

    while (AES->STATUS & AES_STATUS_RUNNING)
      ;

    /* Save encrypted/decrypted data */
    for (i = 3; i >= 0; i--)
    {
      _out[i] = __REV(AES->DATA) ^ _in[i];
    }
    _out += 4;
    _in  += 4;
  }
}
#endif


/***************************************************************************//**
 * @brief
 *   Counter (CTR) cipher mode encryption/decryption, 128 bit key.
 *
 * @details
 *   Encryption:
 * @verbatim
 *           Counter                    Counter
 *              |                          |
 *              V                          V
 *       +--------------+           +--------------+
 * Key ->| Block cipher |     Key ->| Block cipher |
 *       |  encryption  |           |  encryption  |
 *       +--------------+           +--------------+
 *              |                          |
 * Plaintext ->XOR            Plaintext ->XOR
 *              |                          |
 *              V                          V
 *         Ciphertext                 Ciphertext
 * @endverbatim
 *   Decryption:
 * @verbatim
 *           Counter                    Counter
 *              |                          |
 *              V                          V
 *       +--------------+           +--------------+
 * Key ->| Block cipher |     Key ->| Block cipher |
 *       |  encryption  |           |  encryption  |
 *       +--------------+           +--------------+
 *               |                          |
 * Ciphertext ->XOR           Ciphertext ->XOR
 *               |                          |
 *               V                          V
 *           Plaintext                  Plaintext
 * @endverbatim
 *   Please refer to general comments on layout and byte ordering of parameters.
 *
 * @param[out] out
 *   Buffer to place encrypted/decrypted data. Must be at least @p len long. It
 *   may be set equal to @p in, in which case the input buffer is overwritten.
 *
 * @param[in] in
 *   Buffer holding data to encrypt/decrypt. Must be at least @p len long.
 *
 * @param[in] len
 *   Number of bytes to encrypt/decrypt. Must be a multiple of 16.
 *
 * @param[in] key
 *   128 bit encryption key.
 *   On devices supporting key buffering this argument can be null, if so, the
 *   key will not be loaded, as it is assumed the key has been loaded
 *   into KEYHA previously.
 *
 * @param[in,out] ctr
 *   128 bit initial counter value. The counter is updated after each AES
 *   block encoding through use of @p ctrFunc.
 *
 * @param[in] ctrFunc
 *   Function used to update counter value.
 ******************************************************************************/
void AES_CTR128(uint8_t *out,
                const uint8_t *in,
                unsigned int len,
                const uint8_t *key,
                uint8_t *ctr,
                AES_CtrFuncPtr_TypeDef ctrFunc)
{
  int            i;
  uint32_t       *_out = (uint32_t *)out;
  const uint32_t *_in  = (const uint32_t *)in;
  const uint32_t *_key = (const uint32_t *)key;
  uint32_t       *_ctr = (uint32_t *)ctr;

  EFM_ASSERT(!(len % AES_BLOCKSIZE));
  EFM_ASSERT(ctrFunc);

  #if defined( AES_CTRL_KEYBUFEN )
  AES->CTRL = AES_CTRL_KEYBUFEN | AES_CTRL_DATASTART;
  #else
  AES->CTRL = AES_CTRL_DATASTART;
  #endif

  #if defined( AES_CTRL_KEYBUFEN )
  if (key)
  {
    /* Load key into high key for key buffer usage */
    for (i = 3; i >= 0; i--)
    {
      AES->KEYHA = __REV(_key[i]);
    }
  }
  #endif

  /* Encrypt/decrypt data */
  len /= AES_BLOCKSIZE;
  while (len--)
  {
    #if !defined( AES_CTRL_KEYBUFEN )
    /* Load key */
    for (i = 3; i >= 0; i--)
    {
      AES->KEYLA = __REV(_key[i]);
    }
    #endif

    /* Load ctr to be encrypted/decrypted */
    for (i = 3; i >= 0; i--)
    {
      AES->DATA = __REV(_ctr[i]);
    }
    /* Increment ctr for next use */
    ctrFunc(ctr);

    /* Wait for completion */
    while (AES->STATUS & AES_STATUS_RUNNING)
      ;

    /* Save encrypted/decrypted data */
    for (i = 3; i >= 0; i--)
    {
      _out[i] = __REV(AES->DATA) ^ _in[i];
    }
    _out += 4;
    _in  += 4;
  }
}


#if defined( AES_CTRL_AES256 )
/***************************************************************************//**
 * @brief
 *   Counter (CTR) cipher mode encryption/decryption, 256 bit key.
 *
 * @details
 *   Please see AES_CTR128() for CTR figure.
 *
 *   Please refer to general comments on layout and byte ordering of parameters.
 *
 * @param[out] out
 *   Buffer to place encrypted/decrypted data. Must be at least @p len long. It
 *   may be set equal to @p in, in which case the input buffer is overwritten.
 *
 * @param[in] in
 *   Buffer holding data to encrypt/decrypt. Must be at least @p len long.
 *
 * @param[in] len
 *   Number of bytes to encrypt/decrypt. Must be a multiple of 16.
 *
 * @param[in] key
 *   256 bit encryption key.
 *
 * @param[in,out] ctr
 *   128 bit initial counter value. The counter is updated after each AES
 *   block encoding through use of @p ctrFunc.
 *
 * @param[in] ctrFunc
 *   Function used to update counter value.
 ******************************************************************************/
void AES_CTR256(uint8_t *out,
                const uint8_t *in,
                unsigned int len,
                const uint8_t *key,
                uint8_t *ctr,
                AES_CtrFuncPtr_TypeDef ctrFunc)
{
  int            i;
  int            j;
  uint32_t       *_out = (uint32_t *)out;
  const uint32_t *_in  = (const uint32_t *)in;
  const uint32_t *_key = (const uint32_t *)key;
  uint32_t       *_ctr = (uint32_t *)ctr;

  EFM_ASSERT(!(len % AES_BLOCKSIZE));
  EFM_ASSERT(ctrFunc);

  /* Select encryption mode, with auto trigger */
  AES->CTRL = AES_CTRL_AES256 | AES_CTRL_DATASTART;

  /* Encrypt/decrypt data */
  len /= AES_BLOCKSIZE;
  while (len--)
  {
    /* Load key and block to be encrypted/decrypted */
    for (i = 3, j = 7; i >= 0; i--, j--)
    {
      AES->KEYLA = __REV(_key[j]);
      AES->KEYHA = __REV(_key[i]);
      /* Write data last, since will trigger encryption on last iteration */
      AES->DATA = __REV(_ctr[i]);
    }
    /* Increment ctr for next use */
    ctrFunc(ctr);

    /* Wait for completion */
    while (AES->STATUS & AES_STATUS_RUNNING)
      ;

    /* Save encrypted/decrypted data */
    for (i = 3; i >= 0; i--)
    {
      _out[i] = __REV(AES->DATA) ^ _in[i];
    }
    _out += 4;
    _in  += 4;
  }
}
#endif


/***************************************************************************//**
 * @brief
 *   Update last 32 bits of 128 bit counter, by incrementing with 1.
 *
 * @details
 *   Notice that no special consideration is given to possible wrap around. If
 *   32 least significant bits are 0xFFFFFFFF, they will be updated to 0x00000000,
 *   ignoring overflow.
 *
 *   Please refer to general comments on layout and byte ordering of parameters.
 *
 * @param[in,out] ctr
 *   Buffer holding 128 bit counter to be updated.
 ******************************************************************************/
void AES_CTRUpdate32Bit(uint8_t *ctr)
{
  uint32_t *_ctr = (uint32_t *)ctr;

  _ctr[3] = __REV(__REV(_ctr[3]) + 1);
}


/***************************************************************************//**
 * @brief
 *   Generate 128 bit decryption key from 128 bit encryption key. The decryption
 *   key is used for some cipher modes when decrypting.
 *
 * @details
 *   Please refer to general comments on layout and byte ordering of parameters.
 *
 * @param[out] out
 *   Buffer to place 128 bit decryption key. Must be at least 16 bytes long. It
 *   may be set equal to @p in, in which case the input buffer is overwritten.
 *
 * @param[in] in
 *   Buffer holding 128 bit encryption key. Must be at least 16 bytes long.
 ******************************************************************************/
void AES_DecryptKey128(uint8_t *out, const uint8_t *in)
{
  int            i;
  uint32_t       *_out = (uint32_t *)out;
  const uint32_t *_in  = (const uint32_t *)in;

  /* Load key */
  for (i = 3; i >= 0; i--)
  {
    AES->KEYLA = __REV(_in[i]);
  }

  /* Do dummy encryption to generate decrypt key */
  AES->CTRL = 0;
  AES_IntClear(AES_IF_DONE);
  AES->CMD = AES_CMD_START;

  /* Wait for completion */
  while (AES->STATUS & AES_STATUS_RUNNING)
    ;

  /* Save decryption key */
  for (i = 3; i >= 0; i--)
  {
    _out[i] = __REV(AES->KEYLA);
  }
}


#if defined( AES_CTRL_AES256 )
/***************************************************************************//**
 * @brief
 *   Generate 256 bit decryption key from 256 bit encryption key. The decryption
 *   key is used for some cipher modes when decrypting.
 *
 * @details
 *   Please refer to general comments on layout and byte ordering of parameters.
 *
 * @param[out] out
 *   Buffer to place 256 bit decryption key. Must be at least 32 bytes long. It
 *   may be set equal to @p in, in which case the input buffer is overwritten.
 *
 * @param[in] in
 *   Buffer holding 256 bit encryption key. Must be at least 32 bytes long.
 ******************************************************************************/
void AES_DecryptKey256(uint8_t *out, const uint8_t *in)
{
  int            i;
  int            j;
  uint32_t       *_out = (uint32_t *)out;
  const uint32_t *_in  = (const uint32_t *)in;

  /* Load key */
  for (i = 3, j = 7; i >= 0; i--, j--)
  {
    AES->KEYLA = __REV(_in[j]);
    AES->KEYHA = __REV(_in[i]);
  }

  /* Do dummy encryption to generate decrypt key */
  AES->CTRL = AES_CTRL_AES256;
  AES->CMD  = AES_CMD_START;

  /* Wait for completion */
  while (AES->STATUS & AES_STATUS_RUNNING)
    ;

  /* Save decryption key */
  for (i = 3, j = 7; i >= 0; i--, j--)
  {
    _out[j] = __REV(AES->KEYLA);
    _out[i] = __REV(AES->KEYHA);
  }
}
#endif


/***************************************************************************//**
 * @brief
 *   Electronic Codebook (ECB) cipher mode encryption/decryption, 128 bit key.
 *
 * @details
 *   Encryption:
 * @verbatim
 *          Plaintext                  Plaintext
 *              |                          |
 *              V                          V
 *       +--------------+           +--------------+
 * Key ->| Block cipher |     Key ->| Block cipher |
 *       |  encryption  |           |  encryption  |
 *       +--------------+           +--------------+
 *              |                          |
 *              V                          V
 *         Ciphertext                 Ciphertext
 * @endverbatim
 *   Decryption:
 * @verbatim
 *         Ciphertext                 Ciphertext
 *              |                          |
 *              V                          V
 *       +--------------+           +--------------+
 * Key ->| Block cipher |     Key ->| Block cipher |
 *       |  decryption  |           |  decryption  |
 *       +--------------+           +--------------+
 *              |                          |
 *              V                          V
 *          Plaintext                  Plaintext
 * @endverbatim
 *   Please refer to general comments on layout and byte ordering of parameters.
 *
 * @param[out] out
 *   Buffer to place encrypted/decrypted data. Must be at least @p len long. It
 *   may be set equal to @p in, in which case the input buffer is overwritten.
 *
 * @param[in] in
 *   Buffer holding data to encrypt/decrypt. Must be at least @p len long.
 *
 * @param[in] len
 *   Number of bytes to encrypt/decrypt. Must be a multiple of 16.
 *
 * @param[in] key
 *   When doing encryption, this is the 128 bit encryption key. When doing
 *   decryption, this is the 128 bit decryption key. The decryption key may
 *   be generated from the encryption key with AES_DecryptKey128().
 *
 * @param[in] encrypt
 *   Set to true to encrypt, false to decrypt.
 ******************************************************************************/
void AES_ECB128(uint8_t *out,
                const uint8_t *in,
                unsigned int len,
                const uint8_t *key,
                bool encrypt)
{
  int            i;
  uint32_t       *_out = (uint32_t *)out;
  const uint32_t *_in  = (const uint32_t *)in;
  const uint32_t *_key = (const uint32_t *)key;

  EFM_ASSERT(!(len % AES_BLOCKSIZE));

  #if defined( AES_CTRL_KEYBUFEN )
  /* Load key into high key for key buffer usage */
  for (i = 3; i >= 0; i--)
  {
    AES->KEYHA = __REV(_key[i]);
  }
  #endif

  if (encrypt)
  {
    /* Select encryption mode */
    #if defined( AES_CTRL_KEYBUFEN )
    AES->CTRL = AES_CTRL_KEYBUFEN | AES_CTRL_DATASTART;
    #else
    AES->CTRL = AES_CTRL_DATASTART;
    #endif
  }
  else
  {
    /* Select decryption mode */
    #if defined( AES_CTRL_KEYBUFEN )
    AES->CTRL = AES_CTRL_DECRYPT | AES_CTRL_KEYBUFEN | AES_CTRL_DATASTART;
    #else
    AES->CTRL = AES_CTRL_DECRYPT | AES_CTRL_DATASTART;
    #endif
  }

  /* Encrypt/decrypt data */
  len /= AES_BLOCKSIZE;
  while (len--)
  {
    #if !defined( AES_CTRL_KEYBUFEN )
    /* Load key */
    for (i = 3; i >= 0; i--)
    {
      AES->KEYLA = __REV(_key[i]);
    }
    #endif

    /* Load block to be encrypted/decrypted */
    for (i = 3; i >= 0; i--)
    {
      AES->DATA = __REV(_in[i]);
    }
    _in += 4;

    /* Wait for completion */
    while (AES->STATUS & AES_STATUS_RUNNING)
      ;

    /* Save encrypted/decrypted data */
    for (i = 3; i >= 0; i--)
    {
      _out[i] = __REV(AES->DATA);
    }
    _out += 4;
  }
}


#if defined( AES_CTRL_AES256 )
/***************************************************************************//**
 * @brief
 *   Electronic Codebook (ECB) cipher mode encryption/decryption, 256 bit key.
 *
 * @details
 *   Please see AES_ECB128() for ECB figure.
 *
 *   Please refer to general comments on layout and byte ordering of parameters.
 *
 * @param[out] out
 *   Buffer to place encrypted/decrypted data. Must be at least @p len long. It
 *   may be set equal to @p in, in which case the input buffer is overwritten.
 *
 * @param[in] in
 *   Buffer holding data to encrypt/decrypt. Must be at least @p len long.
 *
 * @param[in] len
 *   Number of bytes to encrypt/decrypt. Must be a multiple of 16.
 *
 * @param[in] key
 *   When doing encryption, this is the 256 bit encryption key. When doing
 *   decryption, this is the 256 bit decryption key. The decryption key may
 *   be generated from the encryption key with AES_DecryptKey256().
 *
 * @param[in] encrypt
 *   Set to true to encrypt, false to decrypt.
 ******************************************************************************/
void AES_ECB256(uint8_t *out,
                const uint8_t *in,
                unsigned int len,
                const uint8_t *key,
                bool encrypt)
{
  int            i;
  int            j;
  uint32_t       *_out = (uint32_t *)out;
  const uint32_t *_in  = (const uint32_t *)in;
  const uint32_t *_key = (const uint32_t *)key;

  EFM_ASSERT(!(len % AES_BLOCKSIZE));

  if (encrypt)
  {
    /* Select encryption mode */
    AES->CTRL = AES_CTRL_AES256 | AES_CTRL_DATASTART;
  }
  else
  {
    /* Select decryption mode */
    AES->CTRL = AES_CTRL_DECRYPT | AES_CTRL_AES256 | AES_CTRL_DATASTART;
  }

  /* Encrypt/decrypt data */
  len /= AES_BLOCKSIZE;
  while (len--)
  {
    /* Load key and block to be encrypted/decrypted */
    for (i = 3, j = 7; i >= 0; i--, j--)
    {
      AES->KEYLA = __REV(_key[j]);
      AES->KEYHA = __REV(_key[i]);
      /* Write data last, since will trigger encryption on last iteration */
      AES->DATA = __REV(_in[i]);
    }
    _in += 4;

    /* Wait for completion */
    while (AES->STATUS & AES_STATUS_RUNNING)
      ;

    /* Save encrypted/decrypted data */
    for (i = 3; i >= 0; i--)
    {
      _out[i] = __REV(AES->DATA);
    }
    _out += 4;
  }
}
#endif


/***************************************************************************//**
 * @brief
 *   Output feedback (OFB) cipher mode encryption/decryption, 128 bit key.
 *
 * @details
 *   Encryption:
 * @verbatim
 *          InitVector    +----------------+
 *              |         |                |
 *              V         |                V
 *       +--------------+ |        +--------------+
 * Key ->| Block cipher | |  Key ->| Block cipher |
 *       |  encryption  | |        |  encryption  |
 *       +--------------+ |        +--------------+
 *              |         |                |
 *              |---------+                |
 *              V                          V
 * Plaintext ->XOR            Plaintext ->XOR
 *              |                          |
 *              V                          V
 *         Ciphertext                 Ciphertext
 * @endverbatim
 *   Decryption:
 * @verbatim
 *          InitVector    +----------------+
 *              |         |                |
 *              V         |                V
 *       +--------------+ |        +--------------+
 * Key ->| Block cipher | |  Key ->| Block cipher |
 *       |  encryption  | |        |  encryption  |
 *       +--------------+ |        +--------------+
 *              |         |                |
 *              |---------+                |
 *              V                          V
 * Ciphertext ->XOR           Ciphertext ->XOR
 *              |                          |
 *              V                          V
 *          Plaintext                  Plaintext
 * @endverbatim
 *   Please refer to general comments on layout and byte ordering of parameters.
 *
 * @param[out] out
 *   Buffer to place encrypted/decrypted data. Must be at least @p len long. It
 *   may be set equal to @p in, in which case the input buffer is overwritten.
 *
 * @param[in] in
 *   Buffer holding data to encrypt/decrypt. Must be at least @p len long.
 *
 * @param[in] len
 *   Number of bytes to encrypt/decrypt. Must be a multiple of 16.
 *
 * @param[in] key
 *   128 bit encryption key.
 *
 * @param[in] iv
 *   128 bit initalization vector to use.
 ******************************************************************************/
void AES_OFB128(uint8_t *out,
                const uint8_t *in,
                unsigned int len,
                const uint8_t *key,
                const uint8_t *iv)
{
  int            i;
  uint32_t       *_out = (uint32_t *)out;
  const uint32_t *_in  = (const uint32_t *)in;
  const uint32_t *_key = (const uint32_t *)key;
  const uint32_t *_iv  = (const uint32_t *)iv;

  EFM_ASSERT(!(len % AES_BLOCKSIZE));

  /* Select encryption mode, trigger explicitly by command */
  #if defined( AES_CTRL_KEYBUFEN )
  AES->CTRL = AES_CTRL_KEYBUFEN;
  #else
  AES->CTRL = 0;
  #endif

  /* Load key into high key for key buffer usage */
  /* Load initialization vector */
  for (i = 3; i >= 0; i--)
  {
    #if defined( AES_CTRL_KEYBUFEN )
    AES->KEYHA = __REV(_key[i]);
    #endif
    AES->DATA  = __REV(_iv[i]);
  }

  /* Encrypt/decrypt data */
  len /= AES_BLOCKSIZE;
  while (len--)
  {
    #if !defined( AES_CTRL_KEYBUFEN )
    /* Load key */
    for (i = 3; i >= 0; i--)
    {
      AES->KEYLA = __REV(_key[i]);
    }
    #endif

    AES->CMD = AES_CMD_START;

    /* Wait for completion */
    while (AES->STATUS & AES_STATUS_RUNNING)
      ;

    /* Save encrypted/decrypted data */
    for (i = 3; i >= 0; i--)
    {
      _out[i] = __REV(AES->DATA) ^ _in[i];
    }
    _out += 4;
    _in  += 4;
  }
}


#if defined( AES_CTRL_AES256 )
/***************************************************************************//**
 * @brief
 *   Output feedback (OFB) cipher mode encryption/decryption, 256 bit key.
 *
 * @details
 *   Please see AES_OFB128() for OFB figure.
 *
 *   Please refer to general comments on layout and byte ordering of parameters.
 *
 * @param[out] out
 *   Buffer to place encrypted/decrypted data. Must be at least @p len long. It
 *   may be set equal to @p in, in which case the input buffer is overwritten.
 *
 * @param[in] in
 *   Buffer holding data to encrypt/decrypt. Must be at least @p len long.
 *
 * @param[in] len
 *   Number of bytes to encrypt/decrypt. Must be a multiple of 16.
 *
 * @param[in] key
 *   256 bit encryption key.
 *
 * @param[in] iv
 *   128 bit initalization vector to use.
 ******************************************************************************/
void AES_OFB256(uint8_t *out,
                const uint8_t *in,
                unsigned int len,
                const uint8_t *key,
                const uint8_t *iv)
{
  int            i;
  int            j;
  uint32_t       *_out = (uint32_t *)out;
  const uint32_t *_in  = (const uint32_t *)in;
  const uint32_t *_key = (const uint32_t *)key;
  const uint32_t *_iv  = (const uint32_t *)iv;

  EFM_ASSERT(!(len % AES_BLOCKSIZE));

  /* Select encryption mode, trigger explicitly by command */
  AES->CTRL = AES_CTRL_AES256;

  /* Load initialization vector */
  for (i = 3; i >= 0; i--)
  {
    AES->DATA = __REV(_iv[i]);
  }

  /* Encrypt/decrypt data */
  len /= AES_BLOCKSIZE;
  while (len--)
  {
    /* Load key */
    for (i = 3, j = 7; i >= 0; i--, j--)
    {
      AES->KEYLA = __REV(_key[j]);
      AES->KEYHA = __REV(_key[i]);
    }

    AES->CMD = AES_CMD_START;

    /* Wait for completion */
    while (AES->STATUS & AES_STATUS_RUNNING)
      ;

    /* Save encrypted/decrypted data */
    for (i = 3; i >= 0; i--)
    {
      _out[i] = __REV(AES->DATA) ^ _in[i];
    }
    _out += 4;
    _in  += 4;
  }
}
#endif


/** @} (end addtogroup AES) */
/** @} (end addtogroup emlib) */
#endif /* defined(AES_COUNT) && (AES_COUNT > 0) */
