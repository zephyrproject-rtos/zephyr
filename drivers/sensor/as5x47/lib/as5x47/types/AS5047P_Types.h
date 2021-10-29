/**
 * @file AS5047P_Types.h
 * @author Jonas Merkle [JJM] (jonas@jjm.one)
 * @brief This headerfile contains type definitions for the AS5047P Library.
 * @version 2.1.5
 * @date 2021-04-10
 * 
 * @copyright Copyright (c) 2021 Jonas Merkle. This project is released under the GPL-3.0 License License.
 * 
 */

#ifndef AS5047P_TYPES_h
#define AS5047P_TYPES_h

#include <inttypes.h>

#if defined(ARDUINO_ARCH_SAMD) || defined(CORE_TEENSY)
#include <string>
#endif


#define AS5047P_TYPES_WRITE_CMD 0       ///< Write command flag.
#define AS5047P_TYPES_READ_CMD 1        ///< Read command flag.

#define AS5047P__TYPES_ERROR_STRING_BUFFER_SIZE 600     ///< buffer size for error string

/**
 * @namespace AS5047P_Types
 * @brief The namespace for all custom types needed for the AS5047P sensor.
 */
namespace AS5047P_Types {

    // Errors ------------------------------------------------------

    /**
     * @enum ERROR_Names
     * @brief Enum that holds the different error names an there according bit number in the raw error information byte.
     */
    enum ERROR_Names : uint8_t {

        SENS_SPI_FRAMING_ERROR = 1,
        SENS_SPI_INVALID_CMD = 2,
        SENS_SPI_PARITY_ERROR = 4,

        SENS_OFFSET_COMP_ERROR = 8,
        SENS_CORDIC_OVERFLOW_ERROR = 16,
        SENS_MAG_TOO_HIGH = 32,
        SENS_MAG_TOO_LOW = 64,

        CONT_SPI_PARITY_ERROR = 1,
        CONT_GENERAL_COM_ERROR = 2,
        CONT_WRITE_VERIFY_FAILED = 4,

    };

    /**
     * @class ERROR_t
     * @brief Provides a representation for a "ERROR Information".
     */
    class ERROR_t {

    public:

        /**
         * @typedef SensorSideErrors_t
         * @brief Provides a new datatype for "Sensor Side Errors".
         */
        typedef union {

            /**
             * @typedef SensorSideErrorsFlags_t
             * @brief Provides a new datatype for "Sensor Side Error Flags".
             */
            typedef struct __attribute__ ((__packed__)) {

                uint8_t SENS_SPI_FRAMING_ERROR: 1;           ///< Framing error: is set to 1 when a non-compliant SPI frame is detected.
                uint8_t SENS_SPI_INVALID_CMD: 1;             ///< Invalid command error: set to 1 by reading or writing an invalid register address.
                uint8_t SENS_SPI_PARITY_ERROR: 1;            ///< Parity error

                uint8_t SENS_OFFSET_COMP_ERROR: 1;           ///< Diagnostics: Offset compensation LF=0:internal offset loops not ready regulated LF=1:internal offset loop finished.
                uint8_t SENS_CORDIC_OVERFLOW_ERROR: 1;       ///< Diagnostics: CORDIC overflow.
                uint8_t SENS_MAG_TOO_HIGH: 1;                ///< Diagnostics: Magnetic field strength too high; AGC=0x00.
                uint8_t SENS_MAG_TOO_LOW: 1;                 ///< Diagnostics: Magnetic field strength too low; AGC=0xFF.

            } SensorSideErrorsFlags_t;

            uint8_t raw = 0;                            ///< Error data (RAW).
            SensorSideErrorsFlags_t flags;              ///< Error data.

        } SensorSideErrors_t;

        /**
         * @typedef ControllerSideErrors_t
         * @brief Provides a new datatype for "Controller Side Errors".
         */
        typedef union {

            /**
             * @typedef ControllerSideErrorsFlags_t
             * @brief Provides a new datatype for "Controller Side Error Flags".
             */
            typedef struct __attribute__ ((__packed__)) {

                uint8_t CONT_SPI_PARITY_ERROR: 1;        ///< Parity error.

                uint8_t CONT_GENERAL_COM_ERROR: 1;       ///< An error occurred during the communication with the sensor. See sensor side errors for more information.

                uint8_t CONT_WRITE_VERIFY_FAILED: 1;     ///< Could not verify the new content of a written register.

            } ControllerSideErrorsFlags_t;

            uint8_t raw = 0;                            ///< Error data (RAW).
            ControllerSideErrorsFlags_t flags;          ///< Error data.

        } ControllerSideErrors_t;

