// /**
//  * 
// */
#define DT_DRV_COMPAT sitronix_st7735
#include "display_st7735.h"
#include "drivers/lcd_driver_api.h"
#include "font.h"


// #define ST7735_CMD_DATA_PIN     DT_INST_GPIO_PIN(0, cmd_data_gpios)
// #define ST7735_RES_PIN          DT_INST_GPIO_PIN(0, reset_gpios)
// #define ST7735_BLK_PIN          DT_INST_GPIO_PIN(0, blk_gpios)
// #define ST7735_CS_PIN           DT_INST_SPI_DEV_CS_GPIOS_PIN(0)


// #define RES_FLAGS	    DT_INST_GPIO_FLAGS(0, reset_gpios)
// #define CMD_DATA_FLAGS  DT_INST_GPIO_FLAGS(0, cmd_data_gpios)
// #define BLK_FLAGS       DT_INST_GPIO_FLAGS(0, blk_gpios)


// static struct st7735_data st7735_data_instance={
//     .height=LCD_HIGH,
//     .width=LCD_WIDTH,
// };

// void st7735_set_cmd(void *param)
// {
//     struct st7735_data* data=(struct st7735_data*)param;
//     gpio_pin_set(data->cmd_data_gpio, ST7735_CMD_DATA_PIN, 0);
// }
// void st7735_set_data(void *param)
// {
//     struct st7735_data* data=(struct st7735_data*)param;
//     gpio_pin_set(data->cmd_data_gpio, ST7735_CMD_DATA_PIN, 1);
// }
// static inline void st7735_resetpin_low(void *param)
// {
//     struct st7735_data* data=(struct st7735_data*)param;
//     gpio_pin_set(data->reset_gpio,ST7735_RES_PIN,0);
// }
// static inline void st7735_resetpin_high(void *param)
// {
//     struct st7735_data* data=(struct st7735_data*)param;
//     gpio_pin_set(data->reset_gpio,ST7735_RES_PIN,1);
// }

// static inline void st7735_blk_close(void *param)
// {
//     struct st7735_data* data=(struct st7735_data*)param;
//     gpio_pin_set(data->blk_gpio,ST7735_BLK_PIN,0);
// }
// static inline void st7735_blk_open(void *param)
// {
//     struct st7735_data* data=(struct st7735_data*)param;
//     gpio_pin_set(data->blk_gpio,ST7735_BLK_PIN,1);
// }

// static void st7735_transmit(struct st7735_data* data,uint8_t cmd,uint8_t* tx_data,size_t tx_count)
// {
//     struct spi_buf tx_buf={
//         .buf=&cmd,
//         .len=1,
//     };//先发送命令,接下来再开始发送数据
//     struct spi_buf_set tx_bufs={
//         .buffers=&tx_buf,
//         .count=1,
//     };
//     st7735_set_cmd(data);
//     // k_sleep(K_USEC(20));
//     spi_write(data->spi_dev,&data->spi_config,&tx_bufs);

//     if(tx_data!=NULL)
//     {
//         tx_buf.buf=tx_data;
//         tx_buf.len=tx_count;
//         st7735_set_data(data);
//         // k_sleep(K_USEC(20));
//         spi_write(data->spi_dev,&data->spi_config,&tx_bufs);
//     }

// }

// static void write_data(struct st7735_data* data,uint8_t* tx_data,size_t tx_count)
// {
//     struct spi_buf tx_buf={
//         .buf=tx_data,
//         .len=tx_count,
//     };//先发送命令,接下来再开始发送数据
//     struct spi_buf_set tx_bufs={
//         .buffers=&tx_buf,
//         .count=1,
//     };
//     st7735_set_data(data);
//     // k_sleep(K_USEC(20));
//     spi_write(data->spi_dev,&data->spi_config,&tx_bufs);
// }

// void setCursor(uint8_t xstart,uint8_t ystart,uint8_t xend,uint8_t yend)
// {
//     uint8_t CASET_follow[]={0,xstart,0,xend};
//     uint8_t RASET_follow[]={0,ystart,0,yend};
//     st7735_transmit(&st7735_data_instance,ST7735_CMD_CASET,CASET_follow,sizeof(CASET_follow));
//     st7735_transmit(&st7735_data_instance,ST7735_CMD_RASET,RASET_follow,sizeof(RASET_follow));
//     st7735_transmit(&st7735_data_instance,ST7735_CMD_RAMWR,NULL,0);
// }

// void LCD_FILL(uint8_t xstart,uint8_t ystart,uint8_t xend,uint8_t yend,uint16_t color)
// {
//     uint32_t pixel=(xend-xstart+1)*(yend-ystart+1);
//     uint8_t color_data[]={((color>>8)&0xff),(color&0xff)};
//     setCursor(xstart,ystart,xend,yend);
//     while(pixel>0)
//     {
//         pixel--;
//         write_data(&st7735_data_instance,color_data,sizeof(color_data));
//     }
// }



