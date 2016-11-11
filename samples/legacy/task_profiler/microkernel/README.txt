Quark SE dev board functionality can be optimized by adjusting the
serial baud rate and enabling the real time clock and using it for
timestamping.

This optimization can be added to the board using the configuration
file: prj_quark_se_c1000_devboard.conf

Override the default project configuration file like this:

make BOARD=quark_se_c1000_devboard CONF_FILE=prj_quark_se_c1000_devboard.conf
