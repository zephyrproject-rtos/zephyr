// /**
//  * 
// */
#define DT_DRV_COMPAT sitronix_st7735
#include "display_st7735.h"
#include "drivers/lcd_driver_api.h"
#include "font.h"


#define ST7735_CMD_DATA_PIN     DT_INST_GPIO_PIN(0, cmd_data_gpios)
#define ST7735_RES_PIN          DT_INST_GPIO_PIN(0, reset_gpios)
#define ST7735_BLK_PIN          DT_INST_GPIO_PIN(0, blk_gpios)
#define ST7735_CS_PIN           DT_INST_SPI_DEV_CS_GPIOS_PIN(0)

#define RES_FLAGS	    DT_INST_GPIO_FLAGS(0, reset_gpios)
#define CMD_DATA_FLAGS  DT_INST_GPIO_FLAGS(0, cmd_data_gpios)
#define BLK_FLAGS       DT_INST_GPIO_FLAGS(0, blk_gpios)



static struct st7735_data st7735_info=
{
    .height=LCD_HIGH,
    .width=LCD_WIDTH,
};

#define ST7735_DC_LOW()   gpio_pin_set(st7735_info.cmd_data_gpio, ST7735_CMD_DATA_PIN, 0);
#define ST7735_DC_HIGH()  gpio_pin_set(st7735_info.cmd_data_gpio, ST7735_CMD_DATA_PIN, 1);
#define ST7735_BLK_OPEN()   gpio_pin_set(st7735_info.blk_gpio, ST7735_BLK_PIN, 1);



static void st7735_transmit(const struct device* dev, uint8_t cmd,const uint8_t *tx_data,uint8_t tx_cnt)
{
    struct st7735_data* data = (struct st7735_data*)dev->data;
    struct spi_buf tx_buf={
        .buf=&cmd,
        .len=1,
    };//先发送命令,接下来再开始发送数据
    struct spi_buf_set tx_bufs={
        .buffers=&tx_buf,
        .count=1,
    };
    if(cmd != ST7735_CMD_NULL){
        ST7735_DC_LOW();
        spi_write(data->spi_dev,&data->spi_config,&tx_bufs);
    }
    if(tx_data!=NULL)
    {
        tx_buf.buf=tx_data;
        tx_buf.len=tx_cnt;
        ST7735_DC_HIGH();
        spi_write(data->spi_dev,&data->spi_config,&tx_bufs);
    }
}




/**
 * 初始化IO和SPI
*/
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
    data->spi_config.operation=SPI_OP_MODE_MASTER|SPI_WORD_SET(8)|SPI_MODE_CPHA|SPI_MODE_CPOL;
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

    data->blk_gpio=device_get_binding(DT_INST_GPIO_LABEL(0, blk_gpios));
    if(data->blk_gpio==NULL)
    {
        printk("data->blk_gpio is null\n");
        return -EPERM;
    }
    gpio_pin_configure(data->blk_gpio,ST7735_BLK_PIN,GPIO_OUTPUT|BLK_FLAGS);

    ST7735_BLK_OPEN();


    gpio_pin_set(data->reset_gpio,ST7735_RES_PIN,0);
    k_sleep(K_MSEC(10));
    gpio_pin_set(data->reset_gpio,ST7735_RES_PIN,1);
    k_sleep(K_MSEC(10));

    printk("lcd init ok~\n");

    return 0;

}










static const struct lcd_driver_api  st7735_api={
    .write = st7735_transmit,
};

DEVICE_DT_INST_DEFINE(0, &st7735_init,
	      st7735_pm_control, &st7735_info, NULL, APPLICATION,
	      CONFIG_APPLICATION_INIT_PRIORITY, &st7735_api);