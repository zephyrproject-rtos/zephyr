/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <string.h>
#include <sys/cdefs.h>
#include <sys/param.h>
#include "esp_check.h"
#include "esp_rom_gpio.h"
#include "soc/rmt_periph.h"
#include "soc/rtc.h"
#include "hal/rmt_ll.h"
#include "hal/gpio_hal.h"
#include "driver/gpio.h"
#include "rmt_private.h"
#include "esp_memory_utils.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(espressif_rmt_tx, CONFIG_ESPRESSIF_RMT_LOG_LEVEL);

#include <zephyr/drivers/misc/espressif_rmt/rmt_tx.h>

struct rmt_sync_manager_t {
    rmt_group_t *group;    /**< which group the synchro belongs to */
    uint32_t channel_mask; /**< Mask of channels that are managed */
    size_t array_size;     /**< Size of the `tx_channel_array` */
    rmt_channel_handle_t tx_channel_array[]; /**< Array of TX channels that are managed */
};

static int rmt_del_tx_channel(rmt_channel_handle_t channel);
static int rmt_tx_modulate_carrier(rmt_channel_handle_t channel, const rmt_carrier_config_t *config);
static int rmt_tx_enable(rmt_channel_handle_t channel);
static int rmt_tx_disable(rmt_channel_handle_t channel);
static void rmt_tx_default_isr(void *args);
static void rmt_tx_do_transaction(rmt_tx_channel_t *tx_chan, rmt_tx_trans_desc_t *t);

#if SOC_RMT_SUPPORT_DMA
static bool rmt_dma_tx_eof_cb(gdma_channel_handle_t dma_chan, gdma_event_data_t *event_data, void *user_data);

static int rmt_tx_init_dma_link(rmt_tx_channel_t *tx_channel, const rmt_tx_channel_config_t *config)
{
    esp_err_t ret;
    rmt_symbol_word_t *dma_mem_base = heap_caps_calloc(1, sizeof(rmt_symbol_word_t) * config->mem_block_symbols, RMT_MEM_ALLOC_CAPS | MALLOC_CAP_DMA);
    if (!dma_mem_base) {
        LOG_ERR("no mem for tx DMA buffer");
        return -ENOMEM;
    }
    tx_channel->base.dma_mem_base = dma_mem_base;
    for (int i = 0; i < RMT_DMA_NODES_PING_PONG; i++) {
        /* each descriptor shares half of the DMA buffer */
        tx_channel->dma_nodes[i].buffer = dma_mem_base + tx_channel->ping_pong_symbols * i;
        tx_channel->dma_nodes[i].dw0.size = tx_channel->ping_pong_symbols * sizeof(rmt_symbol_word_t);
        /* the ownership will be switched to DMA in `rmt_tx_do_transaction()` */
        tx_channel->dma_nodes[i].dw0.owner = DMA_DESCRIPTOR_BUFFER_OWNER_CPU;
        /* each node can generate the DMA eof interrupt, and the driver will do a ping-pong trick in the eof callback */
        tx_channel->dma_nodes[i].dw0.suc_eof = 1;
    }
    gdma_channel_alloc_config_t dma_chan_config = {
        .direction = GDMA_CHANNEL_DIRECTION_TX,
    };
    ret = gdma_new_channel(&dma_chan_config, &tx_channel->base.dma_chan);
    if (ret) {
        LOG_ERR("allocate TX DMA channel failed");
        return -ENODEV;
    }
    gdma_strategy_config_t gdma_strategy_conf = {
        .auto_update_desc = true,
        .owner_check = true,
    };
    gdma_apply_strategy(tx_channel->base.dma_chan, &gdma_strategy_conf);
    gdma_tx_event_callbacks_t cbs = {
        .on_trans_eof = rmt_dma_tx_eof_cb,
    };
    gdma_register_tx_event_callbacks(tx_channel->base.dma_chan, &cbs, tx_channel);
    return 0;
}
#endif

static int rmt_tx_register_to_group(rmt_tx_channel_t *tx_channel, const rmt_tx_channel_config_t *config)
{
    size_t mem_block_num = 0;
    /* start to search for a free channel */
    /* a channel can take up its neighbour's memory block, so the neighbour channel won't work, we should skip these "invaded" ones */
    int channel_scan_start = RMT_TX_CHANNEL_OFFSET_IN_GROUP;
    int channel_scan_end = RMT_TX_CHANNEL_OFFSET_IN_GROUP + SOC_RMT_TX_CANDIDATES_PER_GROUP;
    k_spinlock_key_t key;

#if SOC_RMT_SUPPORT_DMA
    if (config->flags.with_dma) {
        /* for DMA mode, the memory block number is always 1; for non-DMA mode, memory block number is configured by user */
        mem_block_num = 1;
        /* Only the last channel has the DMA capability */
        channel_scan_start = RMT_TX_CHANNEL_OFFSET_IN_GROUP + SOC_RMT_TX_CANDIDATES_PER_GROUP - 1;
        tx_channel->ping_pong_symbols = config->mem_block_symbols / 2;
    } else {
#endif
        /* one channel can occupy multiple memory blocks */
        mem_block_num = config->mem_block_symbols / SOC_RMT_MEM_WORDS_PER_CHANNEL;
        if (mem_block_num * SOC_RMT_MEM_WORDS_PER_CHANNEL < config->mem_block_symbols) {
            mem_block_num++;
        }
        tx_channel->ping_pong_symbols = mem_block_num * SOC_RMT_MEM_WORDS_PER_CHANNEL / 2;
#if SOC_RMT_SUPPORT_DMA
    }
#endif
    tx_channel->base.mem_block_num = mem_block_num;

    /* search free channel and then register to the group */
    /* memory blocks used by one channel must be continuous */
    uint32_t channel_mask = (1 << mem_block_num) - 1;
    rmt_group_t *group = NULL;
    int channel_id = -1;
    for (int i = 0; i < SOC_RMT_GROUPS; i++) {
        group = rmt_acquire_group_handle(i);
        if (!group) {
            LOG_ERR("No memory available for group: %d", i);
            return -ENOMEM;
        }
        key = k_spin_lock(&group->spinlock);
        for (int j = channel_scan_start; j < channel_scan_end; j++) {
            if (!(group->occupy_mask & (channel_mask << j))) {
                group->occupy_mask |= (channel_mask << j);
                /* the channel ID should index from 0 */
                channel_id = j - RMT_TX_CHANNEL_OFFSET_IN_GROUP;
                group->tx_channels[channel_id] = tx_channel;
                break;
            }
        }
        k_spin_unlock(&group->spinlock, key);
        if (channel_id < 0) {
            /* didn't find a capable channel in the group, don't forget to release the group handle */
            rmt_release_group_handle(group);
            group = NULL;
        } else {
            tx_channel->base.channel_id = channel_id;
            tx_channel->base.channel_mask = channel_mask;
            tx_channel->base.group = group;
            break;
        }
    }
    if (channel_id < 0) {
        LOG_ERR("No tx channel available");
        return -ENOMEM;
    }

    return 0;
}

