/* Copyright (c) 2016, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice, this
 *      list of conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *
 *   3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *      contributors may be used to endorse or promote products derived from
 *      this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/**
 * @brief This module contains generic 802.15.4 radio driver for nRF SoC devices.
 *
 */

#ifndef NRF_RADIO802154_H_
#define NRF_RADIO802154_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize 802.15.4 driver.
 *
 * @note This function shall be called once, before any other function from this module.
 *
 * Initialize radio peripheral to Sleep state.
 */
void nrf_drv_radio802154_init(void);

/**
 * @brief Get channel on which the radio operates right now.
 */
uint8_t nrf_drv_radio802154_channel_get(void);

/**
 * @section Setting addresses and Pan Id of this device.
 */

/**
 * @brief Set PAN Id used by this device.
 *
 * @param[in]  p_pan_id  Pointer to PAN Id (2 bytes, little-endian).
 *
 * This function makes copy of the PAN Id.
 */
void nrf_drv_radio802154_pan_id_set(const uint8_t *p_pan_id);

/**
 * @brief Set Extended Address of this device.
 *
 * @param[in]  p_extended_address  Pointer to extended address (8 bytes, little-endian).
 *
 * This function makes copy of the address.
 */
void nrf_drv_radio802154_extended_address_set(const uint8_t *p_extended_address);

/**
 * @brief Set Short Address of this device.
 *
 * @param[in]  p_short_address  Pointer to short address (2 bytes, little-endian).
 *
 * This function makes copy of the address.
 */
void nrf_drv_radio802154_short_address_set(const uint8_t *p_short_address);


/**
 * @section Functions to request FSM transitions.
 *
 *          receive()       transmit()
 *          -------->       -------->
 *     Sleep         Receive         Transmit
 *          <-------- |  /|\<--------
 *           sleep()  |   |   receive() / transmitted() / busy_channel()
 *                    |   |
 * energy_detection() |   | energy_detected()
 *                   \|/  |
 *               Energy detection
 */

/**
 * @brief Change radio state to Sleep.
 *
 * @note This function should be called only if radio is in Receive state.
 *
 * Sleep state is the lowest power state. In this state radio cannot transmit or receive frames.
 *
 * @return  true   If the radio changes it's state to low power mode.
 * @return  false  If the driver could not schedule changing state.
 */
bool nrf_drv_radio802154_sleep(void);

/**
 * @brief Change radio state to Receive.
 *
 * @note This function should be called in Sleep or Transmit state.
 *
 * In Receive state radio receives frames and automatically sends ACK frames when appropriate.
 * Received frame is reported to higher layer by nrf_radio802154_received() call.
 *
 * @param[in] channel   Channel number on which radio will receive.
 * @param[in] force_rx  If set to true then function is allowed to stop an ongoing
 *                      operation. If set to false then the function will succeed
 *                      only when radio is in RX or SLEEP state.
 *
 * @return  true   If the reception procedure was scheduled.
 * @return  false  If the driver could not schedule the reception procedure.
 */
bool nrf_drv_radio802154_receive(uint8_t channel, bool force_rx);

/**
 * @brief Change radio state to Transmit.
 *
 * @note This function should be called in Receive state. In other states transmission will be
 *       scheduled.
 *
 * In Transmit state radio transmits given frame. If requested it waits for ACK frame.
 * Radio driver wait infinitely for ACK frame. Higher layer is responsible to call
 * nrf_radio802154_receive() after ACK timeout.
 * Transmission result is reported to higher layer by nrf_radio802154_transmitted() or
 * nrf_radio802154_busy_channel() calls.
 *
 * @param[in]  p_data   Pointer to array containing data to transmit. First byte should contain
 *                      frame length and following bytes should contain data. CRC is computed
 *                      automatically by radio hardware and can contain any bytes.
 * @param[in]  channel  Channel number on which radio will transmit given frame.
 * @param[in]  power    Transmission power [dBm]. Given value is rounded up to nearest permitted
 *                      value.
 *
 * @return  true   If the transmission procedure was scheduled.
 * @return  false  If the driver could not schedule the transmission procedure.
 */
bool nrf_drv_radio802154_transmit(const uint8_t *p_data, uint8_t channel, int8_t power);

/**
 * @brief Change radio state to Energy Detection.
 *
 * @note This function should be called in Receive state. In other states energy detection
 *       procedure will be scheduled.
 *
 * In Energy Detection state radio detects maximum energy for given time. Result of the detection
 * is reported to the higher layer by nrf_radio802154_energy_detected() call.
 *
 * @param[in]  channel  Channel number on which radio will detect energy.
 * @param[in]  time_us  Duration of energy detection procedure. Given value is rounded up to
 *                      multiplication of 10s (128 us).
 *
 * @return  true   If the energy detection procedure was scheduled.
 * @return  false  If the driver could not schedule the energy detection procedure.
 */
bool nrf_drv_radio802154_energy_detection(uint8_t channel, uint32_t time_us);


/**
 * @section Calls to higher layer.
 */

/**
 * @brief Notify that frame was received.
 *
 * @note Buffer pointed by the p_data pointer is not modified by the radio driver (and can't
 *       be used to receive a frame) until nrf_drv_radio802154_buffer_free() function is called.
 * @note Buffer pointed by the p_data pointer may be modified by the function handler (and other
 *       modules) until nrf_drv_radio802154_buffer_free() function is called.
 *
 * @param[in]  p_data  Pointer to buffer containing received data. First byte in the buffer is
 *                     length of the frame and following bytes is the frame itself (after PHR).
 * @param[in]  power   RSSI of received frame.
 * @param[in]  lqi     LQI of received frame.
 */
