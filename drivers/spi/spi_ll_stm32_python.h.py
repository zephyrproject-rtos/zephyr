cogeno.import_module('devicedeclare')

device_configs = ['CONFIG_SPI_{}'.format(x) for x in range(1, 7)]
driver_names = ['SPI_{}'.format(x) for x in range(1, 7)]
device_inits = 'spi_stm32_init'
device_levels = 'POST_KERNEL'
device_prios = 'CONFIG_SPI_INIT_PRIORITY'
device_api = 'spi_stm32_driver_api'
device_info = \
"""
#if CONFIG_SPI_STM32_INTERRUPT
DEVICE_DECLARE(${device-name});
static void ${device-config-irq}(struct device *dev)
{
        IRQ_CONNECT(${interrupts/0/irq}, ${interrupts/0/priority}, \\
                    spi_stm32_isr, \\
                    DEVICE_GET(${device-name}), 0);
        irq_enable(${interrupts/0/irq});
}
#endif
static const struct spi_stm32_config ${device-config-info} = {
        .spi = (SPI_TypeDef *)${reg/0/address/0},
        .pclken.bus = ${clocks/0/bus},
        .pclken.enr = ${clocks/0/bits},
#if CONFIG_SPI_STM32_INTERRUPT
        .config_irq = ${device-config-irq},
#endif
};
static struct spi_stm32_data ${device-data} = {
        SPI_CONTEXT_INIT_LOCK(${device-data}, ctx),
        SPI_CONTEXT_INIT_SYNC(${device-data}, ctx),
};
"""

devicedeclare.device_declare_multi( \
    device_configs,
    driver_names,
    device_inits,
    device_levels,
    device_prios,
    device_api,
    device_info)
