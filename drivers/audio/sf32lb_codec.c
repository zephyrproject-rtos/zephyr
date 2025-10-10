/*
 * Copyright (c) 2025 SiFli Technologies(Nanjing) Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/audio/codec.h>
#include "bf0_hal_dma.h"
#include "bf0_hal_audcodec.h"
#include "bf0_hal_rcc.h"
#include "bf0_hal_pmu.h"
#include "bf0_hal_cortex.h"
#include "bf0_hal.h"

#include "sf32lb_codec.h"
#include "register.h"

#define LOG_LEVEL CONFIG_AUDIO_CODEC_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(siflicodec);

extern void set_pll_freq_type(uint8_t type);
extern void HAL_TURN_ON_PLL();
extern void HAL_TURN_OFF_PLL();


//todo: should using system irq device tree

#define AUDCODEC_ADC0_DMA_IRQ               DMAC1_CH4_IRQn
#define AUDCODEC_ADC0_DMA_IRQ_PRIO          0
#define AUDCODEC_ADC0_DMA_INSTANCE          DMA1_Channel4
#define AUDCODEC_ADC0_DMA_REQUEST           39
#define AUDCODEC_DAC0_DMA_IRQ_PRIO          0
#define AUDCODEC_DAC0_DMA_INSTANCE          DMA2_Channel1
#define AUDCODEC_DAC0_DMA_IRQ               DMAC2_CH1_IRQn
#define AUDCODEC_DAC0_DMA_REQUEST           41
//#define AUDCODEC_DAC0_DMA_IRQHandler      DMAC2_CH1_IRQHandler
//#define AUDCODEC_ADC0_DMA_IRQHandler      DMAC1_CH4_IRQHandler


#define CODEC_CLK_USING_PLL         0
#define AUDCODEC_MIN_VOLUME         -36
#define AUDCODEC_MAX_VOLUME         54

typedef enum AUDIO_PLL_STATE_TAG
{
    AUDIO_PLL_CLOSED,
    AUDIO_PLL_OPEN,
    AUDIO_PLL_ENABLE,
} AUDIO_PLL_STATE;

struct bf0_audio_private {
    AUDCODEC_HandleTypeDef  audcodec;
    uint8_t                 *tx_buf;
    uint8_t                 *tx_write_ptr;
    uint8_t                 *rx_buf;
    uint32_t                tx_block_size;
    uint32_t                rx_block_size;
    uint8_t                 tx_enable;
    uint8_t                 rx_enable;
    uint8_t                 tx_instanc;
    uint8_t                 rx_instanc;
    uint8_t                 last_volume;
    AUDIO_PLL_STATE         pll_state;
    uint32_t                pll_samplerate;
    void (*tx_done)(void);
    void (*rx_done)(uint8_t *pbuf, uint32_t len);
};

struct sf32lb_codec_driver_config {
    uint8_t      pll_select;    
};

static struct bf0_audio_private h_aud_codec;

static const int volume_level_to_dac_gain[16] = {-55, -34, -32, -30, -28, -26, -24, -22, -20, -17, -14, -11, -10, -8, -6, -2};

static const struct sf32lb_codec_driver_config config = {0};

#undef LOG_INF
#define LOG_INF printf
#if (LOG_LEVEL >= LOG_LEVEL_DEBUG)
#else
#endif

static void clear_pll_enable_flag();

#if CODEC_CLK_USING_PLL
static void pll_freq_fine_tuning(int delta)
{
    uint32_t  pll_value = 0;
    int sdin = 0, cfw = 0;

    __ASSERT((delta >= -0xFFFFF) && (delta <= 0xFFFFF), "pll tuning");
    pll_value = READ_REG(hwp_audcodec->PLL_CFG3);
    sdin = (int32_t)(pll_value & AUDCODEC_PLL_CFG3_SDIN_Msk);
    cfw = (int32_t)((pll_value & AUDCODEC_PLL_CFG3_FCW_Msk) >> AUDCODEC_PLL_CFG3_FCW_Pos);

    if ((sdin + delta) < 0)
    {
        cfw -= 1;
        sdin = 0xFFFFF + sdin + delta;
    }
    else if ((sdin + delta) >= 0xFFFFF)
    {
        cfw += 1;
        sdin = sdin + delta - 0xFFFFF;
    }
    else // 0 <=  (sdin + delta) < 0xFFFFF
    {
        sdin = sdin + delta;
    }

    MODIFY_REG(hwp_audcodec->PLL_CFG3, AUDCODEC_PLL_CFG3_SDIN_Msk, \
                MAKE_REG_VAL(sdin, AUDCODEC_PLL_CFG3_SDIN_Msk, AUDCODEC_PLL_CFG3_SDIN_Pos));
    MODIFY_REG(hwp_audcodec->PLL_CFG3, AUDCODEC_PLL_CFG3_FCW_Msk, \
                MAKE_REG_VAL(cfw, AUDCODEC_PLL_CFG3_FCW_Msk, AUDCODEC_PLL_CFG3_FCW_Pos));
    MODIFY_REG(hwp_audcodec->PLL_CFG3, AUDCODEC_PLL_CFG3_SDM_UPDATE_Msk, \
                MAKE_REG_VAL(1, AUDCODEC_PLL_CFG3_SDM_UPDATE_Msk, AUDCODEC_PLL_CFG3_SDM_UPDATE_Pos));
}

static void pll_freq_grade_set(uint8_t gr)
{
    uint32_t  pll_value = 0;
    int delta = 0;
    double grade = 0.0, cfw = 0.0, sdin = 0.0;

    pll_value = READ_REG(hwp_audcodec->PLL_CFG3);
    sdin = (double)(pll_value & AUDCODEC_PLL_CFG3_SDIN_Msk);
    cfw = (double)((pll_value & AUDCODEC_PLL_CFG3_FCW_Msk) >> AUDCODEC_PLL_CFG3_FCW_Pos);

    grade = (cfw + 3.0 + sdin / 0xFFFFF);
    switch (gr)
    {
    case PLL_ADD_ONE_HUND_PPM:
        grade = grade * 0xFFFFF / 10000; //100PPM
        break;
    case PLL_ADD_TWO_HUND_PPM:
        grade = grade * 0xFFFFF / 5000; //200PPM
        break;
    case PLL_SUB_ONE_HUND_PPM:
        grade = -grade * 0xFFFFF / 10000;
        break;
    case PLL_SUB_TWO_HUND_PPM:
        grade = -grade * 0xFFFFF / 5000;
        break;
    case PLL_ADD_TEN_PPM:
        grade = grade * 0xFFFFF / 100000; //10PPM
        break;
    case PLL_SUB_TEN_PPM:
        grade = -grade * 0xFFFFF / 100000; // 10PPM
        break;
    default:
        __ASSERT(0, "");
    }

    delta = round(grade);
    pll_freq_fine_tuning(delta);
}
#endif

#if AVDD_V18_ENABLE
    #define SINC_GAIN   0xa0
#else
    #define SINC_GAIN   0x14D
#endif

static const AUDCODE_DAC_CLK_CONFIG_TYPE codec_dac_clk_config[9] =
{
#if CODEC_CLK_USING_PLL
    {48000, 1, 1, 0, SINC_GAIN, 1,  5, 4, 2, 20, 20, 0},
    {32000, 1, 1, 1, SINC_GAIN, 1,  5, 4, 2, 20, 20, 0},
    {24000, 1, 1, 5, SINC_GAIN, 1, 10, 2, 2, 10, 10, 1},
    {16000, 1, 1, 4, SINC_GAIN, 1,  5, 4, 2, 20, 20, 0},
    {12000, 1, 1, 7, SINC_GAIN, 1, 20, 2, 1,  5,  5, 1},
    { 8000, 1, 1, 8, SINC_GAIN, 1, 10, 2, 2, 10, 10, 1},
#else
    {48000, 0, 1, 0, SINC_GAIN, 0,  5, 4, 2, 20, 20, 0},
    {32000, 0, 1, 1, SINC_GAIN, 0,  5, 4, 2, 20, 20, 0},
    {24000, 0, 1, 5, SINC_GAIN, 0, 10, 2, 2, 10, 10, 1},
    {16000, 0, 1, 4, SINC_GAIN, 0,  5, 4, 2, 20, 20, 0},
    {12000, 0, 1, 7, SINC_GAIN, 0, 20, 2, 1,  5,  5, 1},
    { 8000, 0, 1, 8, SINC_GAIN, 0, 10, 2, 2, 10, 10, 1},
#endif
    {44100, 1, 1, 0, SINC_GAIN, 1,  5, 4, 2, 20, 20, 0},
    {22050, 1, 1, 5, SINC_GAIN, 1, 10, 2, 2, 10, 10, 1},
    {11025, 1, 1, 7, SINC_GAIN, 1, 20, 2, 1,  5,  5, 1},
};

const AUDCODE_ADC_CLK_CONFIG_TYPE   codec_adc_clk_config[9] =
{
#if CODEC_CLK_USING_PLL
    {48000, 1,  5, 0, 1, 1, 5, 0},
    {32000, 1,  5, 1, 1, 1, 5, 0},
    {24000, 1, 10, 0, 1, 0, 5, 2},
    {16000, 1, 10, 1, 1, 0, 5, 2},
    {12000, 1, 10, 2, 1, 0, 5, 2},
    { 8000, 1, 10, 3, 1, 0, 5, 2},
#else
    {48000, 0,  5, 0, 0, 1, 5, 0},
    {32000, 0,  5, 1, 0, 1, 5, 0},
    {24000, 0, 10, 0, 0, 0, 5, 2},
    {16000, 0, 10, 1, 0, 0, 5, 2},
    {12000, 0, 10, 2, 0, 0, 5, 2},
    { 8000, 0, 10, 3, 0, 0, 5, 2},
#endif
    {44100, 1,  5, 0, 1, 1, 5, 1},
    {22050, 1,  5, 2, 1, 1, 5, 1},
    {11025, 1, 10, 2, 1, 0, 5, 3},
};

typedef struct pll_vco
{
    uint32_t freq;
    uint32_t vco_value;
    uint32_t target_cnt;
} pll_vco_t;

pll_vco_t g_pll_vco_tab[2] =
{
    {48, 0, 2001},
    {44, 0, 1834},
};

int bf0_pll_calibration()
{
    uint32_t pll_cnt;
    uint32_t xtal_cnt;
    uint32_t fc_vco;
    uint32_t fc_vco_min;
    uint32_t fc_vco_max;
    uint32_t delta_cnt = 0;
    uint32_t delta_cnt_min = 0;
    uint32_t delta_cnt_max = 0;
    uint32_t delta_fc_vco;
    uint32_t target_cnt;

    HAL_PMU_EnableAudio(1);
    HAL_RCC_EnableModule(RCC_MOD_AUDCODEC_HP);
    HAL_RCC_EnableModule(RCC_MOD_AUDCODEC_LP);

    HAL_TURN_ON_PLL();

    // VCO freq calibration
    hwp_audcodec->PLL_CFG0 |= AUDCODEC_PLL_CFG0_OPEN;
    hwp_audcodec->PLL_CFG2 |= AUDCODEC_PLL_CFG2_EN_LF_VCIN;
    hwp_audcodec->PLL_CAL_CFG = (0    << AUDCODEC_PLL_CAL_CFG_EN_Pos) |
                                (2000 << AUDCODEC_PLL_CAL_CFG_LEN_Pos);

    for (uint8_t i = 0; i < sizeof(g_pll_vco_tab) / sizeof(g_pll_vco_tab[0]); i++)
    {
        target_cnt = g_pll_vco_tab[i].target_cnt;
        fc_vco = 16;
        delta_fc_vco = 8;
        // setup calibration and run
        // target pll_cnt = ceil(46MHz/48MHz*2000)+1 = 1918
        // target difference between pll_cnt and xtal_cnt should be less than 1
        while (delta_fc_vco != 0)
        {
            hwp_audcodec->PLL_CFG0 &= ~AUDCODEC_PLL_CFG0_FC_VCO;
            hwp_audcodec->PLL_CFG0 |= (fc_vco << AUDCODEC_PLL_CFG0_FC_VCO_Pos);
            hwp_audcodec->PLL_CAL_CFG |= AUDCODEC_PLL_CAL_CFG_EN;
            while (!(hwp_audcodec->PLL_CAL_CFG & AUDCODEC_PLL_CAL_CFG_DONE_Msk));
            pll_cnt = (hwp_audcodec->PLL_CAL_RESULT >> AUDCODEC_PLL_CAL_RESULT_PLL_CNT_Pos);
            xtal_cnt = (hwp_audcodec->PLL_CAL_RESULT & AUDCODEC_PLL_CAL_RESULT_XTAL_CNT_Msk);
            hwp_audcodec->PLL_CAL_CFG &= ~AUDCODEC_PLL_CAL_CFG_EN;
            if (pll_cnt < target_cnt)
            {
                fc_vco = fc_vco + delta_fc_vco;
                delta_cnt = target_cnt - pll_cnt;
            }
            else if (pll_cnt > target_cnt)
            {
                fc_vco = fc_vco - delta_fc_vco;
                delta_cnt = pll_cnt - target_cnt;
            }
            delta_fc_vco = delta_fc_vco >> 1;
        }

        LOG_INF("call par CFG1(%x)\r\n", hwp_audcodec->PLL_CFG1);

        if (fc_vco == 0)
        {
            fc_vco_min = 0;
        }
        else
        {
            fc_vco_min = fc_vco - 1;
        }
        if (fc_vco == 31)
        {
            fc_vco_max = fc_vco;
        }
        else
        {
            fc_vco_max = fc_vco + 1;
        }

        hwp_audcodec->PLL_CFG0 &= ~AUDCODEC_PLL_CFG0_FC_VCO;
        hwp_audcodec->PLL_CFG0 |= (fc_vco_min << AUDCODEC_PLL_CFG0_FC_VCO_Pos);
        hwp_audcodec->PLL_CAL_CFG |= AUDCODEC_PLL_CAL_CFG_EN;

        LOG_INF("fc %d, xtal %d, pll %d\r\n", fc_vco, xtal_cnt, pll_cnt);

        while (!(hwp_audcodec->PLL_CAL_CFG & AUDCODEC_PLL_CAL_CFG_DONE_Msk));

        pll_cnt = (hwp_audcodec->PLL_CAL_RESULT >> AUDCODEC_PLL_CAL_RESULT_PLL_CNT_Pos);
        hwp_audcodec->PLL_CAL_CFG &= ~AUDCODEC_PLL_CAL_CFG_EN;
        if (pll_cnt < target_cnt)
        {
            delta_cnt_min = target_cnt - pll_cnt;
        }
        else if (pll_cnt > target_cnt)
        {
            delta_cnt_min = pll_cnt - target_cnt;
        }

        hwp_audcodec->PLL_CFG0 &= ~AUDCODEC_PLL_CFG0_FC_VCO;
        hwp_audcodec->PLL_CFG0 |= (fc_vco_max << AUDCODEC_PLL_CFG0_FC_VCO_Pos);
        hwp_audcodec->PLL_CAL_CFG |= AUDCODEC_PLL_CAL_CFG_EN;

        while (!(hwp_audcodec->PLL_CAL_CFG & AUDCODEC_PLL_CAL_CFG_DONE_Msk));

        pll_cnt = (hwp_audcodec->PLL_CAL_RESULT >> AUDCODEC_PLL_CAL_RESULT_PLL_CNT_Pos);
        hwp_audcodec->PLL_CAL_CFG &= ~AUDCODEC_PLL_CAL_CFG_EN;
        if (pll_cnt < target_cnt)
        {
            delta_cnt_max = target_cnt - pll_cnt;
        }
        else if (pll_cnt > target_cnt)
        {
            delta_cnt_max = pll_cnt - target_cnt;
        }

        if (delta_cnt_min <= delta_cnt && delta_cnt_min <= delta_cnt_max)
        {
            g_pll_vco_tab[i].vco_value = fc_vco_min;
        }
        else if (delta_cnt_max <= delta_cnt && delta_cnt_max <= delta_cnt_min)
        {
            g_pll_vco_tab[i].vco_value = fc_vco_max;
        }
        else
        {
            g_pll_vco_tab[i].vco_value = fc_vco;
        }
    }
    hwp_audcodec->PLL_CFG2 &= ~AUDCODEC_PLL_CFG2_EN_LF_VCIN;
    hwp_audcodec->PLL_CFG0 &= ~AUDCODEC_PLL_CFG0_OPEN;

    HAL_TURN_OFF_PLL();

    return 0;
}

/**
 * @brief  enable PLL function
 * @param freq - frequency
 * @param type - 0:1024 series, 1:1000 series
 * @return
 */
