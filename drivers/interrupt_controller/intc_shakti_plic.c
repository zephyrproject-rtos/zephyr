// #include "pwm_driver.h"
// #include "plic_driver.h"
// #include "platform.h"
// #include "log.h"
// #include "stddef.h"
// #include "gpio.h"
// #include "utils.h"

/*
   Global interrupt data maintenance structure
*/

// Zephyr-plic_shakti

#define DT_DRV_COMPAT shakti_plic0
#define PLIC_BASE_ADDRESS DT_INST_PROP(0, base)

//--------------------------------

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <soc.h>

#include <zephyr/sw_isr_table.h>
#include <zephyr/drivers/interrupt_controller/riscv_plic.h>
#include <zephyr/irq.h>

//Defines

#define PLIC_PRIORITY_OFFSET            0x0000UL
#define PLIC_PENDING_OFFSET             0x1000UL
#define PLIC_ENABLE_OFFSET              0x2000UL

//#if defined(SOS) 
#define PLIC_THRESHOLD_OFFSET           0x200000UL
#define PLIC_CLAIM_OFFSET               0x200004UL

#define PLIC_REG_OFFSET                 ((uint32_t)PLIC_BASE_ADDRESS + PLIC_THRESHOLD_OFFSET)

#define PLIC_MAX_INTERRUPT_SRC          58 // set this value to CONFIG_NUM_IRQS
#define PLIC_EN_SIZE                    ((uint32_t)(PLIC_MAX_INTERRUPT_SRC/32) * (sizeof(uint32_t))) 
#define PLIC_PRIORITY_SHIFT_PER_INT     2 // to calculate offset for priority reg

typedef struct plic_shakti_regs_t
{
    uint32_t priority_thershold;
    uint32_t claim_register;
    uint32_t interrupt_complete;

}plic_regs_t;

static int track_irq_num;
volatile int key=0;

// plic_fptr_t isr_table[PLIC_MAX_INTERRUPT_SRC];
// interrupt_data_t hart0_interrupt_matrix[PLIC_MAX_INTERRUPT_SRC];

/** @fn void interrupt_complete(uint32_t interrupt_id)
 * @brief write the int_id to complete register
 * @details Signals completion of interrupt. From s/w side the interrupt claim/complete register is written with the interrupt id.
 * @param uint32_t interrupt_id
 */
// inline static void interrupt_complete(uint32_t interrupt_id)
// {
// 	log_trace("\ninterrupt_complete entered\n");

// 	uint32_t *claim_addr =  (uint32_t *) (PLIC_BASE_ADDRESS +
// 					      PLIC_CLAIM_OFFSET);

// 	*claim_addr = interrupt_id;
// 	hart0_interrupt_matrix[interrupt_id].state = SERVICED;
// 	hart0_interrupt_matrix[interrupt_id].count++;

// 	log_debug("interrupt id %d, state changed to %d\n", interrupt_id,
// 		  hart0_interrupt_matrix[interrupt_id].state);

// 	log_debug("interrupt id = %x \n reset to default values state = %x \
// 		  \n priority = %x\n count = %x\n", \
// 		  hart0_interrupt_matrix[interrupt_id].id, \
// 		  hart0_interrupt_matrix[interrupt_id].state, \
// 		  hart0_interrupt_matrix[interrupt_id].priority, \
// 		  hart0_interrupt_matrix[interrupt_id].count);

// 	log_trace("interrupt_complete exited\n");
// }

/** @fn uint32_t interrupt_claim_request()
 * @brief know the id of the interrupt
 * @details read the interrupt claim register to know the interrupt id
 *           of the highest priority pending interrupt
 * @return uint32_t
 */
// inline static uint32_t interrupt_claim_request()
// {
// 	uint32_t *interrupt_claim_address = NULL;
// 	uint32_t interrupt_id;

// 	log_trace("\ninterrupt_claim_request entered\n");

// 	/*
// 	   return the interrupt id. This will be used to index into the plic isr table.
// 	   From the isr table, the exact isr will be called

// 	   refer https://gitlab.com/shaktiproject/uncore/devices/blob/master/plic/plic.bsv as on 26/8/2019
// 	 */

