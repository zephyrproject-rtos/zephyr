/*
 * Copyright (c) 2020 Mario Jaun
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if DT_NODE_EXISTS(DT_NODELABEL(dtcm))
.dtcm_noinit (NOLOAD) : SUBALIGN(4)
 {
  __dtcm_noinit_start = .;
  *(.dtcm_noinit)
  *(".dtcm_noinit.*")
  *(".kernel_noinit.*")
  *(".noinit.*kernel/init.*")
  *(".noinit.*kernel/mempool.*")
  __dtcm_noinit_end = .;
 } > DTCM
#endif
