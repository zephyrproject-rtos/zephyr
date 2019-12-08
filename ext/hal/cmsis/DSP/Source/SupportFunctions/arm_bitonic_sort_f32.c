/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_bitonic_sort_f32.c
 * Description:  Floating point bitonic sort
 *
 * $Date:        2019
 * $Revision:    V1.6.0
 *
 * Target Processor: Cortex-M and Cortex-A cores
 * -------------------------------------------------------------------- */
/*
 * Copyright (C) 2010-2019 ARM Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "arm_math.h"
#include "arm_sorting.h"


#if !defined(ARM_MATH_NEON)

static void arm_bitonic_sort_core_f32(float32_t *pSrc, uint32_t n, uint8_t dir)
{
    uint32_t step;
    uint32_t k, j;
    float32_t *leftPtr, *rightPtr;
    float32_t temp;

    step = n>>1;
    leftPtr = pSrc;
    rightPtr = pSrc+n-1;

    for(k=0; k<step; k++)
    {
	if(dir == (*leftPtr > *rightPtr))
	{
            // Swap
	    temp=*leftPtr;
	    *leftPtr=*rightPtr;
	    *rightPtr=temp;
	}

	leftPtr++;  // Move right
	rightPtr--; // Move left
    }

    // Merge
    for(step=(n>>2); step>0; step/=2)
    {
	for(j=0; j<n; j=j+step*2)
	{
	    leftPtr  = pSrc+j;
	    rightPtr = pSrc+j+step;

	    for(k=0; k<step; k++)
	    {
		if(*leftPtr > *rightPtr)
		{
		    // Swap
	    	    temp=*leftPtr;
		    *leftPtr=*rightPtr;
		    *rightPtr=temp;
		}

		leftPtr++;
		rightPtr++;
	    }
	}
    }
}
#endif

#if defined(ARM_MATH_NEON)


static float32x4x2_t arm_bitonic_resort_8_f32(float32x4_t a, float32x4_t b, uint8_t dir)
{
    /* Start with two vectors:
     * +---+---+---+---+
     * | a | b | c | d |
     * +---+---+---+---+
     * +---+---+---+---+
     * | e | f | g | h |
     * +---+---+---+---+
     * All the elements of the first are guaranteed to be less than or equal to
     * all of the elements in the second, and both vectors are bitonic.
     * We need to perform these operations to completely sort both lists:
     * vminmax([abcd],[efgh]) 
     * vminmax([acbd],[egfh]) 
     */
    vtrn128_64q(a, b);
    /* +---+---+---+---+
     * | a | b | e | f |
     * +---+---+---+---+
     * +---+---+---+---+
     * | c | d | g | h |
     * +---+---+---+---+
     */
    if(dir)
        vminmaxq(a, b);
    else
        vminmaxq(b, a);
    
    vtrn128_32q(a, b);
    /* +---+---+---+---+
     * | a | c | e | g |
     * +---+---+---+---+
     * +---+---+---+---+
     * | b | d | f | h |
     * +---+---+---+---+
     */
    if(dir)
        vminmaxq(a, b);
    else
        vminmaxq(b, a);
    
    return vzipq_f32(a, b);
}


static float32x4x2_t arm_bitonic_merge_8_f32(float32x4_t a, float32x4_t b, uint8_t dir)
{
    /* a and b are guaranteed to be bitonic */
    // Reverse the element of the second vector
    b = vrev128q_f32(b);

    // Compare the two vectors
    if(dir)
        vminmaxq(a, b);
    else
    vminmaxq(b, a);

    // Merge the two vectors
    float32x4x2_t ab = arm_bitonic_resort_8_f32(a, b, dir);

    return ab;
}

static void arm_bitonic_resort_16_f32(float32_t * pOut, float32x4x2_t a, float32x4x2_t b, uint8_t dir)
{
    /* Start with two vectors:
     * +---+---+---+---+---+---+---+---+
     * | a | b | c | d | e | f | g | h |
     * +---+---+---+---+---+---+---+---+
     * +---+---+---+---+---+---+---+---+
     * | i | j | k | l | m | n | o | p |
     * +---+---+---+---+---+---+---+---+
     * All the elements of the first are guaranteed to be less than or equal to
     * all of the elements in the second, and both vectors are bitonic.
     * We need to perform these operations to completely sort both lists:
     * vminmax([abcd],[efgh]) vminmax([ijkl],[mnop])
     * vminmax([abef],[cdgh]) vminmax([ijmn],[klop])
     * vminmax([acef],[bdfh]) vminmax([ikmo],[jlmp])
     */

    vtrn256_128q(a, b);
    /* +---+---+---+---+---+---+---+---+
     * | a | b | c | d | i | j | k | l |
     * +---+---+---+---+---+---+---+---+
     * +---+---+---+---+---+---+---+---+
     * | e | f | g | h | m | n | o | p |
     * +---+---+---+---+---+---+---+---+
     */
    if(dir)
        vminmax256q(a, b);
    else
        vminmax256q(b, a);
    
    vtrn256_64q(a, b);
    
    /* +---+---+---+---+---+---+---+---+
     * | a | b | e | f | i | j | m | n |
     * +---+---+---+---+---+---+---+---+
     * +---+---+---+---+---+---+---+---+
     * | c | d | g | h | k | l | o | p |
     * +---+---+---+---+---+---+---+---+
     */
    if(dir)
        vminmax256q(a, b);
    else
        vminmax256q(b, a);
    
    vtrn256_32q(a, b);
    /* We now have:
     * +---+---+---+---+---+---+---+---+
     * | a | c | e | g | i | k | m | o |
     * +---+---+---+---+---+---+---+---+
     * +---+---+---+---+---+---+---+---+
     * | b | d | f | h | j | l | n | p |
     * +---+---+---+---+---+---+---+---+
     */
    if(dir)
        vminmax256q(a, b);
    else
        vminmax256q(b, a);
    
    float32x4x2_t out1 = vzipq_f32(a.val[0], b.val[0]);
    float32x4x2_t out2 = vzipq_f32(a.val[1], b.val[1]);
    
    vst1q_f32(pOut, out1.val[0]);
    vst1q_f32(pOut+4, out1.val[1]);
    vst1q_f32(pOut+8, out2.val[0]);
    vst1q_f32(pOut+12, out2.val[1]);
} 

