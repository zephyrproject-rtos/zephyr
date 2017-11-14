/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2016 Altera Corporation, San Jose, California, USA.           *
* All rights reserved.                                                        *
*                                                                             *
* Permission is hereby granted, free of charge, to any person obtaining a     *
* copy of this software and associated documentation files (the "Software"),  *
* to deal in the Software without restriction, including without limitation   *
* the rights to use, copy, modify, merge, publish, distribute, sublicense,    *
* and/or sell copies of the Software, and to permit persons to whom the       *
* Software is furnished to do so, subject to the following conditions:        *
*                                                                             *
* The above copyright notice and this permission notice shall be included in  *
* all copies or substantial portions of the Software.                         *
*                                                                             *
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR  *
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,    *
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE *
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER      *
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING     *
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER         *
* DEALINGS IN THE SOFTWARE.                                                   *
*                                                                             *
*                                                                             *
******************************************************************************/
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "io.h"
#include "priv/alt_busy_sleep.h"
#include "sys/alt_errno.h"
#include "sys/alt_irq.h"
#include "sys/alt_stdio.h"
#include "sys/alt_alarm.h"
#include "altera_avalon_i2c.h"



/* for all functions in this file, see the altera_avalon_i2c.h file for more complete function descriptions. */

/* optional irq callback */
static void optional_irq_callback(void * context)
{
   int timeout=100000;
   alt_u32 bytes_read;

   ALT_AVALON_I2C_DEV_t *i2c_dev = context;
   IRQ_DATA_t *irq = i2c_dev->callback_context;

   if (irq->irq_busy==2)  /*receive request*/
   {
       alt_avalon_i2c_rx_read_available(i2c_dev, irq->buffer, irq->size, &bytes_read);
       irq->size-=bytes_read;
       irq->buffer+=bytes_read;
       if (irq->size > 0)
       {
         /* clear ISR register content */
         alt_avalon_i2c_int_clear(i2c_dev,ALT_AVALON_I2C_ISR_ALL_CLEARABLE_INTS_MSK);
         /* re-enable the RX_READY interrupt */
         alt_avalon_i2c_int_enable(i2c_dev,ALT_AVALON_I2C_ISER_RX_READY_EN_MSK);
         return;
       }
    }

    /*transaction should be done so no or minimal looping should occur*/
    /*for a write, this code will only be reached after the cmd fifo is*/
    /*empty (sent).  For a read this code will only be reached after all*/
    /*bytes have been received.*/
    while (alt_avalon_i2c_is_busy(i2c_dev)) 
    { 
      if (--timeout == 0)
      {
         break;
      }
    }

    /*disable the ip.  The ip is disabled and enabled for each transaction.*/
    alt_avalon_i2c_disable(i2c_dev);

    irq->irq_busy=0;
}

void alt_avalon_i2c_register_optional_irq_handler(ALT_AVALON_I2C_DEV_t *i2c_dev,IRQ_DATA_t * irq_data)
{
   irq_data->irq_busy=0;
   alt_avalon_i2c_register_callback(i2c_dev,optional_irq_callback,0,irq_data);
}

/* The list of registered i2c components */
ALT_LLIST_HEAD(alt_avalon_i2c_list);

/* Interrupt handler for the AVALON_I2C module. */
/* Interrupts are not re-enabled in this handler */
static void alt_avalon_i2c_irq(void *context)
{
    ALT_AVALON_I2C_DEV_t *dev = (ALT_AVALON_I2C_DEV_t *) context;
    alt_irq_context cpu_sr;
     
    /*disable i2c interrupts*/
    alt_avalon_i2c_int_disable(dev,ALT_AVALON_I2C_ISR_ALLINTS_MSK);
    
    /* clear irq status */
    alt_avalon_i2c_int_clear(dev,ALT_AVALON_I2C_ISR_ALL_CLEARABLE_INTS_MSK);
 
    /* 
    * Other interrupts are explicitly disabled if callbacks
    * are registered because there is no guarantee that they are 
    * pre-emption-safe. This allows the driver to support 
    * interrupt pre-emption.
    */
    if(dev->callback) 
    {
        cpu_sr = alt_irq_disable_all();
        dev->callback(dev);
        alt_irq_enable_all(cpu_sr);
    }

    return;
}

/* Associate a user-specific routine with the i2c interrupt handler. */
void alt_avalon_i2c_register_callback(
    ALT_AVALON_I2C_DEV_t *dev,
    alt_avalon_i2c_callback callback,
    alt_u32 control,
    void *context)
{
    dev->callback         = callback;
    dev->callback_context = context;
    dev->control          = control;

    return ;
}

 /* Initializes the I2C Module. This routine is called
 * from the ALT_AVALON_I2C_INIT macro and is called automatically
 * by alt_sys_init.c */
