FILE(GLOB amDCT_Sources RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
  "*.c"
  "*.cpp"
  "*.hpp"
  "*.h"
  "avs/*.h"
  "quant/*.h"
  "quant/*.c"
  "quant/*.cpp"
  "dct/*.h"
  "dct/*.cpp"
)

# remove intel only files from the list when ENABLE_INTEL_SIMD is not defined
IF(ENABLE_INTEL_SIMD)
  # Intel intrinsics: all files remain
ELSE()
  set(exclude_patterns "_sse" "_sse2" "_ssse3" "_sse41" "_sse42" "_avx" "_avx2" "_avx512")
  set(filtered_sources "")

  # Check each source file against the patterns
  foreach(source_file ${amDCT_Sources})
    set(should_keep TRUE)
    foreach(pattern ${exclude_patterns})
        if(${source_file} MATCHES ".*${pattern}\\.(cpp|h)$")
            set(should_keep FALSE)
            break()
        endif()
    endforeach()
    
    if(should_keep)
        list(APPEND filtered_sources ${source_file})
    endif()
  endforeach()

  set(amDCT_Sources ${filtered_sources})

endif()

message("${amDCT_Sources}")

IF( MSVC_IDE )
    # Ninja, unfortunately, seems to have some issues with using rc.exe
    LIST(APPEND amDCT_Sources "amDCT.rc")
ENDIF()
