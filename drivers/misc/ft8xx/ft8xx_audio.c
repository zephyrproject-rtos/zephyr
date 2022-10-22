/*
 * Copyright (c) 2022
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/misc/ft8xx/ft8xx_audio.h>


#include <stdint.h>
#include <string.h>
#include <stdalign.h>

#include <zephyr/drivers/misc/ft8xx/ft8xx_common.h>
#include <zephyr/drivers/misc/ft8xx/ft8xx_memory.h>
#include "ft8xx_drv.h"


int ft8xx_audio_load(const struct device *dev, uint32_t start_address, uint8_t* sample, uint32_t sample_length)
{
    const struct ft8xx_data *data = dev->data;
    const struct ft8xx_config *config = dev->config;
    union ft8xx_bus *bus = config->bus;

    // check memory 64 bit alignment 

        if (start_address & 0x7)
        {
            LOG_ERR("FT8xx Bad Memory alignment");
            return -ENOTSUP;
        }

        if (sample_length % 8)
        {
            LOG_ERR("FT8xx Bad Memory alignment");
            return -ENOTSUP;
        }

        if (start_address > data->memory_map->FT800_RAM_G_END)
         {
            LOG_ERR("FT8xx Memory out of Range");
            return -ENOTSUP;
         }        

        if ((start_address + sample_length) > data->memory_map->FT800_RAM_G_END)
         {
            LOG_ERR("FT8xx Memory out of Range");
            return -ENOTSUP;
         }

        ft8xx_wrblock(bus, data->memory_map->FT800_RAM_G, start_address,sample,sample_length );   





}

int ft8xx_audio_play(const struct device *dev, uint32_t start_address, uint32_t sample_length, uint8_t audio_format, uint16_t sample_freq, uint8_t vol, bool loop)
{
    const struct ft8xx_data *data = dev->data;
    const struct ft8xx_config *config = dev->config;
    union ft8xx_bus *bus = config->bus;

    ft8xx_wr8(bus, data->register_map->REG_VOL_PB, vol);    
    ft8xx_wr32(bus, data->register_map->REG_PLAYBACK_START, start_address);   
    ft8xx_wr32(bus, data->register_map->REG_PLAYBACK_LENGTH, sample_length);       
    ft8xx_wr16(bus, data->register_map->REG_PLAYBACK_FREQ, sample_freq);       
    ft8xx_wr8(bus, data->register_map->REG_PLAYBACK_FORMAT, audio_format);       
    ft8xx_wr8(bus, data->register_map->REG_PLAYBACK_LOOP, loop);       
    ft8xx_wr8(bus, data->register_map->REG_PLAYBACK_PLAY, 1);       

    return ft8xx_rd8(bus, data->register_map->REG_PLAYBACK_PLAY);
}

int ft8xx_audio_get_status(const struct device *dev)
{
    const struct ft8xx_data *data = dev->data;
    const struct ft8xx_config *config = dev->config;
    union ft8xx_bus *bus = config->bus;

    return ft8xx_rd8(bus, data->register_map->REG_PLAYBACK_PLAY);

}

int ft8xx_audio_stop(const struct device *dev)
{
    const struct ft8xx_data *data = dev->data;
    const struct ft8xx_config *config = dev->config;
    union ft8xx_bus *bus = config->bus;

    ft8xx_wr32(bus, data->register_map->REG_PLAYBACK_LENGTH, 0);
    ft8xx_wr8(bus, data->register_map->REG_PLAYBACK_PLAY, 1);

    return ft8xx_rd8(bus, data->register_map->REG_PLAYBACK_PLAY);    

}

int ft8xx_audio_synth_start(const struct device *dev, uint8_t sound, uint8_t note, uint8_t vol)
{
    const struct ft8xx_data *data = dev->data;
    const struct ft8xx_config *config = dev->config;
    union ft8xx_bus *bus = config->bus;

    ft8xx_wr8(bus, data->register_map->REG_VOL_SOUND, vol);       
    ft8xx_wr16(bus, data->register_map->REG_SOUND, ((note << 8) | sound ));
    ft8xx_wr8(bus, data->register_map->REG_PLAY, 1);  

    return ft8xx_rd8(bus, data->register_map->REG_PLAY);   
}

int ft8xx_audio_synth_get_status(const struct device *dev)
{
    const struct ft8xx_data *data = dev->data;
    const struct ft8xx_config *config = dev->config;
    union ft8xx_bus *bus = config->bus;

    return ft8xx_rd8(bus, data->register_map->REG_PLAY);   
}

int ft8xx_audio_synth_stop(const struct device *dev)
{
    const struct ft8xx_data *data = dev->data;
    const struct ft8xx_config *config = dev->config;
    union ft8xx_bus *bus = config->bus;
     
    ft8xx_wr16(bus, data->register_map->REG_SOUND, 0);
    ft8xx_wr8(bus, data->register_map->REG_PLAY, 1);      

    return ft8xx_rd8(bus, data->register_map->REG_PLAY);   
}