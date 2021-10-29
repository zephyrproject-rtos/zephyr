/**
 * @file AS5047P.h
 * @author Jonas Merkle [JJM] (jonas@jjm.one)
 * @brief This is the main sourcefile of the AS5047P Library.
 * @version 2.1.5
 * @date 2021-04-10
 * 
 * @copyright Copyright (c) 2021 Jonas Merkle. This project is released under the GPL-3.0 License License.
 * 
 */

#include "AS5047P.h"

#include "util/AS5047P_Util.h"

// Constructors ------------------------------------------------

AS5047P::AS5047P(struct spi_dt_spec spi_spec) : __spiInterface(spi_spec) {

}

// -------------------------------------------------------------

// Init --------------------------------------------------------

bool AS5047P::checkSPICon() {

    // test write to an readonly register (error register)
    __spiInterface.write(AS5047P_Types::ERRFL_t::REG_ADDRESS, 0x0007);

    // read the error register (should contain an error)
    AS5047P_Types::ERRFL_t errorReg = read_ERRFL();

    // if error register contains no errors something is not right.
    return (
            errorReg.data.values.FRERR == 0 &&
            errorReg.data.values.INVCOMM == 0 &&
            errorReg.data.values.PARERR == 1
    );

}

bool AS5047P::initSPI() {

    __spiInterface.init();

    return checkSPICon();

}

// -------------------------------------------------------------

// Util --------------------------------------------------------

bool AS5047P::checkForComErrorF(AS5047P_Types::ERROR_t *errorOut) {

    // read the error reg
    AS5047P_Types::ERROR_t e;
    auto errorReg = AS5047P::read_ERRFL(&e, true, false, false);

    // write error info from current communication in errorOut
    errorOut->controllerSideErrors.flags.CONT_SPI_PARITY_ERROR = e.controllerSideErrors.flags.CONT_SPI_PARITY_ERROR;

    // write the ERRFL register content in errorOut
    errorOut->sensorSideErrors.flags.SENS_SPI_FRAMING_ERROR |= errorReg.data.values.FRERR;
    errorOut->sensorSideErrors.flags.SENS_SPI_INVALID_CMD |= errorReg.data.values.INVCOMM;
    errorOut->sensorSideErrors.flags.SENS_SPI_PARITY_ERROR |= errorReg.data.values.PARERR;

    // check for no errors
    if (!errorReg.data.values.FRERR &&
        !errorReg.data.values.INVCOMM &&
        !errorReg.data.values.PARERR &&
        !errorOut->controllerSideErrors.flags.CONT_SPI_PARITY_ERROR) {
        return true;
    } else {
        return false;
    }
}

bool AS5047P::checkForSensorErrorF(AS5047P_Types::ERROR_t *errorOut) {

    // read the diag reg
    AS5047P_Types::ERROR_t e;
    auto diagReg = AS5047P::read_DIAAGC(&e, true, false, false);

    // write error info from current communication in errorOut
    errorOut->controllerSideErrors.flags.CONT_SPI_PARITY_ERROR = e.controllerSideErrors.flags.CONT_SPI_PARITY_ERROR;

    // write the ERRFL register content in errorOut
    errorOut->sensorSideErrors.flags.SENS_CORDIC_OVERFLOW_ERROR |= diagReg.data.values.COF;
    errorOut->sensorSideErrors.flags.SENS_OFFSET_COMP_ERROR |= ~diagReg.data.values.LF;         // bit flip: O = not compensated -> error 
    errorOut->sensorSideErrors.flags.SENS_MAG_TOO_HIGH |= diagReg.data.values.MAGH;
    errorOut->sensorSideErrors.flags.SENS_MAG_TOO_LOW |= diagReg.data.values.MAGL;

    // check for no errors
    if (!diagReg.data.values.COF &&
        !diagReg.data.values.LF &&
        !diagReg.data.values.MAGH &&
        !diagReg.data.values.MAGL &&
        !errorOut->controllerSideErrors.flags.CONT_SPI_PARITY_ERROR) {
        return true;
    } else {
        return false;
    }
}