int bf0_enable_pll(uint32_t freq, uint8_t type)
{
    uint8_t freq_type;
    uint8_t test_result = -1;
    uint8_t vco_index = 0;

    LOG_INF("enable pll \n");

    freq_type = type << 1;
    if ((freq == 44100) || (freq == 22050) || (freq == 11025))
    {
        vco_index = 1;
        freq_type += 1;
    }

    HAL_TURN_ON_PLL();

    hwp_audcodec->PLL_CFG0 &= ~AUDCODEC_PLL_CFG0_FC_VCO;
    hwp_audcodec->PLL_CFG0 |= (g_pll_vco_tab[vco_index].vco_value << AUDCODEC_PLL_CFG0_FC_VCO_Pos);

    LOG_INF("new PLL_ENABLE vco:%d, freq_type:%d\n", g_pll_vco_tab[vco_index].vco_value, freq_type);
    do
    {
        test_result = updata_pll_freq(freq_type);
    }
    while (test_result != 0);

    return test_result;
}

int bf0_update_pll(uint32_t freq, uint8_t type)
{
    uint8_t freq_type;
    uint8_t test_result = -1;
    uint8_t vco_index = 0;

    freq_type = type << 1;
    if ((freq == 44100) || (freq == 22050) || (freq == 11025))
    {
        vco_index = 1;
        freq_type += 1;
    }

    hwp_audcodec->PLL_CFG0 &= ~AUDCODEC_PLL_CFG0_FC_VCO;
    hwp_audcodec->PLL_CFG0 |= (g_pll_vco_tab[vco_index].vco_value << AUDCODEC_PLL_CFG0_FC_VCO_Pos);

    LOG_INF("new PLL_ENABLE vco:%d, freq_type:%d\n", g_pll_vco_tab[vco_index].vco_value, freq_type);
    do
    {
        test_result = updata_pll_freq(freq_type);
    }
    while (test_result != 0);

    return test_result;
}


