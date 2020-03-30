/* SPDX-License-Identifier: Apache-2.0 */

/* SoC level DTS fixup file */

/* CCM configuration */
#define DT_DCCM_BASE_ADDRESS       DT_REG_ADDR(DT_INST(0, arc_dccm))
#define DT_DCCM_SIZE               (DT_REG_SIZE(DT_INST(0, arc_dccm)) >> 10)

#define DT_ICCM_BASE_ADDRESS       DT_REG_ADDR(DT_INST(0, arc_iccm))
#define DT_ICCM_SIZE               (DT_REG_SIZE(DT_INST(0, arc_iccm)) >> 10)

/* End of SoC Level DTS fixup file */