void nrf_drv_radio802154_received(uint8_t * p_data, int8_t power, int8_t lqi);

/**
 * @brief Notify that frame was transmitted.
 *
 * @note If ACK was requested for transmitted frame this function is called after proper ACK is
 *       received. If ACK was not requested this function is called just after transmission is
 *       ended.
 *
 * @param[in]  pending_bit  Value of pending bit in received ACK or false if ACK was not requested.
 */
void nrf_drv_radio802154_transmitted(bool pending_bit);

/**
 * @brief Notify that frame was not transmitted due to busy channel.
 *
 * This function is called if CCA procedure (performed just before transmission) fails.
 */
void nrf_drv_radio802154_busy_channel(void);

/**
 * @brief Notify that Energy Detection procedure finished.
 *
 * @param[in]  result  Maximum energy detected during Energy Detection procedure.
 */
void nrf_drv_radio802154_energy_detected(int8_t result);


/**
 * @section Driver memory management
 */

/**
 * @brief Notify driver that buffer containing received frame is not used anymore.
 *
 * @note The buffer pointed by the @p p_data pointer may be modified by this function.
 *
 * @param[in]  p_data  A pointer to the buffer containing received data that is no more needed by
 *                     the higher layer.
 */
void nrf_drv_radio802154_buffer_free(uint8_t * p_data);


/**
 * @section RSSI measurement function.
 */

/**
 * @brief Begin RSSI measurement.
 *
 * @note This function should be called in Receive state.
 *
 * Begin RSSI measurement. The result will be available in 8 uS. The result can be read by
 * nrf_radio802154_rssi_last_get() function.
 */
void nrf_drv_radio802154_rssi_measure(void);

/**
 * @brief Get result of last RSSI measurement.
 *
 * @returns RSSI measurement result [dBm].
 */
int8_t nrf_drv_radio802154_rssi_last_get(void);


/**
 * @section Promiscuous mode.
 */

/**
 * @brief Enable or disable promiscuous radio mode.
 *
 * In promiscuous mode driver notifies higher layer that it received any frame (regardless
 * frame type or destination address).
 * In normal mode (not promiscuous) higher layer is not notified about ACK frames and frames with
 * unknown type. Also frames with destination address not matching this device address are ignored.
 *
 * @param[in]  enabled  If promiscuous mode should be enabled.
 */
void nrf_drv_radio802154_promiscuous_set(bool enabled);

/**
 * @brief Check if radio is in promiscuous mode.
 *
 * @retval True   Radio is in promiscuous mode.
 * @retval False  Radio is not in promiscuous mode.
 */
bool nrf_drv_radio802154_promiscuous_get(void);


/**
 * @section Setting pending bit in automatically transmitted ACK frames.
 */

/**
 * @brief Enable or disable setting pending bit in automatically transmitted ACK frames.
 *
 * Radio driver automatically sends ACK frames in response to unicast frames destined to this node.
 * Pending bit in ACK frame can be set or cleared regarding data in pending buffer destined to ACK
 * destination.
 *
 * If setting pending bit in ACK frames is disabled, pending bit in every ACK frame is set.
 * If setting pending bit in ACK frames is enabled, radio driver checks if there is data
 * in pending buffer destined to ACK destination. If there is no such data, pending bit is cleared.
 *
 * @note It is possible that if there is a lot of supported peers radio driver cannot verify
 *       if there is pending data before ACK is sent. In this case pending bit is set.
 *
 * @param[in]  enabled  If setting pending bit in ACK frames is enabled.
 */
void nrf_drv_radio802154_auto_pending_bit_set(bool enabled);

/**
 * @brief Add address of peer node for which there is pending data in the buffer.
 *
 * @note This function makes a copy of given address.
 *
 * @param[in]  p_addr    Array of bytes containing address of the node (little-endian).
 * @param[in]  extended  If given address is Extended MAC Address or Short MAC Address.
 *
 * @retval True   Address successfully added to the list.
 * @retval False  There is not enough memory to store this address in the list.
 */
bool nrf_drv_radio802154_pending_bit_for_addr_set(const uint8_t *p_addr, bool extended);

/**
 * @brief Remove address of peer node for which there is no more pending data in the buffer.
 *
 * @param[in]  p_addr    Array of bytes containing address of the node (little-endian).
 * @param[in]  extended  If given address is Extended MAC Address or Short MAC Address.
 *
 * @retval True   Address successfully removed from the list.
 * @retval False  There is no such address in the list.
 */
bool nrf_drv_radio802154_pending_bit_for_addr_clear(const uint8_t *p_addr, bool extended);

/**
 * @brief Remove all addresses of given type from pending bit list.
 *
 * @param[in]  extended  If function should remove all Exnteded MAC Adresses of all Short Addresses.
 */
void nrf_drv_radio802154_pending_bit_for_addr_reset(bool extended);

/**
 * @brief Radio IRQ handler.
 */
void nrf_drv_radio802154_irq_handler(void);

#ifdef __cplusplus
}
#endif

#endif /* NRF_RADIO802154_H_ */
