include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

target_include_directories(app PRIVATE $ENV{ZEPHYR_BASE}/subsys/net/ip)
target_sources(app PRIVATE src/leds-demo.c)
target_sources_ifdef(CONFIG_NET_L2_IEEE802154 app PRIVATE src/ieee802154_settings.c)