// 	interrupt_claim_address = (uint32_t *)(PLIC_BASE_ADDRESS +
// 					       PLIC_CLAIM_OFFSET);

// 	interrupt_id = *interrupt_claim_address;

// 	log_debug("interrupt id [%x] claimed  at address %x\n", interrupt_id,
// 		  interrupt_claim_address );

// 	log_trace("interrupt_claim_request exited\n");

// 	return interrupt_id;
// }

/** @fn void mach_plic_handler(uintptr_t int_id, uintptr_t epc)
 * @brief handle machine mode plic interrupts
 * @details find the int id that caused of interrupt, 
 *	    process it and complete the interrupt.
 * @param uintptr_t int_id
 * @param uintptr_t epc
 */
void plic_irq_handler( const void *arg)
{
	uint32_t  interrupt_id;

    volatile plic_regs_t *regs = (volatile plic_regs_t *) PLIC_REG_OFFSET;

    struct _isr_table_entry *int_handle_t;

    interrupt_id = regs->claim_register;

    track_irq_num = interrupt_id;

    if (track_irq_num == 0U || track_irq_num >= CONFIG_NUM_IRQS)
	    z_irq_spurious(NULL);

	// log_trace("\nmach_plic_handler entered\n");

	// interrupt_id = interrupt_claim_request();

	// log_debug("interrupt id claimed = %x\n", interrupt_id);

	// if (interrupt_id <= 0 || interrupt_id > PLIC_MAX_INTERRUPT_SRC)
	// {
	// 	log_fatal("Fatal error, interrupt id [%x] claimed is wrong\n", interrupt_id);
	// }

	/*
	   clear IP bit ?

	   After the highest-priority pending interrupt is claimed by a target and the corresponding
	   IP bit is cleared, other lower-priority pending interrupts might then become visible to
	   the target, and so the PLIC EIP bit might not be cleared after a claim

	   reference - risc v priv spec v1.10 section 7.10 Interrupt Claims
	 */

	/*change state to active*/
	// hart0_interrupt_matrix[interrupt_id].state = ACTIVE;

	// log_debug("interrupt id %d, state changed to %d\n",
	// 	  interrupt_id,hart0_interrupt_matrix[interrupt_id].state);

	/*call relevant interrupt service routine*/
    int_handle_t = (struct _isr_table_entry *)&_sw_isr_table[interrupt_id+31];
    int_handle_t->isr(int_handle_t->arg);

	// isr_table[interrupt_id](interrupt_id);

	//interrupt_complete(interrupt_id);

    regs->claim_register = track_irq_num;

	// log_debug("interrupt id %d complete \n", interrupt_id);

	// log_trace("\nmach_plic_handler exited\n");
}

/** @fn uint32_t isr_default(uint32_t interrupt_id) 
 * @brief default interrupt service routine
 * @details Default isr. Use it when you dont know what to do with interrupts
 * @param uint32_t interrupt_id
 * @return uint32_t
 */
static inline void isr_default(uint32_t interrupt_id)
{
	// log_trace("\nisr_default entered\n");

    printk("Entered isr_default\n");

// 	if( interrupt_id > 0 && interrupt_id < 7 )  //PWM Interrupts
// 	{
// 		/*
// 		   Assuming 6 pwm's are there
// 		 */
// /**#ifndef SOS
// 		if(pwm_check_continuous_mode((6-interrupt_id)) == 0)
// 		{
// 			set_pwm_control_register((6-interrupt_id),0x80);
// 		}
// #endif
// */
// 	}

	// log_info("interrupt [%d] serviced\n",interrupt_id);

	// log_trace("\nisr_default exited\n");
}

int riscv_plic_get_irq(void)
{
	return track_irq_num;
}

/** @fn void interrupt_enable(uint32_t interrupt_id)
 * @brief enable the interrupt
 * @details A single bit that enables an interrupt. The bit position corresponds to the interrupt id
 * @param uint32_t interrupt_id
 */