void alt_avalon_i2c_init (ALT_AVALON_I2C_DEV_t *dev)
{
    extern alt_llist alt_avalon_i2c_list;
    ALT_AVALON_I2C_MASTER_CONFIG_t cfg;
    int error;

    /* disable ip */
    alt_avalon_i2c_disable(dev);

    /* Disable interrupts */
    alt_avalon_i2c_int_disable(dev,ALT_AVALON_I2C_ISR_ALLINTS_MSK);

    /* clear ISR register content */
    alt_avalon_i2c_int_clear(dev,ALT_AVALON_I2C_ISR_ALL_CLEARABLE_INTS_MSK);
    
    /* set the cmd fifo threshold */
    alt_avalon_i2c_tfr_cmd_fifo_threshold_set(dev,ALT_AVALON_I2C_TFR_CMD_FIFO_NOT_FULL);
    
    /* set the tx fifo threshold */
    alt_avalon_i2c_rx_fifo_threshold_set(dev,ALT_AVALON_I2C_RX_DATA_FIFO_FULL);
    
    /* set the default bus speed */
    cfg.speed_mode = ALT_AVALON_I2C_SPEED_STANDARD;
    
    /*set the address mode */
    cfg.addr_mode = ALT_AVALON_I2C_ADDR_MODE_7_BIT;
    
    /* set the bus speed */
    alt_avalon_i2c_master_config_speed_set(dev,&cfg,ALT_AVALON_I2C_SS_MAX_HZ);
    
    /* write the cfg information */
    alt_avalon_i2c_master_config_set(dev,&cfg);
    
    /* Register this instance of the i2c controller with HAL */
    alt_dev_llist_insert((alt_dev_llist*) dev, &alt_avalon_i2c_list);

    /*
     * Creating semaphores used to protect access to the registers 
     * when running in a multi-threaded environment.
     */
    error = ALT_SEM_CREATE (&dev->regs_lock, 1);

    if (!error)
    {        
        /* Install IRQ handler */
        alt_ic_isr_register(dev->irq_controller_ID, dev->irq_ID, alt_avalon_i2c_irq, dev, 0x0);
    }
    else
    {
        alt_printf("failed to create semaphores\n");
    }

    return;

}

/*  Retrieve a pointer to the i2c instance */
ALT_AVALON_I2C_DEV_t* alt_avalon_i2c_open(const char* name)
{
    ALT_AVALON_I2C_DEV_t* dev = NULL;

    dev = (ALT_AVALON_I2C_DEV_t*) alt_find_dev (name, &alt_avalon_i2c_list);

    return dev;
}

/* enable the avalon i2c ip */
ALT_AVALON_I2C_STATUS_CODE alt_avalon_i2c_enable(ALT_AVALON_I2C_DEV_t *i2c_dev)
{
   IRQ_DATA_t *irq_data = i2c_dev->callback_context;
   alt_u32 enable_status;
       
   /*if the ip is already enabled, return a busy status*/
   enable_status = (IORD_ALT_AVALON_I2C_CTRL(i2c_dev->i2c_base) & ALT_AVALON_I2C_CTRL_EN_MSK) >> ALT_AVALON_I2C_CTRL_EN_OFST;
   if (enable_status)
   {
     return ALT_AVALON_I2C_BUSY;
   }
   
   /*if the optional irq callback is registered ensure irq_busy is 0*/
   if (i2c_dev->callback == optional_irq_callback)
   {
     irq_data->irq_busy=0;
   }
   
   /* enable ip */
   IORMW_ALT_AVALON_I2C_CTRL(i2c_dev->i2c_base,ALT_AVALON_I2C_CTRL_EN_MSK,ALT_AVALON_I2C_CTRL_EN_MSK);

   return ALT_AVALON_I2C_SUCCESS;
}

/* disable the avalon i2c ip */
void alt_avalon_i2c_disable(ALT_AVALON_I2C_DEV_t *i2c_dev)
{
   /* disable ip */
   IORMW_ALT_AVALON_I2C_CTRL(i2c_dev->i2c_base,0,ALT_AVALON_I2C_CTRL_EN_MSK);

}

/* populate the the master config structure from the register values */
void alt_avalon_i2c_master_config_get(ALT_AVALON_I2C_DEV_t *i2c_dev,
                                          ALT_AVALON_I2C_MASTER_CONFIG_t* cfg)
{

    cfg->addr_mode = i2c_dev->address_mode;
    cfg->speed_mode = (IORD_ALT_AVALON_I2C_CTRL(i2c_dev->i2c_base) & ALT_AVALON_I2C_CTRL_BUS_SPEED_MSK) >> ALT_AVALON_I2C_CTRL_BUS_SPEED_OFST;    

    cfg->scl_hcnt = (IORD_ALT_AVALON_I2C_SCL_HIGH(i2c_dev->i2c_base) & ALT_AVALON_I2C_SCL_HIGH_COUNT_PERIOD_MSK) >> ALT_AVALON_I2C_SCL_HIGH_COUNT_PERIOD_OFST;    
    cfg->scl_lcnt = (IORD_ALT_AVALON_I2C_SCL_LOW(i2c_dev->i2c_base) & ALT_AVALON_I2C_SCL_LOW_COUNT_PERIOD_MSK) >> ALT_AVALON_I2C_SCL_LOW_COUNT_PERIOD_OFST;    
    cfg->sda_cnt = (IORD_ALT_AVALON_I2C_SDA_HOLD(i2c_dev->i2c_base) & ALT_AVALON_I2C_SDA_HOLD_COUNT_PERIOD_MSK) >> ALT_AVALON_I2C_SDA_HOLD_COUNT_PERIOD_OFST;    
}

/* set the registers from the master config structure */
void alt_avalon_i2c_master_config_set(ALT_AVALON_I2C_DEV_t *i2c_dev,
                                          const ALT_AVALON_I2C_MASTER_CONFIG_t* cfg)
{
    i2c_dev->address_mode   =   cfg->addr_mode;
    IORMW_ALT_AVALON_I2C_CTRL(i2c_dev->i2c_base,(cfg->speed_mode) << ALT_AVALON_I2C_CTRL_BUS_SPEED_OFST,ALT_AVALON_I2C_CTRL_BUS_SPEED_MSK);

    IOWR_ALT_AVALON_I2C_SCL_HIGH(i2c_dev->i2c_base,cfg->scl_hcnt);
    IOWR_ALT_AVALON_I2C_SCL_LOW(i2c_dev->i2c_base,cfg->scl_lcnt);
    IOWR_ALT_AVALON_I2C_SDA_HOLD(i2c_dev->i2c_base,cfg->sda_cnt);
}

