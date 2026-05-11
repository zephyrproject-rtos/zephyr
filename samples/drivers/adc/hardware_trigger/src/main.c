#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, 4);

#define SAMPLING_RATE 8000

enum adc_action sample_end_event(const struct device* dev, const struct adc_sequence* sequence, uint16_t sampling_index);

const struct device* timer = DEVICE_DT_GET(DT_CHOSEN(adc_counter));
struct counter_top_cfg timer_cfg;

struct adc_dt_spec channel = ADC_DT_SPEC_GET(DT_PATH(zephyr_user));
struct adc_sequence_options opts = {
    .callback = sample_end_event,
};

struct adc_sequence seq;
struct k_poll_signal signal;
uint16_t sample[16000] = { 0 };


int main(void){
    seq.options = &opts;
    timer_cfg.ticks = (counter_get_frequency(timer) / SAMPLING_RATE);
    timer_cfg.flags = 0;
 

    (void)k_poll_signal_init(&signal);

    int err = adc_sequence_init_dt(&channel, &seq);
    if (err){
        LOG_ERR("Failed to init sequence");
        return err;
    }

    err = adc_sequence_configure_dt(sizeof(sample), sample, &seq);
    if (err){
        LOG_ERR("Failed to configure sequence");
        return err;
    }

    err = counter_set_top_value(timer, &timer_cfg);
    if (err){
        LOG_ERR("Failed to set timer configuration");
        return err;
    }

    err = adc_read_async_dt(&channel, &seq, &signal);
    if (err){
        LOG_ERR("Failed to start reading");
        return err;
    }

    return 0;
}

enum adc_action sample_end_event(const struct device* dev, const struct adc_sequence* sequence, uint16_t sampling_index){
    LOG_DBG("Finished sampling for 2 seconds");
    LOG_HEXDUMP_INF(sample, 20, "Buffer values are: ");
    
    return ADC_ACTION_FINISH;
}