static void rmt_tx_unregister_from_group(rmt_channel_t *channel, rmt_group_t *group)
{
    k_spinlock_key_t key = k_spin_lock(&group->spinlock);
    group->tx_channels[channel->channel_id] = NULL;
    group->occupy_mask &= ~(channel->channel_mask << (channel->channel_id + RMT_TX_CHANNEL_OFFSET_IN_GROUP));
    k_spin_unlock(&group->spinlock, key);
    /* channel has a reference on group, release it now */
    rmt_release_group_handle(group);
}

static int rmt_tx_create_trans_queue(rmt_tx_channel_t *tx_channel, const rmt_tx_channel_config_t *config)
{
    int rc;

    tx_channel->queue_size = config->trans_queue_depth;
    for (int i = 0; i < RMT_TX_QUEUE_MAX; i++) {
        tx_channel->trans_queue_structs[i] = heap_caps_calloc(1, sizeof(char *) * config->trans_queue_depth, RMT_MEM_ALLOC_CAPS);
        if (!tx_channel->trans_queue_structs[i]) {
            LOG_ERR("Unable to allocate memory for queue storage");
            return -ENOMEM;
        }
        k_msgq_init(&tx_channel->trans_queues[i], (char *)tx_channel->trans_queue_structs[i], sizeof(rmt_tx_trans_desc_t *), config->trans_queue_depth);
    }
    /* initialize the ready queue */
    rmt_tx_trans_desc_t *p_trans_desc = NULL;
    for (int i = 0; i < config->trans_queue_depth; i++) {
        p_trans_desc = &tx_channel->trans_desc_pool[i];
        rc = k_msgq_put(&tx_channel->trans_queues[RMT_TX_QUEUE_READY], &p_trans_desc, K_NO_WAIT);
        if (rc) {
            LOG_ERR("Ready queue is full");
            return rc;
        }
    }
    return 0;
}

static int rmt_tx_destroy(rmt_tx_channel_t *tx_channel)
{
    esp_err_t ret;
    if (tx_channel->base.intr) {
        ret = esp_intr_free(tx_channel->base.intr);
        if (ret) {
            LOG_ERR("delete interrupt service failed");
            return -ENODEV;
        }
    }
#if CONFIG_PM_ENABLE
    if (tx_channel->base.pm_lock) {
        ret = esp_pm_lock_delete(tx_channel->base.pm_lock);
        if (ret) {
            LOG_ERR("delete pm_lock failed");
            return -ENODEV;
        }
    }
#endif
#if SOC_RMT_SUPPORT_DMA
    if (tx_channel->base.dma_chan) {
        ret = gdma_del_channel(tx_channel->base.dma_chan);
        if (ret) {
            LOG_ERR("delete dma channel failed");
            return -ENODEV;
        }
    }
#endif
    for (int i = 0; i < RMT_TX_QUEUE_MAX; i++) {
        k_msgq_cleanup(&tx_channel->trans_queues[i]);
    }
    for (int i = 0; i < RMT_TX_QUEUE_MAX; i++) {
        if (tx_channel->trans_queue_structs[i]) {
            free(tx_channel->trans_queue_structs[i]);
        }
    }
    if (tx_channel->base.dma_mem_base) {
        free(tx_channel->base.dma_mem_base);
    }
    if (tx_channel->base.group) {
        /* de-register channel from RMT group */
        rmt_tx_unregister_from_group(&tx_channel->base, tx_channel->base.group);
    }
    free(tx_channel);
    return 0;
}

