cmake_minimum_required(VERSION 3.9)
# Keep the version below in sync with the one in db.h
project(kirisamedb VERSION 1.0 LANGUAGES C CXX)

# C standard can be overridden when this is used as a sub-project.
if(NOT CMAKE_C_STANDARD)
  # This project can use C11, but will gracefully decay down to C89.
  set(CMAKE_C_STANDARD 11)
  set(CMAKE_C_STANDARD_REQUIRED OFF)
  set(CMAKE_C_EXTENSIONS OFF)
endif(NOT CMAKE_C_STANDARD)

# C++ standard can be overridden when this is used as a sub-project.
if(NOT CMAKE_CXX_STANDARD)
  # This project requires C++11.
  set(CMAKE_CXX_STANDARD 11)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)
endif(NOT CMAKE_CXX_STANDARD)

if (WIN32)
  set(KIRISAMEDB_PLATFORM_NAME KIRISAMEDB_PLATFORM_WINDOWS)
  # TODO(cmumford): Make UNICODE configurable for Windows.
  add_definitions(-D_UNICODE -DUNICODE)
else (WIN32)
  set(KIRISAMEDB_PLATFORM_NAME KIRISAMEDB_PLATFORM_POSIX)
endif (WIN32)

option(KIRISAMEDB_BUILD_TESTS "Build KIRISAMEDB's unit tests" ON)
option(KIRISAMEDB_BUILD_BENCHMARKS "Build KIRISAMEDB's benchmarks" ON)
option(KIRISAMEDB_INSTALL "Install KIRISAMEDB's header and library" ON)

include(CheckIncludeFile)
check_include_file("unistd.h" HAVE_UNISTD_H)

include(CheckLibraryExists)
check_library_exists(crc32c crc32c_value "" HAVE_CRC32C)
check_library_exists(snappy snappy_compress "" HAVE_SNAPPY)
check_library_exists(zstd zstd_compress "" HAVE_ZSTD)
check_library_exists(tcmalloc malloc "" HAVE_TCMALLOC)