void bf0_disable_pll()
{
    HAL_TURN_OFF_PLL();
    clear_pll_enable_flag();
    LOG_INF("PLL disable\n");
}


static void clear_pll_enable_flag()
{
    h_aud_codec.pll_state = AUDIO_PLL_CLOSED;
}

void AUDCODEC_DAC0_DMA_IRQHandler(void)
{
    HAL_DMA_IRQHandler(h_aud_codec.audcodec.hdma[HAL_AUDCODEC_DAC_CH0]);
}

void AUDCODEC_ADC0_DMA_IRQHandler(void)
{
    HAL_DMA_IRQHandler(h_aud_codec.audcodec.hdma[HAL_AUDCODEC_ADC_CH0]);
}

static int bf0_audio_init(const struct device *dev)
{
    const struct sf32lb_codec_driver_config *dev_cfg = dev->config;
    struct bf0_audio_private*priv = dev->data;

    AUDCODEC_HandleTypeDef *haudcodec = (AUDCODEC_HandleTypeDef *) & (priv->audcodec);

    haudcodec->hdma[HAL_AUDCODEC_DAC_CH0] = k_aligned_alloc(sizeof(uint32_t), sizeof(DMA_HandleTypeDef));

    if (NULL == haudcodec->hdma[HAL_AUDCODEC_DAC_CH0])
    {
        __ASSERT(0, "audio mem");
        return -ENOMEM;;
    }
    memset(haudcodec->hdma[HAL_AUDCODEC_DAC_CH0], 0, sizeof(DMA_HandleTypeDef));

    haudcodec->hdma[HAL_AUDCODEC_DAC_CH0]->Instance = AUDCODEC_DAC0_DMA_INSTANCE;
    haudcodec->hdma[HAL_AUDCODEC_DAC_CH0]->Init.Request = AUDCODEC_DAC0_DMA_REQUEST;

    haudcodec->hdma[HAL_AUDCODEC_ADC_CH0] = k_aligned_alloc(sizeof(uint32_t), sizeof(DMA_HandleTypeDef));

    if (NULL == haudcodec->hdma[HAL_AUDCODEC_ADC_CH0])
    {
        __ASSERT(0, "audio mem");
        return -ENOMEM;;
    }
    memset(haudcodec->hdma[HAL_AUDCODEC_ADC_CH0], 0, sizeof(DMA_HandleTypeDef));

    haudcodec->hdma[HAL_AUDCODEC_ADC_CH0]->Instance = AUDCODEC_ADC0_DMA_INSTANCE;
    haudcodec->hdma[HAL_AUDCODEC_ADC_CH0]->Init.Request = AUDCODEC_ADC0_DMA_REQUEST;

    // set clock
    haudcodec->Init.en_dly_sel = 0;
    haudcodec->Init.dac_cfg.opmode = 1;
    haudcodec->Init.adc_cfg.opmode = 1;

    //bf0_enable_pll(44100, 1); //RCC ENABLE
    HAL_PMU_EnableAudio(1);
    HAL_RCC_EnableModule(RCC_MOD_AUDCODEC_HP);
    HAL_RCC_EnableModule(RCC_MOD_AUDCODEC_LP);

    HAL_AUDCODEC_Init(haudcodec);
    LOG_INF("bf0_audio_init done");
    return 0;
}