static void arm_bitonic_merge_16_f32(float32_t * pOut, float32x4x2_t a, float32x4x2_t b, uint8_t dir)
{
    // Merge two preordered float32x4x2_t
    vrev256q_f32(b);

    if(dir)
        vminmax256q(a, b);
    else
        vminmax256q(b, a);

    arm_bitonic_resort_16_f32(pOut, a, b, dir);
}

static void arm_bitonic_sort_16_f32(float32_t *pSrc, float32_t *pDst, uint8_t dir)
{
    float32x4_t a;
    float32x4_t b;
    float32x4_t c;
    float32x4_t d;

    // Load 16 samples
    a = vld1q_f32(pSrc);
    b = vld1q_f32(pSrc+4);
    c = vld1q_f32(pSrc+8); 
    d = vld1q_f32(pSrc+12);
    
    // Bitonic sorting network for 4 samples x 4 times
    if(dir)
    {
        vminmaxq(a, b);
        vminmaxq(c, d);
        
        vminmaxq(a, d);
        vminmaxq(b, c);
        
        vminmaxq(a, b);
        vminmaxq(c, d);
    }
    else
    {
        vminmaxq(b, a);
        vminmaxq(d, c);
        
        vminmaxq(d, a);
        vminmaxq(c, b);
        
        vminmaxq(b, a);
        vminmaxq(d, c);
    }

    float32x4x2_t ab = vtrnq_f32 (a, b);
    float32x4x2_t cd = vtrnq_f32 (c, d);
    
    // Transpose 4 ordered arrays of 4 samples
    a = vcombine_f32(vget_low_f32(ab.val[0]), vget_low_f32(cd.val[0]));
    b = vcombine_f32(vget_low_f32(ab.val[1]), vget_low_f32(cd.val[1]));
    c = vcombine_f32(vget_high_f32(ab.val[0]), vget_high_f32(cd.val[0]));
    d = vcombine_f32(vget_high_f32(ab.val[1]), vget_high_f32(cd.val[1]));

    // Merge pairs of arrays of 4 samples
    ab = arm_bitonic_merge_8_f32(a, b, dir);
    cd = arm_bitonic_merge_8_f32(c, d, dir);
    
    // Merge arrays of 8 samples
    arm_bitonic_merge_16_f32(pDst, ab, cd, dir);
}





static void arm_bitonic_merge_32_f32(float32_t * pSrc, float32x4x2_t ab1, float32x4x2_t ab2, float32x4x2_t cd1, float32x4x2_t cd2, uint8_t dir)
{
    //Compare
    if(dir)
    {
        vminmax256q(ab1, cd1);
        vminmax256q(ab2, cd2);
    }
    else
    {
        vminmax256q(cd1, ab1);
        vminmax256q(cd2, ab2);
    }
    //Transpose 256
    float32x4_t temp;

    temp = ab2.val[0];
    ab2.val[0] = cd1.val[0];
    cd1.val[0] = temp;
    temp = ab2.val[1];
    ab2.val[1] = cd1.val[1];
    cd1.val[1] = temp;

    //Compare
    if(dir)
    {
        vminmax256q(ab1, cd1);
        vminmax256q(ab2, cd2);
    }
    else
    {
        vminmax256q(cd1, ab1);
        vminmax256q(cd2, ab2);
    }
    
    //Transpose 128
    arm_bitonic_merge_16_f32(pSrc+0,  ab1, cd1, dir);
    arm_bitonic_merge_16_f32(pSrc+16, ab2, cd2, dir);
}

