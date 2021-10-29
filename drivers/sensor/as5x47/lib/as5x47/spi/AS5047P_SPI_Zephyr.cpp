/**
 * @file AS5047P_SPI_Arduino.cpp
 * @author Jonas Merkle [JJM] (jonas@jjm.one)
 * @brief This sourefile contains the implementation of the Arduino SPI bus handler for the AS5047P Library.
 * @version 2.1.5
 * @date 2021-04-10
 * 
 * @copyright Copyright (c) 2021 Jonas Merkle. This project is released under the GPL-3.0 License License.
 * 
 */

#include "AS5047P_SPI_Zephyr.h"

#include <logging/log.h>
#include "as5x47/types/AS5047P_Types.h"

LOG_MODULE_REGISTER(as5047, LOG_LEVEL_INF);

namespace AS5047P_ComBackend {

    AS5047P_SPI::AS5047P_SPI(struct spi_dt_spec spi_spec) :
            spi_spec(spi_spec) {}

    void AS5047P_SPI::init() {
        if (!spi_is_ready(&spi_spec)) {
            LOG_ERR("Bus not ready");
        }
    }

    void AS5047P_SPI::write(const uint16_t regAddress, const uint16_t data) {
        const struct spi_buf tx_buf[2] = {
                {
                        // Always send register address first
                        .buf = (void *) &regAddress,
                        .len = sizeof(regAddress)
                },
                {
                        // Then send whatever data there is to send
                        .buf = (void *) &data,
                        .len = sizeof(data)
                }
        };
        const struct spi_buf_set tx = {
                .buffers = tx_buf,
                .count = 2
        };

        // Write-only: No RX buffer necessary
        auto err = spi_write_dt(&spi_spec, &tx);
        if (err != 0) {
            LOG_ERR("Error reading from SPI: %d", err);
        }
    }


    uint16_t AS5047P_SPI::read(const uint16_t regAddress) {
        uint16_t read_val;
        const struct spi_buf tx_buf[2] = {
                {
                        // Always send register address first
                        .buf = (void *) &regAddress,
                        .len = sizeof(regAddress)
                },
                {
                        // Then send whatever data there is to send
                        .buf = &read_val,
                        .len = sizeof(read_val)
                }
        };
        const struct spi_buf_set tx = {
                .buffers = tx_buf,
                .count = 2
        };

        const struct spi_buf_set rx = {
                .buffers = tx_buf,
                .count = 2
        };

        auto err = spi_transceive_dt(&spi_spec, &tx, &rx);
        if (err != 0) {
            LOG_ERR("Error reading from SPI: %d", err);
        }
        return read_val;
    }
}