static void bf0_audio_pll_config(struct bf0_audio_private*priv, const AUDCODE_ADC_CLK_CONFIG_TYPE *adc_cfg,
                                const AUDCODE_DAC_CLK_CONFIG_TYPE *dac_cfg)
{
    if (priv->tx_enable)
    {
        if (priv->rx_enable)
        {
            __ASSERT(adc_cfg->samplerate == dac_cfg->samplerate, "");
        }

        if (dac_cfg->clk_src_sel) //pll
        {
            if (priv->pll_state == AUDIO_PLL_CLOSED)
            {
                bf0_enable_pll(dac_cfg->samplerate, 1);
                priv->pll_state = AUDIO_PLL_ENABLE;
                priv->pll_samplerate = dac_cfg->samplerate;
            }
            else
            {
                bf0_update_pll(dac_cfg->samplerate, 1);
                priv->pll_state = AUDIO_PLL_ENABLE;
                priv->pll_samplerate = dac_cfg->samplerate;
            }
        }
        else //xtal
        {
            if (priv->pll_state == AUDIO_PLL_CLOSED)
            {
                HAL_TURN_ON_PLL();
                priv->pll_state = AUDIO_PLL_OPEN;
            }
        }
    }
    else if (priv->rx_enable)
    {
        if (adc_cfg->clk_src_sel) //pll
        {
            if (priv->pll_state == AUDIO_PLL_CLOSED)
            {
                bf0_enable_pll(adc_cfg->samplerate, 1);
                priv->pll_state = AUDIO_PLL_ENABLE;
                priv->pll_samplerate = adc_cfg->samplerate;
            }
            else
            {
                bf0_update_pll(adc_cfg->samplerate, 1);
                priv->pll_state = AUDIO_PLL_ENABLE;
                priv->pll_samplerate = adc_cfg->samplerate;
            }
        }
        else //xtal
        {
            if (priv->pll_state == AUDIO_PLL_CLOSED)
            {
                HAL_TURN_ON_PLL();
                priv->pll_state = AUDIO_PLL_OPEN;
            }
        }
    }

    LOG_INF("pll config state:%d, samplerate:%d \n", priv->pll_state, priv->pll_samplerate);
}

