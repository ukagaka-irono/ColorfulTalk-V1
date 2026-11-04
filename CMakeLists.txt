cmake_minimum_required(VERSION 3.20)
project(ColorfulTalkV1 VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(MSVC)
  add_compile_options(/utf-8 /W3)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
else()
  add_compile_options(-Wall -Wextra)
endif()

# 組み込む声: F1 F2 F3 M1 M2 C1 E1 W1 R1 K1
set(CT1_VOICE "F1" CACHE STRING "Voice built into the V1 library")
set(_ct1_voices F1 F2 F3 M1 M2 C1 E1 W1 R1 K1)
list(FIND _ct1_voices "${CT1_VOICE}" CT1_VOICE_ID)
if(CT1_VOICE_ID LESS 0)
  message(FATAL_ERROR "CT1_VOICE must be one of: ${_ct1_voices}")
endif()

# ---- 共通（カナ記号列のパーサ）と固定小数点コア --------------------------------
add_library(ct_common STATIC common/src/ct_koe.c common/src/ct_wav.c)
target_include_directories(ct_common PUBLIC common/include)

set(FX_SOURCES fx/src/fx_api.c fx/src/fx_math.c fx/src/fx_tables.c fx/src/fx_phoneme.c
               fx/src/fx_prosody.c fx/src/fx_frame.c fx/src/fx_synth.c)
add_library(ct_fx STATIC ${FX_SOURCES})
target_include_directories(ct_fx PUBLIC fx/include PRIVATE fx/src)
target_link_libraries(ct_fx PUBLIC ct_common)

# ---- V1 ------------------------------------------------------------------
set(CT1_SOURCES src/ct1_api.c src/ct1_voices.c)

add_library(colorful_talk_v1 STATIC ${CT1_SOURCES})
target_include_directories(colorful_talk_v1 PUBLIC include PRIVATE src)
target_compile_definitions(colorful_talk_v1 PRIVATE CT1_VOICE_ID=${CT1_VOICE_ID})
target_link_libraries(colorful_talk_v1 PUBLIC ct_fx)
set_target_properties(colorful_talk_v1 PROPERTIES OUTPUT_NAME colorful_talk_v1_static)

add_library(colorful_talk_v1_dll SHARED ${CT1_SOURCES} ${FX_SOURCES} common/src/ct_koe.c common/src/ct_wav.c)
target_include_directories(colorful_talk_v1_dll PUBLIC include PRIVATE src fx/include fx/src common/include)
target_compile_definitions(colorful_talk_v1_dll PRIVATE CT_BUILD_DLL CT1_VOICE_ID=${CT1_VOICE_ID})
set_target_properties(colorful_talk_v1_dll PROPERTIES OUTPUT_NAME colorful_talk_v1 C_VISIBILITY_PRESET hidden)

# 開発・テスト用: 10 種の声をすべて含む
add_library(colorful_talk_v1_all STATIC ${CT1_SOURCES})
target_include_directories(colorful_talk_v1_all PUBLIC include src)
target_compile_definitions(colorful_talk_v1_all PUBLIC CT1_VOICE_ALL=1 PRIVATE CT1_VOICE_ID=0)
target_link_libraries(colorful_talk_v1_all PUBLIC ct_fx)

# ---- ツール ----------------------------------------------------------------
add_executable(ctalk tools/ctalk.c)
target_link_libraries(ctalk PRIVATE colorful_talk_v1_all)
if(WIN32)
  target_link_libraries(ctalk PRIVATE winmm)
endif()

# 表と声の生成（開発時のみ）: cmake --build build --target fxgen_run
add_executable(fxgen EXCLUDE_FROM_ALL tools/fxgen.c)
if(NOT WIN32)
  target_link_libraries(fxgen PRIVATE m)
endif()
add_custom_target(fxgen_run
  COMMAND fxgen ${CMAKE_SOURCE_DIR}/fx/src/fx_tables.c ${CMAKE_SOURCE_DIR}/src/ct1_voices.c
  DEPENDS fxgen
  COMMENT "Regenerating fx_tables.c and ct1_voices.c")

# ---- テスト ----------------------------------------------------------------
enable_testing()
add_executable(test_koe tests/test_koe.c)
target_link_libraries(test_koe PRIVATE ct_common)
add_test(NAME koe COMMAND test_koe)

add_executable(test_v1 tests/test_v1.c tests/ct_test_audio.c)
target_include_directories(test_v1 PRIVATE tests fx/src)
target_link_libraries(test_v1 PRIVATE colorful_talk_v1_all)
add_test(NAME v1 COMMAND test_v1)