int rmt_new_tx_channel(const rmt_tx_channel_config_t *config, rmt_channel_handle_t *ret_chan)
{
    esp_err_t ret;
    rmt_tx_channel_t *tx_channel = NULL;
    k_spinlock_key_t key;
    int rc = 0;

    /* Check if priority is valid */
    if (config->intr_priority) {
        if (!(config->intr_priority > 0) || !(1 << (config->intr_priority) & RMT_ALLOW_INTR_PRIORITY_MASK)) {
            LOG_ERR("Invalid interrupt priority: %d", config->intr_priority);
            return -EINVAL;
        }
    }
    if (!(config && ret_chan && config->resolution_hz && config->trans_queue_depth)) {
    	LOG_ERR("Invalid argument");
        return -EINVAL;
    }
    if (!GPIO_IS_VALID_GPIO(config->gpio_num)) {
    	LOG_ERR("Invalid GPIO number: %d", config->gpio_num);
        return -EINVAL;
    }
    if (!(((config->mem_block_symbols & 0x01) == 0) && (config->mem_block_symbols >= SOC_RMT_MEM_WORDS_PER_CHANNEL))) {
    	LOG_ERR("Parameter mem_block_symbols must be even and at least: %d", SOC_RMT_MEM_WORDS_PER_CHANNEL);
        return -EINVAL;
    }
#if SOC_RMT_SUPPORT_DMA
    /* We only support 2 nodes ping-pong, if the configured memory block size needs more than two DMA descriptors, should treat it as invalid */
    if (!(config->mem_block_symbols <= RMT_DMA_DESC_BUF_MAX_SIZE * RMT_DMA_NODES_PING_PONG / sizeof(rmt_symbol_word_t))) {
    	LOG_ERR("Parameter mem_block_symbols can't exceed: %d", RMT_DMA_DESC_BUF_MAX_SIZE * RMT_DMA_NODES_PING_PONG / sizeof(rmt_symbol_word_t));
        return -EINVAL;
    }
#else
    if (config->flags.with_dma) {
    	LOG_ERR("DMA not supported");
        return -ENODEV;
    }
#endif

    /* malloc channel memory */
    uint32_t mem_caps = RMT_MEM_ALLOC_CAPS;
#if SOC_RMT_SUPPORT_DMA
    if (config->flags.with_dma) {
        /* DMA descriptors must be placed in internal SRAM */
        mem_caps |= MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA;
    }
#endif
    tx_channel = heap_caps_calloc(1, sizeof(rmt_tx_channel_t) + sizeof(rmt_tx_trans_desc_t) * config->trans_queue_depth, mem_caps);
    if (!tx_channel) {
       LOG_ERR("Unable to allocate tx channel");
       return -ENOMEM;
    }
    /* create transaction queues */
    rc = rmt_tx_create_trans_queue(tx_channel, config);
    if (rc) {
        LOG_ERR("Unable to install transaction queues");
        goto err;
    }
    /* register the channel to group */
    rc = rmt_tx_register_to_group(tx_channel, config);
    if (rc) {
        LOG_ERR("Unable to register tx channel");
        goto err;
    }

    rmt_group_t *group = tx_channel->base.group;
    rmt_hal_context_t *hal = &group->hal;
    int channel_id = tx_channel->base.channel_id;
    int group_id = group->group_id;

    /* reset channel, make sure the TX engine is not working, and events are cleared */
    key = k_spin_lock(&group->spinlock);
    rmt_hal_tx_channel_reset(&group->hal, channel_id);
    k_spin_unlock(&group->spinlock, key);
    /* install tx interrupt */
    /* --- install interrupt service */
    /* interrupt is mandatory to run basic RMT transactions, so it's not lazy installed in `rmt_tx_register_event_callbacks()` */
    /* 1-- Set user specified priority to `group->intr_priority` */
    bool priority_conflict = rmt_set_intr_priority_to_group(group, config->intr_priority);
    if (priority_conflict) {
        LOG_ERR("Parameter intr_priority conflict");
        rc = -EINVAL;
        goto err;
    }
    /* 2-- Get interrupt allocation flag */
    int isr_flags = rmt_get_isr_flags(group);
    /* 3-- Allocate interrupt using isr_flag */
    ret = esp_intr_alloc_intrstatus(rmt_periph_signals.groups[group_id].irq, isr_flags,
                                    (uint32_t) rmt_ll_get_interrupt_status_reg(hal->regs),
                                    RMT_LL_EVENT_TX_MASK(channel_id), rmt_tx_default_isr, tx_channel,
                                    &tx_channel->base.intr);
    if (ret) {
        LOG_ERR("Installation of tx interrupt failed");
        rc = -ENODEV;
        goto err;
    }
#if SOC_RMT_SUPPORT_DMA
    /* install DMA service */
    if (config->flags.with_dma) {
        rc = rmt_tx_init_dma_link(tx_channel, config);
        if (rc) {
            LOG_ERR("Installation of tx DMA failed");
            goto err;
        }
    }
#endif
    /* select the clock source */
    rc = rmt_select_periph_clock(&tx_channel->base, config->clk_src);
    if (rc) {
        LOG_ERR("Configuration of clock source failed");
        goto err;
    }
    /* set channel clock resolution, find the divider to get the closest resolution */
    uint32_t real_div = (group->resolution_hz + config->resolution_hz / 2) / config->resolution_hz;
    rmt_ll_tx_set_channel_clock_div(hal->regs, channel_id, real_div);
    /* resolution lost due to division, calculate the real resolution */
    tx_channel->base.resolution_hz = group->resolution_hz / real_div;
    if (tx_channel->base.resolution_hz != config->resolution_hz) {
        LOG_WRN("Channel resolution loss, real=%"PRIu32, tx_channel->base.resolution_hz);
    }

    rmt_ll_tx_set_mem_blocks(hal->regs, channel_id, tx_channel->base.mem_block_num);
    /* set limit threshold, after transmit ping_pong_symbols size, an interrupt event would be generated */
    rmt_ll_tx_set_limit(hal->regs, channel_id, tx_channel->ping_pong_symbols);
    /* disable carrier modulation by default, can reenable by `rmt_apply_carrier()` */
    rmt_ll_tx_enable_carrier_modulation(hal->regs, channel_id, false);
    /* idle level is determined by register value */
    rmt_ll_tx_fix_idle_level(hal->regs, channel_id, 0, true);
    /* always enable tx wrap, both DMA mode and ping-pong mode rely this feature */
    rmt_ll_tx_enable_wrap(hal->regs, channel_id, true);

    /* GPIO Matrix/MUX configuration */
    tx_channel->base.gpio_num = config->gpio_num;
    gpio_config_t gpio_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        /* also enable the input path if `io_loop_back` is on, this is useful for bi-directional buses */
        .mode = (config->flags.io_od_mode ? GPIO_MODE_OUTPUT_OD : GPIO_MODE_OUTPUT) | (config->flags.io_loop_back ? GPIO_MODE_INPUT : 0),
        .pull_down_en = false,
        .pull_up_en = true,
        .pin_bit_mask = 1ULL << config->gpio_num,
    };
    ret = gpio_config(&gpio_conf);
    if (ret) {
        LOG_ERR("GPIO configuration failed");
        rc = -EINVAL;
        goto err;
    }
    esp_rom_gpio_connect_out_signal(config->gpio_num,
                                    rmt_periph_signals.groups[group_id].channels[channel_id + RMT_TX_CHANNEL_OFFSET_IN_GROUP].tx_sig,
                                    config->flags.invert_out, false);

    atomic_init(&tx_channel->base.fsm, RMT_FSM_INIT);
    tx_channel->base.direction = RMT_CHANNEL_DIRECTION_TX;
    tx_channel->base.hw_mem_base = &RMTMEM.channels[channel_id + RMT_TX_CHANNEL_OFFSET_IN_GROUP].symbols[0];
    /* polymorphic methods */
    tx_channel->base.del = rmt_del_tx_channel;
    tx_channel->base.set_carrier_action = rmt_tx_modulate_carrier;
    tx_channel->base.enable = rmt_tx_enable;
    tx_channel->base.disable = rmt_tx_disable;
    /* return general channel handle */
    *ret_chan = &tx_channel->base;
    LOG_DBG("New tx channel(%d,%d) at %p, gpio=%d, res=%"PRIu32"Hz, hw_mem_base=%p, dma_mem_base=%p, ping_pong_size=%zu, queue_depth=%zu",
            group_id, channel_id, tx_channel, config->gpio_num, tx_channel->base.resolution_hz,
            tx_channel->base.hw_mem_base, tx_channel->base.dma_mem_base, tx_channel->ping_pong_symbols, tx_channel->queue_size);
    return 0;

err:
    if (tx_channel) {
        rmt_tx_destroy(tx_channel);
    }
    return rc;
}

static int rmt_del_tx_channel(rmt_channel_handle_t channel)
{
    if (atomic_load(&channel->fsm) != RMT_FSM_INIT) {
        LOG_ERR("Channel not initialized");
        return -ENODEV;
    }
    rmt_tx_channel_t *tx_chan = __containerof(channel, rmt_tx_channel_t, base);
    rmt_group_t *group = channel->group;
    int group_id = group->group_id;
    int channel_id = channel->channel_id;
    LOG_DBG("del tx channel(%d,%d)", group_id, channel_id);
    /* recycle memory resource */
    return rmt_tx_destroy(tx_chan);
}

