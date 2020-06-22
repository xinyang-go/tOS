find_path(Backtrace_INCLUDE_DIR backtrace.h)
find_library(Backtrace_LIBRARY backtrace)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
        Backtrace DEFAULT_MSG Backtrace_INCLUDE_DIR Backtrace_LIBRARY
)
