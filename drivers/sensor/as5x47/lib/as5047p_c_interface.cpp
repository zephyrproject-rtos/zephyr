//
// Created by jonasotto on 11/2/21.
//

#include <AS5047P/src/AS5047P.h>
#include "as5047p_c_interface.h"

extern "C" {
float readAngleDegree(AS5047P_handle h, bool withDAEC, AS5047P_ERROR *errorOut, bool verifyParity,
                      bool checkForComError, bool checkForSensorError) {
    AS5047P dev(h);
    AS5047P_Types::ERROR_t error;
    auto angle = dev.readAngleDegree(withDAEC, &error, verifyParity, checkForComError, checkForSensorError);
    errorOut->controllerSideErrors.raw = error.controllerSideErrors.raw;
    errorOut->sensorSideErrors.raw = errorOut->sensorSideErrors.raw;
    return angle;
}


bool noError(const AS5047P_ERROR *error) {
    return error->sensorSideErrors.raw == 0 && error->controllerSideErrors.raw == 0;
}

bool initSPI(AS5047P_handle h) {
    AS5047P dev(h);
    return dev.initSPI();
}
}
