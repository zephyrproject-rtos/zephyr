#include <zephyr/drivers/can.h>
#include <Can_Transmit.h>
#include <stddef.h>

void tx_callback(const struct device *dev, int error, void *arg)
{
	// char *sender = (char *)arg;

	// ARG_UNUSED(dev);

	// if (error != 0) {
	// 	printf("Callback! error-code: %d\nSender: %s\n",
	// 	       error, sender);
	// }
}

bool Can_Transmit_BuildAndQueueMessage(const struct device * dev, uint32_t arbitration,
				      uint16_t length,
				      bool is_extended,
				      bool loopback,
				      const uint8_t *data)
{
	struct can_frame msg;

    msg.flags = is_extended ? CAN_FRAME_IDE : 0U;
	msg.id = arbitration;
	msg.dlc = length;
	// msg.TransmitLoopbackMessage = loopback;

    memcpy(msg.data, data, length > CAN_MAX_DLC ? CAN_MAX_DLC : length);

	can_send(dev, &msg, K_FOREVER, tx_callback, NULL);

	return true;
}

void Can_Transmit_SendQueues(void)
{
}
