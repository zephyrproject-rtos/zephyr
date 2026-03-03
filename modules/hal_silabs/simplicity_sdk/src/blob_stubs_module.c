/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Empty function stubs to enable building with CONFIG_BUILD_ONLY_NO_BLOBS on modules.
 */

#include <stdbool.h>

#include <rail.h>
#include "pa_curve_types_efr32.h"

const RAIL_TxPowerCurvesConfigAlt_t RAIL_TxPowerCurvesVbat;
const RAIL_TxPowerCurvesConfigAlt_t RAIL_TxPowerCurvesDcdc;

RAIL_Status_t RAIL_VerifyTxPowerCurves(const struct RAIL_TxPowerCurvesConfigAlt *config)
{
	return RAIL_STATUS_NO_ERROR;
}

void RAIL_EnablePaCal(bool enable)
{
}
