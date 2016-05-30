#ifndef _MQTT_SHELL_H_
#define _MQTT_SHELL_H_

int shell_repeat_until_ok(int argc, char *argv[]);
int shell_repeat(int argc, char *argv[]);
int shell_connect(int argc, char *argv[]);
int shell_disconnect(int argc, char *argv[]);
int shell_subs(int argc, char *argv[]);
int shell_ping(int argc, char *argv[]);
int shell_pub(int argc, char *argv[]);
int shell_read(int argc, char *argv[]);

#endif