void plic_irq_enable(uint32_t interrupt_id)
{
	uint32_t *interrupt_enable_addr;
	uint32_t current_value = 0x00, new_value;

	// log_trace("\ninterrupt_enable entered \n");

	// log_info("interrupt_id = %x\n", interrupt_id);

	// log_debug("PLIC BASE ADDRESS = %x, PLIC ENABLE OFFSET = %x\n" \
	// 		,PLIC_BASE_ADDRESS, PLIC_ENABLE_OFFSET);
	interrupt_enable_addr = (uint32_t *) (PLIC_BASE_ADDRESS +
			PLIC_ENABLE_OFFSET +
			(((interrupt_id / 32)-1)*sizeof(uint32_t)));

	current_value = *interrupt_enable_addr;

	// log_info("interrupt_enable_addr = %x current_value = %x \n", \
	// 		interrupt_enable_addr, current_value);

	/*set the bit corresponding to the interrupt src*/
	new_value = current_value | (0x1 << ((interrupt_id % 32)+1)); //Changed thissssss

	key = irq_lock();
	*((uint32_t*)interrupt_enable_addr) = new_value;
	irq_unlock(key);

	// log_debug("value read: new_value = %x\n", new_value);

	// log_trace("\ninterrupt_enable exited \n");
}

/** @fn void interrupt_disable(uint32_t interrupt_id) 
 * @brief disable an interrupt
 * @details A single bit that enables an interrupt.
 *          The bit position corresponds to the interrupt id
 * @param uint32_t interrupt_id
 */
void plic_shakti_irq_disable(uint32_t interrupt_id)
{
	uint32_t *interrupt_disable_addr = 0;
	uint32_t current_value = 0x00, new_value;

	// log_trace("\ninterrupt_disable entered \n");

	// log_debug("interrupt_id = %x\n", interrupt_id);

	// log_debug("PLIC BASE ADDRESS = %x, PLIC ENABLE OFFSET = %x interrupt_id = %x\n",
	// 	  PLIC_BASE_ADDRESS, PLIC_ENABLE_OFFSET, interrupt_id);

	interrupt_disable_addr = (uint32_t *) (PLIC_BASE_ADDRESS +
					      PLIC_ENABLE_OFFSET +
					      (interrupt_id / 32)*sizeof(uint32_t));

	current_value = *interrupt_disable_addr;

	// log_debug("interrupt_disable_addr = %x current_value = %x \n",
	// 	  interrupt_disable_addr, current_value);   

	/*unset the bit corresponding to the interrupt src*/
	new_value = current_value & (~(0x1 << ((interrupt_id % 32) + 1)));

	key = irq_lock();
	*interrupt_disable_addr = new_value;
	irq_unlock(key);

	// hart0_interrupt_matrix[interrupt_id].state = INACTIVE;

	// log_debug("interrupt id %d, state changed to %d\n",
	// 	  interrupt_id,hart0_interrupt_matrix[interrupt_id].state);

	// log_trace("interrupt_disable exited\n");
}

/** @fn void set_interrupt_threshold(uint32_t priority_value)
 * @brief set priority threshold for all interrupts
 * @details set a threshold on interrrupt priority. Any interruptthat has lesser priority than the threshold is ignored.
 * @param uint32_t priority_value
 */
void plic_shakti_set_irq_threshold(uint32_t priority_value)
{
	// log_trace("\nset interrupt_threshold entered\n");

	uint32_t *interrupt_threshold_priority = NULL;

	interrupt_threshold_priority = (uint32_t *) (PLIC_BASE_ADDRESS +
						     PLIC_THRESHOLD_OFFSET);

	*interrupt_threshold_priority = priority_value;

	// log_info("plic threshold set to %d\n", *interrupt_threshold_priority);

	// log_trace("set interrupt_threshold exited\n");
}

/** @fn void set_interrupt_priority(uint32_t priority_value, uint32_t int_id)
 * @brief set priority for an interrupt source
 * @details set priority for each interrupt. This is a 4 byte field.
 * @param uint32_t priority_value
 * @param uint32_t int_id
 */
