/**
 * 
*/
#define DT_DRV_COMPAT sitronix_st7735
#include "display_st7735.h"
#include "drivers/lcd_display.h"

#define ST7735_CMD_DATA_PIN     DT_INST_GPIO_PIN(0, cmd_data_gpios)
#define ST7735_RES_PIN          DT_INST_GPIO_PIN(0, reset_gpios)
#define ST7735_BLK_PIN          DT_INST_GPIO_PIN(0, blk_gpios)
#define ST7735_CS_PIN           DT_INST_SPI_DEV_CS_GPIOS_PIN(0)


#define RES_FLAGS	    DT_INST_GPIO_FLAGS(0, reset_gpios)
#define CMD_DATA_FLAGS  DT_INST_GPIO_FLAGS(0, cmd_data_gpios)
#define BLK_FLAGS       DT_INST_GPIO_FLAGS(0, blk_gpios)


static struct st7735_data st7735_data_instance={
    .height=LCD_HIGH,
    .width=LCD_WIDTH,
};

void st7735_set_cmd(void *param)
{
    struct st7735_data* data=(struct st7735_data*)param;
    printk("%d==set:0\n",ST7735_CMD_DATA_PIN);
    gpio_pin_set(data->cmd_data_gpio, ST7735_CMD_DATA_PIN, 0);
}
void st7735_set_data(void *param)
{
    struct st7735_data* data=(struct st7735_data*)param;
    printk("%d==set:1\n",ST7735_CMD_DATA_PIN);
    gpio_pin_set(data->cmd_data_gpio, ST7735_CMD_DATA_PIN, 1);
}
static inline void st7735_resetpin_low(void *param)
{
    struct st7735_data* data=(struct st7735_data*)param;
    gpio_pin_set(data->reset_gpio,ST7735_RES_PIN,0);
}
static inline void st7735_resetpin_high(void *param)
{
    struct st7735_data* data=(struct st7735_data*)param;
    gpio_pin_set(data->reset_gpio,ST7735_RES_PIN,1);
}

// static inline void st7735_blk_close(const struct st7735_data *data)
// {
//     gpio_pin_set(data->blk_gpio,ST7735_BLK_PIN,0);
// }
// static inline void st7735_blk_open(const struct st7735_data *data)
// {
//     printk("open blk:%d\n",ST7735_BLK_PIN);
//     gpio_pin_set(data->blk_gpio,ST7735_BLK_PIN,1);
// }

static void st7735_transmit(struct st7735_data* data,uint8_t cmd,uint8_t* tx_data,size_t tx_count)
{
    struct spi_buf tx_buf={
        .buf=&cmd,
        .len=1,
    };//先发送命令,接下来再开始发送数据
    struct spi_buf_set tx_bufs={
        .buffers=&tx_buf,
        .count=1,
    };
    st7735_set_cmd(data);
    k_sleep(K_MSEC(20));
    spi_write(data->spi_dev,&data->spi_config,&tx_bufs);

    if(tx_data!=NULL)
    {
        tx_buf.buf=tx_data;
        tx_buf.len=tx_count;
        st7735_set_data(data);
        k_sleep(K_MSEC(20));
        spi_write(data->spi_dev,&data->spi_config,&tx_bufs);
    }

}

static void write_data(struct st7735_data* data,uint8_t* tx_data,size_t tx_count)
{
    struct spi_buf tx_buf={
        .buf=tx_data,
        .len=tx_count,
    };//先发送命令,接下来再开始发送数据
    struct spi_buf_set tx_bufs={
        .buffers=&tx_buf,
        .count=1,
    };
    st7735_set_data(data);
    k_sleep(K_MSEC(20));
    spi_write(data->spi_dev,&data->spi_config,&tx_bufs);
}

void setCursor(uint8_t xstart,uint8_t ystart,uint8_t xend,uint8_t yend)
{
    uint8_t CASET_follow[]={0,xstart,0,xend};
    uint8_t RASET_follow[]={0,ystart,0,yend};
    st7735_transmit(&st7735_data_instance,ST7735_CMD_CASET,CASET_follow,sizeof(CASET_follow));
    st7735_transmit(&st7735_data_instance,ST7735_CMD_RASET,RASET_follow,sizeof(RASET_follow));
    st7735_transmit(&st7735_data_instance,ST7735_CMD_RAMWR,NULL,0);
}