bool AS5047P::verifyWittenRegF(uint16_t regAddress, uint16_t expectedData) {

    // check parity of expected data
    if (!AS5047P_Util::parityCheck(expectedData)) {
        return false;
    }

    // send read command
    AS5047P_Types::SPI_Command_Frame_t readCMD(regAddress, AS5047P_TYPES_READ_CMD);

    // read register content
    AS5047P_Types::SPI_ReadData_Frame_t recData(__spiInterface.read(readCMD.data.raw));

    // check parity of received data
    if (!AS5047P_Util::parityCheck(recData.data.raw)) {
        return false;
    }

    // check read reg data and expected data and return the result
    return recData.data.raw == expectedData;
}

#if defined(ARDUINO_ARCH_SAMD) || defined(CORE_TEENSY)

std::string AS5047P::readStatusAsStdString() {

    AS5047P_Types::ERRFL_t errorReg = read_ERRFL();
    AS5047P_Types::DIAAGC_t diagReg = read_DIAAGC();

    std::string str;
    str.reserve(AS5047P_INFO_STRING_BUFFER_SIZE);

    str.append("#########################\n");
    str.append(" Error Information:\n");
    str.append("-------------------------\n");
    str.append("- Framing error:   ");
    str.append(AS5047P_Util::to_string(errorReg.data.values.FRERR));
    str.append("\n");
    str.append("- Invalid command: ");
    str.append(AS5047P_Util::to_string(errorReg.data.values.INVCOMM));
    str.append("\n");
    str.append("- Parity error:    ");
    str.append(AS5047P_Util::to_string(errorReg.data.values.PARERR));
    str.append("\n");
    str.append("#########################\n");
    str.append(" Diagnostic Information: \n");
    str.append("-------------------------\n");
    str.append("- AGC Value:       ");
    str.append(AS5047P_Util::to_string(diagReg.data.values.AGC));
    str.append("\n");
    str.append("- Offset comp.:    ");
    str.append(AS5047P_Util::to_string(diagReg.data.values.LF));
    str.append("\n");
    str.append("- CORDIC overflow: ");
    str.append(AS5047P_Util::to_string(diagReg.data.values.COF));
    str.append("\n");
    str.append("- MAG too high:    ");
    str.append(AS5047P_Util::to_string(diagReg.data.values.MAGH));
    str.append("\n");
    str.append("- MAG too low:     ");
    str.append(AS5047P_Util::to_string(diagReg.data.values.MAGL));
    str.append("\n");
    str.append("#########################\n");

    str.shrink_to_fit();

    return str;

}
#endif


// -------------------------------------------------------------

// Read High-Level ---------------------------------------------

uint16_t AS5047P::readMagnitude(AS5047P_Types::ERROR_t *errorOut, bool verifyParity, bool checkForComError,
                                bool checkForSensorError) {

    AS5047P_Types::MAG_t res = AS5047P::read_MAG(errorOut, verifyParity, checkForComError, checkForSensorError);
    return res.data.values.CMAG;

}

uint16_t
AS5047P::readAngleRaw(bool withDAEC, AS5047P_Types::ERROR_t *errorOut, bool verifyParity, bool checkForComError,
                      bool checkForSensorError) {

    if (withDAEC) {
        AS5047P_Types::ANGLECOM_t res = AS5047P::read_ANGLECOM(errorOut, verifyParity, checkForComError,
                                                               checkForSensorError);
        return res.data.values.DAECANG;
    } else {
        AS5047P_Types::ANGLEUNC_t res = AS5047P::read_ANGLEUNC(errorOut, verifyParity, checkForComError,
                                                               checkForSensorError);
        return res.data.values.CORDICANG;
    }

}