static void arm_bitonic_merge_64_f32(float32_t * pSrc, uint8_t dir)
{
    float32x4x2_t ab1, ab2, ab3, ab4;
    float32x4x2_t cd1, cd2, cd3, cd4;

    //Load and reverse second array
    ab1.val[0] = vld1q_f32(pSrc+0 );
    ab1.val[1] = vld1q_f32(pSrc+4 );
    ab2.val[0] = vld1q_f32(pSrc+8 ); 
    ab2.val[1] = vld1q_f32(pSrc+12);
    ab3.val[0] = vld1q_f32(pSrc+16);
    ab3.val[1] = vld1q_f32(pSrc+20);
    ab4.val[0] = vld1q_f32(pSrc+24); 
    ab4.val[1] = vld1q_f32(pSrc+28);

    vldrev128q_f32(cd4.val[1], pSrc+32);
    vldrev128q_f32(cd4.val[0], pSrc+36);
    vldrev128q_f32(cd3.val[1], pSrc+40);
    vldrev128q_f32(cd3.val[0], pSrc+44);
    vldrev128q_f32(cd2.val[1], pSrc+48);
    vldrev128q_f32(cd2.val[0], pSrc+52);
    vldrev128q_f32(cd1.val[1], pSrc+56);
    vldrev128q_f32(cd1.val[0], pSrc+60);
    
    //Compare
    if(dir)
    {
        vminmax256q(ab1, cd1);
        vminmax256q(ab2, cd2);
        vminmax256q(ab3, cd3);
        vminmax256q(ab4, cd4);
    }
    else
    {
        vminmax256q(cd1, ab1);
        vminmax256q(cd2, ab2);
        vminmax256q(cd3, ab3);
        vminmax256q(cd4, ab4);
    }

    //Transpose 512
    float32x4_t temp;

    temp = ab3.val[0];
    ab3.val[0] = cd1.val[0];
    cd1.val[0] = temp;
    temp = ab3.val[1];
    ab3.val[1] = cd1.val[1];
    cd1.val[1] = temp;
    temp = ab4.val[0];
    ab4.val[0] = cd2.val[0];
    cd2.val[0] = temp;
    temp = ab4.val[1];
    ab4.val[1] = cd2.val[1];
    cd2.val[1] = temp;

    //Compare
    if(dir)
    {
        vminmax256q(ab1, cd1);
        vminmax256q(ab2, cd2);
        vminmax256q(ab3, cd3);
        vminmax256q(ab4, cd4);
    }
    else
    {
        vminmax256q(cd1, ab1);
        vminmax256q(cd2, ab2);
        vminmax256q(cd3, ab3);
        vminmax256q(cd4, ab4);
    }
    
    //Transpose 256
    arm_bitonic_merge_32_f32(pSrc+0,  ab1, ab2, cd1, cd2, dir);
    arm_bitonic_merge_32_f32(pSrc+32, ab3, ab4, cd3, cd4, dir);
}

