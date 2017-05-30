/**
 * \file
 *
 * \brief HTTP base entity.
 *
 * Copyright (c) 2016 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */

/**
 * \addtogroup sam0_httpc_group
 * @{
 */

#ifndef HTTP_ENTITY_H_INCLUDED
#define HTTP_ENTITY_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

/** 
 * \brief A structure that the implementation of HTTP entity. 
 */
struct http_entity {
	/** A flag for the using the chunked encoding transfer or not. */
	uint8_t is_chunked;
	/** 
	 * \brief Get content mime type. 
	 *
	 * \param[in]  priv_data       Private data of this entity.
	 *
	 * \return     Content type of entity.
	 */
	const char* (*get_contents_type)(void *priv_data);
	/** 
	 * \brief Get content length. 
	 * If using the chunked encoding, This function does not needed. 
	 * 
	 * \param[in]  priv_data       Private data of this entity.
	 *
	 * \return     Content length of entity.
	 */
	int (*get_contents_length)(void *priv_data);
	/** 
	 * \brief Read the content.
	 * 
	 * \param[in]  priv_data       Private data of this entity.
	 * \param[in]  buffer          A buffer that stored read data.
	 * \param[in]  size            Maximum size of the buffer.
	 * \param[in]  written         total size of ever read.
	 *
	 * \return     Read size.
	 */
	int (*read)(void *priv_data, char *buffer, uint32_t size, uint32_t written);
	/** 
	 * \brief Close the entity.
	 * Completed to send request. So release the resource.
	 * 
	 * \param[in]  priv_data       Private data of this entity.
	 */
	void (*close)(void *priv_data);
	/** Private data of this entity. Stored various data necessary for the operation of the entity. */
	void *priv_data;

};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* HTTP_ENTITY_H_INCLUDED */
