/* SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 * Copyright (c) 2017  SiFive Inc. All rights reserved.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the FreeBSD License.   This program is distributed in the hope that
 * it will be useful, but WITHOUT ANY WARRANTY expressed or implied,
 * including the implied warranties of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE.  A copy of this license is available at
 * http://www.opensource.org/licenses.
 */

.text
.global memset
.type	memset, @function
memset:
#if defined(PREFER_SIZE_OVER_SPEED) || defined(__OPTIMIZE_SIZE__)
  mv t1, a0
  beqz a2, 2f

1:
  sb a1, 0(t1)
  add   a2, a2, -1
  add   t1, t1, 1
  bnez a2, 1b

2:
  ret
#else
  li t1, 15
  move a4, a0
  bleu a2, t1, .Ltiny
  and a5, a4, 15
  bnez a5, .Lmisaligned

.Laligned:
  bnez a1, .Lwordify

.Lwordified:
  and a3, a2, ~15
  and a2, a2, 15
  add a3, a3, a4

#if __riscv_xlen == 64
1:sd a1, 0(a4)
  sd a1, 8(a4)
#else
1:sw a1, 0(a4)
  sw a1, 4(a4)
  sw a1, 8(a4)
  sw a1, 12(a4)
#endif
  add a4, a4, 16
  bltu a4, a3, 1b

  bnez a2, .Ltiny
  ret

.Ltiny:
  sub a3, t1, a2
  sll a3, a3, 2
1:auipc t0, %pcrel_hi(.Ltable)
  add a3, a3, t0
.option push
.option norvc
.Ltable_misaligned:
  jr a3, %pcrel_lo(1b)
.Ltable:
  sb a1,14(a4)
  sb a1,13(a4)
  sb a1,12(a4)
  sb a1,11(a4)
  sb a1,10(a4)
  sb a1, 9(a4)
  sb a1, 8(a4)
  sb a1, 7(a4)
  sb a1, 6(a4)
  sb a1, 5(a4)
  sb a1, 4(a4)
  sb a1, 3(a4)
  sb a1, 2(a4)
  sb a1, 1(a4)
  sb a1, 0(a4)
.option pop
  ret

.Lwordify:
  and a1, a1, 0xFF
  sll a3, a1, 8
  or  a1, a1, a3
  sll a3, a1, 16
  or  a1, a1, a3
#if __riscv_xlen == 64
  sll a3, a1, 32
  or  a1, a1, a3
#endif
  j .Lwordified

.Lmisaligned:
  sll a3, a5, 2
1:auipc t0, %pcrel_hi(.Ltable_misaligned)
  add a3, a3, t0
  mv t0, ra
  jalr a3, %pcrel_lo(1b)
  mv ra, t0

  add a5, a5, -16
  sub a4, a4, a5
  add a2, a2, a5
  bleu a2, t1, .Ltiny
  j .Laligned
#endif
  .size	memset, .-memset