int rmt_new_sync_manager(const rmt_sync_manager_config_t *config, rmt_sync_manager_handle_t *ret_synchro)
{
#if !SOC_RMT_SUPPORT_TX_SYNCHRO
    LOG_ERR("Sync manager not supported");
    return -ENODEV;
#else
    k_spinlock_key_t key;
    rmt_sync_manager_t *synchro = NULL;
    if (!(config && ret_synchro && config->tx_channel_array && config->array_size)) {
        LOG_ERR("Invalid argument");
        return -EINVAL;
    }
    synchro = heap_caps_calloc(1, sizeof(rmt_sync_manager_t) + sizeof(rmt_channel_handle_t) * config->array_size, RMT_MEM_ALLOC_CAPS);
    if (!synchro) {
        LOG_ERR("Unable to allocate memory for sync manager");
        return -ENOMEM;
    }
    for (size_t i = 0; i < config->array_size; i++) {
        synchro->tx_channel_array[i] = config->tx_channel_array[i];
    }
    synchro->array_size = config->array_size;

    int group_id = config->tx_channel_array[0]->group->group_id;
    /* acquire group handle, increase reference count */
    rmt_group_t *group = rmt_acquire_group_handle(group_id);
    /* sanity check */
    assert(group);
    synchro->group = group;
    /* calculate the mask of the channels to be managed */
    uint32_t channel_mask = 0;
    rmt_channel_handle_t channel = NULL;
    for (size_t i = 0; i < config->array_size; i++) {
        channel = config->tx_channel_array[i];
        if (channel->direction != RMT_CHANNEL_DIRECTION_TX) {
            LOG_ERR("sync manager supports TX channel only");
            rc = -EINVAL;
            goto err;
        }
        if (channel->group != group) {
            LOG_ERR("Channels to be managed should locate in the same group");
            rc = -EINVAL;
            goto err;
        }
        if (atomic_load(&channel->fsm) != RMT_FSM_ENABLE) {
            LOG_ERR("Channel not in enable state");
            rc = -ENODEV;
            goto err;
        }
        channel_mask |= 1 << channel->channel_id;
    }
    synchro->channel_mask = channel_mask;

    /* search and register sync manager to group */
    bool new_synchro = false;
    key = k_spin_lock(&group->spinlock);
    if (group->sync_manager == NULL) {
        group->sync_manager = synchro;
        new_synchro = true;
    }
    k_spin_unlock(&group->spinlock, key);
    if (!new_synchro) {
        LOG_ERR("No free sync manager in the group");
        rc = -ENOMEM;
        goto err;
    }

    /* enable sync manager */
    key = k_spin_lock(&group->spinlock);
    rmt_ll_tx_enable_sync(group->hal.regs, true);
    rmt_ll_tx_sync_group_add_channels(group->hal.regs, channel_mask);
    rmt_ll_tx_reset_channels_clock_div(group->hal.regs, channel_mask);
    /* ensure the reading cursor of each channel is pulled back to the starting line */
    for (size_t i = 0; i < config->array_size; i++) {
        rmt_ll_tx_reset_pointer(group->hal.regs, config->tx_channel_array[i]->channel_id);
    }
    k_spin_unlock(&group->spinlock, key);

    *ret_synchro = synchro;
    LOG_DBG("new sync manager at %p, with channel mask:%02"PRIx32, synchro, synchro->channel_mask);
    return 0;

err:
    if (synchro) {
        if (synchro->group) {
            rmt_release_group_handle(synchro->group);
        }
        free(synchro);
    }
    return rc;
#endif
}

int rmt_sync_reset(rmt_sync_manager_handle_t synchro)
{
#if !SOC_RMT_SUPPORT_TX_SYNCHRO
    LOG_ERR("Sync manager not supported");
    return -ENODEV;
#else
    k_spinlock_key_t key;
    if (!synchro) {
        LOG_ERR("Invalid argument");
        return -EINVAL;
    }
    rmt_group_t *group = synchro->group;
    key = k_spin_lock(&group->spinlock);
    rmt_ll_tx_reset_channels_clock_div(group->hal.regs, synchro->channel_mask);
    for (size_t i = 0; i < synchro->array_size; i++) {
        rmt_ll_tx_reset_pointer(group->hal.regs, synchro->tx_channel_array[i]->channel_id);
    }
    k_spin_unlock(&group->spinlock, key);
    return 0;
#endif
}

int rmt_del_sync_manager(rmt_sync_manager_handle_t synchro)
{
#if !SOC_RMT_SUPPORT_TX_SYNCHRO
    LOG_ERR("Sync manager not supported");
    return -ENODEV;
#else
    k_spinlock_key_t key;
    if (!synchro) {
        LOG_ERR("Invalid argument");
        return -EINVAL;
    }
    rmt_group_t *group = synchro->group;
    int group_id = group->group_id;
    key = k_spin_lock(&group->spinlock);
    group->sync_manager = NULL;
    /* disable sync manager */
    rmt_ll_tx_enable_sync(group->hal.regs, false);
    rmt_ll_tx_sync_group_remove_channels(group->hal.regs, synchro->channel_mask);
    k_spin_unlock(&group->spinlock, key);

    free(synchro);
    LOG_DBG("del sync manager in group(%d)", group_id);
    rmt_release_group_handle(group);
    return 0;
#endif
}

int rmt_tx_register_event_callbacks(rmt_channel_handle_t channel, const rmt_tx_event_callbacks_t *cbs, void *user_data)
{
    if (!(channel && cbs)) {
        LOG_ERR("Invalid argument");
        return -EINVAL;
    }
    if (channel->direction != RMT_CHANNEL_DIRECTION_TX) {
        LOG_ERR("invalid channel direction");
        return -EINVAL;
    }
    rmt_tx_channel_t *tx_chan = __containerof(channel, rmt_tx_channel_t, base);

#if CONFIG_ESPRESSIF_RMT_ISR_IRAM_SAFE
    if (cbs->on_trans_done) {
        if (!esp_ptr_in_iram(cbs->on_trans_done)) {
            LOG_ERR("on_trans_done callback not in IRAM");
            return -EINVAL;
        }
    }
    if (user_data) {
        if (!esp_ptr_internal(user_data)) {
            LOG_ERR("user context not in internal RAM");
            return -EINVAL;
        }
    }
#endif

    tx_chan->on_trans_done = cbs->on_trans_done;
    tx_chan->user_data = user_data;
    return 0;
}