/* This function returns the speed based on parameters of the
 * I2C master configuration.
*/
ALT_AVALON_I2C_STATUS_CODE alt_avalon_i2c_master_config_speed_get(ALT_AVALON_I2C_DEV_t *i2c_dev,
                                                const ALT_AVALON_I2C_MASTER_CONFIG_t* cfg,
                                                alt_u32 * speed_in_hz)
{

   if ((cfg->scl_lcnt == 0) || (cfg->scl_hcnt == 0))
   {
       return ALT_AVALON_I2C_BAD_ARG;
   }
    
   *speed_in_hz = (i2c_dev->ip_freq_in_hz) / (cfg->scl_lcnt + cfg->scl_hcnt);

   return ALT_AVALON_I2C_SUCCESS;
}

/*This is a utility function that computes parameters for the I2C master
 * configuration that best matches the speed requested. */
 ALT_AVALON_I2C_STATUS_CODE alt_avalon_i2c_master_config_speed_set(ALT_AVALON_I2C_DEV_t *i2c_dev,
                                                ALT_AVALON_I2C_MASTER_CONFIG_t * cfg,
                                                alt_u32 speed_in_hz)
{
    alt_u32 scl_lcnt,scl_hcnt;

    /* If speed is not standard or fast return range error */
    if ((speed_in_hz > ALT_AVALON_I2C_FS_MAX_HZ) || (speed_in_hz < ALT_AVALON_I2C_SS_MIN_HZ) || (speed_in_hz == 0))
    {
        return ALT_AVALON_I2C_RANGE;
    }

     /* <lcount> = <internal clock> / 2 * <speed, Hz> */
    scl_lcnt = (i2c_dev->ip_freq_in_hz) / (speed_in_hz << 1);

    /* adjust h/l by predetermined amount */
    scl_hcnt = scl_lcnt + ALT_AVALON_I2C_DIFF_LCNT_HCNT;
    scl_lcnt = scl_lcnt - ALT_AVALON_I2C_DIFF_LCNT_HCNT;

    if (speed_in_hz > ALT_AVALON_I2C_FS_MIN_HZ)
    {
       cfg->speed_mode = ALT_AVALON_I2C_SPEED_FAST;
    }
    else 
    {
       cfg->speed_mode = ALT_AVALON_I2C_SPEED_STANDARD;    
    }

    cfg->scl_lcnt = scl_lcnt;
    cfg->scl_hcnt = scl_hcnt;
    cfg->sda_cnt  = scl_lcnt - (scl_lcnt / 2);

    return ALT_AVALON_I2C_SUCCESS;

}

/*Returns ALT_AVALON_I2C_TRUE if the I2C controller is busy. The I2C controller is busy if
 * not in the IDLE state */
ALT_AVALON_I2C_STATUS_CODE alt_avalon_i2c_is_busy(ALT_AVALON_I2C_DEV_t *i2c_dev)
{

    if (IORD_ALT_AVALON_I2C_STATUS(i2c_dev->i2c_base) & ALT_AVALON_I2C_STATUS_CORE_STATUS_MSK)
    {
       return ALT_AVALON_I2C_TRUE;
    }

    return ALT_AVALON_I2C_FALSE;
}

/*Read all available bytes from the receive FIFO up to max_bytes_to_read.  If max_bytes_to_read = 0 then read all available */
void alt_avalon_i2c_rx_read_available(ALT_AVALON_I2C_DEV_t *i2c_dev, alt_u8 *buffer, alt_u32 max_bytes_to_read, alt_u32 *bytes_read)
{
    *bytes_read = 0;
    
    while (IORD_ALT_AVALON_I2C_RX_DATA_FIFO_LVL(i2c_dev->i2c_base))
    {
       buffer[*bytes_read] = (alt_u8)IORD_ALT_AVALON_I2C_RX_DATA(i2c_dev->i2c_base);
       *bytes_read+=1; 
       if ((*bytes_read == max_bytes_to_read) && (max_bytes_to_read != 0)) break;       
    }
}

/*when a byte is available, reads a single data byte from the receive FIFO. */
ALT_AVALON_I2C_STATUS_CODE alt_avalon_i2c_rx_read(ALT_AVALON_I2C_DEV_t *i2c_dev, alt_u8 *val)
{
    alt_u32 status = ALT_AVALON_I2C_SUCCESS;
    alt_u32 timeout = 100000;


    while (IORD_ALT_AVALON_I2C_RX_DATA_FIFO_LVL(i2c_dev->i2c_base) == 0)
    {
      if (timeout<10) alt_busy_sleep(10000);
      if (--timeout == 0)
      {
        status = ALT_AVALON_I2C_TIMEOUT;
        break;
      }
    }

    *val = (alt_u8)IORD_ALT_AVALON_I2C_RX_DATA(i2c_dev->i2c_base);
        
    return status;
}

/* When space is available, writes the Transfer Command FIFO. */
ALT_AVALON_I2C_STATUS_CODE alt_avalon_i2c_cmd_write(ALT_AVALON_I2C_DEV_t *i2c_dev, 
                                                      alt_u8 val,
                                                      alt_u8 issue_restart,
                                                      alt_u8 issue_stop)
{
    alt_u32 timeout = 10000;
    ALT_AVALON_I2C_STATUS_CODE status = ALT_AVALON_I2C_SUCCESS;


    while ((IORD_ALT_AVALON_I2C_ISR(i2c_dev->i2c_base) & ALT_AVALON_I2C_ISR_TX_READY_MSK)==0) 
    {
      if (timeout<10) alt_busy_sleep(10000);    
      if (--timeout == 0)
      {
        return ALT_AVALON_I2C_TIMEOUT;
      }
    }

    IOWR_ALT_AVALON_I2C_TFR_CMD(i2c_dev->i2c_base,val |
                                     (issue_restart << ALT_AVALON_I2C_TFR_CMD_STA_OFST) |
                                     (issue_stop << ALT_AVALON_I2C_TFR_CMD_STO_OFST));


    /*check for nack error*/
    alt_avalon_i2c_check_nack(i2c_dev,&status);
    
    /*check for arb lost*/
    alt_avalon_i2c_check_arblost(i2c_dev,&status);
    
    return status;
}