void plic_shakti_set_priority(uint32_t priority_value, uint32_t int_id)
{
	// log_trace("\n set interrupt priority entered %x\n", priority_value);

	uint32_t * interrupt_priority_address;

	/*
	   base address + priority offset + 4*interruptId
	 */

	interrupt_priority_address = (uint32_t *) (PLIC_BASE_ADDRESS +
						   PLIC_PRIORITY_OFFSET +
						   (int_id <<
						    PLIC_PRIORITY_SHIFT_PER_INT));

	// log_debug("interrupt_priority_address = %x\n", interrupt_priority_address);

	// log_debug("current data at interrupt_priority_address = %x\n", *interrupt_priority_address);

	*interrupt_priority_address = priority_value;

	// log_debug(" new data at interrupt_priority_address = %x\n", *interrupt_priority_address);

	// log_trace("set interrupt priority exited\n");
}

/** @fn void configure_interrupt_pin(uint32_t id)
 * @brief configure a gpio pin for each interrupt
 * @details enable the corresponding gpio pin for a interrupt as read.
 * @param uint32_t id
 */
// void configure_interrupt_pin( __attribute__((unused)) uint32_t id)
// {
// 	log_trace("\nconfigure interrupt pin entered\n");

// 	uint32_t read_data;

// 	/*
// 	   GPIO0 -> Int id 7
// 	   GPIO15 -> Int id 21
// 	   Refer platform.h for full memory map.
// 	 */

// 	read_data = read_word(GPIO_DIRECTION_CNTRL_REG);

// 	log_debug("GPIO DIRECTION REGISTER VALUE = %x\n", read_data);

// 	write_word(GPIO_DIRECTION_CNTRL_REG, (read_data) & \
// 		   (0xFFFFFFFF & ~(1 << (id))));

// 	log_debug("Data written to GPIO DIRECTION CTRL REG = %x\n", \
// 		  read_word(GPIO_DIRECTION_CNTRL_REG));

// 	log_trace("configure interrupt pin exited\n");
// }

/** @fn void plic_init
 * @brief intitializes the plic module
 * @details Intitializes the plic registers to default values.
 *          Sets up the plic meta data table. Assigns the plic
 *          handler to mcause_interrupt_table.,By default interrupts are disabled.
 */
void plic_shakti_init(const struct device *dev)
{
	// uint32_t int_id = 0;

    volatile uint32_t *interrupt_disable_addr = (uint32_t *) (PLIC_BASE_ADDRESS + PLIC_ENABLE_OFFSET);
    volatile uint32_t *interrupt_threshold_priority = (uint32_t *) (PLIC_BASE_ADDRESS + PLIC_THRESHOLD_OFFSET);    

    volatile plic_regs_t *regs = (volatile plic_regs_t *)PLIC_REG_OFFSET;
    for (int i = 0; i < PLIC_EN_SIZE; i++)
    {
        *interrupt_disable_addr = 0U;
        interrupt_disable_addr++;
    }

    // for (int j = 1; j <= PLIC_MAX_INTERRUPT_SRC; j++)
    // {
    //     interrupt_threshold_priority += (j << PLIC_PRIORITY_SHIFT_PER_INT)
    //     *interrupt_threshold_priority = 0U;
    // }

    regs->priority_thershold = 0U;

    	/* Setup IRQ handler for PLIC driver */
	IRQ_CONNECT(RISCV_MACHINE_EXT_IRQ,
		    0,
		    plic_irq_handler,
		    NULL,
		    0);

	/* Enable IRQ for PLIC driver */
	irq_enable(RISCV_MACHINE_EXT_IRQ);
    
}

/** @fn void configure_interrupt(uint32_t int_id)
 * @brief configure the interrupt pin and enable bit
 * @details Enables the interrupt and corresponding physical pin (id needed).
 *          This function needs to be part of interrupt trigger and handling flow
 * @warning Here it is assumed, to have a one to one mapping
 *          between interrupt enable bit and interrupt pin
 * @param uint32_t int_id
 */
// void configure_interrupt(uint32_t int_id)
// {
// 	log_trace("\nconfigure_interrupt entered \n");

// 	/*
// 	   Call only for GPIO pins
// 	 */
// //	if(int_id >0 && int_id <= 32)
// //	{
// //		configure_interrupt_pin(int_id);
// //	}

// 	interrupt_enable(int_id);

// 	log_trace("configure_interrupt exited \n");
// }

DEVICE_DT_INST_DEFINE(0, plic_shakti_init, NULL, NULL, NULL,
		      PRE_KERNEL_1, CONFIG_INTC_INIT_PRIORITY, NULL);