void LCD_FILL(uint8_t xstart,uint8_t ystart,uint8_t xend,uint8_t yend,uint16_t color)
{
    uint32_t pixel=(xend-xstart+1)*(yend-ystart+1);
    setCursor(xstart,ystart,xend,yend);
    uint8_t color_data[]={((color>>8)&0xff),(color&0xff)};

    for(int i=0;i<sizeof(color_data);i++)
    {
        printk("color_data:%0x\n",color_data[i]);
    }
    while(pixel>0)
    {
        pixel--;
        write_data(&st7735_data_instance,color_data,sizeof(color_data));
    }
}

void LCD_Init(void)
{
    uint8_t B1_follow[]={0x05,0x3C,0x3C};
    uint8_t B2_follow[]={0x05,0x3C,0x3C};
    uint8_t B3_follow[]={0x05,0x3C,0x3C,0x05,0x3C,0x3C};
    uint8_t B4_follow[]={0x03};
    uint8_t C0_follow[]={0x28,0x08,0x04};
    uint8_t C1_follow[]={0xC0};
    uint8_t C2_follow[]={0x0D,0x00};
    uint8_t C3_follow[]={0x8D,0x2A};
    uint8_t C4_follow[]={0x8D,0xEE};
    uint8_t C5_follow[]={0x1A};
    uint8_t MXY_follow[]={0xC0,0x00,0xA0,0x60};
    uint8_t E0_follow[]={0x04,0x22,0x07,0x0A,0x2E,0x30,0x25,0x2A,0x28,0x26,0x2E,0x3A,0x00,0x01,0x03,0x13};
    uint8_t E1_follow[]={0x04,0x16,0x06,0x0D,0x2D,0x26,0x23,0x27,0x27,0x25,0x2D,0x3B,0x00,0x01,0x04,0x13};
    uint8_t CASET_follow[]={0,0,0,0x7f};
    uint8_t RASET_follow[]={0,0,0,0x9f};
    uint8_t COLMOD_follow[]={0x05};
    // st7735_blk_open(&st7735_data_instance);
    
    if((LCD_DIR==1)||(LCD_DIR==3))
    {
        st7735_data_instance.width=LCD_WIDTH;
        st7735_data_instance.height=LCD_HIGH;
    }else{
        st7735_data_instance.width=LCD_HIGH;
        st7735_data_instance.height=LCD_WIDTH;
    }

    st7735_resetpin_low(&st7735_data_instance);
    k_sleep(K_MSEC(10));
    st7735_resetpin_high(&st7735_data_instance);
    k_sleep(K_MSEC(10));

    st7735_transmit(&st7735_data_instance,ST7735_CMD_SLEEP_OUT,NULL,0);
    k_sleep(K_MSEC(100));//唤醒屏幕

    st7735_transmit(&st7735_data_instance,ST7735_CMD_RGBCTRL,B1_follow,sizeof(B1_follow));

    st7735_transmit(&st7735_data_instance,ST7735_CMD_PORCTRL,B2_follow,sizeof(B2_follow));

    st7735_transmit(&st7735_data_instance,ST7735_CMD_B3,B3_follow,sizeof(B3_follow));
    st7735_transmit(&st7735_data_instance,ST7735_CMD_B4,B4_follow,sizeof(B4_follow));
    st7735_transmit(&st7735_data_instance,ST7735_CMD_LCMCTRL,C0_follow,sizeof(C0_follow));
    st7735_transmit(&st7735_data_instance,ST7735_CMD_C1,C1_follow,sizeof(C1_follow));

    st7735_transmit(&st7735_data_instance,ST7735_CMD_VDVVRHEN,C2_follow,sizeof(C2_follow));
    st7735_transmit(&st7735_data_instance,ST7735_CMD_VRH,C3_follow,sizeof(C3_follow));
    st7735_transmit(&st7735_data_instance,ST7735_CMD_VDS,C4_follow,sizeof(C4_follow));
    st7735_transmit(&st7735_data_instance,ST7735_CMD_C5,C5_follow,sizeof(C5_follow));

    if(LCD_DIR==1)st7735_transmit(&st7735_data_instance,ST7735_CMD_MADCTL,&MXY_follow[0],1);
    if(LCD_DIR==2)st7735_transmit(&st7735_data_instance,ST7735_CMD_MADCTL,&MXY_follow[1],1);
    if(LCD_DIR==3)st7735_transmit(&st7735_data_instance,ST7735_CMD_MADCTL,&MXY_follow[2],1);
    if(LCD_DIR==4)st7735_transmit(&st7735_data_instance,ST7735_CMD_MADCTL,&MXY_follow[3],1);
    
    st7735_transmit(&st7735_data_instance,ST7735_CMD_PVGAMCTRL,E0_follow,sizeof(E0_follow));
    st7735_transmit(&st7735_data_instance,ST7735_CMD_NVGAMCTRL,E1_follow,sizeof(E1_follow));
    st7735_transmit(&st7735_data_instance,ST7735_CMD_CASET,CASET_follow,sizeof(CASET_follow));

    st7735_transmit(&st7735_data_instance,ST7735_CMD_RASET,RASET_follow,sizeof(RASET_follow));
    st7735_transmit(&st7735_data_instance,ST7735_CMD_COLMOD,COLMOD_follow,sizeof(COLMOD_follow));

    st7735_transmit(&st7735_data_instance,ST7735_CMD_DISP_ON,NULL,0);
    


    LCD_FILL(1,1,st7735_data_instance.width,st7735_data_instance.height,BLUE);



}