float
AS5047P::readAngleDegree(bool withDAEC, AS5047P_Types::ERROR_t *errorOut, bool verifyParity, bool checkForComError,
                         bool checkForSensorError) {

    if (withDAEC) {
        AS5047P_Types::ANGLECOM_t res = AS5047P::read_ANGLECOM(errorOut, verifyParity, checkForComError,
                                                               checkForSensorError);
        return (res.data.values.DAECANG / (float) 16384) * 360;
    } else {
        AS5047P_Types::ANGLEUNC_t res = AS5047P::read_ANGLEUNC(errorOut, verifyParity, checkForComError,
                                                               checkForSensorError);
        return (res.data.values.CORDICANG / (float) 16384) * 360;
    }

}

// -------------------------------------------------------------

// Template functions ------------------------------------------

template<class T>
T
AS5047P::readReg(AS5047P_Types::ERROR_t *errorOut, bool verifyParity, bool checkForComError, bool checkForSensorError) {

    // send read command
    AS5047P_Types::SPI_Command_Frame_t readCMD(T::REG_ADDRESS, AS5047P_TYPES_READ_CMD);

    // read data
    AS5047P_Types::SPI_ReadData_Frame_t recData(__spiInterface.read(readCMD.data.raw));

    if (errorOut == nullptr) {
        return T(recData.data.raw);
    }

    // reset error data
    *errorOut = AS5047P_Types::ERROR_t();

    // verify parity bit
    if (verifyParity) {
        errorOut->controllerSideErrors.flags.CONT_SPI_PARITY_ERROR = !AS5047P_Util::parityCheck(recData.data.raw);
    }

    // check for communication error
    if (checkForComError) {
        checkForComErrorF(errorOut);
    }

    // check for sensor error
    if (checkForSensorError) {
        checkForSensorErrorF(errorOut);

        // check for communication error
        if (checkForComError) {
            checkForComErrorF(errorOut);
        }
    }

    return T(recData.data.raw);

}

template<class T>
bool
AS5047P::writeReg(const T *regData, AS5047P_Types::ERROR_t *errorOut, bool checkForComError, bool verifyWittenReg) {

    // write register data
    __spiInterface.write(T::REG_ADDRESS, regData->data.raw);

    if (errorOut == nullptr) {
        return true;
    }

    // reset error data
    *errorOut = AS5047P_Types::ERROR_t();

    // check for communication error
    if (checkForComError) {
        checkForComErrorF(errorOut);
    }

    // check for sensor error
    if (verifyWittenReg) {
        checkForSensorErrorF(errorOut);

        // check for communication error
        if (checkForComError) {
            checkForComErrorF(errorOut);
        }
    }

    // check error information and return
    return errorOut->noError();;

}

// -------------------------------------------------------------

// Read Volatile Registers -------------------------------------

auto AS5047P::read_ERRFL(AS5047P_Types::ERROR_t *errorOut, bool verifyParity, bool checkForComError,
                         bool checkForSensorError) -> AS5047P_Types::ERRFL_t {

    return readReg<AS5047P_Types::ERRFL_t>(errorOut, verifyParity, checkForComError, checkForSensorError);

}

auto AS5047P::read_PROG(AS5047P_Types::ERROR_t *errorOut, bool verifyParity, bool checkForComError,
                        bool checkForSensorError) -> AS5047P_Types::PROG_t {

    return readReg<AS5047P_Types::PROG_t>(errorOut, verifyParity, checkForComError, checkForSensorError);

}

auto AS5047P::read_DIAAGC(AS5047P_Types::ERROR_t *errorOut, bool verifyParity, bool checkForComError,
                          bool checkForSensorError) -> AS5047P_Types::DIAAGC_t {

    return readReg<AS5047P_Types::DIAAGC_t>(errorOut, verifyParity, checkForComError, checkForSensorError);

}

auto AS5047P::read_MAG(AS5047P_Types::ERROR_t *errorOut, bool verifyParity, bool checkForComError,
                       bool checkForSensorError) -> AS5047P_Types::MAG_t {

    return readReg<AS5047P_Types::MAG_t>(errorOut, verifyParity, checkForComError, checkForSensorError);

}