include(CheckCXXSymbolExists)
# Using check_cxx_symbol_exists() instead of check_c_symbol_exists() because
# we're including the header from C++, and feature detection should use the same
# compiler language that the project will use later. Principles aside, some
# versions of do not expose fdatasync() in <unistd.h> in standard C mode
# (-std=c11), but do expose the function in standard C++ mode (-std=c++11).
check_cxx_symbol_exists(fdatasync "unistd.h" HAVE_FDATASYNC)
check_cxx_symbol_exists(F_FULLFSYNC "fcntl.h" HAVE_FULLFSYNC)
check_cxx_symbol_exists(O_CLOEXEC "fcntl.h" HAVE_O_CLOEXEC)

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  # Disable C++ exceptions.
  string(REGEX REPLACE "/EH[a-z]+" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHs-c-")
  add_definitions(-D_HAS_EXCEPTIONS=0)

  # Disable RTTI.
  string(REGEX REPLACE "/GR" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /GR-")
else(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  # Enable strict prototype warnings for C code in clang and gcc.
  if(NOT CMAKE_C_FLAGS MATCHES "-Wstrict-prototypes")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wstrict-prototypes")
  endif(NOT CMAKE_C_FLAGS MATCHES "-Wstrict-prototypes")

  # Disable C++ exceptions.
  string(REGEX REPLACE "-fexceptions" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions")

  # Disable RTTI.
  string(REGEX REPLACE "-frtti" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
endif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")

# Test whether -Wthread-safety is available. See
# https://clang.llvm.org/docs/ThreadSafetyAnalysis.html
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag(-Wthread-safety HAVE_CLANG_THREAD_SAFETY)

# Used by googletest.
check_cxx_compiler_flag(-Wno-missing-field-initializers
                        KIRISAMEDB_HAVE_NO_MISSING_FIELD_INITIALIZERS)

include(CheckCXXSourceCompiles)

# Test whether C++17 __has_include is available.
check_cxx_source_compiles("
#if defined(__has_include) &&  __has_include(<string>)
#include <string>
#endif
int main() { std::string str; return 0; }
" HAVE_CXX17_HAS_INCLUDE)

set(KIRISAMEDB_PUBLIC_INCLUDE_DIR "include")
set(KIRISAMEDB_PORT_CONFIG_DIR "port")

configure_file(
  "port/port_config.h.in"
  "${PROJECT_BINARY_DIR}/${KIRISAMEDB_PORT_CONFIG_DIR}/port_config.h"
)

include_directories(
  "${PROJECT_BINARY_DIR}/include"
  "."
)

if(BUILD_SHARED_LIBS)
  # Only export KIRISAMEDB_EXPORT symbols from the shared library.
  add_compile_options(-fvisibility=hidden)
endif(BUILD_SHARED_LIBS)

# Must be included before CMAKE_INSTALL_INCLUDEDIR is used.
include(GNUInstallDirs)

add_library(kirisamedb "")
target_sources(kirisamedb
  PRIVATE
    "${PROJECT_BINARY_DIR}/${KIRISAMEDB_PORT_CONFIG_DIR}/port_config.h"
    "db/builder.cpp"
    "db/builder.h"
    "db/c.cpp"
    "db/db_impl.cpp"
    "db/db_impl.h"
    "db/db_iter.cpp"
    "db/db_iter.h"
    "db/dbformat.cpp"
    "db/dbformat.h"
    "db/dumpfile.cpp"
    "db/filename.cpp"
    "db/filename.h"
    "db/log_format.h"
    "db/log_reader.cpp"
    "db/log_reader.h"
    "db/log_writer.cpp"
    "db/log_writer.h"
    "db/memtable.cpp"
    "db/memtable.h"
    "db/repair.cpp"
    "db/skiplist.h"
    "db/snapshot.h"
    "db/table_cache.cpp"
    "db/table_cache.h"
    "db/version_edit.cpp"
    "db/version_edit.h"
    "db/version_set.cpp"
    "db/version_set.h"
    "db/write_batch_internal.h"
    "db/write_batch.cpp"
    "port/port_stdcxx.h"
    "port/port.h"
    "port/thread_annotations.h"
    "table/block_builder.cpp"
    "table/block_builder.h"
    "table/block.cpp"
    "table/block.h"
    "table/filter_block.cpp"
    "table/filter_block.h"
    "table/format.cpp"
    "table/format.h"
    "table/iterator_wrapper.h"
    "table/iterator.cpp"
    "table/merger.cpp"
    "table/merger.h"
    "table/table_builder.cpp"
    "table/table.cpp"
    "table/two_level_iterator.cpp"
    "table/two_level_iterator.h"
    "util/arena.cpp"
    "util/arena.h"
    "util/bloom.cpp"
    "util/cache.cpp"
    "util/coding.cpp"
    "util/coding.h"
    "util/comparator.cpp"
    "util/crc32c.cpp"
    "util/crc32c.h"
    "util/env.cpp"
    "util/filter_policy.cpp"
    "util/hash.cpp"
    "util/hash.h"
    "util/logging.cpp"
    "util/logging.h"
    "util/mutexlock.h"
    "util/no_destructor.h"
    "util/options.cpp"
    "util/random.h"
    "util/status.cpp"

  # Only CMake 3.3+ supports PUBLIC sources in targets exported by "install".
  $<$<VERSION_GREATER:CMAKE_VERSION,3.2>:PUBLIC>
    "${KIRISAMEDB_PUBLIC_INCLUDE_DIR}/c.h"
    "${KIRISAMEDB_PUBLIC_INCLUDE_DIR}/cache.h"
    "${KIRISAMEDB_PUBLIC_INCLUDE_DIR}/comparator.h"
    "${KIRISAMEDB_PUBLIC_INCLUDE_DIR}/db.h"
    "${KIRISAMEDB_PUBLIC_INCLUDE_DIR}/dumpfile.h"
    "${KIRISAMEDB_PUBLIC_INCLUDE_DIR}/env.h"
    "${KIRISAMEDB_PUBLIC_INCLUDE_DIR}/export.h"
    "${KIRISAMEDB_PUBLIC_INCLUDE_DIR}/filter_policy.h"
    "${KIRISAMEDB_PUBLIC_INCLUDE_DIR}/iterator.h"
    "${KIRISAMEDB_PUBLIC_INCLUDE_DIR}/options.h"
    "${KIRISAMEDB_PUBLIC_INCLUDE_DIR}/slice.h"
    "${KIRISAMEDB_PUBLIC_INCLUDE_DIR}/status.h"
    "${KIRISAMEDB_PUBLIC_INCLUDE_DIR}/table_builder.h"
    "${KIRISAMEDB_PUBLIC_INCLUDE_DIR}/table.h"
    "${KIRISAMEDB_PUBLIC_INCLUDE_DIR}/write_batch.h"
)

if (WIN32)
  target_sources(kirisamedb
    PRIVATE
      "util/env_windows.cpp"
      "util/windows_logger.h"
  )
else (WIN32)
  target_sources(kirisamedb
    PRIVATE
      "util/env_posix.cpp"
      "util/posix_logger.h"
  )
endif (WIN32)

# MemEnv is not part of the interface and could be pulled to a separate library.
target_sources(kirisamedb
  PRIVATE
    "helpers/memenv/memenv.cpp"
    "helpers/memenv/memenv.h"
)

target_include_directories(kirisamedb
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

set_target_properties(kirisamedb
  PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR})

target_compile_definitions(kirisamedb
  PRIVATE
    # Used by include/export.h when building shared libraries.
    KIRISAMEDB_COMPILE_LIBRARY
    # Used by port/port.h.
    ${KIRISAMEDB_PLATFORM_NAME}=1
)
if (NOT HAVE_CXX17_HAS_INCLUDE)
  target_compile_definitions(kirisamedb
    PRIVATE
      KIRISAMEDB_HAS_PORT_CONFIG_H=1
  )
endif(NOT HAVE_CXX17_HAS_INCLUDE)

if(BUILD_SHARED_LIBS)
  target_compile_definitions(kirisamedb
    PUBLIC
      # Used by include/export.h.
      KIRISAMEDB_SHARED_LIBRARY
  )
endif(BUILD_SHARED_LIBS)

if(HAVE_CLANG_THREAD_SAFETY)
  target_compile_options(kirisamedb
    PUBLIC
      -Werror -Wthread-safety)
endif(HAVE_CLANG_THREAD_SAFETY)

if(HAVE_CRC32C)
  target_link_libraries(kirisamedb crc32c)
endif(HAVE_CRC32C)
if(HAVE_SNAPPY)
  target_link_libraries(kirisamedb snappy)
endif(HAVE_SNAPPY)
if(HAVE_ZSTD)
  target_link_libraries(kirisamedb zstd)
endif(HAVE_ZSTD)
if(HAVE_TCMALLOC)
  target_link_libraries(kirisamedb tcmalloc)
endif(HAVE_TCMALLOC)

# Needed by port_stdcxx.h
find_package(Threads REQUIRED)
target_link_libraries(kirisamedb Threads::Threads)

add_executable(kirisamedbutil
  "db/kirisamedbutil.cpp"
)
target_link_libraries(kirisamedbutil kirisamedb)

if(KIRISAMEDB_BUILD_TESTS)
  enable_testing()

  # Prevent overriding the parent project's compiler/linker settings on Windows.
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  set(install_gtest OFF)
  set(install_gmock OFF)
  set(build_gmock ON)

  # This project is tested using GoogleTest.
  add_subdirectory("third_party/googletest")

  # GoogleTest triggers a missing field initializers warning.
  if(KIRISAMEDB_HAVE_NO_MISSING_FIELD_INITIALIZERS)
    set_property(TARGET gtest
        APPEND PROPERTY COMPILE_OPTIONS -Wno-missing-field-initializers)
    set_property(TARGET gmock
        APPEND PROPERTY COMPILE_OPTIONS -Wno-missing-field-initializers)
  endif(KIRISAMEDB_HAVE_NO_MISSING_FIELD_INITIALIZERS)

  add_executable(kirisamedb_tests "")
  target_sources(kirisamedb_tests
    PRIVATE
      # "db/fault_injection_test.cpp"
      # "issues/issue178_test.cpp"
      # "issues/issue200_test.cpp"
      # "issues/issue320_test.cpp"
      "${PROJECT_BINARY_DIR}/${KIRISAMEDB_PORT_CONFIG_DIR}/port_config.h"
      # "util/env_test.cpp"
      "util/status_test.cpp"
      "util/no_destructor_test.cpp"
      "util/testutil.cpp"
      "util/testutil.h"
  )
  if(NOT BUILD_SHARED_LIBS)
    target_sources(kirisamedb_tests
      PRIVATE
        "db/autocompact_test.cpp"
        "db/corruption_test.cpp"
        "db/db_test.cpp"
        "db/dbformat_test.cpp"
        "db/filename_test.cpp"
        "db/log_test.cpp"
        "db/recovery_test.cpp"
        "db/skiplist_test.cpp"
        "db/version_edit_test.cpp"
        "db/version_set_test.cpp"
        "db/write_batch_test.cpp"
        "helpers/memenv/memenv_test.cpp"
        "table/filter_block_test.cpp"
        "table/table_test.cpp"
        "util/arena_test.cpp"
        "util/bloom_test.cpp"
        "util/cache_test.cpp"
        "util/coding_test.cpp"
        "util/crc32c_test.cpp"
        "util/hash_test.cpp"
        "util/logging_test.cpp"
    )
  endif(NOT BUILD_SHARED_LIBS)
  target_link_libraries(kirisamedb_tests kirisamedb gmock gtest gtest_main)
  target_compile_definitions(kirisamedb_tests
    PRIVATE
      ${KIRISAMEDB_PLATFORM_NAME}=1
  )
  if (NOT HAVE_CXX17_HAS_INCLUDE)
    target_compile_definitions(kirisamedb_tests
      PRIVATE
        KIRISAMEDB_HAS_PORT_CONFIG_H=1
    )
  endif(NOT HAVE_CXX17_HAS_INCLUDE)

  add_test(NAME "kirisamedb_tests" COMMAND "kirisamedb_tests")

  function(kirisamedb_test test_file)
    get_filename_component(test_target_name "${test_file}" NAME_WE)

    add_executable("${test_target_name}" "")
    target_sources("${test_target_name}"
      PRIVATE
        "${PROJECT_BINARY_DIR}/${KIRISAMEDB_PORT_CONFIG_DIR}/port_config.h"
        "util/testutil.cpp"
        "util/testutil.h"

        "${test_file}"
    )
    target_link_libraries("${test_target_name}" kirisamedb gmock gtest)
    target_compile_definitions("${test_target_name}"
      PRIVATE
        ${KIRISAMEDB_PLATFORM_NAME}=1
    )
    if (NOT HAVE_CXX17_HAS_INCLUDE)
      target_compile_definitions("${test_target_name}"
        PRIVATE
          KIRISAMEDB_HAS_PORT_CONFIG_H=1
      )
    endif(NOT HAVE_CXX17_HAS_INCLUDE)

    add_test(NAME "${test_target_name}" COMMAND "${test_target_name}")
  endfunction(kirisamedb_test)

  kirisamedb_test("db/c_test.c")

  if(NOT BUILD_SHARED_LIBS)
    # TODO(costan): This test also uses
    #               "util/env_{posix|windows}_test_helper.h"
    if (WIN32)
      kirisamedb_test("util/env_windows_test.cpp")
    else (WIN32)
      kirisamedb_test("util/env_posix_test.cpp")
    endif (WIN32)
  endif(NOT BUILD_SHARED_LIBS)
endif(KIRISAMEDB_BUILD_TESTS)

if(KIRISAMEDB_BUILD_BENCHMARKS)
  # This project uses Google benchmark for benchmarking.
  set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
  set(BENCHMARK_ENABLE_EXCEPTIONS OFF CACHE BOOL "" FORCE)
  add_subdirectory("third_party/benchmark")

  function(kirisamedb_benchmark bench_file)
    get_filename_component(bench_target_name "${bench_file}" NAME_WE)

    add_executable("${bench_target_name}" "")
    target_sources("${bench_target_name}"
      PRIVATE
        "${PROJECT_BINARY_DIR}/${KIRISAMEDB_PORT_CONFIG_DIR}/port_config.h"
        "util/histogram.cpp"
        "util/histogram.h"
        "util/testutil.cpp"
        "util/testutil.h"

        "${bench_file}"
    )
    target_link_libraries("${bench_target_name}" kirisamedb gmock gtest benchmark)
    target_compile_definitions("${bench_target_name}"
      PRIVATE
        ${KIRISAMEDB_PLATFORM_NAME}=1
    )
    if (NOT HAVE_CXX17_HAS_INCLUDE)
      target_compile_definitions("${bench_target_name}"
        PRIVATE
          KIRISAMEDB_HAS_PORT_CONFIG_H=1
      )
    endif(NOT HAVE_CXX17_HAS_INCLUDE)
  endfunction(kirisamedb_benchmark)

  if(NOT BUILD_SHARED_LIBS)
    kirisamedb_benchmark("benchmarks/db_bench.cpp")
  endif(NOT BUILD_SHARED_LIBS)

  check_library_exists(sqlite3 sqlite3_open "" HAVE_SQLITE3)
  if(HAVE_SQLITE3)
    kirisamedb_benchmark("benchmarks/db_bench_sqlite3.cpp")
    target_link_libraries(db_bench_sqlite3 sqlite3)
  endif(HAVE_SQLITE3)

  # check_library_exists is insufficient here because the library names have
  # different manglings when compiled with clang or gcc, at least when installed
  # with Homebrew on Mac.
  set(OLD_CMAKE_REQURED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES})
  list(APPEND CMAKE_REQUIRED_LIBRARIES kyotocabinet)
  check_cxx_source_compiles("
#include <kcpolydb.h>

int main() {
  kyotocabinet::TreeDB* db = new kyotocabinet::TreeDB();
  delete db;
  return 0;
}
  "  HAVE_KYOTOCABINET)
  set(CMAKE_REQUIRED_LIBRARIES ${OLD_CMAKE_REQURED_LIBRARIES})
  if(HAVE_KYOTOCABINET)
    kirisamedb_benchmark("benchmarks/db_bench_tree_db.cpp")
    target_link_libraries(db_bench_tree_db kyotocabinet)
  endif(HAVE_KYOTOCABINET)
endif(KIRISAMEDB_BUILD_BENCHMARKS)

if(KIRISAMEDB_INSTALL)
  install(TARGETS kirisamedb
    EXPORT kirisamedbTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )
  install(
    FILES
      "${KIRISAMEDB_PUBLIC_INCLUDE_DIR}/c.h"
      "${KIRISAMEDB_PUBLIC_INCLUDE_DIR}/cache.h"
      "${KIRISAMEDB_PUBLIC_INCLUDE_DIR}/comparator.h"
      "${KIRISAMEDB_PUBLIC_INCLUDE_DIR}/db.h"
      "${KIRISAMEDB_PUBLIC_INCLUDE_DIR}/dumpfile.h"
      "${KIRISAMEDB_PUBLIC_INCLUDE_DIR}/env.h"
      "${KIRISAMEDB_PUBLIC_INCLUDE_DIR}/export.h"
      "${KIRISAMEDB_PUBLIC_INCLUDE_DIR}/filter_policy.h"
      "${KIRISAMEDB_PUBLIC_INCLUDE_DIR}/iterator.h"
      "${KIRISAMEDB_PUBLIC_INCLUDE_DIR}/options.h"
      "${KIRISAMEDB_PUBLIC_INCLUDE_DIR}/slice.h"
      "${KIRISAMEDB_PUBLIC_INCLUDE_DIR}/status.h"
      "${KIRISAMEDB_PUBLIC_INCLUDE_DIR}/table_builder.h"
      "${KIRISAMEDB_PUBLIC_INCLUDE_DIR}/table.h"
      "${KIRISAMEDB_PUBLIC_INCLUDE_DIR}/write_batch.h"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/kirisamedb"
  )

  include(CMakePackageConfigHelpers)
  configure_package_config_file(
    "cmake/${PROJECT_NAME}Config.cmake.in"
    "${PROJECT_BINARY_DIR}/cmake/${PROJECT_NAME}Config.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
  )
  write_basic_package_version_file(
    "${PROJECT_BINARY_DIR}/cmake/${PROJECT_NAME}ConfigVersion.cmake"
    COMPATIBILITY SameMajorVersion
  )
  install(
    EXPORT kirisamedbTargets
    NAMESPACE kirisamedb::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
  )
  install(
    FILES
      "${PROJECT_BINARY_DIR}/cmake/${PROJECT_NAME}Config.cmake"
      "${PROJECT_BINARY_DIR}/cmake/${PROJECT_NAME}ConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
  )
endif(KIRISAMEDB_INSTALL)