/*send 7 or 10 bit i2c address to cmd fifo*/
ALT_AVALON_I2C_STATUS_CODE alt_avalon_i2c_send_address(ALT_AVALON_I2C_DEV_t *i2c_dev,
                                                       const alt_u32 rw_bit,
                                                       const alt_u8 issue_restart)
{
    alt_u32 status;
        
    if (i2c_dev->address_mode == ALT_AVALON_I2C_ADDR_MODE_10_BIT)
    {
       status = alt_avalon_i2c_cmd_write(i2c_dev,(((i2c_dev->master_target_address | TARGET_ADDR_MASK_10BIT) >> 7) & 0xfe) | rw_bit,issue_restart,ALT_AVALON_I2C_NO_STOP);
       status = alt_avalon_i2c_cmd_write(i2c_dev,i2c_dev->master_target_address & 0xff,ALT_AVALON_I2C_NO_RESTART,ALT_AVALON_I2C_NO_STOP);      
    }
    else
    {
       status = alt_avalon_i2c_cmd_write(i2c_dev,(i2c_dev->master_target_address << 1) | rw_bit,issue_restart,ALT_AVALON_I2C_NO_STOP);
    }
    
    return status;
}

/* This function returns the current target address. */
void alt_avalon_i2c_master_target_get(ALT_AVALON_I2C_DEV_t * i2c_dev, alt_u32 * target_addr)
{
    *target_addr=i2c_dev->master_target_address;
}

/* This function updates the target address for any upcoming I2C bus IO. */
void alt_avalon_i2c_master_target_set(ALT_AVALON_I2C_DEV_t * i2c_dev, alt_u32 target_addr)
{
    i2c_dev->master_target_address=target_addr;
}

/*if nack detected, status is set to ALT_AVALON_I2C_NACK_ERR*/
void alt_avalon_i2c_check_nack(ALT_AVALON_I2C_DEV_t *i2c_dev,ALT_AVALON_I2C_STATUS_CODE * status)
{    
    if (IORD_ALT_AVALON_I2C_ISR(i2c_dev->i2c_base) & ALT_AVALON_I2C_ISR_NACK_DET_MSK)
    {
      *status=ALT_AVALON_I2C_NACK_ERR;
    }
}

/*if arb lost is detected, status is set to ALT_AVALON_I2C_ARB_LOST_ERR*/
void alt_avalon_i2c_check_arblost(ALT_AVALON_I2C_DEV_t *i2c_dev,ALT_AVALON_I2C_STATUS_CODE * status)
{      
    if (IORD_ALT_AVALON_I2C_ISR(i2c_dev->i2c_base) & ALT_AVALON_I2C_ISR_ARBLOST_DET_MSK)
    {
      *status=ALT_AVALON_I2C_ARB_LOST_ERR;
    }
}

ALT_AVALON_I2C_STATUS_CODE alt_avalon_i2c_interrupt_transaction_status(ALT_AVALON_I2C_DEV_t *i2c_dev)
{
    ALT_AVALON_I2C_STATUS_CODE status = ALT_AVALON_I2C_SUCCESS;
    IRQ_DATA_t *irq_data = i2c_dev->callback_context;
    alt_u32 timeout=10000 * irq_data->size + 10000;
    alt_u32 saveints,temp_bytes_read;
    
    /* save current enabled interrupts */
    alt_avalon_i2c_enabled_ints_get(i2c_dev,&saveints);
    
    /* disable the enabled interrupts */
    alt_avalon_i2c_int_disable(i2c_dev,saveints);
    
    alt_avalon_i2c_check_nack(i2c_dev,&status);

    if (status!=ALT_AVALON_I2C_SUCCESS)
    {
      if (irq_data->irq_busy)
      {
        while (alt_avalon_i2c_is_busy(i2c_dev))
        {
              if (timeout<10) alt_busy_sleep(10000);
              if (--timeout == 0)
              {
                 status = ALT_AVALON_I2C_TIMEOUT;
                 break;
              }
        }
          
        /*clear any rx entries */
        alt_avalon_i2c_rx_read_available(i2c_dev, irq_data->buffer,0,&temp_bytes_read);
       
        /*disable the ip.  The ip is disabled and enabled for each transaction. */
        alt_avalon_i2c_disable(i2c_dev);
          
        /*abort the transaction */
        irq_data->irq_busy=0;
      }
      
      /*return nack error so transaction can be retried*/
      return status;
    }
    
    if (irq_data->irq_busy)
    {
        /*re-enable the interrupts*/
        alt_avalon_i2c_int_enable(i2c_dev,saveints);
        
        /*return transaction still busy*/
        return ALT_AVALON_I2C_BUSY;
    }
    
    /*return transaction completed status, ok to do another transaction*/
    return ALT_AVALON_I2C_SUCCESS;
}

/*transmit function with retry and optionally interrupts*/
ALT_AVALON_I2C_STATUS_CODE alt_avalon_i2c_master_tx(ALT_AVALON_I2C_DEV_t *i2c_dev,
                                       const alt_u8 * buffer,
                                       const alt_u32 size,
                                       const alt_u8 use_interrupts)
{
    ALT_AVALON_I2C_STATUS_CODE status;
    alt_u32 retry=10000;  
    
    while (retry--)
    {
      if (retry<10) alt_busy_sleep(10000);
      if (use_interrupts)
      {
         status = alt_avalon_i2c_master_transmit_using_interrupts(i2c_dev, buffer, size, ALT_AVALON_I2C_NO_RESTART, ALT_AVALON_I2C_STOP); 
      }
      else
      {
         status = alt_avalon_i2c_master_transmit(i2c_dev, buffer, size, ALT_AVALON_I2C_NO_RESTART, ALT_AVALON_I2C_STOP);
      }
      if ((status==ALT_AVALON_I2C_ARB_LOST_ERR) || (status==ALT_AVALON_I2C_NACK_ERR) || (status==ALT_AVALON_I2C_BUSY)) continue;
      break;
    }

    return status;
}        