int rmt_transmit(rmt_channel_handle_t channel, rmt_encoder_t *encoder, const void *payload, size_t payload_bytes, const rmt_transmit_config_t *config)
{
    int rc;
    if (!(channel && encoder && payload && payload_bytes && config)) {
        LOG_ERR("Invalid argument");
        return -EINVAL;
    }
    if (channel->direction != RMT_CHANNEL_DIRECTION_TX) {
        LOG_ERR("Invalid channel direction");
        return -EINVAL;
    }
#if !SOC_RMT_SUPPORT_TX_LOOP_COUNT
    if (config->loop_count > 0) {
        LOG_ERR("Loop count is not supported");
        return -EINVAL;
    }
#endif
#if CONFIG_ESPRESSIF_RMT_ISR_IRAM_SAFE
    /* payload is retrieved by the encoder, we should make sure it's still accessible even when the cache is disabled */
    if (!esp_ptr_internal(payload)) {
        LOG_ERR("Payload not in internal RAM");
        return -EINVAL;
    }
#endif
    k_timeout_t queue_wait_ticks = K_FOREVER;
    if (config->flags.queue_nonblocking) {
        queue_wait_ticks = K_NO_WAIT;
    }
    rmt_tx_channel_t *tx_chan = __containerof(channel, rmt_tx_channel_t, base);
    rmt_tx_trans_desc_t *t = NULL;
    /* acquire one transaction description from ready queue or complete queue */
    if (k_msgq_get(&tx_chan->trans_queues[RMT_TX_QUEUE_READY], &t, K_NO_WAIT) != 0) {
        if (k_msgq_get(&tx_chan->trans_queues[RMT_TX_QUEUE_COMPLETE], &t, queue_wait_ticks) == 0) {
            tx_chan->num_trans_inflight--;
        }
    }
    if (!t) {
        LOG_ERR("No free transaction descriptor, please consider increasing trans_queue_depth");
        return -ENODEV;
    }

    /* fill in the transaction descriptor */
    memset(t, 0, sizeof(rmt_tx_trans_desc_t));
    t->encoder = encoder;
    t->payload = payload;
    t->payload_bytes = payload_bytes;
    t->loop_count = config->loop_count;
    t->remain_loop_count = t->loop_count;
    t->flags.eot_level = config->flags.eot_level;

    /* send the transaction descriptor to queue */
    if (k_msgq_put(&tx_chan->trans_queues[RMT_TX_QUEUE_PROGRESS], &t, K_NO_WAIT) == 0) {
        tx_chan->num_trans_inflight++;
    } else {
        /* put the trans descriptor back to ready_queue */
        rc = k_msgq_put(&tx_chan->trans_queues[RMT_TX_QUEUE_READY], &t, K_NO_WAIT);
        if (rc) {
            LOG_ERR("ready queue full");
            return -ENODEV;
        }
    }

    /* check if we need to start one pending transaction */
    rmt_fsm_t expected_fsm = RMT_FSM_ENABLE;
    if (atomic_compare_exchange_strong(&channel->fsm, &expected_fsm, RMT_FSM_RUN_WAIT)) {
        /* check if we need to start one transaction */
        if (k_msgq_get(&tx_chan->trans_queues[RMT_TX_QUEUE_PROGRESS], &t, K_NO_WAIT) == 0) {
            atomic_store(&channel->fsm, RMT_FSM_RUN);
            rmt_tx_do_transaction(tx_chan, t);
        } else {
            atomic_store(&channel->fsm, RMT_FSM_ENABLE);
        }
    }

    return 0;
}

int rmt_tx_wait_all_done(rmt_channel_handle_t channel, k_timeout_t wait_ticks)
{
    int rc;
    if (!channel) {
        LOG_ERR("Invalid argument");
        return -EINVAL;
    }
    rmt_tx_channel_t *tx_chan = __containerof(channel, rmt_tx_channel_t, base);
    /* recycle all transaction that are on the fly */
    rmt_tx_trans_desc_t *t = NULL;
    size_t num_trans_inflight = tx_chan->num_trans_inflight;
    for (size_t i = 0; i < num_trans_inflight; i++) {
        rc = k_msgq_get(&tx_chan->trans_queues[RMT_TX_QUEUE_COMPLETE], &t, wait_ticks);
        if (rc) {
            LOG_ERR("Flush timeout");
            return -ETIMEDOUT;
        }
        rc = k_msgq_put(&tx_chan->trans_queues[RMT_TX_QUEUE_READY], &t, K_NO_WAIT);
        if (rc) {
            LOG_ERR("Ready queue full");
            return -ENODEV;
        }
        tx_chan->num_trans_inflight--;
    }
    return 0;
}

static void IRAM_ATTR rmt_tx_mark_eof(rmt_tx_channel_t *tx_chan)
{
    rmt_channel_t *channel = &tx_chan->base;
    rmt_group_t *group = channel->group;
    int channel_id = channel->channel_id;
    rmt_symbol_word_t *mem_to = channel->dma_chan ? channel->dma_mem_base : channel->hw_mem_base;
    rmt_tx_trans_desc_t *cur_trans = tx_chan->cur_trans;
    dma_descriptor_t *desc = NULL;
    k_spinlock_key_t key;

    /* a RMT word whose duration is zero means a "stop" pattern */
    mem_to[tx_chan->mem_off++] = (rmt_symbol_word_t) {
        .duration0 = 0,
        .level0 = cur_trans->flags.eot_level,
        .duration1 = 0,
        .level1 = cur_trans->flags.eot_level,
    };

    size_t off = 0;
    if (channel->dma_chan) {
        if (tx_chan->mem_off <= tx_chan->ping_pong_symbols) {
            desc = &tx_chan->dma_nodes[0];
            off = tx_chan->mem_off;
        } else {
            desc = &tx_chan->dma_nodes[1];
            off = tx_chan->mem_off - tx_chan->ping_pong_symbols;
        }
        desc->dw0.owner = DMA_DESCRIPTOR_BUFFER_OWNER_DMA;
        desc->dw0.length = off * sizeof(rmt_symbol_word_t);
        /* break down the DMA descriptor link */
        desc->next = NULL;
    } else {
        key = k_spin_lock(&group->spinlock);
        /*  This is the end of a sequence of encoding sessions, disable the threshold interrupt as no more data will be put into RMT memory block */
        rmt_ll_enable_interrupt(group->hal.regs, RMT_LL_EVENT_TX_THRES(channel_id), false);
        k_spin_unlock(&group->spinlock, key);
    }
}

static size_t IRAM_ATTR rmt_encode_check_result(rmt_tx_channel_t *tx_chan, rmt_tx_trans_desc_t *t)
{
    rmt_encode_state_t encode_state = RMT_ENCODING_RESET;
    rmt_encoder_handle_t encoder = t->encoder;
    size_t encoded_symbols = encoder->encode(encoder, &tx_chan->base, t->payload, t->payload_bytes, &encode_state);
    if (encode_state & RMT_ENCODING_COMPLETE) {
        t->flags.encoding_done = true;
        /* inserting EOF symbol if there's extra space */
        if (!(encode_state & RMT_ENCODING_MEM_FULL)) {
            rmt_tx_mark_eof(tx_chan);
            encoded_symbols += 1;
        }
    }

    /* for loop transaction, the memory block should accommodate all encoded RMT symbols */
    if (t->loop_count != 0) {
        if (unlikely(encoded_symbols > tx_chan->base.mem_block_num * SOC_RMT_MEM_WORDS_PER_CHANNEL)) {
            ESP_DRAM_LOGE("rmt", "encoding artifacts can't exceed hw memory block for loop transmission");
        }
    }

    return encoded_symbols;
}