// void drawPoint(uint16_t x, uint16_t y, uint16_t color)
// {
//     uint8_t color_data[]={((color>>8)&0xff),(color&0xff)};
//     setCursor(x, y, x, y);      //设置光标位置 
//     write_data(&st7735_data_instance,color_data,sizeof(color_data));    
// }

// void drawAscii(uint16_t x,uint16_t y,uint8_t num,uint8_t size,uint32_t fColor, uint32_t bColor)
// {  			
//     uint8_t temp;
// 	uint16_t y0=y;
    
// 	uint8_t csize=(size/8+((size%8)?1:0))*(size/2);		   // 得到字体一个字符对应点阵集所占的字节数	
//  	num=num-' ';                                       // 得到偏移后的值（ASCII字库是从空格开始取模，所以-' '就是对应字符的字库）
// 	for(uint8_t t=0;t<csize;t++)
// 	{   
// 		if(size==12)         temp=asc2_1206[num][t];   // 调用1206字体
// 		else if(size==16)    temp=asc2_1608[num][t];   // 调用1608字体
// 		else if(size==24)    temp=asc2_2412[num][t];   // 调用2412字体
// 		else if(size==32)    temp=asc2_3216[num][t];   // 调用3216字体
// 		else return;								   // 没有的字库
		
// 		for(uint8_t t1=0; t1<8; t1++)
// 		{			    
//             if(temp&0x80)   drawPoint (x, y, fColor);  // 字体 画点 
//             else            drawPoint (x, y, bColor);  // 背景 画点
//             temp<<=1;
// 			y++;
// 			if(y>=st7735_data_instance.height)    return;		       // 超出屏幕高度(底)
// 			if((y-y0)==size)
// 			{
// 				y=y0;
// 				x++;
// 				if(x>=st7735_data_instance.width) return;              // 超出屏幕宽度(宽)
// 				break;
// 			}
// 		}  	 
// 	}  	    	 	  
// } 




































// void LCD_Init(void)
// {
//     uint8_t B1_follow[]={0x05,0x3C,0x3C};
//     uint8_t B2_follow[]={0x05,0x3C,0x3C};
//     uint8_t B3_follow[]={0x05,0x3C,0x3C,0x05,0x3C,0x3C};
//     uint8_t B4_follow[]={0x03};
//     uint8_t C0_follow[]={0x28,0x08,0x04};
//     uint8_t C1_follow[]={0xC0};
//     uint8_t C2_follow[]={0x0D,0x00};
//     uint8_t C3_follow[]={0x8D,0x2A};
//     uint8_t C4_follow[]={0x8D,0xEE};
//     uint8_t C5_follow[]={0x1A};
//     uint8_t MXY_follow[]={0xC0,0x00,0xA0,0x60};
//     uint8_t E0_follow[]={0x04,0x22,0x07,0x0A,0x2E,0x30,0x25,0x2A,0x28,0x26,0x2E,0x3A,0x00,0x01,0x03,0x13};
//     uint8_t E1_follow[]={0x04,0x16,0x06,0x0D,0x2D,0x26,0x23,0x27,0x27,0x25,0x2D,0x3B,0x00,0x01,0x04,0x13};
//     uint8_t CASET_follow[]={0,0,0,0x7f};
//     uint8_t RASET_follow[]={0,0,0,0x9f};
//     uint8_t COLMOD_follow[]={0x05};
//     st7735_blk_open(&st7735_data_instance);
    
//     if((LCD_DIR==1)||(LCD_DIR==3))
//     {
//         st7735_data_instance.width=LCD_WIDTH;
//         st7735_data_instance.height=LCD_HIGH;
//     }else{
//         st7735_data_instance.width=LCD_HIGH;
//         st7735_data_instance.height=LCD_WIDTH;
//     }

//     st7735_resetpin_low(&st7735_data_instance);
//     k_sleep(K_MSEC(10));
//     st7735_resetpin_high(&st7735_data_instance);
//     k_sleep(K_MSEC(10));

//     st7735_transmit(&st7735_data_instance,ST7735_CMD_SLEEP_OUT,NULL,0);
//     k_sleep(K_MSEC(100));//唤醒屏幕

//     st7735_transmit(&st7735_data_instance,ST7735_CMD_RGBCTRL,B1_follow,sizeof(B1_follow));

//     st7735_transmit(&st7735_data_instance,ST7735_CMD_PORCTRL,B2_follow,sizeof(B2_follow));

//     st7735_transmit(&st7735_data_instance,ST7735_CMD_B3,B3_follow,sizeof(B3_follow));
//     st7735_transmit(&st7735_data_instance,ST7735_CMD_B4,B4_follow,sizeof(B4_follow));
//     st7735_transmit(&st7735_data_instance,ST7735_CMD_LCMCTRL,C0_follow,sizeof(C0_follow));
//     st7735_transmit(&st7735_data_instance,ST7735_CMD_C1,C1_follow,sizeof(C1_follow));