static void arm_bitonic_merge_128_f32(float32_t * pSrc, uint8_t dir)
{
    float32x4x2_t ab1, ab2, ab3, ab4, ab5, ab6, ab7, ab8;
    float32x4x2_t cd1, cd2, cd3, cd4, cd5, cd6, cd7, cd8;

    //Load and reverse second array
    ab1.val[0] = vld1q_f32(pSrc+0 );
    ab1.val[1] = vld1q_f32(pSrc+4 );
    ab2.val[0] = vld1q_f32(pSrc+8 ); 
    ab2.val[1] = vld1q_f32(pSrc+12);
    ab3.val[0] = vld1q_f32(pSrc+16);
    ab3.val[1] = vld1q_f32(pSrc+20);
    ab4.val[0] = vld1q_f32(pSrc+24); 
    ab4.val[1] = vld1q_f32(pSrc+28);
    ab5.val[0] = vld1q_f32(pSrc+32);
    ab5.val[1] = vld1q_f32(pSrc+36);
    ab6.val[0] = vld1q_f32(pSrc+40); 
    ab6.val[1] = vld1q_f32(pSrc+44);
    ab7.val[0] = vld1q_f32(pSrc+48);
    ab7.val[1] = vld1q_f32(pSrc+52);
    ab8.val[0] = vld1q_f32(pSrc+56); 
    ab8.val[1] = vld1q_f32(pSrc+60);

    vldrev128q_f32(cd8.val[1], pSrc+64);
    vldrev128q_f32(cd8.val[0], pSrc+68);
    vldrev128q_f32(cd7.val[1], pSrc+72);
    vldrev128q_f32(cd7.val[0], pSrc+76);
    vldrev128q_f32(cd6.val[1], pSrc+80);
    vldrev128q_f32(cd6.val[0], pSrc+84);
    vldrev128q_f32(cd5.val[1], pSrc+88);
    vldrev128q_f32(cd5.val[0], pSrc+92);
    vldrev128q_f32(cd4.val[1], pSrc+96);
    vldrev128q_f32(cd4.val[0], pSrc+100);
    vldrev128q_f32(cd3.val[1], pSrc+104);
    vldrev128q_f32(cd3.val[0], pSrc+108);
    vldrev128q_f32(cd2.val[1], pSrc+112);
    vldrev128q_f32(cd2.val[0], pSrc+116);
    vldrev128q_f32(cd1.val[1], pSrc+120);
    vldrev128q_f32(cd1.val[0], pSrc+124);
    
    //Compare
    if(dir)
    {
        vminmax256q(ab1, cd1);
        vminmax256q(ab2, cd2);
        vminmax256q(ab3, cd3);
        vminmax256q(ab4, cd4);
        vminmax256q(ab5, cd5);
        vminmax256q(ab6, cd6);
        vminmax256q(ab7, cd7);
        vminmax256q(ab8, cd8);
    }
    else
    {
        vminmax256q(cd1, ab1);
        vminmax256q(cd2, ab2);
        vminmax256q(cd3, ab3);
        vminmax256q(cd4, ab4);
        vminmax256q(cd5, ab5);
        vminmax256q(cd6, ab6);
        vminmax256q(cd7, ab7);
        vminmax256q(cd8, ab8);
    }
    
    //Transpose
    float32x4_t temp;

    temp = ab5.val[0];
    ab5.val[0] = cd1.val[0];
    cd1.val[0] = temp;
    temp = ab5.val[1];
    ab5.val[1] = cd1.val[1];
    cd1.val[1] = temp;
    temp = ab6.val[0];
    ab6.val[0] = cd2.val[0];
    cd2.val[0] = temp;
    temp = ab6.val[1];
    ab6.val[1] = cd2.val[1];
    cd2.val[1] = temp;
    temp = ab7.val[0];
    ab7.val[0] = cd3.val[0];
    cd3.val[0] = temp;
    temp = ab7.val[1];
    ab7.val[1] = cd3.val[1];
    cd3.val[1] = temp;
    temp = ab8.val[0];
    ab8.val[0] = cd4.val[0];
    cd4.val[0] = temp;
    temp = ab8.val[1];
    ab8.val[1] = cd4.val[1];
    cd4.val[1] = temp;

    //Compare
    if(dir)
    {
        vminmax256q(ab1, cd1);
        vminmax256q(ab2, cd2);
        vminmax256q(ab3, cd3);
        vminmax256q(ab4, cd4);
        vminmax256q(ab5, cd5);
        vminmax256q(ab6, cd6);
        vminmax256q(ab7, cd7);
        vminmax256q(ab8, cd8);
    }
    else
    {
        vminmax256q(cd1, ab1);
        vminmax256q(cd2, ab2);
        vminmax256q(cd3, ab3);
        vminmax256q(cd4, ab4);
        vminmax256q(cd5, ab5);
        vminmax256q(cd6, ab6);
        vminmax256q(cd7, ab7);
        vminmax256q(cd8, ab8);
    }

    vst1q_f32(pSrc,     ab1.val[0]);
    vst1q_f32(pSrc+4,   ab1.val[1]);
    vst1q_f32(pSrc+8,   ab2.val[0]);
    vst1q_f32(pSrc+12,  ab2.val[1]);
    vst1q_f32(pSrc+16,  ab3.val[0]);
    vst1q_f32(pSrc+20,  ab3.val[1]);
    vst1q_f32(pSrc+24,  ab4.val[0]);
    vst1q_f32(pSrc+28,  ab4.val[1]);
    vst1q_f32(pSrc+32,  cd1.val[0]);
    vst1q_f32(pSrc+36,  cd1.val[1]);
    vst1q_f32(pSrc+40,  cd2.val[0]);
    vst1q_f32(pSrc+44,  cd2.val[1]);
    vst1q_f32(pSrc+48,  cd3.val[0]);
    vst1q_f32(pSrc+52,  cd3.val[1]);
    vst1q_f32(pSrc+56,  cd4.val[0]);
    vst1q_f32(pSrc+60,  cd4.val[1]);
    vst1q_f32(pSrc+64,  ab5.val[0]);
    vst1q_f32(pSrc+68,  ab5.val[1]);
    vst1q_f32(pSrc+72,  ab6.val[0]);
    vst1q_f32(pSrc+76,  ab6.val[1]);
    vst1q_f32(pSrc+80,  ab7.val[0]);
    vst1q_f32(pSrc+84,  ab7.val[1]);
    vst1q_f32(pSrc+88,  ab8.val[0]);
    vst1q_f32(pSrc+92,  ab8.val[1]);
    vst1q_f32(pSrc+96,  cd5.val[0]);
    vst1q_f32(pSrc+100, cd5.val[1]);
    vst1q_f32(pSrc+104, cd6.val[0]);
    vst1q_f32(pSrc+108, cd6.val[1]);
    vst1q_f32(pSrc+112, cd7.val[0]);
    vst1q_f32(pSrc+116, cd7.val[1]);
    vst1q_f32(pSrc+120, cd8.val[0]);
    vst1q_f32(pSrc+124, cd8.val[1]);

    //Transpose
    arm_bitonic_merge_64_f32(pSrc+0 , dir);
    arm_bitonic_merge_64_f32(pSrc+64, dir);
}