        SensorSideErrors_t sensorSideErrors;            ///< The actual sensor side error data of a "ERROR Information".
        ControllerSideErrors_t controllerSideErrors;    ///< The actual controller side error data data of a "ERROR Information".

        /**
         * Main Constructor.
         * @param sensorSideErrorsRaw The sensor side error raw data (default: 0).
         * @param controllerSideErrorsRaw The controller side error raw data (default: 0).
         */
        ERROR_t(uint8_t sensorSideErrorsRaw = 0, uint8_t controllerSideErrorsRaw = 0);

        /**
         * Checks if no error occurred.
         * @return True on success, else false.
         */
        bool noError();

#if defined(ARDUINO_ARCH_SAMD) || defined(CORE_TEENSY)

        /**
         * Converts the error information into an human readable string.
         * @return A std::string with all error information.
         */
        std::string toStdString();

#endif

    };

    // -------------------------------------------------------------

    // SPI Frames --------------------------------------------------

    /**
     * @class SPI_Command_Frame_t
     * @brief Provides a representation for a "SPI Command Frame".
     */
    class SPI_Command_Frame_t {

    public:

        /**
         * @typedef SPI_Command_Frame_data_t
         * @brief Provides a new datatype for the data of a "SPI Command Frame".
         */
        typedef union {

            /**
             * @typedef SPI_Command_Frame_values_t
             * @brief Provides a new datatype for the single values of a "SPI Command Frame".
             */
            typedef struct __attribute__ ((__packed__)) {

                uint16_t ADDR: 14;       ///< Address to read or write.
                uint16_t RW: 1;          ///< 0: Write 1: Read.
                uint16_t PARC: 1;        ///< Parity bit (even) calculated on the lower 15 bits of command frame.

            } SPI_Command_Frame_values_t;

            uint16_t raw = 0;                       ///< Register values (RAW).
            SPI_Command_Frame_values_t values;      ///< Register values.

        } SPI_Command_Frame_data_t;

        SPI_Command_Frame_data_t data;              ///< The actual data of a "SPI Command Frame".

        /**
         * Constructor.
         * @param raw Two bytes of raw data.
         */
        SPI_Command_Frame_t(uint16_t raw);

        /**
         * Constructor.
         * @param ADDR 14 bit address.
         * @param RW 0: Write 1: Read.
         */
        SPI_Command_Frame_t(uint16_t ADDR, uint16_t RW);

    };


    /**
     * @class SPI_ReadData_Frame_t
     * @brief Provides a representation for a "SPI Read Data Frame".
     */
    class SPI_ReadData_Frame_t {

    public:

        /**
         * @typedef SPI_ReadData_Frame_data_t
         * @brief Provides a new datatype for the data of a "SPI Read Data Frame".
         */
        typedef union {

            /**
             * @typedef SPI_ReadData_Frame_t
             * @brief Provides a new datatype for the single values of a "SPI Read Data Frame".
             */
            typedef struct __attribute__ ((__packed__)) {

                uint16_t DATA: 14;       ///< Address to read or write.
                uint16_t EF: 1;          ///< 0: No command frame error occurred 1: Error occurred.
                uint16_t PARD: 1;        ///< Parity bit (even) calculated on the lower 15 bits.

            } SPI_ReadData_Frame_values_t;

            uint16_t raw = 0;                       ///< Register values (RAW).
            SPI_ReadData_Frame_values_t values;     ///< Register values.

        } SPI_ReadData_Frame_data_t;

        SPI_ReadData_Frame_data_t data;             ///< The actual data of a "SPI Read Data Frame".

        /**
         * Constructor.
         * @param raw Two bytes of raw data.
         */
        SPI_ReadData_Frame_t(uint16_t raw);

