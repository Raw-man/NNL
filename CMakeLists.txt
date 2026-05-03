cmake_minimum_required(VERSION 3.20)

project(NNL LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

if(NOT GIT_COMMIT_HASH)
    set(GIT_COMMIT_HASH "")
endif()

option(NNL_ENABLE_LOGGING "Enable runtime diagnostics (warnings and errors)" OFF)

option(NNL_THROW_ON_CONTRACT_VIOLATION "Throw an exception on contract violation instead of terminating" OFF)

option(NNL_BUILD_TESTS "Build unit tests" ${PROJECT_IS_TOP_LEVEL})

message(STATUS "NNL: commit ${GIT_COMMIT_HASH}")

if(NOT CMAKE_CXX_BYTE_ORDER STREQUAL "LITTLE_ENDIAN")
    message(FATAL_ERROR "Little Endian required. Detected: ${CMAKE_CXX_BYTE_ORDER}")
endif()

include(FetchContent)

set(_NNL_ORIG_BUILD_SHARED ${BUILD_SHARED_LIBS})

set(BUILD_SHARED_LIBS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NNL_BUILD_TESTS)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

    set(INSTALL_GTEST OFF CACHE INTERNAL "")
    set(BUILD_GMOCK OFF CACHE INTERNAL "")

    FetchContent_Declare(
      googletest
      GIT_REPOSITORY https://github.com/google/googletest.git
      GIT_TAG v1.17.0
      DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )

    FetchContent_MakeAvailable(googletest)
    enable_testing()
    add_subdirectory(tests)
endif()

find_package(Doxygen)

if(Doxygen_FOUND)

    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in
                   ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)

    add_custom_target(doc
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM)
else()
    message(STATUS "NNL: Doxygen not found - 'doc' target will not be available")
endif()


FetchContent_Declare(
  tristripper
  GIT_REPOSITORY https://github.com/GPSnoopy/TriStripper.git
  GIT_TAG a591babacf2ec65a134d7a7a86e824ee98accad3
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE

  SOURCE_SUBDIR src
)

set(GLM_BUILD_LIBRARY OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG 1.0.3
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)