static void arm_bitonic_merge_256_f32(float32_t * pSrc, uint8_t dir)
{
    float32x4x2_t ab1, ab2, ab3, ab4, ab5, ab6, ab7, ab8;
    float32x4x2_t ab9, ab10, ab11, ab12, ab13, ab14, ab15, ab16;
    float32x4x2_t cd1, cd2, cd3, cd4, cd5, cd6, cd7, cd8;
    float32x4x2_t cd9, cd10, cd11, cd12, cd13, cd14, cd15, cd16;

    //Load and reverse second array
    ab1.val[0]  = vld1q_f32(pSrc+0  );
    ab1.val[1]  = vld1q_f32(pSrc+4  );
    ab2.val[0]  = vld1q_f32(pSrc+8  ); 
    ab2.val[1]  = vld1q_f32(pSrc+12 );
    ab3.val[0]  = vld1q_f32(pSrc+16 );
    ab3.val[1]  = vld1q_f32(pSrc+20 );
    ab4.val[0]  = vld1q_f32(pSrc+24 ); 
    ab4.val[1]  = vld1q_f32(pSrc+28 );
    ab5.val[0]  = vld1q_f32(pSrc+32 );
    ab5.val[1]  = vld1q_f32(pSrc+36 );
    ab6.val[0]  = vld1q_f32(pSrc+40 ); 
    ab6.val[1]  = vld1q_f32(pSrc+44 );
    ab7.val[0]  = vld1q_f32(pSrc+48 );
    ab7.val[1]  = vld1q_f32(pSrc+52 );
    ab8.val[0]  = vld1q_f32(pSrc+56 ); 
    ab8.val[1]  = vld1q_f32(pSrc+60 );
    ab9.val[0]  = vld1q_f32(pSrc+64 );
    ab9.val[1]  = vld1q_f32(pSrc+68 );
    ab10.val[0] = vld1q_f32(pSrc+72 ); 
    ab10.val[1] = vld1q_f32(pSrc+76 );
    ab11.val[0] = vld1q_f32(pSrc+80 );
    ab11.val[1] = vld1q_f32(pSrc+84 );
    ab12.val[0] = vld1q_f32(pSrc+88 ); 
    ab12.val[1] = vld1q_f32(pSrc+92 );
    ab13.val[0] = vld1q_f32(pSrc+96 );
    ab13.val[1] = vld1q_f32(pSrc+100);
    ab14.val[0] = vld1q_f32(pSrc+104); 
    ab14.val[1] = vld1q_f32(pSrc+108);
    ab15.val[0] = vld1q_f32(pSrc+112);
    ab15.val[1] = vld1q_f32(pSrc+116);
    ab16.val[0] = vld1q_f32(pSrc+120); 
    ab16.val[1] = vld1q_f32(pSrc+124);

    vldrev128q_f32(cd16.val[1], pSrc+128);
    vldrev128q_f32(cd16.val[0], pSrc+132);
    vldrev128q_f32(cd15.val[1], pSrc+136);
    vldrev128q_f32(cd15.val[0], pSrc+140);
    vldrev128q_f32(cd14.val[1], pSrc+144);
    vldrev128q_f32(cd14.val[0], pSrc+148);
    vldrev128q_f32(cd13.val[1], pSrc+152);
    vldrev128q_f32(cd13.val[0], pSrc+156);
    vldrev128q_f32(cd12.val[1], pSrc+160);
    vldrev128q_f32(cd12.val[0], pSrc+164);
    vldrev128q_f32(cd11.val[1], pSrc+168);
    vldrev128q_f32(cd11.val[0], pSrc+172);
    vldrev128q_f32(cd10.val[1], pSrc+176);
    vldrev128q_f32(cd10.val[0], pSrc+180);
    vldrev128q_f32(cd9.val[1] , pSrc+184);
    vldrev128q_f32(cd9.val[0] , pSrc+188);
    vldrev128q_f32(cd8.val[1] , pSrc+192);
    vldrev128q_f32(cd8.val[0] , pSrc+196);
    vldrev128q_f32(cd7.val[1] , pSrc+200);
    vldrev128q_f32(cd7.val[0] , pSrc+204);
    vldrev128q_f32(cd6.val[1] , pSrc+208);
    vldrev128q_f32(cd6.val[0] , pSrc+212);
    vldrev128q_f32(cd5.val[1] , pSrc+216);
    vldrev128q_f32(cd5.val[0] , pSrc+220);
    vldrev128q_f32(cd4.val[1] , pSrc+224);
    vldrev128q_f32(cd4.val[0] , pSrc+228);
    vldrev128q_f32(cd3.val[1] , pSrc+232);
    vldrev128q_f32(cd3.val[0] , pSrc+236);
    vldrev128q_f32(cd2.val[1] , pSrc+240);
    vldrev128q_f32(cd2.val[0] , pSrc+244);
    vldrev128q_f32(cd1.val[1] , pSrc+248);
    vldrev128q_f32(cd1.val[0] , pSrc+252);
    
    //Compare
    if(dir)
    {
        vminmax256q(ab1 , cd1 );
        vminmax256q(ab2 , cd2 );
        vminmax256q(ab3 , cd3 );
        vminmax256q(ab4 , cd4 );
        vminmax256q(ab5 , cd5 );
        vminmax256q(ab6 , cd6 );
        vminmax256q(ab7 , cd7 );
        vminmax256q(ab8 , cd8 );
        vminmax256q(ab9 , cd9 );
        vminmax256q(ab10, cd10);
        vminmax256q(ab11, cd11);
        vminmax256q(ab12, cd12);
        vminmax256q(ab13, cd13);
        vminmax256q(ab14, cd14);
        vminmax256q(ab15, cd15);
        vminmax256q(ab16, cd16);
    }
    else
    {
        vminmax256q(cd1 , ab1 );
        vminmax256q(cd2 , ab2 );
        vminmax256q(cd3 , ab3 );
        vminmax256q(cd4 , ab4 );
        vminmax256q(cd5 , ab5 );
        vminmax256q(cd6 , ab6 );
        vminmax256q(cd7 , ab7 );
        vminmax256q(cd8 , ab8 );
        vminmax256q(cd9 , ab9 );
        vminmax256q(cd10, ab10);
        vminmax256q(cd11, ab11);
        vminmax256q(cd12, ab12);
        vminmax256q(cd13, ab13);
        vminmax256q(cd14, ab14);
        vminmax256q(cd15, ab15);
        vminmax256q(cd16, ab16);
    }

    //Transpose
    float32x4_t temp;

    temp = ab9.val[0];
    ab9.val[0] = cd1.val[0];
    cd1.val[0] = temp;
    temp = ab9.val[1];
    ab9.val[1] = cd1.val[1];
    cd1.val[1] = temp;
    temp = ab10.val[0];
    ab10.val[0] = cd2.val[0];
    cd2.val[0] = temp;
    temp = ab10.val[1];
    ab10.val[1] = cd2.val[1];
    cd2.val[1] = temp;
    temp = ab11.val[0];
    ab11.val[0] = cd3.val[0];
    cd3.val[0] = temp;
    temp = ab11.val[1];
    ab11.val[1] = cd3.val[1];
    cd3.val[1] = temp;
    temp = ab12.val[0];
    ab12.val[0] = cd4.val[0];
    cd4.val[0] = temp;
    temp = ab12.val[1];
    ab12.val[1] = cd4.val[1];
    cd4.val[1] = temp;
    temp = ab13.val[0];
    ab13.val[0] = cd5.val[0];
    cd5.val[0] = temp;
    temp = ab13.val[1];
    ab13.val[1] = cd5.val[1];
    cd5.val[1] = temp;
    temp = ab14.val[0];
    ab14.val[0] = cd6.val[0];
    cd6.val[0] = temp;
    temp = ab14.val[1];
    ab14.val[1] = cd6.val[1];
    cd6.val[1] = temp;
    temp = ab15.val[0];
    ab15.val[0] = cd7.val[0];
    cd7.val[0] = temp;
    temp = ab15.val[1];
    ab15.val[1] = cd7.val[1];
    cd7.val[1] = temp;
    temp = ab16.val[0];
    ab16.val[0] = cd8.val[0];
    cd8.val[0] = temp;
    temp = ab16.val[1];
    ab16.val[1] = cd8.val[1];
    cd8.val[1] = temp;

    //Compare
    if(dir)
    {
        vminmax256q(ab1 , cd1 );
        vminmax256q(ab2 , cd2 );
        vminmax256q(ab3 , cd3 );
        vminmax256q(ab4 , cd4 );
        vminmax256q(ab5 , cd5 );
        vminmax256q(ab6 , cd6 );
        vminmax256q(ab7 , cd7 );
        vminmax256q(ab8 , cd8 );
        vminmax256q(ab9 , cd9 );
        vminmax256q(ab10, cd10);
        vminmax256q(ab11, cd11);
        vminmax256q(ab12, cd12);
        vminmax256q(ab13, cd13);
        vminmax256q(ab14, cd14);
        vminmax256q(ab15, cd15);
        vminmax256q(ab16, cd16);
    }
    else
    {
        vminmax256q(cd1 , ab1 );
        vminmax256q(cd2 , ab2 );
        vminmax256q(cd3 , ab3 );
        vminmax256q(cd4 , ab4 );
        vminmax256q(cd5 , ab5 );
        vminmax256q(cd6 , ab6 );
        vminmax256q(cd7 , ab7 );
        vminmax256q(cd8 , ab8 );
        vminmax256q(cd9 , ab9 );
        vminmax256q(cd10, ab10);
        vminmax256q(cd11, ab11);
        vminmax256q(cd12, ab12);
        vminmax256q(cd13, ab13);
        vminmax256q(cd14, ab14);
        vminmax256q(cd15, ab15);
        vminmax256q(cd16, ab16);
    }

    vst1q_f32(pSrc,     ab1.val[0] );
    vst1q_f32(pSrc+4,   ab1.val[1] );
    vst1q_f32(pSrc+8,   ab2.val[0] );
    vst1q_f32(pSrc+12,  ab2.val[1] );
    vst1q_f32(pSrc+16,  ab3.val[0] );
    vst1q_f32(pSrc+20,  ab3.val[1] );
    vst1q_f32(pSrc+24,  ab4.val[0] );
    vst1q_f32(pSrc+28,  ab4.val[1] );
    vst1q_f32(pSrc+32,  ab5.val[0] );
    vst1q_f32(pSrc+36,  ab5.val[1] );
    vst1q_f32(pSrc+40,  ab6.val[0] );
    vst1q_f32(pSrc+44,  ab6.val[1] );
    vst1q_f32(pSrc+48,  ab7.val[0] );
    vst1q_f32(pSrc+52,  ab7.val[1] );
    vst1q_f32(pSrc+56,  ab8.val[0] );
    vst1q_f32(pSrc+60,  ab8.val[1] );
    vst1q_f32(pSrc+64,  cd1.val[0] );
    vst1q_f32(pSrc+68,  cd1.val[1] );
    vst1q_f32(pSrc+72,  cd2.val[0] );
    vst1q_f32(pSrc+76,  cd2.val[1] );
    vst1q_f32(pSrc+80,  cd3.val[0] );
    vst1q_f32(pSrc+84,  cd3.val[1] );
    vst1q_f32(pSrc+88,  cd4.val[0] );
    vst1q_f32(pSrc+92,  cd4.val[1] );
    vst1q_f32(pSrc+96,  cd5.val[0] );
    vst1q_f32(pSrc+100, cd5.val[1] );
    vst1q_f32(pSrc+104, cd6.val[0] );
    vst1q_f32(pSrc+108, cd6.val[1] );
    vst1q_f32(pSrc+112, cd7.val[0] );
    vst1q_f32(pSrc+116, cd7.val[1] );
    vst1q_f32(pSrc+120, cd8.val[0] );
    vst1q_f32(pSrc+124, cd8.val[1] );
    vst1q_f32(pSrc+128, ab9.val[0] );
    vst1q_f32(pSrc+132, ab9.val[1] );
    vst1q_f32(pSrc+136, ab10.val[0]);
    vst1q_f32(pSrc+140, ab10.val[1]);
    vst1q_f32(pSrc+144, ab11.val[0]);
    vst1q_f32(pSrc+148, ab11.val[1]);
    vst1q_f32(pSrc+152, ab12.val[0]);
    vst1q_f32(pSrc+156, ab12.val[1]);
    vst1q_f32(pSrc+160, ab13.val[0]);
    vst1q_f32(pSrc+164, ab13.val[1]);
    vst1q_f32(pSrc+168, ab14.val[0]);
    vst1q_f32(pSrc+172, ab14.val[1]);
    vst1q_f32(pSrc+176, ab15.val[0]);
    vst1q_f32(pSrc+180, ab15.val[1]);
    vst1q_f32(pSrc+184, ab16.val[0]);
    vst1q_f32(pSrc+188, ab16.val[1]);
    vst1q_f32(pSrc+192, cd9.val[0] );
    vst1q_f32(pSrc+196, cd9.val[1] );
    vst1q_f32(pSrc+200, cd10.val[0]);
    vst1q_f32(pSrc+204, cd10.val[1]);
    vst1q_f32(pSrc+208, cd11.val[0]);
    vst1q_f32(pSrc+212, cd11.val[1]);
    vst1q_f32(pSrc+216, cd12.val[0]);
    vst1q_f32(pSrc+220, cd12.val[1]);
    vst1q_f32(pSrc+224, cd13.val[0]);
    vst1q_f32(pSrc+228, cd13.val[1]);
    vst1q_f32(pSrc+232, cd14.val[0]);
    vst1q_f32(pSrc+236, cd14.val[1]);
    vst1q_f32(pSrc+240, cd15.val[0]);
    vst1q_f32(pSrc+244, cd15.val[1]);
    vst1q_f32(pSrc+248, cd16.val[0]);
    vst1q_f32(pSrc+252, cd16.val[1]);

    //Transpose
    arm_bitonic_merge_128_f32(pSrc+0  , dir);
    arm_bitonic_merge_128_f32(pSrc+128, dir);
}

