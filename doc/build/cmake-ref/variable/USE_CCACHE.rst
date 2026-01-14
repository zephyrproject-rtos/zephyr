USE_CCACHE
##########

This variable can be set to ``0`` to explicitly disable the use of ccache, even if it is installed
on the system. If set to any other value (or unset) and ccache is found by :cmake:module:`ccache`,
it will be used for compilation and linking.