static void sf32lb_codec_set_dac_volume(const struct device *dev, uint8_t volume)
{
    struct bf0_audio_private*priv = dev->data;
    __ASSERT(&h_aud_codec == priv, "private error");

    AUDCODEC_HandleTypeDef *haudcodec = (AUDCODEC_HandleTypeDef *) & (priv->audcodec);

    if (volume > 15) {
        volume = 15;
    }

    int gain = volume_level_to_dac_gain[volume];
    if (gain > AUDCODEC_MAX_VOLUME)
        gain = AUDCODEC_MAX_VOLUME;
    if (gain < AUDCODEC_MIN_VOLUME)
        gain = AUDCODEC_MIN_VOLUME;

    HAL_AUDCODEC_Config_DACPath_Volume(haudcodec, 0, gain * 2);
    //HAL_AUDCODEC_Config_DACPath_Volume(haudcodec, 1, gain * 2);
    priv->last_volume = volume;
}

static void sf32lb_codec_set_dac_mute(const struct device *dev, bool is_mute)
{
    struct bf0_audio_private*priv = dev->data;
    AUDCODEC_HandleTypeDef *haudcodec = (AUDCODEC_HandleTypeDef *) & (priv->audcodec);

    if (is_mute) {
        HAL_AUDCODEC_Mute_DACPath(haudcodec, 1);
    } else {
        HAL_AUDCODEC_Mute_DACPath(haudcodec, 0);
        sf32lb_codec_set_dac_volume(dev, priv->last_volume);
    }
}

static int sf32lb_codec_write(const struct device *dev, const uint8_t *data, uint32_t size)
{
    struct bf0_audio_private*priv = dev->data;

    if (!data || !size)
        return -EINVAL;;

    __ASSERT(priv->tx_buf && priv->tx_write_ptr, "write buf err");
    __ASSERT(size <= priv->tx_block_size, "tx size err");

    memcpy(priv->tx_write_ptr, data, size);

    if (size < priv->tx_block_size) {
        memset(priv->tx_write_ptr + size, 0, priv->tx_block_size - size);
    }

	return size;
}