#define SWAP(a,i,j)                            \
    temp = vgetq_lane_f32(a, j);                   \
    a = vsetq_lane_f32(vgetq_lane_f32(a, i), a, j);\
    a = vsetq_lane_f32(temp, a, i);

static float32x4_t arm_bitonic_sort_4_f32(float32x4_t a, uint8_t dir)
{
    float32_t temp;


    if( dir==(vgetq_lane_f32(a, 0) > vgetq_lane_f32(a, 1)) )
    {
        SWAP(a,0,1);
    }
    if( dir==(vgetq_lane_f32(a, 2) > vgetq_lane_f32(a, 3)) )
    {
       SWAP(a,2,3);
    }

    if( dir==(vgetq_lane_f32(a, 0) > vgetq_lane_f32(a, 3)) )
    {
      SWAP(a,0,3);
    }
    if( dir==(vgetq_lane_f32(a, 1) > vgetq_lane_f32(a, 2)) )
    {
      SWAP(a,1,2);
    }

    if( dir==(vgetq_lane_f32(a, 0) > vgetq_lane_f32(a, 1)) )
    {
      SWAP(a,0,1);
    }
    if( dir==(vgetq_lane_f32(a, 2)>vgetq_lane_f32(a, 3)) )
    {
      SWAP(a,2,3);
    }

    return a;
}

