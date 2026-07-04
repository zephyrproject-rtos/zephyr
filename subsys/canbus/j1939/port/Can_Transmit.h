#ifndef CAN_TRANSMIT_H_
#define CAN_TRANSMIT_H_

#include <zephyr/drivers/can.h>

bool Can_Transmit_BuildAndQueueMessage(const struct device * dev, uint32_t arbitration,
					      uint16_t length,
					      bool is_extended,
					      bool loopback,
					      const uint8_t *data);
void Can_Transmit_SendQueues(void);

#endif /* CAN_TRANSMIT_H_ */