static void IRAM_ATTR rmt_tx_do_transaction(rmt_tx_channel_t *tx_chan, rmt_tx_trans_desc_t *t)
{
    rmt_channel_t *channel = &tx_chan->base;
    rmt_group_t *group = channel->group;
    rmt_hal_context_t *hal = &group->hal;
    int channel_id = channel->channel_id;
    k_spinlock_key_t key;

    /* update current transaction */
    tx_chan->cur_trans = t;

#if SOC_RMT_SUPPORT_DMA
    if (channel->dma_chan) {
        gdma_reset(channel->dma_chan);
        /* chain the descritpros into a ring, and will break it in `rmt_encode_eof()` */
        for (int i = 0; i < RMT_DMA_NODES_PING_PONG; i++) {
            tx_chan->dma_nodes[i].next = &tx_chan->dma_nodes[i + 1];
            tx_chan->dma_nodes[i].dw0.owner = DMA_DESCRIPTOR_BUFFER_OWNER_CPU;
        }
        tx_chan->dma_nodes[1].next = &tx_chan->dma_nodes[0];
    }
#endif

    /* set transaction specific parameters */
    key = k_spin_lock(&channel->spinlock);
    rmt_ll_tx_reset_pointer(hal->regs, channel_id); /* reset pointer for new transaction */
    rmt_ll_tx_enable_loop(hal->regs, channel_id, t->loop_count != 0);
#if SOC_RMT_SUPPORT_TX_LOOP_AUTO_STOP
    rmt_ll_tx_enable_loop_autostop(hal->regs, channel_id, true);
#endif
#if SOC_RMT_SUPPORT_TX_LOOP_COUNT
    rmt_ll_tx_reset_loop_count(hal->regs, channel_id);
    rmt_ll_tx_enable_loop_count(hal->regs, channel_id, t->loop_count > 0);
    /* transfer loops in batches */
    if (t->remain_loop_count > 0) {
        uint32_t this_loop_count = MIN(t->remain_loop_count, RMT_LL_MAX_LOOP_COUNT_PER_BATCH);
        rmt_ll_tx_set_loop_count(hal->regs, channel_id, this_loop_count);
        t->remain_loop_count -= this_loop_count;
    }
#endif
    k_spin_unlock(&channel->spinlock, key);

    /* enable/disable specific interrupts */
    key = k_spin_lock(&group->spinlock);
#if SOC_RMT_SUPPORT_TX_LOOP_COUNT
    rmt_ll_enable_interrupt(hal->regs, RMT_LL_EVENT_TX_LOOP_END(channel_id), t->loop_count > 0);
#endif
    /* in DMA mode, DMA eof event plays the similar functionality to this threshold interrupt, so only enable it for non-DMA mode */
    if (!channel->dma_chan) {
        /* don't enable threshold interrupt with loop mode on
           threshold interrupt will be disabled in `rmt_encode_eof()` */
        rmt_ll_enable_interrupt(hal->regs, RMT_LL_EVENT_TX_THRES(channel_id), t->loop_count == 0);
        /* Threshold interrupt will be generated by accident, clear it before starting new transmission */
        rmt_ll_clear_interrupt_status(hal->regs, RMT_LL_EVENT_TX_THRES(channel_id));
    }
    /* don't generate trans done event for loop transmission */
    rmt_ll_enable_interrupt(hal->regs, RMT_LL_EVENT_TX_DONE(channel_id), t->loop_count == 0);
    k_spin_unlock(&group->spinlock, key);

    /* at the beginning of a new transaction, encoding memory offset should start from zero. */
    /* It will increase in the encode function e.g. `rmt_encode_copy()` */
    tx_chan->mem_off = 0;
    /* use the full memory block for the beginning encoding session */
    tx_chan->mem_end = tx_chan->ping_pong_symbols * 2;
    /* perform the encoding session, return the number of encoded symbols */
    t->transmitted_symbol_num = rmt_encode_check_result(tx_chan, t);
    /* we're going to perform ping-pong operation, so the next encoding end position is the middle */
    tx_chan->mem_end = tx_chan->ping_pong_symbols;

#if SOC_RMT_SUPPORT_DMA
    if (channel->dma_chan) {
        gdma_start(channel->dma_chan, (intptr_t)tx_chan->dma_nodes);
        /* delay a while, wait for DMA data going to RMT memory block */
        esp_rom_delay_us(1);
    }
#endif
    /* turn on the TX machine */
    key = k_spin_lock(&channel->spinlock);
    rmt_ll_tx_fix_idle_level(hal->regs, channel_id, t->flags.eot_level, true);
    rmt_ll_tx_start(hal->regs, channel_id);
    k_spin_unlock(&channel->spinlock, key);
}

static int rmt_tx_enable(rmt_channel_handle_t channel)
{
#if SOC_RMT_SUPPORT_DMA
    k_spinlock_key_t key;
#endif

    /* can enable the channel when it's in "init" state */
    rmt_fsm_t expected_fsm = RMT_FSM_INIT;
    if (!atomic_compare_exchange_strong(&channel->fsm, &expected_fsm, RMT_FSM_ENABLE_WAIT)) {
        LOG_ERR("Channel not initialized");
        return -ENODEV;
    }
    rmt_tx_channel_t *tx_chan = __containerof(channel, rmt_tx_channel_t, base);

#if CONFIG_PM_ENABLE
    /* acquire power manager lock */
    if (channel->pm_lock) {
        esp_pm_lock_acquire(channel->pm_lock);
    }
#endif

#if SOC_RMT_SUPPORT_DMA
    rmt_group_t *group = channel->group;
    rmt_hal_context_t *hal = &group->hal;
    int channel_id = channel->channel_id;
    if (channel->dma_chan) {
        /* enable the DMA access mode */
        key = k_spin_lock(&channel->spinlock);
        rmt_ll_tx_enable_dma(hal->regs, channel_id, true);
        k_spin_unlock(&channel->spinlock, key);
        gdma_connect(channel->dma_chan, GDMA_MAKE_TRIGGER(GDMA_TRIG_PERIPH_RMT, 0));
    }
#endif

    atomic_store(&channel->fsm, RMT_FSM_ENABLE);

    /* check if we need to start one pending transaction */
    rmt_tx_trans_desc_t *t = NULL;
    expected_fsm = RMT_FSM_ENABLE;
    if (atomic_compare_exchange_strong(&channel->fsm, &expected_fsm, RMT_FSM_RUN_WAIT)) {
        if (k_msgq_get(&tx_chan->trans_queues[RMT_TX_QUEUE_PROGRESS], &t, K_NO_WAIT) == 0) {
            /* sanity check */
            assert(t);
            atomic_store(&channel->fsm, RMT_FSM_RUN);
            rmt_tx_do_transaction(tx_chan, t);
        } else {
            atomic_store(&channel->fsm, RMT_FSM_ENABLE);
        }
    }

    return 0;
}

