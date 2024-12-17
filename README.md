# amDCT

Brought to you by Jim Conklin (C)2017-2019

Additional modernization stuff by Ferenc Pintér (C)2024

amDCT() is an adaptive video filter providing deblocking, sharpening, local range expansion, 
smoothing, and bright noise removal in a single filter that can be used on video of any level 
of quality. These four operations work synergistically to drastically reduce block artifacts 
while maintaining detail and increasing local contrast. 

Only 8 bit YUV(A) or greyscale formats are supported.

See documentation separately.

Build intructions: see below.

## Changelog
- (20241217) v1.4.2 (no release)

  - Source: remove mmx, threading to c++17 to make it more gcc/minGW friendly
  - gcc/minGW build possible
  - C only (zero Intel Intrinsics dependency) is possible by not defining "INTEL_INTRINSICS"
  - Add CMake build environment

- (20241210) v1.4.2
  - Rewrite all external assembly codes (fdct, idct, h263 and mpeg quant-dequants) to Intel intrinsics.
    It's now quicker - sometimes significantly - than the original.
  - Source: changed Windows specific threading code into C 17 version.
  - Source cleanup: removed lots of never used test codes from the source, rewrite some others. Move to cpp.
  - Add ability to pass Avisynth+ frame properties
  - Add support for any 8 bit planar YUV(A) or Y format (was: YV12 only)
  - Copy A alpha plane as well, if exists. (The filter works only on luma channel, other planes are simply copied)
  - Fix: add meaningful error message (Issue #2) for clips with non-mod8 width or height dimensions (was: out of memory)
  - Add Clang-cl LLVM build option, make source Clang friendly
  - Speedup examples:
  
| qtype | 32 bit clangcl | 32 bit msvc | 32 bit old 1.3 | 64 bit clangcl |
|-------|----------------|-------------|----------------|----------------|
| 1     | 6.12 fps       | 5.49 fps    | 5.40 fps       | 6.56 fps       |
| 2     | 6.67 fps       | 5.93 fps    | 5.28 fps       | 7.08 fps       |
| 3     | 4.09 fps       | 3.61 fps    | 3.22 fps       | 4.29 fps       |
| 4     | 6.66 fps       | 5.98 fps    | 5.29 fps       | 7.16 fps       |

- (20241128) (technical update)

  - arrange source to GitHub
  - fixed sharpTAmt typo in the wrapper avs script
  - project is now public on GitHub

- (20241127) v1.4.1, (Ferenc Pintér)

  - project moved to github: https://github.com/pinterf/amDCT
    (private until cleaned up)
  - Built using Visual Studio 2022
  - Changed to AVS 2.6 plugin interface
  - 64 bit build for Avisynth+
  - Added version resource to DLL
  - Drop all inline MMX assembly, convert them to SIMD intrinsics, sometimes based on C code, 
    Since some rewritten parts require SSE4.1 this is the minimum processor requirement.
  - external assembly (fdct, idct, h263 and mpeg quant-dequants) temporarily removed
  - source changes: some efforts made for future C-only (non-Intel) and Windows independent build, cleanup, etc.

- (20190705) v1.3_Testing (Jim Conklin)
  fixes freeing the same memory twice.

- (20190611) v1.2_Testing (Jim Conklin)
  Fixed the "Horrible anomalies", artifacts disappear when removing 'brightStart=205'

- (20190314) v1.1_Testing (Jim Conklin)

- (20170318) v1.0 released (Jim Conklin)

Future todo:

  - Single DLL, optimizations for different CPU instruction sets are chosen automatically.
  - Further code cleanup, remove redundant parameter checks, organize program structure to 
    multi Avisynth-Vaporsynth friendly
  - Reports MT Mode for Avisynth+: MT_NICE_FILTER or MT_???


## Build instructions

Build instructions
==================
VS2022: 

  preferably use the ready-made .sln solution file from Visual Studio IDE
  
  or see below other command line possibilities

Note: for Windows builds, first make a build folder from project root in order to not overwrite my
'official' .sln and vcxproj files.

Windows GCC (mingw installed by msys2):
  from the 'build' folder under project root:

  del .\CMakeCache.txt
  del ..\CMakeCache.txt
  cmake .. -G "MinGW Makefiles" -DENABLE_INTEL_SIMD:bool=on
  @rem test: cmake .. -G "MinGW Makefiles" -DENABLE_INTEL_SIMD:bool=off
  cmake --build . --config Release --clean-first

Windows Intel C++ Compiler 2025:

  del ..\CMakeCache.txt
  del .\CMakeCache.txt
  C:\Program Files (x86)\Intel\oneAPI\setvars.bat
  cmake ../ -T "Intel C++ Compiler 2025" -DCMAKE_CXX_COMPILER="icx.exe"
  cmake --build . --config Release --clean-first

Windows MSVC v141_xp toolset:

  del ..\CMakeCache.txt
  del .\CMakeCache.txt
  C:\Program Files (x86)\Intel\oneAPI\setvars.bat
  cmake .. -G "Visual Studio 17 2022" -A x64 -T "v141_xp" -DWINXP_SUPPORT:bool=on -DENABLE_INTEL_SIMD:bool=ON
  cmake --build . --config Release --clean-first

Windows MSVC (C only - no Intel SSE2 codes - test):

  del ..\CMakeCache.txt
  del .\CMakeCache.txt
  C:\Program Files (x86)\Intel\oneAPI\setvars.bat
  cmake .. -G "Visual Studio 17 2022" -A x64 -DENABLE_INTEL_SIMD:bool=off
  cmake --build . --config Release --clean-first

Linux
  from the 'build' folder under project root:
  ENABLE_INTEL_SIMD is automatically off for non x86 arhitectures

* Clone repo and build
    
        git clone https://github.com/pinterf/amDCT
        cd amDCT
        cmake -B build -S .
        cmake --build build

  Useful hints:        
   build after clean:

        cmake --build build --clean-first

   Force no asm support

        cmake -B build -S . -DENABLE_INTEL_SIMD:bool=off

   delete cmake cache

        rm build/CMakeCache.txt

* Find binaries at
    
        build/amDCT/amDCT.so

* Install binaries

        cd build
        sudo make install


## Links

- Project: https://github.com/pinterf/amDCT
- Forum: https://forum.doom9.org/showthread.php?t=174433
- Additional info: http://avisynth.nl/index.php/External_filters