static float32x4x2_t arm_bitonic_sort_8_f32(float32x4_t a, float32x4_t b, uint8_t dir)
{
    a = arm_bitonic_sort_4_f32(a, dir);
    b = arm_bitonic_sort_4_f32(b, dir);
    return arm_bitonic_merge_8_f32(a, b, dir);
}



#endif

/**
  @ingroup groupSupport
 */

/**
  @defgroup Sorting Vector sorting algorithms

  Sort the elements of a vector

  There are separate functions for floating-point, Q31, Q15, and Q7 data types.
 */

/**
  @addtogroup Sorting
  @{
 */

/**
   * @param[in]  S          points to an instance of the sorting structure.
   * @param[in]  pSrc       points to the block of input data.
   * @param[out] pDst       points to the block of output data
   * @param[in]  blockSize  number of samples to process.
   */
void arm_bitonic_sort_f32(
const arm_sort_instance_f32 * S, 
      float32_t * pSrc,
      float32_t * pDst, 
      uint32_t blockSize)
{
    uint16_t s, i;
    uint8_t dir = S->dir;

#ifdef ARM_MATH_NEON
    (void)s;

    float32_t * pOut;
    uint16_t counter = blockSize>>5;

    if( (blockSize & (blockSize-1)) == 0 ) // Powers of 2 only
    {
        if(pSrc == pDst) // in-place
            pOut = pSrc;
        else
    	    pOut = pDst;
    
        float32x4x2_t ab1, ab2;
        float32x4x2_t cd1, cd2;

	if(blockSize == 1)
		pOut = pSrc;
	else if(blockSize == 2)
	{
            float32_t temp;
            
            if( dir==(pSrc[0]>pSrc[1]) )
            {
                temp = pSrc[1];
                pOut[1] = pSrc[0];
                pOut[0] = temp;
            }
	    else
		pOut = pSrc;
	}
	else if(blockSize == 4)
        {
    	    float32x4_t a = vld1q_f32(pSrc);

    	    a = arm_bitonic_sort_4_f32(a, dir);

    	    vst1q_f32(pOut, a);
        }
        else if(blockSize == 8)
        {
            float32x4_t a;
            float32x4_t b;
            float32x4x2_t ab;
        
            a = vld1q_f32(pSrc);
            b = vld1q_f32(pSrc+4);
        
            ab = arm_bitonic_sort_8_f32(a, b, dir);

            vst1q_f32(pOut,   ab.val[0]);
            vst1q_f32(pOut+4, ab.val[1]);
        }
        else if(blockSize >=16)
        {
            // Order 16 bits long vectors
            for(i=0; i<blockSize; i=i+16)
                arm_bitonic_sort_16_f32(pSrc+i, pOut+i, dir);
        
            // Merge
            for(i=0; i<counter; i++)
            {
                // Load and reverse second vector
                ab1.val[0] = vld1q_f32(pOut+32*i+0 );
                ab1.val[1] = vld1q_f32(pOut+32*i+4 );
                ab2.val[0] = vld1q_f32(pOut+32*i+8 ); 
                ab2.val[1] = vld1q_f32(pOut+32*i+12);

                vldrev128q_f32(cd2.val[1], pOut+32*i+16);
                vldrev128q_f32(cd2.val[0], pOut+32*i+20);
                vldrev128q_f32(cd1.val[1], pOut+32*i+24);
                vldrev128q_f32(cd1.val[0], pOut+32*i+28);

                arm_bitonic_merge_32_f32(pOut+32*i, ab1, ab2, cd1, cd2, dir);
            }
        
            counter = counter>>1;
            for(i=0; i<counter; i++)
                arm_bitonic_merge_64_f32(pOut+64*i, dir);
        
            counter = counter>>1;
            for(i=0; i<counter; i++)
                arm_bitonic_merge_128_f32(pOut+128*i, dir);
        
            counter = counter>>1;
            for(i=0; i<counter; i++)
                arm_bitonic_merge_256_f32(pOut+256*i, dir);

            // Etc...
        }
    }

#else

    float32_t * pA;

    if(pSrc != pDst) // out-of-place
    {   
        memcpy(pDst, pSrc, blockSize*sizeof(float32_t) );
        pA = pDst;
    }
    else
        pA = pSrc;


    if( (blockSize & (blockSize-1)) == 0 ) // Powers of 2 only
    {
        for(s=2; s<=blockSize; s=s*2)
        {
    	    for(i=0; i<blockSize; i=i+s)
    	        arm_bitonic_sort_core_f32(pA+i, s, dir);
        }
    }
#endif
}

/**
  @} end of Sorting group
 */