FetchContent_Declare(
  json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.12.0
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

set(TINYGLTF_BUILD_LOADER_EXAMPLE OFF CACHE INTERNAL "" FORCE)
set(TINYGLTF_HEADER_ONLY ON CACHE INTERNAL "" FORCE)
set(TINYGLTF_INSTALL OFF CACHE INTERNAL "" FORCE)
set(TINYGLTF_INSTALL_VENDOR OFF CACHE INTERNAL "" FORCE)

FetchContent_Declare(
  tinygltf
  GIT_REPOSITORY https://github.com/syoyo/tinygltf.git
  GIT_TAG v2.9.7
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

FetchContent_Declare(
  ordered-map
  GIT_REPOSITORY https://github.com/Tessil/ordered-map.git
  GIT_TAG v1.2.0
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

FetchContent_Declare(
  stb
  GIT_REPOSITORY https://github.com/nothings/stb.git
  GIT_TAG e6cd9561ea6dae43d41633797745789d142b691e
  # Prevents CMake from looking for a CMakeLists.txt that doesn't exist
  SOURCE_SUBDIR "OFF"
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

FetchContent_MakeAvailable(glm ordered-map stb tristripper json tinygltf)

set(BUILD_SHARED_LIBS ${_NNL_ORIG_BUILD_SHARED})

if(tristripper_POPULATED)
    target_include_directories(TriStripper
        PUBLIC "${tristripper_SOURCE_DIR}/include"
    )
endif()

add_library(GPSnoopy::TriStripper ALIAS TriStripper)
add_library(syoyo::tinygltf ALIAS tinygltf)

add_library(stb INTERFACE)
add_library(stb::stb ALIAS stb)


target_include_directories(stb SYSTEM INTERFACE ${stb_SOURCE_DIR})

configure_file("${PROJECT_SOURCE_DIR}/include/NNL/common/src_info.hpp.in"
               "${PROJECT_BINARY_DIR}/generated/NNL/common/src_info.hpp")

add_library(NNL STATIC)

add_library (NNL::NNL ALIAS NNL)

target_sources(NNL PRIVATE
    src/common/logger.cpp
    src/common/panic.cpp

    src/simple_asset/svalue.cpp
    src/simple_asset/saudio.cpp
    src/simple_asset/stexture.cpp
    src/simple_asset/smodel.cpp
    src/simple_asset/sanimation.cpp
    src/simple_asset/sasset3d.cpp
    src/simple_asset/lib_def.cpp
    src/simple_asset/gltf_imp.cpp
    src/simple_asset/gltf_exp.cpp

    src/game_asset/visual/model.cpp
    src/game_asset/visual/vertexde.cpp
    src/game_asset/visual/texture.cpp
    src/game_asset/visual/animation.cpp
    src/game_asset/visual/render.cpp
    src/game_asset/visual/lit.cpp
    src/game_asset/visual/fog.cpp
    src/game_asset/visual/text.cpp
    src/game_asset/visual/ui.cpp
    src/game_asset/visual/visanimation.cpp
    src/game_asset/visual/minimap.cpp

    src/game_asset/behavior/action.cpp
    src/game_asset/interaction/colbox.cpp
    src/game_asset/interaction/collision.cpp
    src/game_asset/interaction/shadow_collision.cpp

    src/game_asset/world/posd.cpp

    src/game_asset/container/dig.cpp
    src/game_asset/container/dig_entry.cpp
    src/game_asset/container/collection.cpp
    src/game_asset/container/asset.cpp
    src/game_asset/container/compression.cpp
    src/game_asset/container/md5list.cpp

    src/game_asset/audio/adpcm.cpp
    src/game_asset/audio/phd.cpp

    src/game_asset/format.cpp

    src/utility/utf8.cpp
    src/utility/filesys.cpp
    src/utility/string.cpp
    src/utility/data.cpp
    src/utility/math.cpp
)

target_sources(NNL PUBLIC
    include/NNL/nnl.hpp

    include/NNL/common/fixed_type.hpp
    include/NNL/common/enum_flag.hpp
    include/NNL/common/contract.hpp
    include/NNL/common/io.hpp
    include/NNL/common/logger.hpp
    include/NNL/common/panic.hpp
    include/NNL/common/exception.hpp

    include/NNL/simple_asset/svalue.hpp
    include/NNL/simple_asset/saudio.hpp
    include/NNL/simple_asset/stexture.hpp
    include/NNL/simple_asset/smodel.hpp
    include/NNL/simple_asset/sanimation.hpp
    include/NNL/simple_asset/sasset3d.hpp

    include/NNL/game_asset/visual/minimap.hpp
    include/NNL/game_asset/visual/model_common.hpp
    include/NNL/game_asset/visual/model.hpp
    include/NNL/game_asset/visual/vertexde.hpp
    include/NNL/game_asset/visual/texture.hpp
    include/NNL/game_asset/visual/animation.hpp
    include/NNL/game_asset/visual/render.hpp
    include/NNL/game_asset/visual/lit.hpp
    include/NNL/game_asset/visual/fog.hpp
    include/NNL/game_asset/visual/text.hpp
    include/NNL/game_asset/visual/ui.hpp
    include/NNL/game_asset/visual/visanimation.hpp

    include/NNL/game_asset/behavior/action_common.hpp
    include/NNL/game_asset/behavior/action.hpp

    include/NNL/game_asset/interaction/colbox.hpp
    include/NNL/game_asset/interaction/collision.hpp
    include/NNL/game_asset/interaction/shadow_collision.hpp

    include/NNL/game_asset/world/posd.hpp

    include/NNL/game_asset/container/dig.hpp
    include/NNL/game_asset/container/dig_entry.hpp
    include/NNL/game_asset/container/collection.hpp
    include/NNL/game_asset/container/asset.hpp

    include/NNL/game_asset/container/md5list.hpp

    include/NNL/game_asset/audio/adpcm.hpp
    include/NNL/game_asset/audio/phd.hpp

    include/NNL/game_asset/format.hpp

    include/NNL/utility/utf8.hpp
    include/NNL/utility/filesys.hpp
    include/NNL/utility/string.hpp
    include/NNL/utility/data.hpp
    include/NNL/utility/math.hpp
    include/NNL/utility/color.hpp
    include/NNL/utility/trait.hpp

    include/NNL/utility/static_set.hpp
    include/NNL/utility/static_vector.hpp
    include/NNL/utility/array3d.hpp
)

target_compile_definitions(NNL
    PUBLIC
        $<$<BOOL:${NNL_ENABLE_LOGGING}>:NNL_ENABLE_LOGGING>
        $<$<BOOL:${NNL_THROW_ON_CONTRACT_VIOLATION}>:NNL_THROW_ON_CONTRACT_VIOLATION>
    PRIVATE
        TINYGLTF_NO_INCLUDE_STB_IMAGE
        TINYGLTF_NO_INCLUDE_STB_IMAGE_WRITE
        TINYGLTF_NO_INCLUDE_JSON
)

target_compile_options(NNL
    PUBLIC
        $<$<CXX_COMPILER_ID:MSVC>:/Zc:preprocessor>
        $<$<CXX_COMPILER_ID:MSVC>:/wd5105>
)

target_include_directories(NNL
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/generated>
    PRIVATE
       ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(NNL
    PUBLIC
        tsl::ordered_map
        glm::glm
    PRIVATE
        nlohmann_json::nlohmann_json
        stb::stb
        syoyo::tinygltf
        GPSnoopy::TriStripper
   )



