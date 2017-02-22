/*
 * Copyright (c) 2017, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the Intel Corporation nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL CORPORATION OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __QM_DMA_H_
#define __QM_DMA_H_

#include "qm_common.h"
#include "qm_soc_regs.h"

/**
 * DMA Driver for Quark Microcontrollers.
 *
 * @defgroup groupDMA DMA
 * @{
 */

/**
 * DMA Handshake Polarity
 */
typedef enum {
	QM_DMA_HANDSHAKE_POLARITY_HIGH = 0x0, /**< Set HS polarity high. */
	QM_DMA_HANDSHAKE_POLARITY_LOW = 0x1   /**< Set HS polarity low. */
} qm_dma_handshake_polarity_t;

/**
 * DMA Burst Transfer Length
 */
typedef enum {
	QM_DMA_BURST_TRANS_LENGTH_1 = 0x0,  /**< Burst length 1 data item. */
	QM_DMA_BURST_TRANS_LENGTH_4 = 0x1,  /**< Burst length 4 data items. */
	QM_DMA_BURST_TRANS_LENGTH_8 = 0x2,  /**< Burst length 8 data items. */
	QM_DMA_BURST_TRANS_LENGTH_16 = 0x3, /**< Burst length 16 data items. */
	QM_DMA_BURST_TRANS_LENGTH_32 = 0x4, /**< Burst length 32 data items. */
	QM_DMA_BURST_TRANS_LENGTH_64 = 0x5, /**< Burst length 64 data items. */
	QM_DMA_BURST_TRANS_LENGTH_128 =
	    0x6,			    /**< Burst length 128 data items. */
	QM_DMA_BURST_TRANS_LENGTH_256 = 0x7 /**< Burst length 256 data items. */
} qm_dma_burst_length_t;

/**
 * DMA Transfer Width
 */
typedef enum {
	QM_DMA_TRANS_WIDTH_8 = 0x0,   /**< Transfer width of 8 bits. */
	QM_DMA_TRANS_WIDTH_16 = 0x1,  /**< Transfer width of 16 bits. */
	QM_DMA_TRANS_WIDTH_32 = 0x2,  /**< Transfer width of 32 bits. */
	QM_DMA_TRANS_WIDTH_64 = 0x3,  /**< Transfer width of 64 bits. */
	QM_DMA_TRANS_WIDTH_128 = 0x4, /**< Transfer width of 128 bits. */
	QM_DMA_TRANS_WIDTH_256 = 0x5  /**< Transfer width of 256 bits. */
} qm_dma_transfer_width_t;

/**
 * DMA channel direction.
 */
typedef enum {
	QM_DMA_MEMORY_TO_MEMORY = 0x0, /**< Memory to memory transfer. */
	QM_DMA_MEMORY_TO_PERIPHERAL =
	    0x1,			  /**< Memory to peripheral transfer. */
	QM_DMA_PERIPHERAL_TO_MEMORY = 0x2 /**< Peripheral to memory transfer. */
} qm_dma_channel_direction_t;

/*
 * DMA Transfer Type
 */
typedef enum {
	QM_DMA_TYPE_SINGLE,	   /**< Single block mode. */
	QM_DMA_TYPE_MULTI_CONT,       /**< Contiguous multiblock mode. */
	QM_DMA_TYPE_MULTI_LL,	 /**< Link list multiblock mode. */
	QM_DMA_TYPE_MULTI_LL_CIRCULAR /**< Link list multiblock mode with cyclic
				       operation. */
} qm_dma_transfer_type_t;

/**
 * DMA channel configuration structure
 */
