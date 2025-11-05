/*
 * Copyright (c) 2025 Institute of Software, Chinese Academy of Sciences (ISCAS)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT riscv_aplic

#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/devicetree/interrupt_controller.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/interrupt_controller/riscv_aplic.h>
#include <zephyr/sw_isr_table.h>
#include <zephyr/irq_multilevel.h>
#include <zephyr/arch/riscv/irq.h>

LOG_MODULE_REGISTER(intc_aplic, CONFIG_INTC_LOG_LEVEL);

extern uint32_t z_get_sw_isr_table_idx(unsigned int irq);


static int aplic_configure_direct_mode(const struct device *dev);
static inline uint32_t aplic_get_idc_claim(const struct device *dev, uint32_t hart_id);
static inline const struct device *aplic_get_dev(void);
 
#define APLIC_DOMAINCFG                0x0000

#define APLIC_SOURCECFG_BASE           0x0004
#define APLIC_SOURCECFG_SIZE           0x0004

#define APLIC_TARGET_BASE              0x3000
#define APLIC_TARGET_SIZE              0x0004

#define APLIC_SETIP_BASE               0x1C00
#define APLIC_CLRIP_BASE               0x1D00
#define APLIC_SETIPNUM_LE              0x2000

#define APLIC_SETIE_BASE               0x1E00
#define APLIC_CLRIE_BASE               0x1F00
#define APLIC_SETIENUM                 0x1EDC
#define APLIC_CLRIENUM                 0x1FDC

#define APLIC_MAX_PRIORITY             255
#ifndef CONFIG_APLIC_MAX_IRQS
#define CONFIG_APLIC_MAX_IRQS          1024
#endif

#define APLIC_DOMAINCFG_IE_MASK        0x00000100
#define APLIC_DOMAINCFG_IE_SHIFT       8
#define APLIC_DOMAINCFG_DM_MASK        0x00000004
#define APLIC_DOMAINCFG_DM_SHIFT       2
#define APLIC_DOMAINCFG_DM_DIRECT      0x0
#define APLIC_DOMAINCFG_DM_MSI         0x1

#define APLIC_SOURCECFG_SM_MASK        0x7
#define APLIC_SOURCECFG_SM_INACTIVE    0x0
#define APLIC_SOURCECFG_SM_DETACHED    0x1
#define APLIC_SOURCECFG_SM_EDGE_RISING 0x4
#define APLIC_SOURCECFG_SM_EDGE_FALLING 0x5
#define APLIC_SOURCECFG_SM_LEVEL_HIGH  0x6
#define APLIC_SOURCECFG_SM_LEVEL_LOW   0x7
#define APLIC_SOURCECFG_D_MASK         0x400
#define APLIC_SOURCECFG_D_SHIFT        10

#define APLIC_TARGETCFG_HART_SHIFT     18
#define APLIC_TARGETCFG_HART_MASK      0x3FFF
#define APLIC_TARGETCFG_PRIORITY_SHIFT 0
#define APLIC_TARGETCFG_PRIORITY_MASK  0xFF
#define APLIC_DEFAULT_PRIORITY         1

#define APLIC_IDC_IDELIVERY            0x0000
#define APLIC_IDC_IFORCE               0x0004
#define APLIC_IDC_ITHRESHOLD           0x0008
#define APLIC_IDC_TOPI                 0x0018
#define APLIC_IDC_CLAIMI               0x001C
#define APLIC_IDC_TOPI_ID_SHIFT        16
#define APLIC_IDC_TOPI_ID_MASK         0x3FF
 
struct aplic_config {
    mem_addr_t base;
    uint32_t num_sources;
    uint32_t max_priority;
    uint32_t num_harts;
    uint32_t irq;
    void (*irq_config_func)(void);
    const struct _isr_table_entry *isr_table;
};

struct aplic_data {
    struct k_spinlock lock;
    uint32_t total_interrupts;
    uint32_t direct_interrupts;
    uint32_t hart_thresholds[CONFIG_MP_MAX_NUM_CPUS];
    uint32_t ie_shadow[(CONFIG_APLIC_MAX_IRQS + 31) / 32];
};
 
static const struct device *aplic_dev = NULL;
static uint32_t save_irq[CONFIG_MP_MAX_NUM_CPUS];
static const struct device *save_dev[CONFIG_MP_MAX_NUM_CPUS];
 
static inline const struct device *aplic_get_dev(void)
{
    return aplic_dev;
}
 
static inline void aplic_write(const struct device *dev, mem_addr_t addr, uint32_t value)
{

    *(volatile uint32_t *)addr = value;
}

static inline uint32_t aplic_read(const struct device *dev, mem_addr_t addr)
{

    return *(volatile uint32_t *)addr;
}
 
static inline mem_addr_t get_sourcecfg_addr(const struct device *dev, uint32_t irq_id)
{
    const struct aplic_config *config = dev->config;

    if (irq_id == 0) {
        return config->base + APLIC_SOURCECFG_BASE;
    }

    return config->base + APLIC_SOURCECFG_BASE + ((irq_id - 1U) * APLIC_SOURCECFG_SIZE);
}
 
static inline mem_addr_t get_targetcfg_addr(const struct device *dev, uint32_t source_id)
{
    const struct aplic_config *config = dev->config;

    if (source_id == 0) {
        return config->base + APLIC_TARGET_BASE;
    }

    return config->base + APLIC_TARGET_BASE + ((source_id - 1U) * APLIC_TARGET_SIZE);
}
 
static inline mem_addr_t get_idc_addr(const struct device *dev, uint32_t hart_id, uint32_t offset)
{
    const struct aplic_config *config = dev->config;
    return config->base + 0x4000 + (hart_id * 32) + offset;
}
 
static atomic_t aplic_handler_called = ATOMIC_INIT(0);

void aplic_direct_mode_handler(const void *arg)
{
    const struct device *dev = (const struct device *)arg;
    struct aplic_data *data;

    atomic_inc(&aplic_handler_called);

    if (!dev) {
        return;
    }

    data = dev->data;
    if (!data) {
        return;
    }

    const struct aplic_config *config = dev->config;
    uint32_t cpu_id = arch_curr_cpu()->id;
    const struct _isr_table_entry *ite;
    uint32_t local_irq;
    uint32_t hw_irq;
    bool processed_any = false;
    save_irq[cpu_id] = 0;
    save_dev[cpu_id] = NULL;
    
    while ((hw_irq = aplic_get_idc_claim(dev, cpu_id)) != 0) {

        local_irq = (hw_irq >> APLIC_IDC_TOPI_ID_SHIFT) & APLIC_IDC_TOPI_ID_MASK;

        if (local_irq == 0) {
            save_irq[cpu_id] = 0;
            save_dev[cpu_id] = NULL;
            continue;
        }

        k_spinlock_key_t key = k_spin_lock(&data->lock);
        data->total_interrupts++;
        data->direct_interrupts++;
        k_spin_unlock(&data->lock, key);
        save_irq[cpu_id] = local_irq;
        save_dev[cpu_id] = dev;
        processed_any = true;

        if (local_irq >= config->num_sources) {
            z_irq_spurious(NULL);
        }

        bool isr_found = false;
        uint32_t irq_l2 = irq_to_level_2(local_irq) | RISCV_IRQ_MEXT;
        uint32_t isr_table_idx = z_get_sw_isr_table_idx(irq_l2);
        
        if (isr_table_idx < CONFIG_NUM_IRQS) {
            ite = &_sw_isr_table[isr_table_idx];

            if ((ite->isr != NULL) && (ite->isr != &z_irq_spurious)) {
                isr_found = true;
                save_irq[cpu_id] = 0;
                save_dev[cpu_id] = NULL;
                ite->isr(ite->arg);
            }
        }
        
        if (!isr_found && local_irq > 0 && local_irq <= config->num_sources) {
            save_irq[cpu_id] = 0;
            save_dev[cpu_id] = NULL;
        }
        
        save_irq[cpu_id] = 0;
        save_dev[cpu_id] = NULL;
    }
    

    if (processed_any) {
        save_irq[cpu_id] = 0;
        save_dev[cpu_id] = NULL;
    }
}
 

static int aplic_configure_direct_mode(const struct device *dev)
{
    const struct aplic_config *config = dev->config;
    struct aplic_data *data = dev->data;
    uint32_t domaincfg;
    uint32_t ie_words = DIV_ROUND_UP(config->num_sources, 32U);

    for (uint32_t word = 0; word < ie_words; word++) {
        mem_addr_t clrie_addr = config->base + APLIC_CLRIE_BASE + (word * sizeof(uint32_t));
        aplic_write(dev, clrie_addr, 0xFFFFFFFFU);
    }
    
    uint32_t default_target = ((0U & APLIC_TARGETCFG_HART_MASK) << APLIC_TARGETCFG_HART_SHIFT) |
                              (APLIC_DEFAULT_PRIORITY & APLIC_TARGETCFG_PRIORITY_MASK);
    
    for (uint32_t i = 1; i <= config->num_sources; i++) {
        mem_addr_t sourcecfg_addr = get_sourcecfg_addr(dev, i);
        mem_addr_t targetcfg_addr = get_targetcfg_addr(dev, i);
        
        aplic_write(dev, sourcecfg_addr, APLIC_SOURCECFG_SM_INACTIVE);
        
        aplic_write(dev, targetcfg_addr, default_target);
    }
    
    aplic_write(dev, config->base + APLIC_DOMAINCFG, 0U);
    
    for (uint32_t hart = 0; hart < config->num_harts; hart++) {
        mem_addr_t threshold_addr = get_idc_addr(dev, hart, APLIC_IDC_ITHRESHOLD);
        aplic_write(dev, threshold_addr, 0U);
        
        mem_addr_t idelivery_addr = get_idc_addr(dev, hart, APLIC_IDC_IDELIVERY);
        aplic_write(dev, idelivery_addr, 1U);
    }
    
    domaincfg = aplic_read(dev, config->base + APLIC_DOMAINCFG);
    domaincfg |= APLIC_DOMAINCFG_IE_MASK;
    aplic_write(dev, config->base + APLIC_DOMAINCFG, domaincfg);
     
    data->direct_interrupts = 0;
    return 0;
}

static inline uint32_t aplic_get_idc_claim(const struct device *dev, uint32_t hart_id)
{
    mem_addr_t claim_addr = get_idc_addr(dev, hart_id, APLIC_IDC_CLAIMI);
    return aplic_read(dev, claim_addr);
}

static inline uint32_t aplic_get_idc_topi(const struct device *dev, uint32_t hart_id)
{
    mem_addr_t topi_addr = get_idc_addr(dev, hart_id, APLIC_IDC_TOPI);
    return aplic_read(dev, topi_addr);
}

static inline void aplic_set_hart_delivery(const struct device *dev, uint32_t hart_id, bool enable)
{
    mem_addr_t idelivery_addr = get_idc_addr(dev, hart_id, APLIC_IDC_IDELIVERY);
    aplic_write(dev, idelivery_addr, enable ? 1U : 0U);
}

uint32_t aplic_read_domain_config(void)
{
    const struct device *dev = aplic_get_dev();
    if (!dev) {
        return 0;
    }
    const struct aplic_config *config = dev->config;
    return aplic_read(dev, config->base + APLIC_DOMAINCFG);
}

uint32_t aplic_read_idc_delivery(void)
{
    const struct device *dev = aplic_get_dev();
    if (!dev) {
        return 0;
    }
    mem_addr_t idelivery_addr = get_idc_addr(dev, 0, APLIC_IDC_IDELIVERY);
    return aplic_read(dev, idelivery_addr);
}

uint32_t aplic_read_idc_threshold(void)
{
    const struct device *dev = aplic_get_dev();
    if (!dev) {
        return 0;
    }
    mem_addr_t ithreshold_addr = get_idc_addr(dev, 0, APLIC_IDC_ITHRESHOLD);
    return aplic_read(dev, ithreshold_addr);
}

uint32_t aplic_read_idc_topi(void)
{
    const struct device *dev = aplic_get_dev();
    if (!dev) {
        return 0;
    }
    mem_addr_t topi_addr = get_idc_addr(dev, 0, APLIC_IDC_TOPI);
    return aplic_read(dev, topi_addr);
}
 
void riscv_aplic_irq_enable(uint32_t irq)
{
    const struct device *dev = aplic_get_dev();
    const struct aplic_config *config;
    struct aplic_data *data;
     
    if (!dev) {
        return;
    }
     
    config = dev->config;
    if ((irq == 0U) || (irq > config->num_sources)) {
        LOG_ERR("APLIC: Invalid IRQ number %u", irq);
        return;
    }
     
    mem_addr_t setienum_addr = config->base + APLIC_SETIENUM;
    aplic_write(dev, setienum_addr, irq);

    data = dev->data;
    uint32_t word = (irq - 1U) / 32U;
    uint32_t bit = (irq - 1U) % 32U;
    k_spinlock_key_t key = k_spin_lock(&data->lock);
    data->ie_shadow[word] |= BIT(bit);
    k_spin_unlock(&data->lock, key);
     
    (void)irq;
}
 
void riscv_aplic_irq_disable(uint32_t irq)
{
    const struct device *dev = aplic_get_dev();
    const struct aplic_config *config;
    struct aplic_data *data;
     
    if (!dev) {
        return;
    }
     
    config = dev->config;
    if ((irq == 0U) || (irq > config->num_sources)) {
        LOG_ERR("APLIC: Invalid IRQ number %u", irq);
        return;
    }
     
    mem_addr_t clrienum_addr = config->base + APLIC_CLRIENUM;
    aplic_write(dev, clrienum_addr, irq);
    
    data = dev->data;
    uint32_t word = (irq - 1U) / 32U;
    uint32_t bit = (irq - 1U) % 32U;
    k_spinlock_key_t key = k_spin_lock(&data->lock);
    data->ie_shadow[word] &= ~BIT(bit);
    k_spin_unlock(&data->lock, key);

}
 
int riscv_aplic_irq_is_enabled(uint32_t irq)
{
    const struct device *dev = aplic_get_dev();
    const struct aplic_config *config;
    struct aplic_data *data;
    uint32_t word, bit;
     
    if (!dev) {
        return 0;
    }
     
    config = dev->config;
    if ((irq == 0U) || (irq > config->num_sources)) {
        LOG_ERR("APLIC: Invalid IRQ number %u", irq);
        return 0;
    }
     
    data = dev->data;
    word = (irq - 1U) / 32U;
    bit = (irq - 1U) % 32U;
    return (data->ie_shadow[word] & BIT(bit)) ? 1 : 0;
}
 
void riscv_aplic_set_priority(uint32_t irq, uint32_t prio)
{
    const struct device *dev = aplic_get_dev();
    const struct aplic_config *config;
    mem_addr_t targetcfg_addr;
    uint32_t targetcfg_value;
     
    if (!dev) {
        return;
    }
     
    config = dev->config;
    if ((irq == 0U) || (irq > config->num_sources)) {
        LOG_ERR("APLIC: Invalid IRQ number %u", irq);
        return;
    }
     
    if (prio > config->max_priority) {
        prio = config->max_priority;
    }
     
    targetcfg_addr = get_targetcfg_addr(dev, irq);
    targetcfg_value = aplic_read(dev, targetcfg_addr);

    targetcfg_value &= ~(APLIC_TARGETCFG_PRIORITY_MASK << APLIC_TARGETCFG_PRIORITY_SHIFT);
    targetcfg_value |= (prio & APLIC_TARGETCFG_PRIORITY_MASK)
             << APLIC_TARGETCFG_PRIORITY_SHIFT;

    aplic_write(dev, targetcfg_addr, targetcfg_value);
    
    uint32_t verify_targetcfg = aplic_read(dev, targetcfg_addr);
    uint32_t verify_hart = (verify_targetcfg >> APLIC_TARGETCFG_HART_SHIFT) & APLIC_TARGETCFG_HART_MASK;
    uint32_t verify_prio = (verify_targetcfg >> APLIC_TARGETCFG_PRIORITY_SHIFT) & APLIC_TARGETCFG_PRIORITY_MASK;
    
    (void)verify_targetcfg; (void)verify_hart; (void)verify_prio; (void)irq; (void)targetcfg_value;
 }
 
void riscv_aplic_irq_set_pending(uint32_t irq)
{
    const struct device *dev = aplic_get_dev();
    const struct aplic_config *config;

    if (!dev) {
        return;
    }

    config = dev->config;
    if ((irq == 0U) || (irq > config->num_sources)) {
        LOG_ERR("APLIC: Invalid IRQ number %u", irq);
        return;
    }

    mem_addr_t sourcecfg_addr = get_sourcecfg_addr(dev, irq);
    uint32_t sourcecfg_value = aplic_read(dev, sourcecfg_addr);
    
    if ((sourcecfg_value & APLIC_SOURCECFG_SM_MASK) == APLIC_SOURCECFG_SM_INACTIVE) {
        LOG_ERR("APLIC: Cannot set pending for INACTIVE source %u (config=0x%08x)", 
                irq, sourcecfg_value);
        return;
    }

    struct aplic_data *data = dev->data;
    uint32_t word = (irq - 1U) / 32U;
    uint32_t bit = (irq - 1U) % 32U;
    bool is_enabled = (data->ie_shadow[word] & BIT(bit)) != 0;
    
    if (!is_enabled) {
        LOG_WRN("APLIC: Setting pending for disabled interrupt %u", irq);
    }

    uint32_t setip_word = (irq - 1U) / 32U;
    uint32_t setip_bit = (irq - 1U) % 32U;
    mem_addr_t clrip_addr = config->base + APLIC_CLRIP_BASE + (setip_word * sizeof(uint32_t));
    aplic_write(dev, clrip_addr, BIT(setip_bit));
    
    k_busy_wait(100);
    
    mem_addr_t setipnum_addr = config->base + APLIC_SETIPNUM_LE;   
    aplic_write(dev, setipnum_addr, irq);  
    __asm__ volatile("fence w,w" ::: "memory");
    
}

int riscv_aplic_irq_is_pending(uint32_t irq)
{
    const struct device *dev = aplic_get_dev();
    
    if (!dev) {
        return 0;
    }

    uint32_t topi = aplic_get_idc_topi(dev, 0);
    uint32_t pending_irq = (topi >> APLIC_IDC_TOPI_ID_SHIFT) & APLIC_IDC_TOPI_ID_MASK;
    const struct aplic_config *config = dev->config;

    if (irq > config->num_sources || irq == 0) {
        return 0;
    }

    uint32_t setip_word = (irq - 1U) / 32U;
    uint32_t setip_bit = (irq - 1U) % 32U;
    mem_addr_t setip_addr = config->base + APLIC_SETIP_BASE + (setip_word * sizeof(uint32_t));
    uint32_t setip_value = aplic_read(dev, setip_addr);
    uint32_t is_pending = (setip_value >> setip_bit) & 1U;

    if (pending_irq == irq) {
        return 1;
    }
    
    return is_pending;
}

 unsigned int riscv_aplic_get_irq(void)
 {
    return save_irq[arch_curr_cpu()->id];
 }
 
 const struct device *riscv_aplic_get_dev(void)
 {
    const struct device *dev = save_dev[arch_curr_cpu()->id];
    return dev ? dev : aplic_dev;
 }
 
 uint32_t riscv_aplic_get_total_interrupts(void)
 {
     const struct device *dev = aplic_get_dev();
     struct aplic_data *data;
     uint32_t count;
     
     if (!dev) {
         return 0;
     }
     
     data = dev->data;
     k_spinlock_key_t key = k_spin_lock(&data->lock);
     count = data->total_interrupts;
     k_spin_unlock(&data->lock, key);
     
     return count;
 }
 
uint32_t riscv_aplic_get_handler_calls(void)
{
    return atomic_get(&aplic_handler_called);
}

uint32_t riscv_aplic_get_direct_interrupts(void)
{
    const struct device *dev = aplic_get_dev();
    struct aplic_data *data;
    uint32_t count;
     
    if (!dev) {
         return 0;
    }
     
    data = dev->data;
    k_spinlock_key_t key = k_spin_lock(&data->lock);
    count = data->direct_interrupts;
    k_spin_unlock(&data->lock, key);
     
    return count;
}
 
void riscv_aplic_reset_stats(void)
{
    const struct device *dev = aplic_get_dev();
    struct aplic_data *data;
     
    if (!dev) {
        return;
    }
     
    data = dev->data;
    k_spinlock_key_t key = k_spin_lock(&data->lock);
    data->total_interrupts = 0;
    data->direct_interrupts = 0;
    k_spin_unlock(&data->lock, key);
     
    LOG_INF("APLIC: Statistics reset");
}
 
int riscv_aplic_set_hart_threshold(uint32_t hart_id, uint32_t threshold)
{
    const struct device *dev = aplic_get_dev();
    const struct aplic_config *config;
     
    if (!dev) {
        return -ENODEV;
    }
     
    config = dev->config;
    if (hart_id >= config->num_harts) {
        return -EINVAL;
    }
     
    mem_addr_t idc_addr = get_idc_addr(dev, hart_id, APLIC_IDC_ITHRESHOLD);
    aplic_write(dev, idc_addr, threshold);
     
    LOG_DBG("APLIC: Set hart %u threshold to %u", hart_id, threshold);
    return 0;
}
 
int riscv_aplic_route_source(uint32_t irq, uint32_t hart)
{
    const struct device *dev_local = aplic_get_dev();
    const struct aplic_config *config_local;
    mem_addr_t targetcfg_addr;
    uint32_t targetcfg_value;

    if (!dev_local) {
        return -ENODEV;
    }

    config_local = dev_local->config;
    if (irq == 0 || irq > config_local->num_sources) {
        return -EINVAL;
    }
    if (hart >= config_local->num_harts) {
        return -EINVAL;
    }

    targetcfg_addr = get_targetcfg_addr(dev_local, irq);
    targetcfg_value = aplic_read(dev_local, targetcfg_addr);
    targetcfg_value &= ~((APLIC_TARGETCFG_HART_MASK) << APLIC_TARGETCFG_HART_SHIFT);
    targetcfg_value |= ((hart & APLIC_TARGETCFG_HART_MASK) << APLIC_TARGETCFG_HART_SHIFT);

    aplic_write(dev_local, targetcfg_addr, targetcfg_value);
    return 0;
}

 int riscv_aplic_configure_source(uint32_t irq, uint32_t mode, uint32_t hart, uint32_t priority)
 {
     const struct device *dev = aplic_get_dev();
     const struct aplic_config *config;
     mem_addr_t sourcecfg_addr, targetcfg_addr;
     uint32_t sourcecfg_value, targetcfg_value;
     
     if (!dev) {
         return -ENODEV;
     }
     
     config = dev->config;
     if (irq > config->num_sources || irq == 0) {
         return -EINVAL;
     }
     
    if (hart >= config->num_harts) {
         return -EINVAL;
     }
     
     if (priority > config->max_priority) {
         priority = config->max_priority;
     }
     
    sourcecfg_addr = get_sourcecfg_addr(dev, irq);
    sourcecfg_value = mode & APLIC_SOURCECFG_SM_MASK;
    aplic_write(dev, sourcecfg_addr, sourcecfg_value);
    
    uint32_t read_sourcecfg = aplic_read(dev, sourcecfg_addr);
    LOG_INF("APLIC: Source %u config: wrote=0x%08x, read=0x%08x", irq, sourcecfg_value, read_sourcecfg);
    
    if (read_sourcecfg == APLIC_SOURCECFG_SM_INACTIVE) {
        LOG_ERR("APLIC: Source %u is INACTIVE - SETIP writes will be ignored!", irq);
    } else {
        LOG_INF("APLIC: Source %u is ACTIVE (mode=0x%x)", irq, read_sourcecfg & APLIC_SOURCECFG_SM_MASK);
    }
    
    targetcfg_addr = get_targetcfg_addr(dev, irq);
    targetcfg_value = ((hart & APLIC_TARGETCFG_HART_MASK) << APLIC_TARGETCFG_HART_SHIFT);
    
    if (priority == 0) {
        priority = APLIC_DEFAULT_PRIORITY;
    }
    targetcfg_value |= (priority & APLIC_TARGETCFG_PRIORITY_MASK);
    aplic_write(dev, targetcfg_addr, targetcfg_value);
     
    LOG_DBG("APLIC: Configured IRQ %u (mode=%u, hart=%u, priority=%u)", 
             irq, sourcecfg_value, hart, priority);
     
    return 0;
}
 
static int aplic_init(const struct device *dev)
{
    const struct aplic_config *config = dev->config;
    struct aplic_data *data = dev->data;
    int ret;
    
    LOG_INF("APLIC: Initializing (base=0x%08lX, sources=%u, max_priority=%u)", 
            config->base, config->num_sources, config->max_priority);
     
    if (config->base == 0) {
        LOG_ERR("APLIC: Invalid base address");
        return -EINVAL;
    }
     
    if (config->num_sources == 0 || config->num_sources > 1023) {
        LOG_ERR("APLIC: Invalid number of sources: %u", config->num_sources);
        return -EINVAL;
    }
     
    data->total_interrupts = 0;
    data->direct_interrupts = 0;

    for (int w = 0; w < (CONFIG_APLIC_MAX_IRQS + 31) / 32; w++) {
        data->ie_shadow[w] = 0U;
    }
     
    ret = aplic_configure_direct_mode(dev);
    if (ret != 0) {
        LOG_ERR("APLIC: Failed to configure direct mode: %d", ret);
        return ret;
    }
     
    if (config->irq_config_func) {
        config->irq_config_func();
    }
     
    aplic_dev = dev;
     
    LOG_INF("APLIC: Initialization complete");
    return 0;
}
 
#define APLIC_IRQ_CONFIG_FUNC_DECLARE(n) static void aplic_irq_config_func_##n(void)

#define APLIC_IRQ_CONFIG_FUNC_DEFINE(n) \
	static void __attribute__((used)) aplic_irq_config_func_##n(void) \
	{ \
		IRQ_CONNECT(RISCV_IRQ_MEXT, 0, aplic_direct_mode_handler, \
			    DEVICE_DT_INST_GET(n), 0); \
		irq_enable(RISCV_IRQ_MEXT); \
    }
 
#define APLIC_INIT(n) \
APLIC_IRQ_CONFIG_FUNC_DECLARE(n); \
\
IRQ_PARENT_ENTRY_DEFINE( \
    aplic##n, DEVICE_DT_INST_GET(n), RISCV_IRQ_MEXT, \
    INTC_INST_ISR_TBL_OFFSET(n), \
    DT_INST_INTC_GET_AGGREGATOR_LEVEL(n)); \
\
static const struct aplic_config aplic_config_##n = { \
    .base = DT_INST_REG_ADDR(n), \
    .num_sources = DT_INST_PROP(n, riscv_num_sources), \
    .max_priority = DT_INST_PROP_OR(n, riscv_max_priority, 7), \
    .num_harts = CONFIG_MP_MAX_NUM_CPUS, \
    .irq = RISCV_IRQ_MEXT, \
    .irq_config_func = aplic_irq_config_func_##n, \
    .isr_table = &_sw_isr_table[INTC_INST_ISR_TBL_OFFSET(n)], \
}; \
 BUILD_ASSERT(DT_INST_REG_ADDR(n) != 0, "APLIC base address is zero"); \
\
 static struct aplic_data aplic_data_##n; \
\
 DEVICE_DT_INST_DEFINE(n, aplic_init, NULL, \
               &aplic_data_##n, &aplic_config_##n, \
               PRE_KERNEL_1, CONFIG_INTC_INIT_PRIORITY, \
               NULL); \
\
APLIC_IRQ_CONFIG_FUNC_DEFINE(n)

DT_INST_FOREACH_STATUS_OKAY(APLIC_INIT)