        /**
         * Constructor.
         * @param ADDR 14 bit data.
         * @param EF 0: No command frame error occurred 1: Error occurred.
         */
        SPI_ReadData_Frame_t(uint16_t ADDR, uint16_t EF);

    };


    /**
     * @class SPI_WriteData_Frame_t
     * @brief Provides a representation for a "SPI Write Data Frame".
     */
    class SPI_WriteData_Frame_t {

    public:

        /**
         * @typedef SPI_WriteData_Frame_data_t
         * @brief Provides a new datatype for the data of a "SPI Write Data Frame".
         */
        typedef union {

            /**
             * @typedef SPI_WriteData_Frame_t
             * @brief Provides a new datatype for the single values of a "SPI Write Data Frame".
             */
            typedef struct __attribute__ ((__packed__)) {

                uint16_t DATA: 1;        ///< Address to read or write.
                uint16_t NC: 1;          ///< Always low.
                uint16_t PARD: 1;        ///< Parity bit (even)

            } SPI_WriteData_Frame_values_t;

            uint16_t raw = 0;                       ///< Register values (RAW).
            SPI_WriteData_Frame_values_t values;    ///< Register values.

        } SPI_WriteData_Frame_data_t;

        SPI_WriteData_Frame_data_t data;            ///< The actual data of a "SPI Write Data Frame".

        /**
         * Constructor.
         * @param raw Two bytes of raw data.
         */
        SPI_WriteData_Frame_t(uint16_t raw);

        /**
         * Constructor.
         * @param ADDR 14 bit data.
         * @param NC Always low (0).
         */
        SPI_WriteData_Frame_t(uint16_t ADDR, uint16_t NC);

    };

    // -------------------------------------------------------------


    // Volatile Registers ------------------------------------------

    /**
     * @class NOP_t
     * @brief Provides a representation of the no operation register of the AS5047P.
     */
    class NOP_t {

    public:

        static const uint16_t REG_ADDRESS = 0x0000;     ///< Register address.

    };

    /**
     * @class ERRFL_t
     * @brief Provides a representation of the error register of the AS5047P.
     */
    class ERRFL_t {

    public:

        /**
         * @typedef ERRFL_data_t
         * @brief Provides a new datatype for the data of a the ERRFL register.
         */
        typedef union {

            /**
             * @typedef ERRFL_values_t
             * @brief Provides a new datatype for the single values of the ERRFL register.
             */
            typedef struct __attribute__ ((__packed__)) {

                uint16_t FRERR: 1;       ///< Framing error: is set to 1 when a non-compliant SPI frame is detected.
                uint16_t INVCOMM: 1;     ///< Invalid command error: set to 1 by reading or writing an invalid register address.
                uint16_t PARERR: 1;      ///< Parity error.

            } ERRFL_values_t;

            uint16_t raw = 0;           ///< Register values (RAW).
            ERRFL_values_t values;      ///< Register values.

        } ERRFL_data_t;

        static const uint16_t REG_ADDRESS = 0x0001;     ///< Register address.
        static const uint16_t REG_DEFAULT = 0x0000;     ///< Register default values.

        ERRFL_data_t data;                ///< The actual data of the register.

        /**
         * Default constructor.
         */
        ERRFL_t();

        /**
         * Constructor.
         * @param raw Two bytes of raw data.
         */
        ERRFL_t(uint16_t raw);

    };

    /**
     * @class PROG_t
     * @brief Provides a representation of the programming register register of the AS5047P.
     */
    class PROG_t {

    public:

        /**
         * @typedef PROG_data_t
         * @brief Provides a new datatype for the data of a the PROG register.
         */
        typedef union {

            /**
             * @typedef PROG_values_t
             * @brief Provides a new datatype for the single values of the PROG register.
             */
            typedef struct __attribute__ ((__packed__)) {

                uint16_t PROGEN: 1;      ///< Program OTP enable: enables programming the entire OTP memory.
                uint16_t OTPREF: 1;      ///< Refreshes the non-volatile memory content with the OTP programmed content.
                uint16_t PROGOTP: 1;     ///< Start OTP programming cycle.
                uint16_t PROGVER: 1;     ///< Program verify: must be set to 1 for verifying the correctness of the OTP programming.

            } PROG_values_t;

            uint16_t raw = 0;           ///< Register values (RAW).
            PROG_values_t values;       ///< Register values.

        } PROG_data_t;