auto AS5047P::read_ANGLEUNC(AS5047P_Types::ERROR_t *errorOut, bool verifyParity, bool checkForComError,
                            bool checkForSensorError) -> AS5047P_Types::ANGLEUNC_t {

    return readReg<AS5047P_Types::ANGLEUNC_t>(errorOut, verifyParity, checkForComError, checkForSensorError);

}

auto AS5047P::read_ANGLECOM(AS5047P_Types::ERROR_t *errorOut, bool verifyParity, bool checkForComError,
                            bool checkForSensorError) -> AS5047P_Types::ANGLECOM_t {

    return readReg<AS5047P_Types::ANGLECOM_t>(errorOut, verifyParity, checkForComError, checkForSensorError);

}

// -------------------------------------------------------------

// Write Volatile Registers ------------------------------------

bool AS5047P::write_PROG(const AS5047P_Types::PROG_t *regData, AS5047P_Types::ERROR_t *errorOut, bool checkForComError,
                         bool verifyWittenReg) {

    return writeReg<AS5047P_Types::PROG_t>(regData, errorOut, checkForComError, verifyWittenReg);

}

// -------------------------------------------------------------

// Read Non-Volatile Registers ---------------------------------

auto AS5047P::read_ZPOSM(AS5047P_Types::ERROR_t *errorOut, bool verifyParity, bool checkForComError,
                         bool checkForSensorError) -> AS5047P_Types::ZPOSM_t {

    return readReg<AS5047P_Types::ZPOSM_t>(errorOut, verifyParity, checkForComError, checkForSensorError);

}

auto AS5047P::read_ZPOSL(AS5047P_Types::ERROR_t *errorOut, bool verifyParity, bool checkForComError,
                         bool checkForSensorError) -> AS5047P_Types::ZPOSL_t {

    return readReg<AS5047P_Types::ZPOSL_t>(errorOut, verifyParity, checkForComError, checkForSensorError);

}

auto AS5047P::read_SETTINGS1(AS5047P_Types::ERROR_t *errorOut, bool verifyParity, bool checkForComError,
                             bool checkForSensorError) -> AS5047P_Types::SETTINGS1_t {

    return readReg<AS5047P_Types::SETTINGS1_t>(errorOut, verifyParity, checkForComError, checkForSensorError);

}

auto AS5047P::read_SETTINGS2(AS5047P_Types::ERROR_t *errorOut, bool verifyParity, bool checkForComError,
                             bool checkForSensorError) -> AS5047P_Types::SETTINGS2_t {

    return readReg<AS5047P_Types::SETTINGS2_t>(errorOut, verifyParity, checkForComError, checkForSensorError);
}

// -------------------------------------------------------------

// Write Non-Volatile Registers --------------------------------

bool
AS5047P::write_ZPOSM(const AS5047P_Types::ZPOSM_t *regData, AS5047P_Types::ERROR_t *errorOut, bool checkForComError,
                     bool verifyWittenReg) {

    return writeReg<AS5047P_Types::ZPOSM_t>(regData, errorOut, checkForComError, verifyWittenReg);

}

bool
AS5047P::write_ZPOSL(const AS5047P_Types::ZPOSL_t *regData, AS5047P_Types::ERROR_t *errorOut, bool checkForComError,
                     bool verifyWittenReg) {

    return writeReg<AS5047P_Types::ZPOSL_t>(regData, errorOut, checkForComError, verifyWittenReg);

}

bool AS5047P::write_SETTINGS1(const AS5047P_Types::SETTINGS1_t *regData, AS5047P_Types::ERROR_t *errorOut,
                              bool checkForComError, bool verifyWittenReg) {

    return writeReg<AS5047P_Types::SETTINGS1_t>(regData, errorOut, checkForComError, verifyWittenReg);

}

bool AS5047P::write_SETTINGS2(const AS5047P_Types::SETTINGS2_t *regData, AS5047P_Types::ERROR_t *errorOut,
                              bool checkForComError, bool verifyWittenReg) {

    return writeReg<AS5047P_Types::SETTINGS2_t>(regData, errorOut, checkForComError, verifyWittenReg);

}

// -------------------------------------------------------------