static int rmt_tx_disable(rmt_channel_handle_t channel)
{
#if CONFIG_PM_ENABLE
    esp_err_t ret;
#endif
    rmt_tx_channel_t *tx_chan = __containerof(channel, rmt_tx_channel_t, base);
    rmt_group_t *group = channel->group;
    rmt_hal_context_t *hal = &group->hal;
    int channel_id = channel->channel_id;
    k_spinlock_key_t key;

    /* can disable the channel when it's in `enable` or `run` state */
    bool valid_state = false;
    rmt_fsm_t expected_fsm = RMT_FSM_ENABLE;
    if (atomic_compare_exchange_strong(&channel->fsm, &expected_fsm, RMT_FSM_INIT_WAIT)) {
        valid_state = true;
    }
    expected_fsm = RMT_FSM_RUN;
    if (atomic_compare_exchange_strong(&channel->fsm, &expected_fsm, RMT_FSM_INIT_WAIT)) {
        valid_state = true;
        /* disable the hardware */
        key = k_spin_lock(&channel->spinlock);
        rmt_ll_tx_enable_loop(hal->regs, channel->channel_id, false);
#if SOC_RMT_SUPPORT_TX_ASYNC_STOP
        rmt_ll_tx_stop(hal->regs, channel->channel_id);
#endif
        k_spin_unlock(&channel->spinlock, key);

        key = k_spin_lock(&group->spinlock);
        rmt_ll_enable_interrupt(hal->regs, RMT_LL_EVENT_TX_MASK(channel_id), false);
#if !SOC_RMT_SUPPORT_TX_ASYNC_STOP
        /* we do a trick to stop the undergoing transmission
           stop interrupt, insert EOF marker to the RMT memory, polling the trans_done event */
        channel->hw_mem_base[0].val = 0;
        while (!(rmt_ll_tx_get_interrupt_status_raw(hal->regs, channel_id) & RMT_LL_EVENT_TX_DONE(channel_id))) {}
#endif
        rmt_ll_clear_interrupt_status(hal->regs, RMT_LL_EVENT_TX_MASK(channel_id));
        k_spin_unlock(&group->spinlock, key);
    }
    if (!valid_state) {
        LOG_ERR("Channel can't be disabled in state %d", expected_fsm);
        return -ENODEV;
    }

#if SOC_RMT_SUPPORT_DMA
    /* disable the DMA */
    if (channel->dma_chan) {
        gdma_stop(channel->dma_chan);
        gdma_disconnect(channel->dma_chan);

        /* disable DMA access mode */
        key = k_spin_lock(&channel->spinlock);
        rmt_ll_tx_enable_dma(hal->regs, channel_id, false);
        k_spin_unlock(&channel->spinlock, key);
    }
#endif

    /* recycle the interrupted transaction */
    if (tx_chan->cur_trans) {
        k_msgq_put(&tx_chan->trans_queues[RMT_TX_QUEUE_COMPLETE], &tx_chan->cur_trans, K_NO_WAIT);
        /* reset corresponding encoder */
        rmt_encoder_reset(tx_chan->cur_trans->encoder);
    }
    tx_chan->cur_trans = NULL;

#if CONFIG_PM_ENABLE
    /* release power manager lock */
    if (channel->pm_lock) {
        ret = esp_pm_lock_release(channel->pm_lock);
        if (ret) {
            LOG_ERR("Release pm_lock failed");
            return -ENODEV;
        }
    }
#endif

    /* finally we switch to the INIT state */
    atomic_store(&channel->fsm, RMT_FSM_INIT);

    return 0;
}

static int rmt_tx_modulate_carrier(rmt_channel_handle_t channel, const rmt_carrier_config_t *config)
{
    rmt_group_t *group = channel->group;
    rmt_hal_context_t *hal = &group->hal;
    int group_id = group->group_id;
    int channel_id = channel->channel_id;
    uint32_t real_frequency = 0;
    k_spinlock_key_t key;

    if (config && config->frequency_hz) {
        /* carrier module works base on group clock */
        uint32_t total_ticks = group->resolution_hz / config->frequency_hz; /* Note this division operation will lose precision */
        uint32_t high_ticks = total_ticks * config->duty_cycle;
        uint32_t low_ticks = total_ticks - high_ticks;

        key = k_spin_lock(&channel->spinlock);
        rmt_ll_tx_set_carrier_level(hal->regs, channel_id, !config->flags.polarity_active_low);
        rmt_ll_tx_set_carrier_high_low_ticks(hal->regs, channel_id, high_ticks, low_ticks);
#if SOC_RMT_SUPPORT_TX_CARRIER_DATA_ONLY
        rmt_ll_tx_enable_carrier_always_on(hal->regs, channel_id, config->flags.always_on);
#endif
        k_spin_unlock(&channel->spinlock, key);
        /* save real carrier frequency */
        real_frequency = group->resolution_hz / total_ticks;
    }

    /* enable/disable carrier modulation */
    key = k_spin_lock(&channel->spinlock);
    rmt_ll_tx_enable_carrier_modulation(hal->regs, channel_id, real_frequency > 0);
    k_spin_unlock(&channel->spinlock, key);

    if (real_frequency > 0) {
        LOG_DBG("enable carrier modulation for channel(%d,%d), freq=%"PRIu32"Hz", group_id, channel_id, real_frequency);
    } else {
        LOG_DBG("disable carrier modulation for channel(%d,%d)", group_id, channel_id);
    }
    return 0;
}

static bool IRAM_ATTR rmt_isr_handle_tx_threshold(rmt_tx_channel_t *tx_chan)
{
    /* continue ping-pong transmission */
    rmt_tx_trans_desc_t *t = tx_chan->cur_trans;
    size_t encoded_symbols = t->transmitted_symbol_num;
    /* encoding finished, only need to send the EOF symbol */
    if (t->flags.encoding_done) {
        rmt_tx_mark_eof(tx_chan);
        encoded_symbols += 1;
    } else {
        encoded_symbols += rmt_encode_check_result(tx_chan, t);
    }
    t->transmitted_symbol_num = encoded_symbols;
    tx_chan->mem_end = tx_chan->ping_pong_symbols * 3 - tx_chan->mem_end; /* mem_end equals to either ping_pong_symbols or ping_pong_symbols*2 */

    return false;
}

static bool IRAM_ATTR rmt_isr_handle_tx_done(rmt_tx_channel_t *tx_chan)
{
    rmt_channel_t *channel = &tx_chan->base;
    rmt_tx_trans_desc_t *trans_desc = NULL;
    bool need_yield = false;

    rmt_fsm_t expected_fsm = RMT_FSM_RUN;
    if (atomic_compare_exchange_strong(&channel->fsm, &expected_fsm, RMT_FSM_ENABLE_WAIT)) {
        trans_desc = tx_chan->cur_trans;
        /* move current finished transaction to the complete queue */
        if (k_msgq_put(&tx_chan->trans_queues[RMT_TX_QUEUE_COMPLETE], &trans_desc, K_NO_WAIT) == 0) {
            need_yield = true;
        }
        tx_chan->cur_trans = NULL;
        atomic_store(&channel->fsm, RMT_FSM_ENABLE);

        /* invoke callback */
        rmt_tx_done_callback_t done_cb = tx_chan->on_trans_done;
        if (done_cb) {
            rmt_tx_done_event_data_t edata = {
                .num_symbols = trans_desc->transmitted_symbol_num,
            };
            if (done_cb(channel, &edata, tx_chan->user_data)) {
                need_yield = true;
            }
        }
    }

    /* let's try start the next pending transaction */
    expected_fsm = RMT_FSM_ENABLE;
    if (atomic_compare_exchange_strong(&channel->fsm, &expected_fsm, RMT_FSM_RUN_WAIT)) {
        if (k_msgq_get(&tx_chan->trans_queues[RMT_TX_QUEUE_PROGRESS], &trans_desc, K_NO_WAIT) == 0) {
            /* sanity check */
            assert(trans_desc);
            atomic_store(&channel->fsm, RMT_FSM_RUN);
            /* begin a new transaction */
            rmt_tx_do_transaction(tx_chan, trans_desc);
            need_yield = true;
        } else {
            atomic_store(&channel->fsm, RMT_FSM_ENABLE);
        }
    }

    return need_yield;
}

