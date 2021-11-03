//
// Created by jonasotto on 11/2/21.
//

#ifndef CAROLO_APP_AS5047P_C_INTERFACE_H
#define CAROLO_APP_AS5047P_C_INTERFACE_H


#ifdef __cplusplus
extern "C" {
#endif

typedef struct __attribute__ ((__packed__)) {
    uint8_t SENS_SPI_FRAMING_ERROR: 1;
    uint8_t SENS_SPI_INVALID_CMD: 1;
    uint8_t SENS_SPI_PARITY_ERROR: 1;

    uint8_t SENS_OFFSET_COMP_ERROR: 1;
    uint8_t SENS_CORDIC_OVERFLOW_ERROR: 1;
    uint8_t SENS_MAG_TOO_HIGH: 1;
    uint8_t SENS_MAG_TOO_LOW: 1;
} SensorSideErrorsFlags_t;

typedef union SensorSideErrors_t {
    uint8_t raw;
    SensorSideErrorsFlags_t flags;
} SensorSideErrors_t;

typedef struct __attribute__ ((__packed__)) {
    uint8_t CONT_SPI_PARITY_ERROR: 1;
    uint8_t CONT_GENERAL_COM_ERROR: 1;
    uint8_t CONT_WRITE_VERIFY_FAILED: 1;
} ControllerSideErrorsFlags_t;

typedef union ControllerSideErrors_t {
    uint8_t raw;
    ControllerSideErrorsFlags_t flags;
} ControllerSideErrors_t;

typedef struct AS5047P_ERROR {
    SensorSideErrors_t sensorSideErrors;
    ControllerSideErrors_t controllerSideErrors;
} AS5047P_ERROR;

bool noError(const AS5047P_ERROR *error);

typedef const struct spi_dt_spec *AS5047P_handle;

bool initSPI(AS5047P_handle h);

/**
 * @param withDAEC default: true
 * @param errorOut default: NULL
 * @param verifyParity default: false
 * @param checkForComError default: false
 * @param checkForSensorError default: false
 */
float readAngleDegree(AS5047P_handle h, bool withDAEC, AS5047P_ERROR *errorOut, bool verifyParity,
                      bool checkForComError, bool checkForSensorError);


#ifdef __cplusplus
}
#endif
#endif //CAROLO_APP_AS5047P_C_INTERFACE_H