/*receive function with retry and optionally interrupts*/
ALT_AVALON_I2C_STATUS_CODE alt_avalon_i2c_master_rx(ALT_AVALON_I2C_DEV_t *i2c_dev,
                                       alt_u8 * buffer,
                                       const alt_u32 size,
                                       const alt_u8 use_interrupts)
{
    ALT_AVALON_I2C_STATUS_CODE status;
    alt_u32 retry=10000;  
    
    if (use_interrupts) 
    {
      while (retry--) 
      {
        if (retry<10) alt_busy_sleep(10000);      
        status = alt_avalon_i2c_master_receive_using_interrupts(i2c_dev, buffer, size, ALT_AVALON_I2C_NO_RESTART, ALT_AVALON_I2C_STOP);     
        if ((status==ALT_AVALON_I2C_ARB_LOST_ERR) || (status==ALT_AVALON_I2C_NACK_ERR) || (status==ALT_AVALON_I2C_BUSY)) continue;
        break;
      }
    }
    else
    {
      while (retry--) 
      {
        if (retry<10) alt_busy_sleep(10000);      
        status = alt_avalon_i2c_master_receive(i2c_dev, buffer, size, ALT_AVALON_I2C_NO_RESTART, ALT_AVALON_I2C_STOP);     
        if ((status==ALT_AVALON_I2C_ARB_LOST_ERR) || (status==ALT_AVALON_I2C_NACK_ERR) || (status==ALT_AVALON_I2C_BUSY)) continue;
        break;
      }
    }
    
    return status;
}        


/*transmit, restart, recieve function using retry and optionally interrupts */
ALT_AVALON_I2C_STATUS_CODE alt_avalon_i2c_master_tx_rx(ALT_AVALON_I2C_DEV_t *i2c_dev,
                                       const alt_u8 * txbuffer,
                                       const alt_u32 txsize,
                                       alt_u8 * rxbuffer,
                                       const alt_u32 rxsize,
                                       const alt_u8 use_interrupts)                                       
{
    ALT_AVALON_I2C_STATUS_CODE status;
    alt_u32 retry=10000;  
    
    if (use_interrupts)
    {
      while (retry--) 
      {
        if (retry<10) alt_busy_sleep(10000);      
        status = alt_avalon_i2c_master_transmit_using_interrupts(i2c_dev, txbuffer, txsize, ALT_AVALON_I2C_NO_RESTART, ALT_AVALON_I2C_NO_STOP);     
        if ((status==ALT_AVALON_I2C_ARB_LOST_ERR) || (status==ALT_AVALON_I2C_NACK_ERR) || (status==ALT_AVALON_I2C_BUSY)) continue;
  
        status = alt_avalon_i2c_master_receive_using_interrupts(i2c_dev, rxbuffer, rxsize, ALT_AVALON_I2C_RESTART, ALT_AVALON_I2C_STOP);     
        if ((status==ALT_AVALON_I2C_ARB_LOST_ERR) || (status==ALT_AVALON_I2C_NACK_ERR) || (status==ALT_AVALON_I2C_BUSY)) continue;
  
        break;
      }
    }
    else 
    {
      while (retry--) 
      {
        if (retry<10) alt_busy_sleep(10000);      
        status = alt_avalon_i2c_master_transmit(i2c_dev, txbuffer, txsize, ALT_AVALON_I2C_NO_RESTART, ALT_AVALON_I2C_NO_STOP);     
        if ((status==ALT_AVALON_I2C_ARB_LOST_ERR) || (status==ALT_AVALON_I2C_NACK_ERR) || (status==ALT_AVALON_I2C_BUSY)) continue;

        status = alt_avalon_i2c_master_receive(i2c_dev, rxbuffer, rxsize, ALT_AVALON_I2C_RESTART, ALT_AVALON_I2C_STOP);     
        if ((status==ALT_AVALON_I2C_ARB_LOST_ERR) || (status==ALT_AVALON_I2C_NACK_ERR) || (status==ALT_AVALON_I2C_BUSY)) continue;
  
        break;
      }
    }
    
    return status;
}                                       
                                   
