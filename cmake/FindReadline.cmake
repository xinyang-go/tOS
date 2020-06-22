find_path(Readline_INCLUDE_DIR readline/readline.h)
find_library(Readline_LIBRARY readline)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
        Readline DEFAULT_MSG Readline_INCLUDE_DIR Readline_LIBRARY
)