        static const uint16_t REG_ADDRESS = 0x0003;     ///< Register address.
        static const uint16_t REG_DEFAULT = 0x0000;     ///< Register default values.

        PROG_data_t data;                               ///< The actual data of the register.

        /**
         * Default constructor.
         */
        PROG_t();

        /**
         * Constructor.
         * @param raw Two bytes of raw data.
         */
        PROG_t(uint16_t raw);

    };

    /**
     * @class DIAAGC_t
     * @brief Provides a representation of the diagnostic and AGC register of the AS5047P.
     */
    class DIAAGC_t {

    public:

        /**
         * @typedef DIAAGC_data_t
         * @brief Provides a new datatype for the data of a the DIAAGC register.
         */
        typedef union {

            /**
             * @typedef DIAAGC_values_t
             * @brief Provides a new datatype for the single values of the DIAAGC register.
             */
            typedef struct __attribute__ ((__packed__)) {

                uint16_t AGC: 8;     ///< Automatic gain control value.
                uint16_t LF: 1;      ///< Diagnostics: Offset compensation LF=0:internal offset loops not ready regulated LF=1:internal offset loop finished.
                uint16_t COF: 1;     ///< Diagnostics: CORDIC overflow.
                uint16_t MAGH: 1;    ///< Diagnostics: Magnetic field strength too high; AGC=0x00.
                uint16_t MAGL: 1;    ///< Diagnostics: Magnetic field strength too low; AGC=0xFF.

            } DIAAGC_values_t;

            uint16_t raw = 0;           ///< Register values (RAW).
            DIAAGC_values_t values;     ///< Register values.

        } DIAAGC_data_t;

        static const uint16_t REG_ADDRESS = 0x3FFC;     ///< Register address.
        static const uint16_t REG_DEFAULT = 0x0180;     ///< Register default values.

        DIAAGC_data_t data;                             ///< The actual data of the register.

        /**
         * Default constructor.
         */
        DIAAGC_t();

        /**
         * Constructor.
         * @param raw Two bytes of raw data.
         */
        DIAAGC_t(uint16_t raw);

    };

    /**
     * @class MAG_t
     * @brief Provides a representation of the CORDIC magnitude register of the AS5047P.
     */
    class MAG_t {

    public:

        /**
         * @typedef MAG_data_t
         * @brief Provides a new datatype for the data of the MAG register.
         */
        typedef union {

            /**
             * @typedef MAG_values_t
             * @brief Provides a new datatype for the single values of the MAG register.
             */
            typedef struct __attribute__ ((__packed__)) {

                uint16_t CMAG: 14;       ///< CORDIC magnitude information.

            } MAG_values_t;

            uint16_t raw = 0;           ///< Register values (RAW).
            MAG_values_t values;        ///< Register values.

        } MAG_data_t;

        static const uint16_t REG_ADDRESS = 0x3FFD;     ///< Register address.
        static const uint16_t REG_DEFAULT = 0x0000;     ///< Register default values.

        MAG_data_t data;                                ///< The actual data of the register.

        /**
         * Default constructor.
         */
        MAG_t();

        /**
         * Constructor.
         * @param raw Two bytes of raw data.
         */
        MAG_t(uint16_t raw);

    };

    /**
     * @class ANGLEUNC_t
     * @brief Provides a representation of the measured angle without dynamic angle error compensation register of the AS5047P.
     */
    class ANGLEUNC_t {

    public:

        /**
         * @typedef ANGLEUNC_data_t
         * @brief Provides a new datatype for the data of the ANGLEUNC register.
         */
        typedef union {

            /**
             * @typedef ANGLEUNC_values_t
             * @brief Provides a new datatype for the single values of the ANGLEUNC register.
             */
            typedef struct __attribute__ ((__packed__)) {

                uint16_t CORDICANG: 14;      ///< Angle information without dynamic angle error compensation.

            } ANGLEUNC_values_t;

            uint16_t raw = 0;               ///< Register values (RAW).
            ANGLEUNC_values_t values;       ///< Register values.

        } ANGLEUNC_data_t;

