#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <stdint.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#define MOTOR_ADC_NODE DT_NODELABEL(motor_current_channel)
#define VREF_ADC_NODE  DT_NODELABEL(vref_channel)

#if !DT_NODE_EXISTS(MOTOR_ADC_NODE) || !DT_NODE_EXISTS(VREF_ADC_NODE)
#error "ADC nodes are not properly defined in device tree"
#endif

static const struct device * const adc_dev = DEVICE_DT_GET(DT_PARENT(MOTOR_ADC_NODE));
static const uint8_t motor_channel_id = DT_REG_ADDR(MOTOR_ADC_NODE);
static const uint8_t vref_channel_id = DT_REG_ADDR(VREF_ADC_NODE);

int main(void)
{
    int err;
    int16_t adc_raw_value;

    LOG_INF("Starting ADC BUG WORKAROUND for nucleo_f103rb...");

    if (!device_is_ready(adc_dev)) {
        LOG_ERR("ADC device is not ready");
        return 0;
    }

    LOG_WRN("Skipping adc_channel_setup() due to suspected driver bug.");

    uint32_t adc_base = DT_REG_ADDR(DT_NODELABEL(adc1));

    volatile uint32_t *ADC_CR2   = (uint32_t *)(adc_base + 0x08);
    volatile uint32_t *ADC_SMPR1 = (uint32_t *)(adc_base + 0x0C);
    volatile uint32_t *ADC_SMPR2 = (uint32_t *)(adc_base + 0x10);

    const uint32_t ADC_CR2_TSVREFE_BIT = 23;
    *ADC_CR2 |= (1 << ADC_CR2_TSVREFE_BIT);

    *ADC_SMPR2 |= (0b111 << (3 * 9));
    *ADC_SMPR1 |= (0b111 << (3 * (17 - 10)));

    k_busy_wait(10);

    while (1) {
        int32_t motor_raw = 0;
        int32_t vref_raw = 0;
        
        struct adc_sequence sequence = {
            .buffer      = &adc_raw_value,
            .buffer_size = sizeof(adc_raw_value),
            .resolution  = 12,
        };

        sequence.channels = BIT(motor_channel_id);
        if (adc_read(adc_dev, &sequence) == 0) {
            motor_raw = adc_raw_value;
        }

        sequence.channels = BIT(vref_channel_id);
        if (adc_read(adc_dev, &sequence) == 0) {
            vref_raw = adc_raw_value;
        }

        int32_t vdd_mv = (vref_raw > 0) ? (1200 * 4095 / vref_raw) : 0;
        int32_t motor_mv = 0;
        
        if (motor_raw > 0 && vdd_mv > 0) {
            motor_mv = motor_raw;
            err = adc_raw_to_millivolts(vdd_mv, ADC_GAIN_1, 12, &motor_mv);
        }
        
        LOG_INF("Motor Raw: %4d, Motor mV: %4d | VDD: %4d mV", motor_raw, motor_mv, vdd_mv);
        k_sleep(K_MSEC(2000));
    }
    return 0;
}