typedef struct {
	/** DMA channel handshake interface ID */
	qm_dma_handshake_interface_t handshake_interface;

	/** DMA channel handshake polarity */
	qm_dma_handshake_polarity_t handshake_polarity;

	/** DMA channel direction */
	qm_dma_channel_direction_t channel_direction;

	/** DMA source transfer width */
	qm_dma_transfer_width_t source_transfer_width;

	/** DMA destination transfer width */
	qm_dma_transfer_width_t destination_transfer_width;

	/** DMA source burst length */
	qm_dma_burst_length_t source_burst_length;

	/** DMA destination burst length */
	qm_dma_burst_length_t destination_burst_length;

	/** DMA transfer type */
	qm_dma_transfer_type_t transfer_type;

	/**
	 * Client callback for DMA transfer ISR
	 *
	 * @param[in] callback_context DMA client context.
	 * @param[in] len              Data length transferred.
	 * @param[in] error            Error code.
	 */
	void (*client_callback)(void *callback_context, uint32_t len,
				int error_code);

	/** DMA client context passed to the callbacks */
	void *callback_context;
} qm_dma_channel_config_t;

/*
 * Multiblock linked list node structure. The client does not need to know the
 * internals of this struct but only its size, so that the correct memory for
 * the linked list can be allocated (one node per DMA transfer block).
 */
typedef struct {
	uint32_t source_address;      /**< Block source address. */
	uint32_t destination_address; /**< Block destination address. */
	uint32_t linked_list_address; /**< Pointer to next LLI. */
	uint32_t ctrl_low;	    /**< Bottom half Ctrl register. */
	uint32_t ctrl_high;	   /**< Top half Ctrl register. */
	/**< Destination/source status writebacks are disabled. */
} qm_dma_linked_list_item_t;

/**
 * DMA single block transfer configuration structure
 */
typedef struct {
	uint32_t block_size;      /**< DMA block size, Min = 1, Max = 4095. */
	uint32_t *source_address; /**< DMA source transfer address. */
	uint32_t *destination_address; /**< DMA destination transfer address. */

} qm_dma_transfer_t;

/**
 * DMA multiblock transfer configuration structure
 */
typedef struct {
	uint32_t *source_address;      /**< First block source address. */
	uint32_t *destination_address; /**< First block destination address. */
	uint16_t block_size; /**< DMA block size, Min = 1, Max = 4095. */
	uint16_t
	    num_blocks; /**< Number of contiguous blocks to be transfered. */
	qm_dma_linked_list_item_t *linked_list_first; /**< First block LLI
							 descriptor or NULL
							 (contiguous mode) */
} qm_dma_multi_transfer_t;