void HAL_AUDCODEC_TxCpltCallback(AUDCODEC_HandleTypeDef *audcodec, int cid)
{
    __ASSERT(audcodec, "");
    __ASSERT(cid == HAL_AUDCODEC_DAC_CH0, "");
    struct bf0_audio_private*priv = CONTAINER_OF(audcodec, struct bf0_audio_private, audcodec);
    priv->tx_write_ptr = priv->tx_buf + priv->tx_block_size;
    if (priv->tx_done) {
	    priv->tx_done();
    }
}
void HAL_AUDCODEC_TxHalfCpltCallback(AUDCODEC_HandleTypeDef *audcodec, int cid)
{
    __ASSERT(audcodec, "");
    __ASSERT(cid == HAL_AUDCODEC_DAC_CH0, "");
    struct bf0_audio_private*priv = CONTAINER_OF(audcodec, struct bf0_audio_private, audcodec);
    priv->tx_write_ptr = priv->tx_buf;
    if (priv->tx_done) {
	    priv->tx_done();
    }
}

void HAL_AUDCODEC_RxCpltCallback(AUDCODEC_HandleTypeDef *audcodec, int cid)
{
    __ASSERT(audcodec, "");
    __ASSERT(cid == HAL_AUDCODEC_ADC_CH0, "");
    struct bf0_audio_private*priv = CONTAINER_OF(audcodec, struct bf0_audio_private, audcodec);

    if (priv->rx_done) {
	    priv->rx_done(priv->rx_buf + priv->rx_block_size, priv->rx_block_size);
    }
}

void HAL_AUDCODEC_RxHalfCpltCallback(AUDCODEC_HandleTypeDef *audcodec, int cid)
{
    __ASSERT(audcodec, "");
    __ASSERT(cid == HAL_AUDCODEC_ADC_CH0, "");
    struct bf0_audio_private*priv = CONTAINER_OF(audcodec, struct bf0_audio_private, audcodec);

    if (priv->rx_done) {
	    priv->rx_done(priv->rx_buf, priv->rx_block_size);
    }
}

int updata_pll_freq(uint8_t type) //type 0: 16k 1024 series  1:44.1k 1024 series 2:16k 1000 series 3: 44.1k 1000 series
{
    hwp_audcodec->PLL_CFG2 |= AUDCODEC_PLL_CFG2_RSTB;
    // wait 50us
    HAL_Delay_us(50);
    if (0 == type)// set pll to 49.152M   [(fcw+3)+sdin/2^20]*6M
    {
        hwp_audcodec->PLL_CFG3 = (201327 << AUDCODEC_PLL_CFG3_SDIN_Pos) |
                                 (5 << AUDCODEC_PLL_CFG3_FCW_Pos) |
                                 (0 << AUDCODEC_PLL_CFG3_SDM_UPDATE_Pos) |
                                 (1 << AUDCODEC_PLL_CFG3_SDMIN_BYPASS_Pos) |
                                 (0 << AUDCODEC_PLL_CFG3_SDM_MODE_Pos) |
                                 (0 << AUDCODEC_PLL_CFG3_EN_SDM_DITHER_Pos) |
                                 (0 << AUDCODEC_PLL_CFG3_SDM_DITHER_Pos) |
                                 (1 << AUDCODEC_PLL_CFG3_EN_SDM_Pos) |
                                 (0 << AUDCODEC_PLL_CFG3_SDMCLK_POL_Pos);
    }
    else if (1 == type)// set pll to 45.1584M
    {
        hwp_audcodec->PLL_CFG3 = (551970 << AUDCODEC_PLL_CFG3_SDIN_Pos) |
                                 (4 << AUDCODEC_PLL_CFG3_FCW_Pos) |
                                 (0 << AUDCODEC_PLL_CFG3_SDM_UPDATE_Pos) |
                                 (1 << AUDCODEC_PLL_CFG3_SDMIN_BYPASS_Pos) |
                                 (0 << AUDCODEC_PLL_CFG3_SDM_MODE_Pos) |
                                 (0 << AUDCODEC_PLL_CFG3_EN_SDM_DITHER_Pos) |
                                 (0 << AUDCODEC_PLL_CFG3_SDM_DITHER_Pos) |
                                 (1 << AUDCODEC_PLL_CFG3_EN_SDM_Pos) |
                                 (0 << AUDCODEC_PLL_CFG3_SDMCLK_POL_Pos);
    }
    else if (2 == type)// set pll to 48M
    {
        hwp_audcodec->PLL_CFG3 = (0 << AUDCODEC_PLL_CFG3_SDIN_Pos) |
                                 (5 << AUDCODEC_PLL_CFG3_FCW_Pos) |
                                 (0 << AUDCODEC_PLL_CFG3_SDM_UPDATE_Pos) |
                                 (1 << AUDCODEC_PLL_CFG3_SDMIN_BYPASS_Pos) |
                                 (0 << AUDCODEC_PLL_CFG3_SDM_MODE_Pos) |
                                 (0 << AUDCODEC_PLL_CFG3_EN_SDM_DITHER_Pos) |
                                 (0 << AUDCODEC_PLL_CFG3_SDM_DITHER_Pos) |
                                 (1 << AUDCODEC_PLL_CFG3_EN_SDM_Pos) |
                                 (0 << AUDCODEC_PLL_CFG3_SDMCLK_POL_Pos);
    }
    else if (3 == type)// set pll to 44.1M
    {
        hwp_audcodec->PLL_CFG3 = (0x5999A << AUDCODEC_PLL_CFG3_SDIN_Pos) |
                                 (4 << AUDCODEC_PLL_CFG3_FCW_Pos) |
                                 (0 << AUDCODEC_PLL_CFG3_SDM_UPDATE_Pos) |
                                 (1 << AUDCODEC_PLL_CFG3_SDMIN_BYPASS_Pos) |
                                 (0 << AUDCODEC_PLL_CFG3_SDM_MODE_Pos) |
                                 (0 << AUDCODEC_PLL_CFG3_EN_SDM_DITHER_Pos) |
                                 (0 << AUDCODEC_PLL_CFG3_SDM_DITHER_Pos) |
                                 (1 << AUDCODEC_PLL_CFG3_EN_SDM_Pos) |
                                 (0 << AUDCODEC_PLL_CFG3_SDMCLK_POL_Pos);
    }
    else
    {
        __ASSERT(0, "");
    }
    hwp_audcodec->PLL_CFG3 |= AUDCODEC_PLL_CFG3_SDM_UPDATE;
    hwp_audcodec->PLL_CFG3 &= ~AUDCODEC_PLL_CFG3_SDMIN_BYPASS;
    hwp_audcodec->PLL_CFG2 &= ~AUDCODEC_PLL_CFG2_RSTB;
    // wait 50us
    HAL_Delay_us(50);
    hwp_audcodec->PLL_CFG2 |= AUDCODEC_PLL_CFG2_RSTB;
    // check pll lock
    //wait(2500);
    HAL_Delay_us(50);

    hwp_audcodec->PLL_CFG1 |= AUDCODEC_PLL_CFG1_CSD_EN | AUDCODEC_PLL_CFG1_CSD_RST;
    HAL_Delay_us(50);
    hwp_audcodec->PLL_CFG1 &= ~AUDCODEC_PLL_CFG1_CSD_RST;
    if (hwp_audcodec->PLL_STAT & AUDCODEC_PLL_STAT_UNLOCK_Msk)
    {
        LOG_INF("pll lock fail! freq_type:%d\n", type);
        return -1;
    }
    else
    {
        LOG_INF("pll lock! freq_type:%d\n", type);
        hwp_audcodec->PLL_CFG1 &= ~AUDCODEC_PLL_CFG1_CSD_EN;
        set_pll_freq_type(type);
    }
    return 0;
}

