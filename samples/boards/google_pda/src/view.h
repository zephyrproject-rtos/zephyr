#ifndef _VIEW_H_
#define _VIEW_H_

enum view_snoop_t {
	VS_SNOOP0,
	VS_SNOOP1,
	VS_SNOOP2,
	VS_SNOOP3
};

enum view_connection_t {
	VS_NO_CONN,
	VS_CC1_CONN,
	VS_CC2_CONN
};

/**
 * @brief Set the cc line that the snooper is monitoring
 *
 * @param vs enumerator that describes which cc lines should be monitored
 */
void view_set_snoop(enum view_snoop_t vs);

/**
 * @brief Records the current cc line connection the Twinkie is detecting
 *
 * @param vs enumerator that describes the cc line that is currently connected
 */
void view_set_connection(enum view_connection_t vc);

/**
 * @brief Returns the value of the cc line currently being snooped. 0 for neither, 3 for both
 *
 * @return 0 - 3 for the current view status
 */
int get_view_snoop();

#endif
