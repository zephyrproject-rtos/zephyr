/*
 * Copyright (c) 2022 Synopsys
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if DT_NODE_HAS_PROP(DT_INST(0, arc_xccm), reg) && \
    (DT_REG_SIZE(DT_INST(0, arc_xccm)) > 0)
#define XCCM_START		DT_REG_ADDR(DT_INST(0, arc_xccm))
#define XCCM_SIZE		DT_REG_SIZE(DT_INST(0, arc_xccm))
#endif

#if DT_NODE_HAS_PROP(DT_INST(0, arc_yccm), reg) && \
    (DT_REG_SIZE(DT_INST(0, arc_yccm)) > 0)
#define YCCM_START		DT_REG_ADDR(DT_INST(0, arc_yccm))
#define YCCM_SIZE		DT_REG_SIZE(DT_INST(0, arc_yccm))
#endif