/*This function issues a write command and transmits data to the I2C bus. */
ALT_AVALON_I2C_STATUS_CODE alt_avalon_i2c_master_transmit(ALT_AVALON_I2C_DEV_t *i2c_dev,
                                        const alt_u8 * buffer,
                                        alt_u32 size,
                                        const alt_u8 issue_restart,
                                        const alt_u8 issue_stop)
{
    ALT_AVALON_I2C_STATUS_CODE status = ALT_AVALON_I2C_SUCCESS;
    alt_u32 timeout=size * 10000;
    
    if (size==0)
    {
      return ALT_AVALON_I2C_SUCCESS;
    }
    
    /*if a new transaction, enable ip and clear int status*/
    if (!issue_restart) 
    {
      /*enable the ip.  The ip is disabled and enabled for each transaction.*/
      status = alt_avalon_i2c_enable(i2c_dev);
      if (status != ALT_AVALON_I2C_SUCCESS)
      {
        return status;
      }

      /*Clear the ISR reg*/
      alt_avalon_i2c_int_clear(i2c_dev,ALT_AVALON_I2C_ISR_ALL_CLEARABLE_INTS_MSK);
    }

    /*Start Write, transmit address. */
    status = alt_avalon_i2c_send_address(i2c_dev,ALT_AVALON_I2C_WRITE,issue_restart);
      
    if (status == ALT_AVALON_I2C_SUCCESS)
    {
        while ((size > 1) && (status == ALT_AVALON_I2C_SUCCESS))
        {
            status = alt_avalon_i2c_cmd_write(i2c_dev, *buffer, ALT_AVALON_I2C_NO_RESTART, ALT_AVALON_I2C_NO_STOP);
            
            ++buffer;
            --size;
        }

        /* Last byte */
        if (status == ALT_AVALON_I2C_SUCCESS)
        {
            status = alt_avalon_i2c_cmd_write(i2c_dev, *buffer, ALT_AVALON_I2C_NO_RESTART, issue_stop);

            ++buffer;
            --size;
        }
    }
    
    /*if end of transaction, wait until the ip is idle then disable the ip*/
    if ((issue_stop) || (status != ALT_AVALON_I2C_SUCCESS)) 
    {

        while (alt_avalon_i2c_is_busy(i2c_dev))
        {
            if (timeout<10) alt_busy_sleep(10000);
            if (--timeout == 0)
            {
               status = ALT_AVALON_I2C_TIMEOUT;
               break;
            }
        }
     
        /*check for a nack error*/
        alt_avalon_i2c_check_nack(i2c_dev,&status);
        
        /*disable the ip.  The ip is disabled and enabled for each transaction.*/
        alt_avalon_i2c_disable(i2c_dev);
    }


    return status;
}

/*This function issues a write command and transmits data to the I2C bus  */
ALT_AVALON_I2C_STATUS_CODE alt_avalon_i2c_master_transmit_using_interrupts(ALT_AVALON_I2C_DEV_t *i2c_dev,
                                        const alt_u8 * buffer,
                                        alt_u32 size,
                                        const alt_u8 issue_restart,
                                        const alt_u8 issue_stop)
{
    ALT_AVALON_I2C_STATUS_CODE status = ALT_AVALON_I2C_SUCCESS;
    alt_u32 timeout=size*10000;
    IRQ_DATA_t *irq_data = i2c_dev->callback_context;    
    
    if (size==0)
    {
      return ALT_AVALON_I2C_SUCCESS;
    }
    
    /*IS the optional interrupt handler registered??*/
    if (i2c_dev->callback != optional_irq_callback)
    {
       return ALT_AVALON_I2C_BAD_ARG;    
    }
    
    /*if a new transaction, enable ip and clear int status*/
    if (!issue_restart) 
    {
      /*enable the ip.  The ip is disabled and enabled for each transaction.*/
      status = alt_avalon_i2c_enable(i2c_dev);
      if (status != ALT_AVALON_I2C_SUCCESS)
      {
        return status;
      }

      /*Clear the ISR reg*/
      alt_avalon_i2c_int_clear(i2c_dev,ALT_AVALON_I2C_ISR_ALL_CLEARABLE_INTS_MSK);
    }

    /*Start Write, transmit address. */
    status = alt_avalon_i2c_send_address(i2c_dev,ALT_AVALON_I2C_WRITE,issue_restart);
        
    if (status == ALT_AVALON_I2C_SUCCESS)
    {
        while ((size > 1) && (status == ALT_AVALON_I2C_SUCCESS))
        {
            status = alt_avalon_i2c_cmd_write(i2c_dev, *buffer, ALT_AVALON_I2C_NO_RESTART, ALT_AVALON_I2C_NO_STOP);
            
            ++buffer;
            --size;
        }

        /* Last byte */
        if (status == ALT_AVALON_I2C_SUCCESS)
        {
            status = alt_avalon_i2c_cmd_write(i2c_dev, *buffer, ALT_AVALON_I2C_NO_RESTART, issue_stop);

            ++buffer;
            --size;
        }
    }
    
    /*if error, wait until the ip is idle then disable the ip*/
    if (status != ALT_AVALON_I2C_SUCCESS) 
    {

        while (alt_avalon_i2c_is_busy(i2c_dev))
        {
            if (timeout<10) alt_busy_sleep(10000);        
            if (--timeout == 0)
            {
               status = ALT_AVALON_I2C_TIMEOUT;
               break;
            }
        }
     
        /*disable the ip.  The ip is disabled and enabled for each transaction.*/
        alt_avalon_i2c_disable(i2c_dev);
    }
    else
    {
       if (issue_stop)
       {
         /* clear ISR register content */
         alt_avalon_i2c_int_clear(i2c_dev,ALT_AVALON_I2C_ISR_ALL_CLEARABLE_INTS_MSK);
         /* set the cmd fifo threshold */
         alt_avalon_i2c_tfr_cmd_fifo_threshold_set(i2c_dev,ALT_AVALON_I2C_TFR_CMD_FIFO_EMPTY);
         /* set the interrupt transaction busy bit */
         irq_data->irq_busy=1;
         /* enable the TX_READY interrupt */
         alt_avalon_i2c_int_enable(i2c_dev,ALT_AVALON_I2C_ISER_TX_READY_EN_MSK);
       }
    }
    
    return status;
}

/*This function receives one or more data bytes transmitted from a slave in 
 * response to read requests issued from this master. */
