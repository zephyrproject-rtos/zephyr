/**
 * @file AS5047P_Util.h
 * @author Jonas Merkle [JJM] (jonas@jjm.one)
 * @brief This headerfile contains util functions for the AS5047P Library.
 * @version 2.1.5
 * @date 2021-04-10
 * 
 * @copyright Copyright (c) 2021 Jonas Merkle. This project is released under the GPL-3.0 License License.
 * 
 */

#ifndef AS5047P_Util_h
#define AS5047P_Util_h

#include <inttypes.h>

#if defined(ARDUINO_ARCH_SAMD) || defined(CORE_TEENSY)
#include <string>
#include <sstream>
#endif

/**
 * @namespace AS5047P_Util
 * @brief The Namespace for some util function for the AS5047P sensor.
 */
namespace AS5047P_Util {

    /**
     * Checks if a data package has an even number of ones or not.
     * @param data The data package.
     * @return True if the number of ones in the data package is even, else false.
     */
    static inline bool hasEvenNoOfBits(uint16_t data) {

        data ^= data >> 8;              // example for 8-bir (this line scales it up to 16 bit)
        data ^= data >> 4;              // ( a b c d e f g h ) xor ( 0 0 0 0 a b c d ) = ( a b c d ae bf cg dh )
        data ^= data >> 2;              // ( a b c d ae bf cg dh ) xor ( 0 0 a b c d ae bf ) = ( a b ac bd ace bdf aceg bdfh )
        data ^= data >> 1;              // ( a b ac bd ace bdf aceg bdfh ) xor ( 0 a b ac bd ace bdf aceg ) = ( a ab abc abcd abcde abcdef abcdefg abcdefgh )
        return (bool) ((~data) & 1);    // if lsb of data is 0 -> data is even. if lsb of data is 1 -> data is odd. 

    }

    /**
     * Checks if the parity information in a data package is correct.
     * @param rawData The raw data package.
     * @return True if the parity information is correct, else false.
     */
    static inline bool parityCheck(uint16_t rawData) {

        return ((bool) ~(rawData & (1 << 15)) && (bool) hasEvenNoOfBits(rawData & 0x7FFF));

    }

    #if defined(ARDUINO_ARCH_SAMD) || defined(CORE_TEENSY)

    /**
     * @brief Convert a value to a string (see std::to_string)
     * 
     * @tparam T The type of the value to convert.
     * @param value The value to convert.
     * @return std::string The result string.
     */
    template <typename T>
    std::string to_string(const T& value) {
        std::stringstream ss;
        ss << value;
        return ss.str();
    }

    #endif
}

#endif // AS5047P_Util_h