        static const uint16_t REG_ADDRESS = 0x3FFE;     ///< Register address.
        static const uint16_t REG_DEFAULT = 0x0000;     ///< Register default values.

        ANGLEUNC_data_t data;                           ///< The actual data of the register.

        /**
         * Default constructor.
         */
        ANGLEUNC_t();

        /**
         * Constructor.
         * @param raw Two bytes of raw data.
         */
        ANGLEUNC_t(uint16_t raw);

    };

    /**
     * @class ANGLECOM_t
     * @brief Provides a representation of the measured angle with dynamic angle error compensation register of the AS5047P.
     */
    class ANGLECOM_t {

    public:

        /**
         * @typedef ANGLECOM_data_t
         * @brief Provides a new datatype for the data of the ANGLECOM register.
         */
        typedef union {

            /**
             * @typedef ANGLECOM_values_t
             * @brief Provides a new datatype for the single values of the ANGLECOM register.
             */
            typedef struct __attribute__ ((__packed__)) {

                uint16_t DAECANG: 14;        ///< Angle information with dynamic angle error compensation.

            } ANGLECOM_values_t;

            uint16_t raw = 0;               ///< Register values (RAW).
            ANGLECOM_values_t values;       ///< Register values.

        } ANGLECOM_data_t;

        static const uint16_t REG_ADDRESS = 0x3FFF;     ///< Register address.
        static const uint16_t REG_DEFAULT = 0x0000;     ///< Register default values.

        ANGLECOM_data_t data;                           ///< The actual data of the register.

        /**
         * Default constructor.
         */
        ANGLECOM_t();

        /**
         * Constructor.
         * @param raw Two bytes of raw data.
         */
        ANGLECOM_t(uint16_t raw);

    };

    // -------------------------------------------------------------

    // Non-Volatile Registers --------------------------------------

    /**
     * @class ZPOSM_t
     * @brief Provides a representation of the zero position MSB register of the AS5047P.
     */
    class ZPOSM_t {

    public:

        /**
         * @typedef ZPOSM_data_t
         * @brief Provides a new datatype for the data of the ZPOSM register.
         */
        typedef union {

            /**
             * @typedef ZPOSM_values_t
             * @brief Provides a new datatype for the single values of the ZPOSM register.
             */
            typedef struct __attribute__ ((__packed__)) {

                uint16_t ZPOSM: 8;       ///< 8 most significant bits of the zero position.

            } ZPOSM_values_t;

            uint16_t raw = 0;           ///< Register values (RAW).
            ZPOSM_values_t values;      ///< Register values.

        } ZPOSM_data_t;

        static const uint16_t REG_ADDRESS = 0x0016;     ///< Register address.
        static const uint16_t REG_DEFAULT = 0x0000;     ///< Register default values.

        ZPOSM_data_t data;                              ///< The actual data of the register.

        /**
         * Default constructor.
         */
        ZPOSM_t();

        /**
         * Constructor.
         * @param raw Two bytes of raw data.
         */
        ZPOSM_t(uint16_t raw);

    };

    /**
     * @class ZPOSL_t
     * @brief Provides a representation of the zero position LSB /MAG diagnostic register of the AS5047P.
     */
    class ZPOSL_t {

    public:

        /**
         * @typedef ZPOSL_data_t
         * @brief Provides a new datatype for the data of the ZPOSL register.
         */
        typedef union {

            /**
             * @typedef ZPOSL_values_t
             * @brief Provides a new datatype for the single values of the ZPOSL register.
             */
            typedef struct __attribute__ ((__packed__)) {

                uint16_t ZPOSL: 6;               ///< 6 least significant bits of the zero position.
                uint16_t comp_l_error_en: 1;     ///< This bit enables the contribution of MAGH (Magnetic field strength too high) to the error flag.
                uint16_t comp_h_error_en: 1;     ///< This bit enables the contribution of MAGL (Magnetic field strength too low) to the error flag.

            } ZPOSL_values_t;

            uint16_t raw = 0;           ///< Register values (RAW).
            ZPOSL_values_t values;      ///< Register values.

        } ZPOSL_data_t;

