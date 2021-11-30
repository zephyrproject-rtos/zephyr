/**
 * @file vesc.h
 * @author Jonas Merkle [JJM] (jonas@jjm.one)
 * @date 30.11.21
 * Description here TODO
 */

#ifndef CAROLO_APP_VESC_H
#define CAROLO_APP_VESC_H

#include <drivers/uart.h>

typedef struct vesc_data {
    //
} vesc_data;

typedef struct vesc_config {
    struct uart_dt_spec uart_spec;
    //VESC_handle vesc;
    //VESC_master_handle vescMaster;
} as5x47_config;

#endif //CAROLO_APP_VESC_H
