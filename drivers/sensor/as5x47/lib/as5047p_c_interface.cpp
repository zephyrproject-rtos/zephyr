//
// Created by jonasotto on 11/2/21.
//

#include "as5047p_c_interface.h"

#include <logging/log.h>
#include <AS5047P/src/AS5047P.h>

LOG_MODULE_REGISTER(as5047p_c_interface, LOG_LEVEL_WRN);

extern "C" {

// Init --------------------------------------------------------

bool initSPI(const AS5047P_handle h) {
	AS5047P dev(h);
	return dev.initSPI();
}

// Util --------------------------------------------------------

//std::string readStatusAsStdString(const AS5047P_handle h) {

}

// -------------------------------------------------------------

// Read High-Level ---------------------------------------------

uint16_t readMagnitude(const AS5047P_handle h, bool *errorOut, bool verifyParity, bool checkForComError, bool checkForSensorError) {
	AS5047P dev(h);
	AS5047P_Types::ERROR_t error;
	auto mag = dev.readMagnitude(withDAEC, &error, verifyParity, checkForComError, checkForSensorError);
	if (errorOut != nullptr) {
		*errorOut = !error.noError();
	}
	if (!error.noError()) {
		LOG_ERR(error.toString());
	}

	return mag;
}

uint16_t readAngleRaw(const AS5047P_handle h, bool withDAEC, bool *errorOut, bool verifyParity, bool checkForComError, bool checkForSensorError) {
	AS5047P dev(h);
	AS5047P_Types::ERROR_t error;
	auto angle = dev.readAngleRaw(withDAEC, &error, verifyParity, checkForComError, checkForSensorError);
	if (errorOut != nullptr) {
		*errorOut = !error.noError();
	}
	if (!error.noError()) {
		LOG_ERR(error.toString());
	}

	return angle;
}

float readAngleDegree(const AS5047P_handle h, bool withDAEC, bool *errorOut, bool verifyParity, bool checkForComError, bool checkForSensorError) {
	AS5047P dev(h);
	AS5047P_Types::ERROR_t error;
	auto angle = dev.readAngleDegree(withDAEC, &error, verifyParity, checkForComError, checkForSensorError);
	if (errorOut != nullptr) {
		*errorOut = !error.noError();
	}
	if (!error.noError()) {
		LOG_ERR(error.toString());
	}

	return angle;
}

// -------------------------------------------------------------

// Read Volatile Registers -------------------------------------

bool read_ERRFL(const AS5047P_handle h, as5047p_ERRFL_data_t *regData, bool verifyParity, bool checkForComError, bool checkForSensorError) {
	AS5047P dev(h);
	AS5047P_Types::ERROR_t error;
	if (regData != nullptr) {
		regData->raw = dev.read_ERRFL(&error, verifyParity, checkForComError, checkForSensorError).data.raw;
	}
	if (!error.noError()) {
		LOG_ERR(error.toString());
	}

	return error.noError();
}

bool read_PROG(const AS5047P_handle h, as5047p_PROG_data_t *regData, bool verifyParity, bool checkForComError, bool checkForSensorError) {
	AS5047P dev(h);
	AS5047P_Types::ERROR_t error;
	if (regData != nullptr) {
		regData->raw = dev.read_PROG(&error, verifyParity, checkForComError, checkForSensorError).data.raw;
	}
	if (!error.noError()) {
		LOG_ERR(error.toString());
	}

	return error.noError();
}

bool read_DIAAGC(const AS5047P_handle h, as5047p_DIAAGC_data_t *regData, bool verifyParity, bool checkForComError, bool checkForSensorError) {
	AS5047P dev(h);
	AS5047P_Types::ERROR_t error;
	if (regData != nullptr) {
		regData->raw = dev.read_DIAAGC(&error, verifyParity, checkForComError, checkForSensorError).data.raw;
	}
	if (!error.noError()) {
		LOG_ERR(error.toString());
	}

	return error.noError();
}

bool read_MAG(const AS5047P_handle h, as5047p_MAG_data_t *regData, bool verifyParity, bool checkForComError, bool checkForSensorError) {
	AS5047P dev(h);
	AS5047P_Types::ERROR_t error;
	if (regData != nullptr) {
		regData->raw = dev.read_MAG(&error, verifyParity, checkForComError, checkForSensorError).data.raw;
	}
	if (!error.noError()) {
		LOG_ERR(error.toString());
	}

	return error.noError();
}

bool read_ANGLEUNC(const AS5047P_handle h, as5047p_ANGLEUNC_data_t *regData, bool verifyParity, bool checkForComError, bool checkForSensorError) {
	AS5047P dev(h);
	AS5047P_Types::ERROR_t error;
	if (regData != nullptr) {
		regData->raw = dev.read_ANGLEUNC(&error, verifyParity, checkForComError, checkForSensorError).data.raw;
	}
	if (!error.noError()) {
		LOG_ERR(error.toString());
	}

	return error.noError();
}

bool read_ANGLECOM(const AS5047P_handle h, as5047p_ANGLECOM_data_t *regData, bool verifyParity, bool checkForComError, bool checkForSensorError) {
	AS5047P dev(h);
	AS5047P_Types::ERROR_t error;
	if (regData != nullptr) {
		regData->raw = dev.read_ANGLECOM(&error, verifyParity, checkForComError, checkForSensorError).data.raw;
	}
	if (!error.noError()) {
		LOG_ERR(error.toString());
	}

	return error.noError();
}

