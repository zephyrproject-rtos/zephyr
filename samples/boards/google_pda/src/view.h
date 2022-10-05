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

void view_set_snoop(enum view_snoop_t vs);
void view_set_connection(enum view_connection_t vc);
int get_view_connection();

#endif