        static const uint16_t REG_ADDRESS = 0x0017;     ///< Register address.
        static const uint16_t REG_DEFAULT = 0x0000;     ///< Register default values.

        ZPOSL_data_t data;                              ///< The actual data of the register.

        /**
         * Default constructor.
         */
        ZPOSL_t();

        /**
         * Constructor.
         * @param raw Two bytes of raw data.
         */
        ZPOSL_t(uint16_t raw);

    };

    /**
     * @class SETTINGS1_t
     * @brief Provides a representation of the custom setting register 1 of the AS5047P.
     */
    class SETTINGS1_t {

    public:

        /**
         * @typedef SETTINGS1_data_t
         * @brief Provides a new datatype for the data of the SETTINGS1 register.
         */
        typedef union {

            /**
             * @typedef SETTINGS1_values_t
             * @brief Provides a new datatype for the single values of the SETTINGS1 register.
             */
            typedef struct __attribute__ ((__packed__)) {

                uint16_t FactorySetting: 1;  ///< Pre-Programmed to 1.
                uint16_t NOISESET: 1;        ///< Noise settings.
                uint16_t DIR: 1;             ///< Rotation direction.
                uint16_t UVW_ABI: 1;         ///< Defines the PWM Output (0 = ABI is operating, W is used as PWM 1 = UVW is operating, I is used as PWM).
                uint16_t DAECDIS: 1;         ///< Disable Dynamic Angle Error Compensation (0 = DAE compensation ON, 1 = DAE compensation OFF).
                uint16_t ABIBIN: 1;          ///< ABI decimal or binary selection of the ABI pulses per revolution.
                uint16_t Dataselect: 1;      ///< This bit defines which data can be read form address 16383dec (3FFFhex). 0->DAECANG 1->CORDICANG.
                uint16_t PWMon: 1;           ///< Enables PWM (setting of UVW_ABI Bit necessary).

            } SETTINGS1_values_t;

            uint16_t raw = 0;           ///< Register values (RAW).
            SETTINGS1_values_t values;  ///< Register values.

        } SETTINGS1_data_t;

        static const uint16_t REG_ADDRESS = 0x0018;     ///< Register address.
        static const uint16_t REG_DEFAULT = 0x0001;     ///< Register default values.

        SETTINGS1_data_t data;                          ///< The actual data of the register.

        /**
         * Default constructor.
         */
        SETTINGS1_t();

        /**
         * Constructor.
         * @param raw Two bytes of raw data.
         */
        SETTINGS1_t(uint16_t raw);

    };

    /**
     * @class SETTINGS2_t
     * @brief Provides a representation of the custom setting register 2 of the AS5047P.
     */
    class SETTINGS2_t {

    public:

        /**
         * @typedef SETTINGS2_data_t
         * @brief Provides a new datatype for the data of the SETTINGS2 register.
         */
        typedef union {

            /**
             * @typedef SETTINGS2_values_t
             * @brief Provides a new datatype for the single values of the SETTINGS2 register.
             */
            typedef struct __attribute__ ((__packed__)) {

                uint16_t UVWPP: 3;       ///< UVW number of pole pairs (000 = 1, 001 = 2, 010 = 3, 011 = 4, 100 = 5, 101 = 6, 110 = 7, 111 = 7).
                uint16_t HYS: 2;         ///< Hysteresis setting.
                uint16_t ABIRES: 3;      ///< Resolution of ABI.

            } SETTINGS2_values_t;

            uint16_t raw = 0;           ///< Register values (RAW).
            SETTINGS2_values_t values;  ///< Register values.

        } SETTINGS2_data_t;

        static const uint16_t REG_ADDRESS = 0x0019;     ///< Register address.
        static const uint16_t REG_DEFAULT = 0x0000;     ///< Register default values.

        SETTINGS2_data_t data;                          ///< The actual data of the register.

        /**
         * Default constructor.
         */
        SETTINGS2_t();

        /**
         * Constructor.
         * @param raw Two bytes of raw data.
         */
        SETTINGS2_t(uint16_t raw);

    };

    // -------------------------------------------------------------
}

#endif // AS5047P_TYPES_h
