/**
 * @file vesc.c
 * @author Jonas Merkle [JJM] (jonas@jjm.one)
 * @date 30.11.21
 * Description here TODO
 */

#include "vesc.h"

#define DT_DRV_COMPAT vesc

#include <devicetree.h>
#include <drivers/sensor.h>
#include <logging/log.h>

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#error "VESC driver enabled without any devices"
#endif

LOG_MODULE_REGISTER(vesc_driver, LOG_LEVEL_DBG);