static int sf32lb_codec_configure(const struct device *dev, struct sf32lb_codec_cfg *cfg)
{
    struct bf0_audio_private*priv = dev->data;

    AUDCODEC_HandleTypeDef *haudcodec = (AUDCODEC_HandleTypeDef *) & (priv->audcodec);

    if (cfg->dir & SF32LB_AUDIO_TX) {
	    priv->tx_done = cfg->tx_done;
	    for (uint8_t i = 0; i < 9; i++) {
	        if (cfg->samplerate == codec_dac_clk_config[i].samplerate) {
    		    haudcodec->Init.samplerate_index = i;
	    	    haudcodec->Init.dac_cfg.dac_clk = (AUDCODE_DAC_CLK_CONFIG_TYPE *)&codec_dac_clk_config[i];
		        break;
	        }
	    }
	    __ASSERT(i < 9, "tx smprate error");

	    priv->tx_block_size = cfg->block_size;
	    if (!priv->tx_buf)
	        priv->tx_buf = k_aligned_alloc(sizeof(uint32_t), priv->tx_block_size * 2);

	    if (!priv->tx_buf) {
	        __ASSERT(priv->tx_buf, "tx mem");
	        return -ENOMEM;
	    }
	    priv->tx_write_ptr = priv->tx_buf;
	    memset(priv->tx_buf, 0, priv->tx_block_size * 2);
	    HAL_AUDCODEC_Config_TChanel(haudcodec, 0, &haudcodec->Init.dac_cfg);
	    priv->tx_instanc = HAL_AUDCODEC_DAC_CH0;
    }

    if (cfg->dir & SF32LB_AUDIO_RX) {
	    priv->rx_done = cfg->rx_done;
	    for (uint8_t i = 0; i < 9; i++) {
		    if (cfg->samplerate == codec_adc_clk_config[i].samplerate)
		    {
			    haudcodec->Init.samplerate_index = i;
			    haudcodec->Init.adc_cfg.adc_clk = (AUDCODE_ADC_CLK_CONFIG_TYPE *)&codec_adc_clk_config[i];
			    break;
		    }
	    }
        __ASSERT(i < 9, "rx smprate error");

        priv->rx_block_size = cfg->block_size;
        if (!priv->rx_buf) {
            priv->rx_buf = k_aligned_alloc(sizeof(uint32_t), priv->rx_block_size * 2);
            __ASSERT(priv->rx_buf, "rx mem");
            return -ENOMEM;
        }
        memset(priv->rx_buf, 0, priv->rx_block_size * 2);
        HAL_AUDCODEC_Config_RChanel(haudcodec, 0, &haudcodec->Init.adc_cfg);
	    priv->rx_instanc = HAL_AUDCODEC_ADC_CH0;
	}
    return 0;
}

