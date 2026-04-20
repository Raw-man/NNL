cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(This tests)


add_executable(${This}
tests.cpp
math_test.cpp
data_test.cpp

compression_test.cpp
model_test.cpp
minimap_test.cpp
vertexde_test.cpp
texture_test.cpp
animation_test.cpp
io_test.cpp
set_test.cpp
action_test.cpp
colbox_test.cpp
collision_test.cpp
shadowcollision_test.cpp
visanimation_test.cpp
posd_test.cpp
text_test.cpp
render_conf_test.cpp
lit_test.cpp
fog_test.cpp
container_test.cpp
format_test.cpp
adpcm_test.cpp

stexture_test.cpp
saudio_test.cpp
svalue_test.cpp
smodel_test.cpp
sasset3d_test.cpp
)


target_link_libraries(${This} PUBLIC gtest_main NNL::NNL)


add_test(NAME ${This} COMMAND ${This} WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} )