//     st7735_transmit(&st7735_data_instance,ST7735_CMD_VDVVRHEN,C2_follow,sizeof(C2_follow));
//     st7735_transmit(&st7735_data_instance,ST7735_CMD_VRH,C3_follow,sizeof(C3_follow));
//     st7735_transmit(&st7735_data_instance,ST7735_CMD_VDS,C4_follow,sizeof(C4_follow));
//     st7735_transmit(&st7735_data_instance,ST7735_CMD_C5,C5_follow,sizeof(C5_follow));

//     if(LCD_DIR==1)st7735_transmit(&st7735_data_instance,ST7735_CMD_MADCTL,&MXY_follow[0],1);
//     if(LCD_DIR==2)st7735_transmit(&st7735_data_instance,ST7735_CMD_MADCTL,&MXY_follow[1],1);
//     if(LCD_DIR==3)st7735_transmit(&st7735_data_instance,ST7735_CMD_MADCTL,&MXY_follow[2],1);
//     if(LCD_DIR==4)st7735_transmit(&st7735_data_instance,ST7735_CMD_MADCTL,&MXY_follow[3],1);
    
//     st7735_transmit(&st7735_data_instance,ST7735_CMD_PVGAMCTRL,E0_follow,sizeof(E0_follow));
//     st7735_transmit(&st7735_data_instance,ST7735_CMD_NVGAMCTRL,E1_follow,sizeof(E1_follow));
//     st7735_transmit(&st7735_data_instance,ST7735_CMD_CASET,CASET_follow,sizeof(CASET_follow));

//     st7735_transmit(&st7735_data_instance,ST7735_CMD_RASET,RASET_follow,sizeof(RASET_follow));
//     st7735_transmit(&st7735_data_instance,ST7735_CMD_COLMOD,COLMOD_follow,sizeof(COLMOD_follow));

//     st7735_transmit(&st7735_data_instance,ST7735_CMD_DISP_ON,NULL,0);
    
//     LCD_FILL(1,1,st7735_data_instance.width,st7735_data_instance.height,GREEN);
// }


// static int st7735_init(const struct device *dev)
// {
//     struct st7735_data* data = (struct st7735_data*)dev->data;
//     data->spi_dev = device_get_binding(DT_INST_BUS_LABEL(0));
//     if(data->spi_dev==NULL)
//     {
//         printk("data->spi_dev is null");
//         return -EPERM;
//     }
//     data->spi_config.frequency=DT_INST_PROP(0, spi_max_frequency);
//     data->spi_config.operation=SPI_OP_MODE_MASTER|SPI_WORD_SET(8)|SPI_MODE_CPHA|SPI_MODE_CPOL;
//     data->spi_config.slave=DT_INST_REG_ADDR(0);


//     data->cs_ctrl.gpio_dev=device_get_binding(DT_INST_SPI_DEV_CS_GPIOS_LABEL(0));
//     data->cs_ctrl.gpio_pin=ST7735_CS_PIN;
//     data->cs_ctrl.gpio_dt_flags=DT_INST_SPI_DEV_CS_GPIOS_FLAGS(0);
//     data->cs_ctrl.delay=0u;
//     data->spi_config.cs=&(data->cs_ctrl);

//     data->reset_gpio=device_get_binding(DT_INST_GPIO_LABEL(0, reset_gpios));
//     if(data->reset_gpio==NULL)
//     {
//         printk("data->reset_gpio is null\n");
//         return -EPERM;
//     }
//     gpio_pin_configure(data->reset_gpio,ST7735_RES_PIN,GPIO_OUTPUT_INACTIVE|RES_FLAGS);


//     data->cmd_data_gpio=device_get_binding(DT_INST_GPIO_LABEL(0, cmd_data_gpios));
//     if(data->cmd_data_gpio==NULL)
//     {
//         printk("data->cmd_data_gpio is null\n");
//         return -EPERM;
//     }
//     gpio_pin_configure(data->cmd_data_gpio,ST7735_CMD_DATA_PIN,GPIO_OUTPUT|CMD_DATA_FLAGS);

//     data->blk_gpio=device_get_binding(DT_INST_GPIO_LABEL(0, blk_gpios));
//     if(data->blk_gpio==NULL)
//     {
//         printk("data->blk_gpio is null\n");
//         return -EPERM;
//     }
//     gpio_pin_configure(data->blk_gpio,ST7735_BLK_PIN,GPIO_OUTPUT|BLK_FLAGS);

// }



// void lcd_clear(uint32_t color)
// {
//     LCD_FILL(1,1,st7735_data_instance.width,st7735_data_instance.height,color);
// }


// static const struct lcd_display_driver_api  st7735_api={
//     .st7735_lcd_init=LCD_Init,
//     .writeAscii=drawAscii,
//     .showclear=lcd_clear,
//     .lcd_fill_area=LCD_FILL,
// };

// DEVICE_DT_INST_DEFINE(0, &st7735_init,
// 	      st7735_pm_control, &st7735_data_instance, NULL, APPLICATION,
// 	      CONFIG_APPLICATION_INIT_PRIORITY, &st7735_api);




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