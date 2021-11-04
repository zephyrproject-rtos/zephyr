//
// Created by jonasotto on 11/2/21.
//

#ifndef CAROLO_APP_AS5047P_C_INTERFACE_H
#define CAROLO_APP_AS5047P_C_INTERFACE_H


#ifdef __cplusplus
extern "C" {
#endif

typedef const struct spi_dt_spec *AS5047P_handle;

bool initSPI(AS5047P_handle h);

float readAngleDegree(AS5047P_handle h, bool withDAEC, bool *errorOut,
                      bool verifyParity, bool checkForComError, bool checkForSensorError);


#ifdef __cplusplus
}
#endif
#endif //CAROLO_APP_AS5047P_C_INTERFACE_H