/**
 * Initialise the DMA controller.
 *
 * The DMA controller and channels are first disabled.
 * All DMA controller interrupts are masked
 * using the controllers interrupt masking registers. The system
 * DMA interrupts are then unmasked. Finally the DMA controller
 * is enabled. This function must only be called once as it
 * resets the DMA controller and interrupt masking.
 *
 * @param[in] dma DMA instance.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_dma_init(const qm_dma_t dma);

/**
 * Setup a DMA channel configuration.
 *
 * Configures the channel source width, burst size, channel direction,
 * handshaking interface, transfer type and registers the client callback and
 * callback context. qm_dma_init() must first be called before configuring a
 * channel. This function only needs to be called once unless a channel is being
 * repurposed or its transfer type is changed.
 *
 * @param[in] dma            DMA instance.
 * @param[in] channel_id     The channel to start.
 * @param[in] channel_config The DMA channel configuration as
 *                           defined by the DMA client. This must not be NULL.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_dma_channel_set_config(const qm_dma_t dma,
			      const qm_dma_channel_id_t channel_id,
			      qm_dma_channel_config_t *const channel_config);

/**
 * Setup a DMA single block transfer.
 *
 * Configure the source address,destination addresses and block size.
 * qm_dma_channel_set_config() must first be called before
 * configuring a transfer. qm_dma_transfer_set_config() must
 * be called before starting every transfer, even if the
 * addresses and block size remain unchanged.
 *
 * @param[in] dma             DMA instance.
 * @param[in] channel_id      The channel to start.
 * @param[in] transfer_config The transfer DMA configuration
 *                            as defined by the dma client.
 *                            This must not be NULL.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_dma_transfer_set_config(const qm_dma_t dma,
			       const qm_dma_channel_id_t channel_id,
			       qm_dma_transfer_t *const transfer_config);

/**
 * Setup a DMA multiblock transfer.
 *
 * If the DMA channel has been configured for contiguous multiblock transfers,
 * this function sets the source address, destination address, block size and
 * number of block parameters needed to perform a continguous multiblock
 * transfer. The linked_list_first parameter in the transfer struct is ignored.
 *
 * If the DMA channel has been configured for linked-list multiblock transfers,
 * this function populates a linked list in the client memory area pointed at
 * by the linked_list_first parameter in the transfer struct, using the provided
 * parameters source address, destination address, block size and number of
 * blocks (equal to the number of LLIs). This function may be called repeteadly
 * in order to add different client buffers for transmission/reception that are
 * not contiguous in memory (scatter-gather) in a buffer chain fashion. When
 * calling qm_dma_transfer_start, the DMA core sees a single linked list built
 * using consecutive calls to this function. Furthermore, if the transfer type
 * is linked-list cyclic, the linked_list_address parameter of the last LLI
 * points at the first LLI. The client needs to allocate enough memory starting
 * at linked_list_first for the whole set of LLIs to fit, i.e. (num_blocks *
 * sizeof(qm_dma_linked_list_item_t)).
 *
 * The DMA driver manages the block interrupts so that only when the last block
 * of a buffer has been transfered, the client callback is invoked. Note that
 * in linked-list mode, each buffer transfer results on a client callback, and
 * all buffers need to contain the same number of blocks.
 *
 * qm_dma_multi_transfer_set_config() must be called before starting every
 * transfer, even if the addresses, block size and other configuration
 * information remain unchanged.
 *
 * @param[in] dma		    DMA instance.
 * @param[in] channel_id	    The channel to start.
 * @param[in] multi_transfer_config The transfer DMA configuration as defined by
 *				    the dma client. This must not be NULL.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_dma_multi_transfer_set_config(
    const qm_dma_t dma, const qm_dma_channel_id_t channel_id,
    qm_dma_multi_transfer_t *const multi_transfer_config);

/**
 * Start a DMA transfer.
 *
 * qm_dma_transfer_set_config() mustfirst be called
 * before starting a transfer.
 *
 * @param[in] dma        DMA instance.
 * @param[in] channel_id The channel to start.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
*/
int qm_dma_transfer_start(const qm_dma_t dma,
			  const qm_dma_channel_id_t channel_id);

/**
 * Terminate a DMA transfer.
 *
 * This function is only called if a transfer needs to be terminated manually.
 * This may be require if an expected transfer complete callback
 * has not been received. Terminating the transfer will
 * trigger the transfer complete callback. The length
 * returned by the callback is the transfer length at the
 * time that the transfer was terminated.
 *
 * @param[in] dma        DMA instance.
 * @param[in] channel_id The channel to stop.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
*/
int qm_dma_transfer_terminate(const qm_dma_t dma,
			      const qm_dma_channel_id_t channel_id);

/**
 * Setup and start memory to memory transfer.
 *
 * This function will setup a memory to memory transfer by
 * calling qm_dma_transfer_setup() and will then start the
 * transfer by calling qm_dma_transfer_start(). This is
 * done for consistency across user applications.
 *
 * @param[in] dma             DMA instance.
 * @param[in] channel_id      The channel to start.
 * @param[in] transfer_config The transfer DMA configuration
 *                            as defined by the dma client.
 *                            This must not be NULL.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
*/
int qm_dma_transfer_mem_to_mem(const qm_dma_t dma,
			       const qm_dma_channel_id_t channel_id,
			       qm_dma_transfer_t *const transfer_config);

/**
 * Save DMA peripheral's context.
 *
 * Saves the configuration of the specified DMA peripheral
 * before entering sleep.
 *
 * @param[in] dma DMA device.
 * @param[out] ctx DMA context structure. This must not be NULL.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_dma_save_context(const qm_dma_t dma, qm_dma_context_t *const ctx);

/**
 * Restore DMA peripheral's context.
 *
 * Restore the configuration of the specified DMA peripheral
 * after exiting sleep.
 *
 * @param[in] dma DMA device.
 * @param[in] ctx DMA context structure. This must not be NULL.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_dma_restore_context(const qm_dma_t dma,
			   const qm_dma_context_t *const ctx);

/**
 * @}
 */

#endif /* __QM_DMA_H_ */