static void sf32lb_codec_start(const struct device *dev, enum sf32lb_audio_dir dir)
{
    struct bf0_audio_private*priv = dev->data;
    AUDCODEC_HandleTypeDef *haudcodec = (AUDCODEC_HandleTypeDef *) &priv->audcodec;
    bf0_audio_pll_config(priv, &codec_adc_clk_config[haudcodec->Init.samplerate_index],
                        &codec_dac_clk_config[haudcodec->Init.samplerate_index]);

    bool start_rx = (!priv->rx_enable && (dir & SF32LB_AUDIO_RX));
    bool start_tx = (!priv->tx_enable && (dir & SF32LB_AUDIO_TX));
    LOG_INF("codec start rx=%d tx=%d", start_rx, start_rx);
    if (start_rx) {
        HAL_AUDCODEC_Receive_DMA(haudcodec, priv->rx_buf, priv->rx_block_size, HAL_AUDCODEC_ADC_CH0);
        HAL_NVIC_EnableIRQ(AUDCODEC_ADC0_DMA_IRQ);
        //IRO_CONNECT()
        priv->rx_instanc = HAL_AUDCODEC_ADC_CH0;
    }

    if (start_tx) {
        HAL_AUDCODEC_Transmit_DMA(haudcodec, priv->tx_buf, priv->tx_block_size, HAL_AUDCODEC_DAC_CH0);
        HAL_NVIC_EnableIRQ(AUDCODEC_DAC0_DMA_IRQ);
    }

    if (start_tx) {
        priv->tx_enable = 1;
        /* enable AUDCODEC at last*/
        __HAL_AUDCODEC_DAC_ENABLE(haudcodec);

        HAL_AUDCODEC_Config_DACPath(haudcodec, 1);
        HAL_AUDCODEC_Config_Analog_DACPath(haudcodec->Init.dac_cfg.dac_clk);
        HAL_AUDCODEC_Config_DACPath(haudcodec, 0);
    }

    if (start_rx) {
        priv->rx_enable = 1;
        HAL_AUDCODEC_Config_Analog_ADCPath(haudcodec->Init.adc_cfg.adc_clk);
        /* enable AUDCODEC at last*/
        __HAL_AUDCODEC_ADC_ENABLE(haudcodec);
    }
}

static void sf32lb_codec_stop(const struct device *dev, enum sf32lb_audio_dir dir)
{
    struct bf0_audio_private*priv = dev->data;
    AUDCODEC_HandleTypeDef *haudcodec = (AUDCODEC_HandleTypeDef *)&priv->audcodec;

    bool stop_rx = (priv->rx_enable && (dir & SF32LB_AUDIO_RX));
    bool stop_tx = (priv->tx_enable && (dir & SF32LB_AUDIO_TX));
    LOG_INF("codec stop rx=%d tx=%d", stop_rx, stop_tx);
    if (stop_rx) {
        HAL_NVIC_DisableIRQ(AUDCODEC_ADC0_DMA_IRQ);
        HAL_AUDCODEC_DMAStop(haudcodec, HAL_AUDCODEC_ADC_CH0);
        haudcodec->State[HAL_AUDCODEC_ADC_CH0] = HAL_AUDCODEC_STATE_READY;
    }

    if (stop_tx) {
        HAL_NVIC_DisableIRQ(AUDCODEC_DAC0_DMA_IRQ);
        HAL_AUDCODEC_DMAStop(haudcodec, HAL_AUDCODEC_DAC_CH0);
        haudcodec->State[HAL_AUDCODEC_DAC_CH0] = HAL_AUDCODEC_STATE_READY;
    }

    if (stop_tx) {
        LOG_INF("audcodec close dac");
        HAL_AUDCODEC_Config_DACPath(haudcodec, 1);
        HAL_AUDCODEC_Close_Analog_DACPath();
        __HAL_AUDCODEC_DAC_DISABLE(haudcodec);
        HAL_AUDCODEC_Clear_All_Channel(haudcodec, 1);
        //if (priv->tx_buf) {
        //  k_free(priv->tx_buf);
        //  priv->tx_buf = NULL;
        //}
        priv->tx_enable = 0;
    }
    if (stop_rx) {
        LOG_INF("audcodec close adc");
        __HAL_AUDCODEC_ADC_DISABLE(haudcodec);
        HAL_AUDCODEC_Close_Analog_ADCPath();
        HAL_AUDCODEC_Clear_All_Channel(haudcodec, 2);

        //if (priv->rx_buf) {
        //  k_free(priv->rx_buf);
        //  priv->rx_buf = NULL;
        //}
        priv->rx_enable = 0;
    }
}

static int codec_driver_init(const struct device *dev)
{
    LOG_INF("%s", __FUNCTION__);
    memset(&h_aud_codec, 0, sizeof(h_aud_codec));
    h_aud_codec.audcodec.Instance = hwp_audcodec;
    bf0_pll_calibration();
    bf0_audio_init(dev);
    return 0;
}

static DEVICE_API(sf32lb_codec,  codec_driver_api) = {
    .configure = sf32lb_codec_configure,
    .start = sf32lb_codec_start,
    .stop = sf32lb_codec_stop,
    .set_dac_volume = sf32lb_codec_set_dac_volume,
    .set_dac_mute = sf32lb_codec_set_dac_mute,
    .write = sf32lb_codec_write,
};


DEVICE_DEFINE(sf32lb_codec, SF32LB_CODEC_NAME,
    codec_driver_init,
    NULL,
    &h_aud_codec,
    &config,
    POST_KERNEL,
    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
    &codec_driver_api);


