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
.global memcpy
.type	memcpy, @function
memcpy:
  mv t1, a0
  beqz a2, 2f

1:
  lb t2, 0(a1)
  sb t2, 0(t1)
  add   a2, a2, -1
  add   t1, t1, 1
  add   a1, a1, 1
  bnez a2, 1b

2:
  ret

  .size	memcpy, .-memcpy
#endif