ALT_AVALON_I2C_STATUS_CODE alt_avalon_i2c_master_receive(ALT_AVALON_I2C_DEV_t *i2c_dev,
                                       alt_u8 * buffer,
                                       const alt_u32 size,
                                       const alt_u8 issue_restart,
                                       const alt_u8 issue_stop)
{
    ALT_AVALON_I2C_STATUS_CODE status = ALT_AVALON_I2C_SUCCESS;
    alt_u32 timeout;
    alt_u32 bytes_read=0;
    alt_u32 bytes_written=0;
    alt_u32 temp_bytes_read;
    
    if (size==0)
    {
      return ALT_AVALON_I2C_SUCCESS;
    }
    
    /*if a new transaction, enable ip and clear int status*/
    if (!issue_restart) 
    {
       /*enable the ip.  The ip is disabled and enabled for each transaction.*/
      status = alt_avalon_i2c_enable(i2c_dev);
      if (status != ALT_AVALON_I2C_SUCCESS)
      {
        return status;
      }

      /*Clear the ISR reg*/
      alt_avalon_i2c_int_clear(i2c_dev,ALT_AVALON_I2C_ISR_ALL_CLEARABLE_INTS_MSK);
    }

    /*Start Write, transmit address. */
    status = alt_avalon_i2c_send_address(i2c_dev,ALT_AVALON_I2C_READ,issue_restart);

    if (status == ALT_AVALON_I2C_SUCCESS)
    {
        while ((bytes_written < (size-1)) && (status == ALT_AVALON_I2C_SUCCESS))
        {
            status = alt_avalon_i2c_cmd_write(i2c_dev, 0, ALT_AVALON_I2C_NO_RESTART, ALT_AVALON_I2C_NO_STOP);
            bytes_written++;
            if (status == ALT_AVALON_I2C_SUCCESS)
            {
               alt_avalon_i2c_rx_read_available(i2c_dev, buffer,0,&temp_bytes_read);
               buffer+=temp_bytes_read;
               bytes_read+=temp_bytes_read;
            }
        }

        /* Last byte */
        if (status == ALT_AVALON_I2C_SUCCESS)
        {
            status = alt_avalon_i2c_cmd_write(i2c_dev, 0, ALT_AVALON_I2C_NO_RESTART, issue_stop);
        }
    }
    
    while ((bytes_read < size) && (status==ALT_AVALON_I2C_SUCCESS)) 
    {
        status=alt_avalon_i2c_rx_read(i2c_dev, buffer);
        buffer++;
        bytes_read++;
    }

    /*if end of transaction, wait until the ip is idle then disable the ip*/
    if ((issue_stop) || (status != ALT_AVALON_I2C_SUCCESS)) 
    {
        timeout=10000 * size;
        while (alt_avalon_i2c_is_busy(i2c_dev))
        {
            if (timeout<10) alt_busy_sleep(10000);
            if (--timeout == 0)
            {
               status = ALT_AVALON_I2C_TIMEOUT;
               break;
            }
        }

        /*check for nack error*/
        alt_avalon_i2c_check_nack(i2c_dev,&status);    
        
        /*disable the ip.  The ip is disabled and enabled for each transaction.*/
        alt_avalon_i2c_disable(i2c_dev);
    }

    return status;
}

/*This function receives one or more data bytes transmitted from a slave in 
 * response to read requests issued from this master.  Uses the optional interrupt routine. */
ALT_AVALON_I2C_STATUS_CODE alt_avalon_i2c_master_receive_using_interrupts(ALT_AVALON_I2C_DEV_t *i2c_dev,
                                       alt_u8 * buffer,
                                       const alt_u32 size,
                                       const alt_u8 issue_restart,
                                       const alt_u8 issue_stop)
{
    ALT_AVALON_I2C_STATUS_CODE status = ALT_AVALON_I2C_SUCCESS;
    IRQ_DATA_t *irq_data = i2c_dev->callback_context;    
    alt_u32 timeout;
    alt_u32 bytes_written=0;
    
    if (size==0)
    {
      return ALT_AVALON_I2C_SUCCESS;
    }
    
    /*Is the optional interrupt handler registered??*/
    if (i2c_dev->callback != optional_irq_callback)
    {
       return ALT_AVALON_I2C_BAD_ARG;    
    }
    
    /*if a new transaction, enable ip and clear int status*/
    if (!issue_restart) 
    {
      /*enable the ip.  The ip is disabled and enabled for each transaction.*/
      status = alt_avalon_i2c_enable(i2c_dev);
      if (status != ALT_AVALON_I2C_SUCCESS)
      {
        return status;
      }

      /*Clear the ISR reg*/
      alt_avalon_i2c_int_clear(i2c_dev,ALT_AVALON_I2C_ISR_ALL_CLEARABLE_INTS_MSK);
      
    }

    /*Start Write, transmit address. */
    status = alt_avalon_i2c_send_address(i2c_dev,ALT_AVALON_I2C_READ,issue_restart);

    if (status == ALT_AVALON_I2C_SUCCESS)
    {
        while ((bytes_written < (size-1)) && (status == ALT_AVALON_I2C_SUCCESS))
        {
            status = alt_avalon_i2c_cmd_write(i2c_dev, 0, ALT_AVALON_I2C_NO_RESTART, ALT_AVALON_I2C_NO_STOP);
            bytes_written++;
        }

        /* Last byte */
        if (status == ALT_AVALON_I2C_SUCCESS)
        {
            status = alt_avalon_i2c_cmd_write(i2c_dev, 0, ALT_AVALON_I2C_NO_RESTART, issue_stop);
        }
    }
    
    /*if error, wait until the ip is idle then disable the ip*/
    if (status != ALT_AVALON_I2C_SUCCESS) 
    {
        timeout=10000 * size;
        while (alt_avalon_i2c_is_busy(i2c_dev))
        {
            if (timeout<10) alt_busy_sleep(10000);
            if (--timeout == 0)
            {
               status = ALT_AVALON_I2C_TIMEOUT;
               break;
            }
        }
     
        /*disable the ip.  The ip is disabled and enabled for each transaction.*/
        alt_avalon_i2c_disable(i2c_dev);
    }
    else
    {
       if (issue_stop)
       {
         /* clear ISR register content */
         alt_avalon_i2c_int_clear(i2c_dev,ALT_AVALON_I2C_ISR_ALL_CLEARABLE_INTS_MSK);
         /* set the cmd fifo threshold */
         alt_avalon_i2c_rx_fifo_threshold_set(i2c_dev,ALT_AVALON_I2C_RX_DATA_FIFO_1_ENTRY);
         /* set the interrupt transaction busy bit  2 = receive */
         irq_data->irq_busy=2;
         
         irq_data->buffer = buffer;
         irq_data->size = size;
         
         /* enable the RX_READY interrupt */
         alt_avalon_i2c_int_enable(i2c_dev,ALT_AVALON_I2C_ISER_RX_READY_EN_MSK);
       }
    }

    return status;
}

