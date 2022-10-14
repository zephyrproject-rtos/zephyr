#ifndef _VIEW_H_
#define _VIEW_H_

/**
 * @brief Set the cc line that the snooper is monitoring
 *
 * @param vs enumerator that describes which cc lines should be monitored
 */
void view_set_snoop(uint8_t vs);

/**
 * @brief Records the current cc line connection the Twinkie is detecting
 *
 * @param vs enumerator that describes the cc line that is currently connected
 */
void view_set_connection(uint8_t vc);

/**
 * @brief Returns the value of the cc line currently being snooped. 0 for neither, 3 for both
 *
 * @return 0 - 3 for the current view status
 */
uint8_t get_view_snoop();

#endif
