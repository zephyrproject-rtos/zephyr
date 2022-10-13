#ifndef _MODEL_H_
#define _MODEL_H_

#include <zephyr/kernel.h>

/* bit positions for the role_mask parameter for setting the role,
   active CC line, and pull resistors of the Twinkie */
#define PULL_RESISTOR_BITS 	(BIT(0) | BIT(1))
#define SINK_BIT 		BIT(2)
#define CC1_CHANNEL_BIT 	BIT(3)
#define CC2_CHANNEL_BIT 	BIT(4)

/**
 * @brief Initializes the snooper model
 *
 * @param dev pointer to the Twinkie device
 *
 * @return 0 on success
 */
int model_init(const struct device *dev);

/**
 * @brief Resets the snooper model
 */
void reset_snooper();

/**
 * @brief Sets the role as source or sink, and set the pull up resistor and active cc line if source
 *
 * @param role_mask a bit mask of the role to be set
 */
void set_role(uint8_t role_mask);

/**
 * @brief Starts or stops the snooper by setting the snoop status
 *
 * @param s true for start, false for stop
 */
void start_snooper(bool s);

/**
 * @brief Sets whether the Twinkie will continuously output data when no pd messages are received
 *
 * @param e true for continuous output, false for output on pd messages only
 */
void set_empty_print(bool e);

#endif