#if SOC_RMT_SUPPORT_TX_LOOP_COUNT
static bool IRAM_ATTR rmt_isr_handle_tx_loop_end(rmt_tx_channel_t *tx_chan)
{
    rmt_channel_t *channel = &tx_chan->base;
    rmt_group_t *group = channel->group;
    rmt_hal_context_t *hal = &group->hal;
    uint32_t channel_id = channel->channel_id;
    rmt_tx_trans_desc_t *trans_desc = NULL;
    bool need_yield = false;
    k_spinlock_key_t key;

    trans_desc = tx_chan->cur_trans;
    if (trans_desc) {
#if !SOC_RMT_SUPPORT_TX_LOOP_AUTO_STOP
        key = k_spin_lock(&channel->spinlock);
        /* This is a workaround for chips that don't support loop auto stop */
        /* Although we stop the transaction immediately in ISR handler, it's still possible that some rmt symbols have sneaked out */
        rmt_ll_tx_stop(hal->regs, channel_id);
        k_spin_unlock(&channel->spinlock, key);
#endif

        /* continue unfinished loop transaction */
        if (trans_desc->remain_loop_count) {
            uint32_t this_loop_count = MIN(trans_desc->remain_loop_count, RMT_LL_MAX_LOOP_COUNT_PER_BATCH);
            trans_desc->remain_loop_count -= this_loop_count;
            key = k_spin_lock(&channel->spinlock);
            rmt_ll_tx_set_loop_count(hal->regs, channel_id, this_loop_count);
            rmt_ll_tx_reset_pointer(hal->regs, channel_id);
            /* continue the loop transmission, don't need to fill the RMT symbols again, just restart the engine */
            rmt_ll_tx_start(hal->regs, channel_id);
            k_spin_unlock(&channel->spinlock, key);
            return need_yield;
        }

        /* loop transaction finished */
        rmt_fsm_t expected_fsm = RMT_FSM_RUN;
        if (atomic_compare_exchange_strong(&channel->fsm, &expected_fsm, RMT_FSM_ENABLE_WAIT)) {
            /* move current finished transaction to the complete queue */
            if (k_msgq_put(&tx_chan->trans_queues[RMT_TX_QUEUE_COMPLETE], &trans_desc, K_NO_WAIT) == 0) {
                need_yield = true;
            }
            tx_chan->cur_trans = NULL;
            atomic_store(&channel->fsm, RMT_FSM_ENABLE);

            /* invoke callback */
            rmt_tx_done_callback_t done_cb = tx_chan->on_trans_done;
            if (done_cb) {
                rmt_tx_done_event_data_t edata = {
                    .num_symbols = trans_desc->transmitted_symbol_num,
                };
                if (done_cb(channel, &edata, tx_chan->user_data)) {
                    need_yield = true;
                }
            }
        }

        /* let's try start the next pending transaction */
        expected_fsm = RMT_FSM_ENABLE;
        if (atomic_compare_exchange_strong(&channel->fsm, &expected_fsm, RMT_FSM_RUN_WAIT)) {
            if (k_msgq_get(&tx_chan->trans_queues[RMT_TX_QUEUE_PROGRESS], &trans_desc, K_NO_WAIT) == 0) {
                /* sanity check */
                assert(trans_desc);
                atomic_store(&channel->fsm, RMT_FSM_RUN);
                /* begin a new transaction */
                rmt_tx_do_transaction(tx_chan, trans_desc);
                need_yield = true;
            } else {
                atomic_store(&channel->fsm, RMT_FSM_ENABLE);
            }
        }
    }

    return need_yield;
}
#endif

static void IRAM_ATTR rmt_tx_default_isr(void *args)
{
    rmt_tx_channel_t *tx_chan = (rmt_tx_channel_t *)args;
    rmt_channel_t *channel = &tx_chan->base;
    rmt_group_t *group = channel->group;
    rmt_hal_context_t *hal = &group->hal;
    uint32_t channel_id = channel->channel_id;

    uint32_t status = rmt_ll_tx_get_interrupt_status(hal->regs, channel_id);
    rmt_ll_clear_interrupt_status(hal->regs, status);

    /* Tx threshold interrupt */
    if (status & RMT_LL_EVENT_TX_THRES(channel_id)) {
        rmt_isr_handle_tx_threshold(tx_chan);
    }

    /* Tx end interrupt */
    if (status & RMT_LL_EVENT_TX_DONE(channel_id)) {
        rmt_isr_handle_tx_done(tx_chan);
    }

#if SOC_RMT_SUPPORT_TX_LOOP_COUNT
    /* Tx loop end interrupt */
    if (status & RMT_LL_EVENT_TX_LOOP_END(channel_id)) {
        rmt_isr_handle_tx_loop_end(tx_chan);
    }
#endif
}

#if SOC_RMT_SUPPORT_DMA
static bool IRAM_ATTR rmt_dma_tx_eof_cb(gdma_channel_handle_t dma_chan, gdma_event_data_t *event_data, void *user_data)
{
    rmt_tx_channel_t *tx_chan = (rmt_tx_channel_t *)user_data;
    dma_descriptor_t *eof_desc = (dma_descriptor_t *)event_data->tx_eof_desc_addr;
    /* if the DMA descriptor link is still a ring (i.e. hasn't broken down by `rmt_tx_mark_eof()`), then we treat it as a valid ping-pong event */
    if (eof_desc->next && eof_desc->next->next) {
        /* continue pingpong transmission */
        rmt_tx_trans_desc_t *t = tx_chan->cur_trans;
        size_t encoded_symbols = t->transmitted_symbol_num;
        if (t->flags.encoding_done) {
            rmt_tx_mark_eof(tx_chan);
            encoded_symbols += 1;
        } else {
            encoded_symbols += rmt_encode_check_result(tx_chan, t);
        }
        t->transmitted_symbol_num = encoded_symbols;
        tx_chan->mem_end = tx_chan->ping_pong_symbols * 3 - tx_chan->mem_end; /* mem_end equals to either ping_pong_symbols or ping_pong_symbols*2 */
        /* tell DMA that we have a new descriptor attached */
        gdma_append(dma_chan);
    }
    return false;
}
#endif
