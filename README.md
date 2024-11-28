# amDCT

Brought to you by Jim Conklin (C)2017-2019
Additional modernization stuff by Ferenc Pintér (C)2024

amDCT() is an adaptive video filter providing deblocking, sharpening, local range expansion, 
smoothing, and bright noise removal in a single filter that can be used on video of any level 
of quality. These four operations work synergistically to drastically reduce block artifacts 
while maintaining detail and increasing local contrast. 

Only YV12 (8 bit YUV 4:2:0) format is supported.

See documentation separately.

## Changelog
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
  - external assembly (dct) temporarily removed
  - source changes: some efforts made for future C-only (non-Intel) and Windows independent build, cleanup, etc.

- (20190705) v1.3_Testing (Jim Conklin)
  fixes freeing the same memory twice.

- (20190611) v1.2_Testing (Jim Conklin)
  Fixed the "Horrible anomalies", artifacts disappear when removing 'brightStart=205'

- (20190314) v1.1_Testing (Jim Conklin)

- (20170318) v1.0 released (Jim Conklin)

Future todo:
  - Single DLL, optimizations for different CPU instruction sets are chosen automatically.
  - Reports MT Mode for Avisynth+: MT_NICE_FILTER or MT_???
  - Support other compilers like MSVC
  - Linux, etc. support
  - CMake build environment

## Links
- Project: https://github.com/pinterf/amDCT
- Forum: https://forum.doom9.org/showthread.php?t=174433
- Additional info: http://avisynth.nl/index.php/External_filters