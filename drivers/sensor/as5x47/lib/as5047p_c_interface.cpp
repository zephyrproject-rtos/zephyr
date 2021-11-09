//
// Created by jonasotto on 11/2/21.
//

#include "as5047p_c_interface.h"

#include <logging/log.h>
#include <AS5047P/src/AS5047P.h>

LOG_MODULE_REGISTER(as5047p_c_interface, LOG_LEVEL_INF);

/**
 * Log error.
 * @param error
 * @param functionName
 * @param description
 * @return
 */
bool handleError(const AS5047P_Types::ERROR_t &error, const char *functionName, const char *description) {
    if (!error.noError()) {
        LOG_ERR("%s, %s: C_GENERAL_COM_ERR: %d, C_SPI_PARITY_ERR: %d, C_WRITE_VERIFY_FAILED: %d, S_CORDIC_OVERFLOW_ERR: %d, "
                "S_OFFSET_COMP_ERR: %d, S_MAG_TOO_HIGH: %d, S_MAG_TOO_LOW: %d, S_SPI_FRAMING_ERR: %d, "
                "S_SPI_INVALID_CMD: %d, S_SPI_PARITY_ERR: %d",
                functionName, description,
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
    return error.noError();
}

#define HANDLE_ERROR_DESC(error, description) handleError((error), __func__, (description))
#define HANDLE_ERROR(error) handleError((error), __func__, "")

extern "C" {

// Init --------------------------------------------------------

bool initializeSensor(const AS5047P_handle h, bool useUVW, int polePairs) {
    AS5047P dev(h);
    if (!dev.initSPI()) {
        LOG_ERR("SPI connection test failed");
        return false;
    }
    LOG_INF("SPI connection test succeeded");

    AS5047P_Types::ERROR_t err;

    AS5047P_Types::SETTINGS1_t s1;
    s1.data.values.UVW_ABI = useUVW ? 1 : 0;
    LOG_INF("Writing SETTINGS1=0x%04x: UVW_ABI = %d",s1.data.raw, s1.data.values.UVW_ABI);
    if (!dev.write_SETTINGS1(&s1, &err, true, true)) {
        HANDLE_ERROR_DESC(err, "settings1");
        return false;
    }

    AS5047P_Types::SETTINGS2_t s2;
    if (polePairs < 1 or polePairs > 7) {
        LOG_ERR("Number of pole pairs %d is not between 1 and 7", polePairs);
        return false;
    }
    s2.data.values.UVWPP = (polePairs - 1) & 0b111;
    LOG_INF("Writing SETTINGS2: UVWPP = %d", s2.data.values.UVWPP);
    if (!dev.write_SETTINGS2(&s2, &err, true, true)) {
        HANDLE_ERROR_DESC(err, "settings2");
        return false;
    }
    return true;
}

// -------------------------------------------------------------

// Read High-Level ---------------------------------------------

bool readMagnitude(const AS5047P_handle h, uint16_t *magnitudeOut, bool verifyParity, bool checkForComError,
                   bool checkForSensorError) {
    AS5047P dev(h);
    AS5047P_Types::ERROR_t error;
    *magnitudeOut = dev.readMagnitude(&error, verifyParity, checkForComError, checkForSensorError);
    return HANDLE_ERROR(error);
}

bool readAngleRaw(const AS5047P_handle h, uint16_t *angleOut, bool withDAEC, bool verifyParity, bool checkForComError,
                  bool checkForSensorError) {
    AS5047P dev(h);
    AS5047P_Types::ERROR_t error;
    *angleOut = dev.readAngleRaw(withDAEC, &error, verifyParity, checkForComError, checkForSensorError);
    return HANDLE_ERROR(error);
}

bool readAngleDegree(const AS5047P_handle h, float *angleOut, bool withDAEC, bool verifyParity, bool checkForComError,
                     bool checkForSensorError) {
    AS5047P dev(h);
    AS5047P_Types::ERROR_t error{};
    *angleOut = dev.readAngleDegree(false, &error, false, false, false);
    return HANDLE_ERROR(error);
}

// -------------------------------------------------------------

// Read Volatile Registers -------------------------------------

bool read_ERRFL(const AS5047P_handle h, as5047p_ERRFL_data_t *regData, bool verifyParity, bool checkForComError,
                bool checkForSensorError) {
    AS5047P dev(h);
    AS5047P_Types::ERROR_t error;
    if (regData != nullptr) {
        regData->raw = dev.read_ERRFL(&error, verifyParity, checkForComError, checkForSensorError).data.raw;
    }

    return HANDLE_ERROR(error);
}

bool read_PROG(const AS5047P_handle h, as5047p_PROG_data_t *regData, bool verifyParity, bool checkForComError,
               bool checkForSensorError) {
    AS5047P dev(h);
    AS5047P_Types::ERROR_t error;
    if (regData != nullptr) {
        regData->raw = dev.read_PROG(&error, verifyParity, checkForComError, checkForSensorError).data.raw;
    }

    return HANDLE_ERROR(error);
}

bool read_DIAAGC(const AS5047P_handle h, as5047p_DIAAGC_data_t *regData, bool verifyParity, bool checkForComError,
                 bool checkForSensorError) {
    AS5047P dev(h);
    AS5047P_Types::ERROR_t error;
    if (regData != nullptr) {
        regData->raw = dev.read_DIAAGC(&error, verifyParity, checkForComError, checkForSensorError).data.raw;
    }

    return HANDLE_ERROR(error);
}

bool read_MAG(const AS5047P_handle h, as5047p_MAG_data_t *regData, bool verifyParity, bool checkForComError,
              bool checkForSensorError) {
    AS5047P dev(h);
    AS5047P_Types::ERROR_t error;
    if (regData != nullptr) {
        regData->raw = dev.read_MAG(&error, verifyParity, checkForComError, checkForSensorError).data.raw;
    }

    return HANDLE_ERROR(error);
}

bool read_ANGLEUNC(const AS5047P_handle h, as5047p_ANGLEUNC_data_t *regData, bool verifyParity, bool checkForComError,
                   bool checkForSensorError) {
    AS5047P dev(h);
    AS5047P_Types::ERROR_t error;
    if (regData != nullptr) {
        regData->raw = dev.read_ANGLEUNC(&error, verifyParity, checkForComError, checkForSensorError).data.raw;
    }

    return HANDLE_ERROR(error);
}

bool read_ANGLECOM(const AS5047P_handle h, as5047p_ANGLECOM_data_t *regData, bool verifyParity, bool checkForComError,
                   bool checkForSensorError) {
    AS5047P dev(h);
    AS5047P_Types::ERROR_t error;
    if (regData != nullptr) {
        regData->raw = dev.read_ANGLECOM(&error, verifyParity, checkForComError, checkForSensorError).data.raw;
    }

    return HANDLE_ERROR(error);
}

// -------------------------------------------------------------

// Write Volatile Registers ------------------------------------

bool
write_PROG(const AS5047P_handle h, const as5047p_PROG_data_t *regData, bool checkForComError, bool verifyWrittenReg) {
    if (regData == nullptr) {
        return false;
    }
    AS5047P_Types::PROG_t reg;
    reg.data.raw = regData->raw;
    AS5047P dev(h);
    AS5047P_Types::ERROR_t error;
    bool res = dev.write_PROG(&reg, &error, checkForComError, verifyWrittenReg);
    HANDLE_ERROR(error);

    return res;
}

// -------------------------------------------------------------

// Read Non-Volatile Registers ---------------------------------

bool read_ZPOSM(const AS5047P_handle h, as5047p_ZPOSM_data_t *regData, bool verifyParity, bool checkForComError,
                bool checkForSensorError) {
    AS5047P dev(h);
    AS5047P_Types::ERROR_t error;
    if (regData != nullptr) {
        regData->raw = dev.read_ZPOSM(&error, verifyParity, checkForComError, checkForSensorError).data.raw;
    }

    return HANDLE_ERROR(error);
}

bool read_ZPOSL(const AS5047P_handle h, as5047p_ZPOSL_data_t *regData, bool verifyParity, bool checkForComError,
                bool checkForSensorError) {
    AS5047P dev(h);
    AS5047P_Types::ERROR_t error;
    if (regData != nullptr) {
        regData->raw = dev.read_ZPOSL(&error, verifyParity, checkForComError, checkForSensorError).data.raw;
    }

    return HANDLE_ERROR(error);
}

bool read_SETTINGS1(const AS5047P_handle h, as5047p_SETTINGS1_data_t *regData, bool verifyParity, bool checkForComError,
                    bool checkForSensorError) {
    AS5047P dev(h);
    AS5047P_Types::ERROR_t error;
    if (regData != nullptr) {
        regData->raw = dev.read_SETTINGS1(&error, verifyParity, checkForComError, checkForSensorError).data.raw;
    }

    return HANDLE_ERROR(error);
}

bool read_SETTINGS2(const AS5047P_handle h, as5047p_SETTINGS2_data_t *regData, bool verifyParity, bool checkForComError,
                    bool checkForSensorError) {
    AS5047P dev(h);
    AS5047P_Types::ERROR_t error;
    if (regData != nullptr) {
        regData->raw = dev.read_SETTINGS2(&error, verifyParity, checkForComError, checkForSensorError).data.raw;
    }

    return HANDLE_ERROR(error);
}

// -------------------------------------------------------------

// Write Non-Volatile Registers --------------------------------

bool
write_ZPOSM(const AS5047P_handle h, const as5047p_ZPOSM_data_t *regData, bool checkForComError, bool verifyWrittenReg) {
    if (regData == nullptr) {
        return false;
    }
    AS5047P_Types::ZPOSM_t reg;
    reg.data.raw = regData->raw;
    AS5047P dev(h);
    AS5047P_Types::ERROR_t error;
    bool res = dev.write_ZPOSM(&reg, &error, checkForComError, verifyWrittenReg);
    HANDLE_ERROR(error);

    return res;
}

bool
write_ZPOSL(const AS5047P_handle h, const as5047p_ZPOSL_data_t *regData, bool checkForComError, bool verifyWrittenReg) {
    if (regData == nullptr) {
        return false;
    }
    AS5047P_Types::ZPOSL_t reg;
    reg.data.raw = regData->raw;
    AS5047P dev(h);
    AS5047P_Types::ERROR_t error;
    bool res = dev.write_ZPOSL(&reg, &error, checkForComError, verifyWrittenReg);
    HANDLE_ERROR(error);

    return res;
}

bool write_SETTINGS1(const AS5047P_handle h, const as5047p_SETTINGS1_data_t *regData, bool checkForComError,
                     bool verifyWittenReg) {
    if (regData == nullptr) {
        return false;
    }
    AS5047P_Types::SETTINGS1_t reg;
    reg.data.raw = regData->raw;
    AS5047P dev(h);
    AS5047P_Types::ERROR_t error;
    bool res = dev.write_SETTINGS1(&reg, &error, checkForComError, verifyWittenReg);
    HANDLE_ERROR(error);

    return res;
}

bool write_SETTINGS2(const AS5047P_handle h, const as5047p_SETTINGS2_data_t *regData, bool checkForComError,
                     bool verifyWrittenReg) {
    if (regData == nullptr) {
        return false;
    }
    AS5047P_Types::SETTINGS2_t reg;
    reg.data.raw = regData->raw;
    AS5047P dev(h);
    AS5047P_Types::ERROR_t error;
    bool res = dev.write_SETTINGS2(&reg, &error, checkForComError, verifyWrittenReg);
    HANDLE_ERROR(error);

    return res;
}

// -------------------------------------------------------------

}
