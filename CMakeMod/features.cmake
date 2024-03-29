# detect available compiler features and libraries
include(CheckCXXSourceCompiles)
include(CheckPrototypeDefinition)
include(CheckCCompilerFlag)
include(CheckTypeSize)
include(CheckFunctionExists)

# check for SSE etc.
if(NOT MSVC)
  check_c_compiler_flag(-faltivec HAVE_ALTIVEC)
  check_c_compiler_flag(-mavx HAVE_AVX)
  check_c_compiler_flag(-mmmx HAVE_MMX)
  option(OSMSCOUT_ENABLE_SSE "Enable SSE support (not working on all platforms!)" OFF)
  if(OSMSCOUT_ENABLE_SSE)
    check_c_compiler_flag(-msse HAVE_SSE)
    check_c_compiler_flag(-msse2 HAVE_SSE2)
    check_c_compiler_flag(-msse3 HAVE_SSE3)
    check_c_compiler_flag(-msse4.1 HAVE_SSE4_1)
    check_c_compiler_flag(-msse4.2 HAVE_SSE4_2)
    check_c_compiler_flag(-mssse3 HAVE_SSSE3)
  endif()
  if(HAVE_SSE2)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse2")
  endif()
else()
  set(HAVE_ALTIVEC OFF)
  set(HAVE_AVX ON)
  set(HAVE_MMX ON)
  set(HAVE_SSE ON)
  set(HAVE_SSE2 ON)
  set(HAVE_SSE3 OFF)
  set(HAVE_SSE4_1 OFF)
  set(HAVE_SSE4_2 OFF)
  set(HAVE_SSSE3 OFF)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:SSE2")
endif()

## marisa visibility is broken, we have to keep default
#if(CMAKE_COMPILER_IS_GNUCXX)
#  check_cxx_compiler_flag(-fvisibility=hidden HAVE_VISIBILITY)
#  if(HAVE_VISIBILITY)
#    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
#  endif()
#else()
#  set(HAVE_VISIBILITY OFF)
#endif()

# check headers exists
include(CheckIncludeFileCXX)
check_include_file(dlfcn.h HAVE_DLFCN_H)
check_include_file(fcntl.h HAVE_FCNTL_H)
check_include_file(inttypes.h HAVE_INTTYPES_H)
check_include_file(memory.h HAVE_MEMORY_H)
check_include_file(stdint.h HAVE_STDINT_H)
check_include_file(stdlib.h HAVE_STDLIB_H)
check_include_file(strings.h HAVE_STRINGS_H)
check_include_file(string.h HAVE_STRING_H)
check_include_file(sys/stat.h HAVE_SYS_STAT_H)
check_include_file(sys/time.h HAVE_SYS_TIME_H)
check_include_file(sys/types.h HAVE_SYS_TYPES_H)
check_include_file(unistd.h HAVE_UNISTD_H)
check_include_file_cxx(codecvt HAVE_CODECVT)
if(${HAVE_STDINT_H} AND ${HAVE_STDLIB_H} AND ${HAVE_INTTYPES_H} AND ${HAVE_STRING_H} AND ${HAVE_MEMORY_H})
  set(STDC_HEADERS ON)
else()
  set(STDC_HEADERS OFF)
endif()

# check data types exists
set(CMAKE_EXTRA_INCLUDE_FILES inttypes.h)
check_type_size(int16_t HAVE_INT16_T)
check_type_size(int32_t HAVE_INT32_T)
check_type_size(int64_t HAVE_INT64_T)
check_type_size(int8_t HAVE_INT8_T)
check_type_size("long long" HAVE_LONG_LONG)
check_type_size(uint16_t HAVE_UINT16_T)
check_type_size(uint32_t HAVE_UINT32_T)
check_type_size(uint64_t HAVE_UINT64_T)
check_type_size(uint8_t HAVE_UINT8_T)
check_type_size("unsigned long long" HAVE_UNSIGNED_LONG_LONG)
set(CMAKE_EXTRA_INCLUDE_FILES wchar.h)
check_type_size(wchar_t SIZEOF_WCHAR_T)
set(CMAKE_EXTRA_INCLUDE_FILES)
check_cxx_source_compiles("
#include <string>
int main()
{
  std::wstring value=L\"Hello\";
}
" HAVE_STD__WSTRING)

# check functions exists
check_function_exists(fseeko HAVE_FSEEKO)
check_function_exists(mmap HAVE_MMAP)
check_function_exists(posix_fadvise HAVE_POSIX_FADVISE)
check_function_exists(posix_madvise HAVE_POSIX_MADVISE)
check_function_exists(mallinfo HAVE_MALLINFO)
check_function_exists(mallinfo2 HAVE_MALLINFO2)

# check libraries and tools

find_package(LibXml2)

if(NOT TARGET LibXml2::LibXml2)
  # libxml2 v2.9.8 doesn't define cmake target
  add_library(LibXml2::LibXml2 SHARED IMPORTED)
  set_target_properties(LibXml2::LibXml2 PROPERTIES
          IMPORTED_LOCATION ${LIBXML2_LIBRARY}
          INTERFACE_INCLUDE_DIRECTORIES ${LIBXML2_INCLUDE_DIR}
  )
endif()

# prepare cmake variables for configuration files
set(OSMSCOUT_HAVE_INT16_T ${HAVE_INT16_T})
set(OSMSCOUT_HAVE_INT32_T ${HAVE_INT32_T})
set(OSMSCOUT_HAVE_INT64_T ${HAVE_INT64_T})
set(OSMSCOUT_HAVE_INT8_T ${HAVE_INT8_T})
set(OSMSCOUT_HAVE_SSE2 ${HAVE_SSE2})
set(OSMSCOUT_GPX_HAVE_LIB_XML ${LIBXML2_FOUND})

find_package(Threads)
if(THREADS_HAVE_PTHREAD_ARG)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${THREADS_PTHREAD_ARG}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${THREADS_PTHREAD_ARG}")
endif()

if (CMAKE_USE_PTHREADS_INIT)
  set(OSMSCOUT_PTHREAD TRUE)
endif()

try_compile(PTHREAD_NAME_OK "${PROJECT_BINARY_DIR}"
        "${PROJECT_SOURCE_DIR}/dependencies/libosmscout/cmake/TestPThreadName.cpp"
        LINK_LIBRARIES pthread
        OUTPUT_VARIABLE PTHREAD_NAME_OUT)
if(PTHREAD_NAME_OK)
  set(OSMSCOUT_PTHREAD_NAME TRUE)
else()
  message(STATUS "TestPThreadName.cpp cannot be compiled: ${PTHREAD_NAME_OUT}")
endif()

find_package(TBB QUIET)
if (TBB_FOUND)
  try_compile(TBB_HAS_SCHEDULER_INIT "${PROJECT_BINARY_DIR}"
          "${PROJECT_SOURCE_DIR}/dependencies/libosmscout/cmake/TestTBBSchedulerInit.cpp"
          LINK_LIBRARIES TBB::tbb)
endif()
set(HAVE_STD_EXECUTION ${TBB_FOUND})

function(create_private_config output name)
  string(REPLACE "-" "_" OSMSCOUT_PRIVATE_CONFIG_HEADER_NAME ${name})
  string(TOUPPER ${OSMSCOUT_PRIVATE_CONFIG_HEADER_NAME} OSMSCOUT_PRIVATE_CONFIG_HEADER_NAME)
  configure_file("${OSMSCOUT_BASE_DIR_SOURCE}/cmake/Config.h.cmake" ${output})
endfunction()
