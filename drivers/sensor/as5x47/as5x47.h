//
// Created by jonasotto on 10/29/21.
//

#ifndef CAROLO_APP_AS5X47_H
#define CAROLO_APP_AS5X47_H

#include <drivers/spi.h>

#include "as5047p_c_interface.h"

typedef struct as5x47_data {
    float angle_deg;
} as5x47_data;

typedef struct as5x47_config {
    struct spi_dt_spec spi_spec;
    AS5047P_handle sensor;
    bool useUVW;
    int uvwPolePairs;
} as5x47_config;

#endif //CAROLO_APP_AS5X47_H