/* Returns the current I2C controller interrupt status conditions. */
void alt_avalon_i2c_int_status_get(ALT_AVALON_I2C_DEV_t *i2c_dev,
                                       alt_u32 *status)
{
    *status = IORD_ALT_AVALON_I2C_ISR(i2c_dev->i2c_base) & IORD_ALT_AVALON_I2C_ISER(i2c_dev->i2c_base);
}

/*Returns the I2C controller raw interrupt status conditions irrespective of
 * the interrupt status condition enablement state. */
void alt_avalon_i2c_int_raw_status_get(ALT_AVALON_I2C_DEV_t *i2c_dev,
                                           alt_u32 *status)
{
    *status = IORD_ALT_AVALON_I2C_ISR(i2c_dev->i2c_base);
}

/*Clears the specified I2C controller interrupt status conditions identified
 * in the mask. */
void alt_avalon_i2c_int_clear(ALT_AVALON_I2C_DEV_t *i2c_dev, const alt_u32 mask)
{
    IOWR_ALT_AVALON_I2C_ISR(i2c_dev->i2c_base,mask);
}

/*Disable the specified I2C controller interrupt status conditions identified in
 * the mask. */
void alt_avalon_i2c_int_disable(ALT_AVALON_I2C_DEV_t *i2c_dev, const alt_u32 mask)
{
   alt_u32 enabled_ints;
    
   alt_avalon_i2c_enabled_ints_get(i2c_dev,&enabled_ints);
   enabled_ints &=  (~mask);
   IOWR_ALT_AVALON_I2C_ISER(i2c_dev->i2c_base,ALT_AVALON_I2C_ISR_ALLINTS_MSK & enabled_ints);
}

/*Enable the specified I2C controller interrupt status conditions identified in
 * the mask. */
void alt_avalon_i2c_int_enable(ALT_AVALON_I2C_DEV_t *i2c_dev, const alt_u32 mask)
{
    alt_u32 enabled_ints;
    
    alt_avalon_i2c_enabled_ints_get(i2c_dev,&enabled_ints);
    enabled_ints |= mask;
    IOWR_ALT_AVALON_I2C_ISER(i2c_dev->i2c_base,ALT_AVALON_I2C_ISR_ALLINTS_MSK & enabled_ints);
}

/*gets the enabled i2c interrupts. */
void alt_avalon_i2c_enabled_ints_get(ALT_AVALON_I2C_DEV_t *i2c_dev, alt_u32 * enabled_ints)
{
    *enabled_ints=IORD_ALT_AVALON_I2C_ISER(i2c_dev->i2c_base) & ALT_AVALON_I2C_ISR_ALLINTS_MSK;
}

/*Gets the current receive FIFO threshold level value. */
void alt_avalon_i2c_rx_fifo_threshold_get(ALT_AVALON_I2C_DEV_t *i2c_dev,
                                              ALT_AVALON_I2C_RX_DATA_FIFO_THRESHOLD_t *threshold)
{
    *threshold = (IORD_ALT_AVALON_I2C_CTRL(i2c_dev->i2c_base) & ALT_AVALON_I2C_CTRL_RX_DATA_FIFO_THD_MSK) >>  ALT_AVALON_I2C_CTRL_RX_DATA_FIFO_THD_OFST;
}

/*sets the current receive FIFO threshold level value. */
void alt_avalon_i2c_rx_fifo_threshold_set(ALT_AVALON_I2C_DEV_t *i2c_dev,
                                              const ALT_AVALON_I2C_RX_DATA_FIFO_THRESHOLD_t threshold)
{
    IORMW_ALT_AVALON_I2C_CTRL(i2c_dev->i2c_base,threshold << ALT_AVALON_I2C_CTRL_RX_DATA_FIFO_THD_OFST,ALT_AVALON_I2C_CTRL_RX_DATA_FIFO_THD_MSK);
}

/*Gets the current Transfer Command FIFO threshold level value.*/
void alt_avalon_i2c_tfr_cmd_fifo_threshold_get(ALT_AVALON_I2C_DEV_t *i2c_dev,
                                              ALT_AVALON_I2C_TFR_CMD_FIFO_THRESHOLD_t *threshold)
{
    *threshold = (IORD_ALT_AVALON_I2C_CTRL(i2c_dev->i2c_base) & ALT_AVALON_I2C_CTRL_TFR_CMD_FIFO_THD_MSK) >> ALT_AVALON_I2C_CTRL_TFR_CMD_FIFO_THD_OFST;
}

/*Sets the current Transfer Command FIFO threshold level value.*/
void alt_avalon_i2c_tfr_cmd_fifo_threshold_set(ALT_AVALON_I2C_DEV_t *i2c_dev,
                                              const ALT_AVALON_I2C_TFR_CMD_FIFO_THRESHOLD_t threshold)
{
  IORMW_ALT_AVALON_I2C_CTRL(i2c_dev->i2c_base,threshold << ALT_AVALON_I2C_CTRL_TFR_CMD_FIFO_THD_OFST,ALT_AVALON_I2C_CTRL_TFR_CMD_FIFO_THD_MSK);
}
  



