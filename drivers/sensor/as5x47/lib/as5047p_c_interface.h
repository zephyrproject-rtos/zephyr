//
// Created by jonasotto on 11/2/21.
//

#ifndef CAROLO_APP_AS5047P_C_INTERFACE_H
#define CAROLO_APP_AS5047P_C_INTERFACE_H
#ifdef __cplusplus
extern "C" {
#endif

// TODO: c wrapper, this is a dummy struct, spi_dt_spec will probably be sufficient
typedef struct AS5047P_handle {
    int i;
} AS5047P_handle;

int getSensorValue(const AS5047P_handle *h);

#ifdef __cplusplus
}
#endif
#endif //CAROLO_APP_AS5047P_C_INTERFACE_H
