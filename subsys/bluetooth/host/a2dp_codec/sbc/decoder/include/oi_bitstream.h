/******************************************************************************
 *
 *  Copyright 2014 The Android Open Source Project
 *  Copyright 2003 - 2004 Open Interface North America, Inc. All rights
 *                        reserved.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/
#ifndef _OI_BITSTREAM_H
#define _OI_BITSTREAM_H

/*******************************************************************************
  $Revision: #1 $
 ******************************************************************************/

/**
@file
Function prototypes and macro definitions for manipulating input and output
bitstreams.

@ingroup codec_internal
*/

/**
@addtogroup codec_internal
@{
*/

#include "oi_codec_sbc_private.h"
#include "oi_stddefs.h"

INLINE void OI_BITSTREAM_ReadInit(OI_BITSTREAM* bs, const OI_BYTE* buffer);

INLINE void OI_BITSTREAM_WriteInit(OI_BITSTREAM* bs, OI_BYTE* buffer);

INLINE uint32_t OI_BITSTREAM_ReadUINT(OI_BITSTREAM* bs, OI_UINT bits);

INLINE uint8_t OI_BITSTREAM_ReadUINT4Aligned(OI_BITSTREAM* bs);

INLINE uint8_t OI_BITSTREAM_ReadUINT8Aligned(OI_BITSTREAM* bs);

INLINE void OI_BITSTREAM_WriteUINT(OI_BITSTREAM* bs, uint16_t value,
                                   OI_UINT bits);

/*
 * Use knowledge that the bitstream is aligned to optimize the write of a byte
 */
PRIVATE void OI_BITSTREAM_WriteUINT8Aligned(OI_BITSTREAM* bs, uint8_t datum);

/*
 * Use knowledge that the bitstream is aligned to optimize the writing of a
 * pair of nibbles.
 */
PRIVATE void OI_BITSTREAM_Write2xUINT4Aligned(OI_BITSTREAM* bs, uint8_t datum1,
                                              uint8_t datum2);

/** Internally the bitstream looks ahead in the stream. When
 * OI_SBC_ReadScalefactors() goes to temporarily break the abstraction, it will
 * need to know where the "logical" pointer is in the stream.
 */
#define OI_BITSTREAM_GetWritePtr(bs) ((bs)->ptr.w - 3)
#define OI_BITSTREAM_GetReadPtr(bs) ((bs)->ptr.r - 3)

/** This is declared here as a macro because decoder.c breaks the bitsream
 * encapsulation for efficiency reasons.
 */
#define OI_BITSTREAM_READUINT(result, bits, ptr, value, bitPtr) \
  do {                                                          \
    OI_ASSERT((bits) <= 16);                                    \
    OI_ASSERT((bitPtr) < 32);                                   \
    OI_ASSERT((bitPtr) >= 0);                                   \
                                                                \
    while ((bitPtr + bits) > 32) {                              \
      (value) = ((value) << 8) | *(ptr)++;                      \
      (bitPtr) -= 8;                                            \
    }                                                           \
                                                                \
    (result) = (value) << (bitPtr);                             \
    (result) >>= 32 - (bits);                                   \
                                                                \
    (bitPtr) += (bits);                                         \
    OI_ASSERT(((bits) == 0) || ((result) < (1u << (bits))));    \
  } while (0)

#define OI_BITSTREAM_WRITEUINT(ptr, value, bitPtr, datum, bits) \
  do {                                                          \
    (bitPtr) -= (bits);                                         \
    (value) |= (datum) << (bitPtr);                             \
                                                                \
    while ((bitPtr) <= 16) {                                    \
      (bitPtr) += 8;                                            \
      *(ptr)++ = (uint8_t)((value) >> 24);                      \
      (value) <<= 8;                                            \
    }                                                           \
  } while (0)

#define OI_BITSTREAM_WRITEFLUSH(ptr, value, bitPtr) \
  do {                                              \
    while ((bitPtr) < 32) {                         \
      (bitPtr) += 8;                                \
      *(ptr)++ = (uint8_t)((value) >> 24);          \
      (value) <<= 8;                                \
    }                                               \
  } while (0)

/**
@}
*/

#endif /* _OI_BITSTREAM_H */