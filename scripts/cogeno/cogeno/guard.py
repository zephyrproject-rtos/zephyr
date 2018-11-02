# Copyright (c) 2018 Bobby Noelte.
#
# SPDX-License-Identifier: Apache-2.0

class GuardMixin(object):
    __slots__ = []


    def outl_guard_config(self, property_name):
        is_config = self.config_property(property_name, 0)
        self.outl("#if {} // Guard({}) {}".format(
            is_config, is_config, property_name))

    def outl_unguard_config(self, property_name):
        is_config = self.config_property(property_name, 0)
        self.outl("#endif // Guard({}) {}".format(is_config, property_name))
