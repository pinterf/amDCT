CMAKE_MINIMUM_REQUIRED( VERSION 3.8.2 )

set(PluginName "amDCT")

if (NOT WIN32)
  string(TOLOWER "${PluginName}" PluginName)
endif()

set(ProjectName "${PluginName}")
project(${ProjectName} LANGUAGES CXX)

Include("Files.cmake")

add_library(${PluginName} SHARED ${amDCT_Sources})

set_target_properties(${PluginName} PROPERTIES "OUTPUT_NAME" "${PluginName}")
if (MINGW)
  set_target_properties(${PluginName} PROPERTIES PREFIX "")
  set_target_properties(${PluginName} PROPERTIES IMPORT_PREFIX "")
endif()

if(ENABLE_INTEL_SIMD)
#require sse4.1 instead of default sse2, if there are mixed parts in the source
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DINTEL_INTRINSICS -msse4.1")
endif()

# Automatically group source files according to directory structure
foreach(FILE ${amDCT_Sources}) 
  get_filename_component(PARENT_DIR "${FILE}" PATH)

  string(REGEX REPLACE "(\\./)" "" GROUP "${PARENT_DIR}")
  string(REPLACE "/" "\\" GROUP "${GROUP}")

  # group into "Source Files" and "Header Files"
  if ("${FILE}" MATCHES ".*\\.cpp")
    set(GROUP "Source Files\\${GROUP}")
  elseif("${FILE}" MATCHES ".*\\.h")
    set(GROUP "Header Files\\${GROUP}")
  endif()

  source_group("${GROUP}" FILES "${FILE}")
endforeach()


if (MSVC_IDE)
  IF(CLANG_IN_VS STREQUAL "1" OR
  IntelLLVM_IN_VS STREQUAL "1")
      # special SSSE3 option for source files with *_ssse3.cpp pattern
      file(GLOB_RECURSE SRCS_SSSE3 "*_ssse3.cpp")
      set_source_files_properties(${SRCS_SSSE3} PROPERTIES COMPILE_FLAGS " -mssse3 ")

      # special SSE4.1 option for source files with *_sse41.cpp pattern
      file(GLOB_RECURSE SRCS_SSE41 "*_sse41.cpp")
      set_source_files_properties(${SRCS_SSE41} PROPERTIES COMPILE_FLAGS " -msse4.1 ")

      # special AVX option for source files with *_avx.cpp pattern
      file(GLOB_RECURSE SRCS_AVX "*_avx.cpp")
      set_source_files_properties(${SRCS_AVX} PROPERTIES COMPILE_FLAGS " -mavx ")

      # special AVX2 option for source files with *_avx2.cpp pattern
      file(GLOB_RECURSE SRCS_AVX2 "*_avx2.cpp")
      set_source_files_properties(${SRCS_AVX2} PROPERTIES COMPILE_FLAGS " -mavx2 -mfma ")

      # special AVX512 option for source files with *_avx512.cpp pattern
      file(GLOB_RECURSE SRCS_AVX512 "*_avx512.cpp")
      set_source_files_properties(${SRCS_AVX512} PROPERTIES COMPILE_FLAGS " -mavx512f -mavx512bw ")
  ELSE()
      # special AVX option for source files with *_avx.cpp pattern
      file(GLOB_RECURSE SRCS_AVX "*_avx.cpp")
      set_source_files_properties(${SRCS_AVX} PROPERTIES COMPILE_FLAGS " /arch:AVX ")

      # special AVX2 option for source files with *_avx2.cpp pattern
      file(GLOB_RECURSE SRCS_AVX2 "*_avx2.cpp")
      set_source_files_properties(${SRCS_AVX2} PROPERTIES COMPILE_FLAGS " /arch:AVX2 ")

      # special AVX512 option for source files with *_avx512.cpp pattern
      file(GLOB_RECURSE SRCS_AVX512 "*_avx512.cpp")
      set_source_files_properties(${SRCS_AVX512} PROPERTIES COMPILE_FLAGS " /arch:AVX512 ")
  ENDIF()
else()
  # special SSSE3 option for source files with *_ssse3.cpp pattern
  file(GLOB_RECURSE SRCS_SSSE3 "*_ssse3.cpp")
  set_source_files_properties(${SRCS_SSSE3} PROPERTIES COMPILE_FLAGS " -mssse3 ")

  # special SSE4.1 option for source files with *_sse41.cpp pattern
  file(GLOB_RECURSE SRCS_SSE41 "*_sse41.cpp")
  set_source_files_properties(${SRCS_SSE41} PROPERTIES COMPILE_FLAGS " -msse4.1 ")

  # special AVX option for source files with *_avx.cpp pattern
  file(GLOB_RECURSE SRCS_AVX "*_avx.cpp")
  set_source_files_properties(${SRCS_AVX} PROPERTIES COMPILE_FLAGS " -mavx ")

  # special AVX2 option for source files with *_avx2.cpp pattern
  file(GLOB_RECURSE SRCS_AVX2 "*_avx2.cpp")
  set_source_files_properties(${SRCS_AVX2} PROPERTIES COMPILE_FLAGS " -mavx2 -mfma ")

  # special AVX512 option for source files with *_avx512.cpp pattern
  file(GLOB_RECURSE SRCS_AVX512 "*_avx512.cpp")
  set_source_files_properties(${SRCS_AVX512} PROPERTIES COMPILE_FLAGS " -mavx512f -mavx512bw ")
endif()


# Specify include directories
target_include_directories(${ProjectName} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
#if there is a dedicated include dir for avisynth.h, add its directory also
#target_include_directories(${ProjectName} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if (CMAKE_SYSTEM_NAME STREQUAL "Haiku")
    set(SYSLIB "root")
elseif(CMAKE_SYSTEM_NAME STREQUAL "OpenBSD")
    set(SYSLIB "pthread")
elseif(MSVC OR MINGW)
    set(SYSLIB "uuid" "winmm" "vfw32" "msacm32" "gdi32" "user32" "advapi32" "ole32" "imagehlp")
else()
    set(SYSLIB "pthread" "dl" "m")
endif()

if(MINGW)
    set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
    set(THREADS_PREFER_PTHREAD_FLAG TRUE)
    find_package(Threads REQUIRED)

    list(APPEND SYSLIB "pthread")
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    list(APPEND SYSLIB "stdc++")
    # stdc++fs was mainlined into stdc++ in GCC 9, but GCC 8 can build it too
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9)
        list(APPEND SYSLIB "stdc++fs")
    endif()
elseif(NOT MSVC AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    list(APPEND SYSLIB "c++")
endif()

target_link_libraries(${ProjectName} ${SYSLIB})

include(GNUInstallDirs)

INSTALL(TARGETS ${ProjectName}
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}/avisynth")
