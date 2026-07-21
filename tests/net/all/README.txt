This test tries to enable all possible networking related config options
and build a sample application. The application is not supposed to be run
as typically the generated configuration is not usable.

The check_net_options.sh script can be used in Linux host to generate
a list of missing network related kconfig options from prj.conf file.

TODO:

* separate conflicting configuration options and create new test cases
  for them
