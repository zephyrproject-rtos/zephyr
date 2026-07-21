set(c23id c2x gnu2x)
set(c17id c17 c18 gnu17 gnu18 "iso9899:2017" "iso9899:2018")
set(c11id c11 gnu11 "iso9899:2011")
set(c99id c99 gnu99 "iso9899:1999")
set(c90id c89 c90 gnu89 gnu90 "iso9899:1990" "iso9899:199409")

set(compile_features_list)

# For each id value above a compile_features_${idval} with a list of supported
# `c_std_XX` values are created for easy lookup.
# For example, the settings
# - `compile_feature_c99` will contain `c_std_90;c_std_99`
# - `compile_feature_iso9899:2011` will contain `c_std_90;c_std_99;c_std_11`
# that can then be used to set CMAKE_C_COMPILE_FEATURES accordingly.
foreach(standard 90 99 11 17 23)
  list(APPEND compile_features_list c_std_${standard})
  foreach(id ${c${standard}id})
    set(compile_features_${id} ${compile_features_list})
  endforeach()
endforeach()

set(compile_features_cpp98 cxx_std_98)
set(compile_features_cpp11 cxx_std_11 ${compile_features_cpp98})
set(compile_features_cpp14 cxx_std_14 ${compile_features_cpp11})
set(compile_features_cpp17 cxx_std_17 ${compile_features_cpp14})
set(compile_features_cpp20 cxx_std_20 ${compile_features_cpp17})
set(compile_features_cpp23 cxx_std_23 ${compile_features_cpp20})