// -------------------------------------------------------------

// Write Volatile Registers ------------------------------------

bool write_PROG(const AS5047P_handle h, const as5047p_PROG_data_t *regData, bool checkForComError, bool verifyWittenReg) {
	if (regData == nullptr) {
		return false;
	}
	AS5047P_Types::PROG_t reg;
	reg.data.raw = regData->raw;
	AS5047P dev(h);
	AS5047P_Types::ERROR_t error;
	bool res = dev.write_PROG(&reg, &error, checkForComError, verifyWittenRegr);
	if (!error.noError()) {
		LOG_ERR(error.toString());
	}

	return res;
}

// -------------------------------------------------------------

// Read Non-Volatile Registers ---------------------------------

bool read_ZPOSM(const AS5047P_handle h, as5047p_ZPOSM_data_t *regData, bool verifyParity, bool checkForComError, bool checkForSensorError) {
	AS5047P dev(h);
	AS5047P_Types::ERROR_t error;
	if (regData != nullptr) {
		regData->raw = dev.read_ZPOSM(&error, verifyParity, checkForComError, checkForSensorError).data.raw;
	}
	if (!error.noError()) {
		LOG_ERR(error.toString());
	}

	return error.noError();
}

bool read_ZPOSL(const AS5047P_handle h, as5047p_ZPOSL_data_t *regData, bool verifyParity, bool checkForComError, bool checkForSensorError) {
	AS5047P dev(h);
	AS5047P_Types::ERROR_t error;
	if (regData != nullptr) {
		regData->raw = dev.read_ZPOSL(&error, verifyParity, checkForComError, checkForSensorError).data.raw;
	}
	if (!error.noError()) {
		LOG_ERR(error.toString());
	}

	return error.noError();
}

bool read_SETTINGS1(const AS5047P_handle h, as5047p_SETTINGS1_data_t *regData, bool verifyParity, bool checkForComError, bool checkForSensorError) {
	AS5047P dev(h);
	AS5047P_Types::ERROR_t error;
	if (regData != nullptr) {
		regData->raw = dev.read_SETTINGS1(&error, verifyParity, checkForComError, checkForSensorError).data.raw;
	}
	if (!error.noError()) {
		LOG_ERR(error.toString());
	}

	return error.noError();
}

bool read_SETTINGS2(const AS5047P_handle h, as5047p_SETTINGS2_data_t *regData, bool verifyParity, bool checkForComError, bool checkForSensorError) {
	AS5047P dev(h);
	AS5047P_Types::ERROR_t error;
	if (regData != nullptr) {
		regData->raw = dev.read_SETTINGS2(&error, verifyParity, checkForComError, checkForSensorError).data.raw;
	}
	if (!error.noError()) {
		LOG_ERR(error.toString());
	}

	return error.noError();
}

// -------------------------------------------------------------

// Write Non-Volatile Registers --------------------------------

bool write_ZPOSM(const AS5047P_handle h, const as5047p_ZPOSM_data_t *regData, bool checkForComError, bool verifyWittenReg) {
	if (regData == nullptr) {
		return false;
	}
	AS5047P_Types::ZPOSM_t reg;
	reg.data.raw = regData->raw;
	AS5047P dev(h);
	AS5047P_Types::ERROR_t error;
	bool res = dev.write_PROG(&reg, &error, checkForComError, verifyWittenRegr);
	if (!error.noError()) {
		LOG_ERR(error.toString());
	}

	return res;
}

bool write_ZPOSL(const AS5047P_handle h, const as5047p_ZPOSL_data_t *regData, bool checkForComError, bool verifyWittenReg) {
	if (regData == nullptr) {
		return false;
	}
	AS5047P_Types::ZPOSL_t reg;
	reg.data.raw = regData->raw;
	AS5047P dev(h);
	AS5047P_Types::ERROR_t error;
	bool res = dev.write_PROG(&reg, &error, checkForComError, verifyWittenRegr);
	if (!error.noError()) {
		LOG_ERR(error.toString());
	}

	return res;
}

bool write_SETTINGS1(const AS5047P_handle h, const as5047p_SETTINGS1_data_t *regData, bool checkForComError, bool verifyWittenReg) {
	if (regData == nullptr) {
		return false;
	}
	AS5047P_Types::SETTINGS1_t reg;
	reg.data.raw = regData->raw;
	AS5047P dev(h);
	AS5047P_Types::ERROR_t error;
	bool res = dev.write_PROG(&reg, &error, checkForComError, verifyWittenRegr);
	if (!error.noError()) {
		LOG_ERR(error.toString());
	}

	return res;
}

bool write_SETTINGS2(const AS5047P_handle h, const as5047p_SETTINGS2_data_t *regData, bool checkForComError, bool verifyWittenReg) {
	if (regData == nullptr) {
		return false;
	}
	AS5047P_Types::SETTINGS2_t reg;
	reg.data.raw = regData->raw;
	AS5047P dev(h);
	AS5047P_Types::ERROR_t error;
	bool res = dev.write_PROG(&reg, &error, checkForComError, verifyWittenRegr);
	if (!error.noError()) {
		LOG_ERR(error.toString());
	}

	return res;
}

// -------------------------------------------------------------

}