static int st7735_init(const struct device *dev)
{
    struct st7735_data* data = (struct st7735_data*)dev->data;
    data->spi_dev = device_get_binding(DT_INST_BUS_LABEL(0));
    if(data->spi_dev==NULL)
    {
        printk("data->spi_dev is null");
        return -EPERM;
    }
    data->spi_config.frequency=DT_INST_PROP(0, spi_max_frequency);
    data->spi_config.operation=SPI_OP_MODE_MASTER|SPI_WORD_SET(8);
    data->spi_config.slave=DT_INST_REG_ADDR(0);


    data->cs_ctrl.gpio_dev=device_get_binding(DT_INST_SPI_DEV_CS_GPIOS_LABEL(0));
    data->cs_ctrl.gpio_pin=ST7735_CS_PIN;
    data->cs_ctrl.gpio_dt_flags=DT_INST_SPI_DEV_CS_GPIOS_FLAGS(0);
    data->cs_ctrl.delay=0u;
    data->spi_config.cs=&(data->cs_ctrl);

    data->reset_gpio=device_get_binding(DT_INST_GPIO_LABEL(0, reset_gpios));
    if(data->reset_gpio==NULL)
    {
        printk("data->reset_gpio is null\n");
        return -EPERM;
    }
    gpio_pin_configure(data->reset_gpio,ST7735_RES_PIN,GPIO_OUTPUT_INACTIVE|RES_FLAGS);


    data->cmd_data_gpio=device_get_binding(DT_INST_GPIO_LABEL(0, cmd_data_gpios));
    if(data->cmd_data_gpio==NULL)
    {
        printk("data->cmd_data_gpio is null\n");
        return -EPERM;
    }
    gpio_pin_configure(data->cmd_data_gpio,ST7735_CMD_DATA_PIN,GPIO_OUTPUT|CMD_DATA_FLAGS);

    // data->blk_gpio=device_get_binding(DT_INST_GPIO_LABEL(0, blk_gpios));
    // if(data->blk_gpio==NULL)
    // {
    //     printk("data->blk_gpio is null\n");
    //     return -EPERM;
    // }
    // gpio_pin_configure(data->blk_gpio,ST7735_BLK_PIN,GPIO_OUTPUT_INIT_HIGH);

}

static const struct lcd_display_driver_api  st7735_api={
    .st7735_lcd_init=LCD_Init,
    // .high=st7735_blk_close,
    // .low=st7735_blk_open,
};

DEVICE_DT_INST_DEFINE(0, &st7735_init,
	      st7735_pm_control, &st7735_data_instance, NULL, APPLICATION,
	      CONFIG_APPLICATION_INIT_PRIORITY, &st7735_api);