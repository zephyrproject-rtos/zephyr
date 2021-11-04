//
// Created by jonasotto on 11/2/21.
//

#include "as5047p_c_interface.h"

#include <logging/log.h>
#include <AS5047P/src/AS5047P.h>

LOG_MODULE_REGISTER(as5047p_c_interface, LOG_LEVEL_WRN);

extern "C" {
float readAngleDegree(AS5047P_handle h, bool withDAEC, bool *errorOut, bool verifyParity,
                      bool checkForComError, bool checkForSensorError) {
    AS5047P dev(h);
    AS5047P_Types::ERROR_t error;
    auto angle = dev.readAngleDegree(withDAEC, &error, verifyParity, checkForComError, checkForSensorError);
    if (errorOut != nullptr) {
        *errorOut = !error.noError();
    }
    if (!error.noError()) {
        LOG_ERR("C_GENERAL_COM_ERR: %d, C_SPI_PARITY_ERR: %d, C_WRITE_VERIFY_FAILED: %d, S_CORDIC_OVERFLOW_ERR: %d, "
                "S_OFFSET_COMP_ERR: %d, S_MAG_TOO_HIGH: %d, S_MAG_TOO_LOW: %d, S_SPI_FRAMING_ERR: %d, "
                "S_SPI_INVALID_CMD: %d, S_SPI_PARITY_ERR: %d",
                error.controllerSideErrors.flags.CONT_GENERAL_COM_ERROR,
                error.controllerSideErrors.flags.CONT_SPI_PARITY_ERROR,
                error.controllerSideErrors.flags.CONT_WRITE_VERIFY_FAILED,
                error.sensorSideErrors.flags.SENS_CORDIC_OVERFLOW_ERROR,
                error.sensorSideErrors.flags.SENS_OFFSET_COMP_ERROR,
                error.sensorSideErrors.flags.SENS_MAG_TOO_HIGH,
                error.sensorSideErrors.flags.SENS_MAG_TOO_LOW,
                error.sensorSideErrors.flags.SENS_SPI_FRAMING_ERROR,
                error.sensorSideErrors.flags.SENS_SPI_INVALID_CMD,
                error.sensorSideErrors.flags.SENS_SPI_PARITY_ERROR);
    }

    return angle;
}

bool initSPI(AS5047P_handle h) {
    AS5047P dev(h);
    return dev.initSPI();
}
}
