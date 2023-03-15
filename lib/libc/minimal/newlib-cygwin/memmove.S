/* SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 * Copyright (c) 2019  SiFive Inc. All rights reserved.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the FreeBSD License.   This program is distributed in the hope that
 * it will be useful, but WITHOUT ANY WARRANTY expressed or implied,
 * including the implied warranties of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE.  A copy of this license is available at
 * http://www.opensource.org/licenses.
 */

#if defined(PREFER_SIZE_OVER_SPEED) || defined(__OPTIMIZE_SIZE__)
.text
.global memmove
.type	memmove, @function
memmove:
  beqz a2, 2f

  mv t1, a0
  li a3, 1
  bgtu  a1, a0, 1f

  li a3, -1
  addi  a4, a2 , -1
  add t1, t1, a4
  add a1, a1, a4

1:
  lb t2, 0(a1)
  sb t2, 0(t1)
  add   a2, a2, -1
  add   t1, t1, a3
  add   a1, a1, a3
  bnez a2, 1b

2:
  ret

  .size	memmove, .-memmove
#endif
