//
// Created by jonasotto on 11/2/21.
//

#ifndef CAROLO_APP_AS5047P_C_INTERFACE_H
#define CAROLO_APP_AS5047P_C_INTERFACE_H

#include "as5047p_c_types.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef const struct spi_dt_spec *AS5047P_handle;

// Init --------------------------------------------------------

/**
 *
 * @param h Sensor instance
 * @param useUVW Use UVW instead of ABI output
 * @param polePairs Number of motor pole pairs for UVW output (between 1 and 7)
 * @return true on success
 */
bool initializeSensor(AS5047P_handle h, bool useUVW, int polePairs);

// Util --------------------------------------------------------

//std::string readStatusAsStdString(AS5047P_handle h);

// -------------------------------------------------------------

// Read High-Level ---------------------------------------------

bool readMagnitude(AS5047P_handle h, uint16_t *magnitudeOut,
                   bool verifyParity, bool checkForComError, bool checkForSensorError);

bool readAngleRaw(AS5047P_handle h, uint16_t *angleOut, bool withDAEC,
                  bool verifyParity, bool checkForComError, bool checkForSensorError);

bool readAngleDegree(AS5047P_handle h, float *angleOut, bool withDAEC,
                     bool verifyParity, bool checkForComError, bool checkForSensorError);

// -------------------------------------------------------------

// Read Volatile Registers -------------------------------------

bool read_ERRFL(AS5047P_handle h, as5047p_ERRFL_data_t *regData,
                bool verifyParity, bool checkForComError, bool checkForSensorError);

bool read_PROG(AS5047P_handle h, as5047p_PROG_data_t *regData,
               bool verifyParity, bool checkForComError, bool checkForSensorError);

bool read_DIAAGC(AS5047P_handle h, as5047p_DIAAGC_data_t *regData,
                 bool verifyParity, bool checkForComError, bool checkForSensorError);

bool read_MAG(AS5047P_handle h, as5047p_MAG_data_t *regData,
              bool verifyParity, bool checkForComError, bool checkForSensorError);

bool read_ANGLEUNC(AS5047P_handle h, as5047p_ANGLEUNC_data_t *regData,
                   bool verifyParity, bool checkForComError, bool checkForSensorError);

bool read_ANGLECOM(AS5047P_handle h, as5047p_ANGLECOM_data_t *regData,
                   bool verifyParity, bool checkForComError, bool checkForSensorError);

// -------------------------------------------------------------

// Write Volatile Registers ------------------------------------

bool write_PROG(AS5047P_handle h, const as5047p_PROG_data_t *regData, bool checkForComError, bool verifyWrittenReg);

// -------------------------------------------------------------

// Read Non-Volatile Registers ---------------------------------

bool read_ZPOSM(AS5047P_handle h, as5047p_ZPOSM_data_t *regData,
                bool verifyParity, bool checkForComError, bool checkForSensorError);

bool read_ZPOSL(AS5047P_handle h, as5047p_ZPOSL_data_t *regData,
                bool verifyParity, bool checkForComError, bool checkForSensorError);

bool read_SETTINGS1(AS5047P_handle h, as5047p_SETTINGS1_data_t *regData,
                    bool verifyParity, bool checkForComError, bool checkForSensorError);

bool read_SETTINGS2(AS5047P_handle h, as5047p_SETTINGS2_data_t *regData,
                    bool verifyParity, bool checkForComError, bool checkForSensorError);

// -------------------------------------------------------------

// Write Non-Volatile Registers --------------------------------

bool write_ZPOSM(AS5047P_handle h, const as5047p_ZPOSM_data_t *regData, bool checkForComError, bool verifyWrittenReg);

bool write_ZPOSL(AS5047P_handle h, const as5047p_ZPOSL_data_t *regData, bool checkForComError, bool verifyWrittenReg);

bool write_SETTINGS1(AS5047P_handle h, const as5047p_SETTINGS1_data_t *regData,
                     bool checkForComError, bool verifyWittenReg);

bool write_SETTINGS2(AS5047P_handle h, const as5047p_SETTINGS2_data_t *regData,
                     bool checkForComError, bool verifyWrittenReg);

// -------------------------------------------------------------

#ifdef __cplusplus
}
#endif
#endif //CAROLO_APP_AS5047P_C_INTERFACE_H
