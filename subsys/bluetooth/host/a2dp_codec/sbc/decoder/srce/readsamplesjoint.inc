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

/*******************************************************************************
 * @file readsamplesjoint.inc
 *
 * This is the body of the generic version of OI_SBC_ReadSamplesJoint().
 * It is designed to be \#included into a function as follows:
    \code
    void OI_SBC_ReadSamplesJoint4(OI_CODEC_SBC_COMMON_CONTEXT *common, OI_BITSTREAM *global_bs)
    {
        #define NROF_SUBBANDS 4
        #include "readsamplesjoint.inc"
        #undef NROF_SUBBANDS
    }

    void OI_SBC_ReadSamplesJoint8(OI_CODEC_SBC_COMMON_CONTEXT *common, OI_BITSTREAM *global_bs)
    {
        #define NROF_SUBBANDS 8
        #include "readsamplesjoint.inc"
        #undef NROF_SUBBANDS
    }
    \endcode
 * Or to make a generic version:
    \code
    void OI_SBC_ReadSamplesJoint(OI_CODEC_SBC_COMMON_CONTEXT *common, OI_BITSTREAM *global_bs)
    {
        OI_UINT nrof_subbands = common->frameInfo.nrof_subbands;

        #define NROF_SUBBANDS nrof_subbands
        #include "readsamplesjoint.inc"
        #undef NROF_SUBBANDS
    }
    \endcode
 * @ingroup codec_internal
 ******************************************************************************/

/*******************************************************************************
  $Revision: #1 $
 ******************************************************************************/

{
    OI_CODEC_SBC_COMMON_CONTEXT *common = &context->common;
    OI_UINT bl = common->frameInfo.nrof_blocks;
    int32_t * RESTRICT s = common->subdata;
    uint8_t *ptr = global_bs->ptr.w;
    uint32_t value = global_bs->value;
    OI_UINT bitPtr = global_bs->bitPtr;
    uint8_t jmask = common->frameInfo.join << (8 - NROF_SUBBANDS);

    do {
        int8_t *sf_array = &common->scale_factor[0];
        uint8_t *bits_array = &common->bits.uint8[0];
        uint8_t joint = jmask;
        OI_UINT sb;
        /*
         * Left channel
         */
        sb = NROF_SUBBANDS;
        do {
            uint32_t raw;
            int32_t dequant;
            uint8_t bits = *bits_array++;
            OI_INT sf = *sf_array++;

            OI_BITSTREAM_READUINT(raw, bits, ptr, value, bitPtr);
            dequant = OI_SBC_Dequant(raw, sf, bits);
            *s++ = dequant;
        } while (--sb);
        /*
         * Right channel
         */
        sb = NROF_SUBBANDS;
        do {
            uint32_t raw;
            int32_t dequant;
            uint8_t bits = *bits_array++;
            OI_INT sf = *sf_array++;

            OI_BITSTREAM_READUINT(raw, bits, ptr, value, bitPtr);
            dequant = OI_SBC_Dequant(raw, sf, bits);
            /*
             * Check if we need to do mid/side
             */
            if (joint & 0x80) {
                int32_t mid = *(s - NROF_SUBBANDS);
                int32_t side = dequant;
                *(s - NROF_SUBBANDS) = mid + side;
                dequant = mid - side;
            }
            joint <<= 1;
            *s++ = dequant;
        } while (--sb);
    } while (--bl);
}