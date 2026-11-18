cmake_minimum_required(VERSION 4.0.2)
project(MeteorShip)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp")
file(GLOB_RECURSE PROJECT_INCLUDE CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/src/*.h")

add_executable(${PROJECT_NAME} "${CMAKE_CURRENT_LIST_DIR}/main.cpp" ${PROJECT_INCLUDE})

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(${PROJECT_NAME} PRIVATE -freflection)
else()
    message(FATAL_ERROR "This project requires GCC 16+ for C++26 reflection, found ${CMAKE_CXX_COMPILER_ID}")
endif()

target_sources(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
# Match BehaviorTree.CPP's expected-lite ABI when its headers are used with C++26.
target_compile_definitions(${PROJECT_NAME} PRIVATE nsel_CONFIG_SELECT_EXPECTED=1)

find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Python3::Python)

set(PYBIND11_FINDPYTHON ON)
find_package(pybind11 CONFIG REQUIRED)
find_package(OpenCL REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE pybind11::embed)
pybind11_add_module(CPPModule ${PROJECT_SOURCES})
target_compile_options(CPPModule PRIVATE -freflection)
target_include_directories(CPPModule PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(CPPModule PRIVATE OpenCL::OpenCL)
target_link_libraries(CPPModule PRIVATE
    Python3::Python
    glad::glad
    SFML::Network SFML::Graphics SFML::Window SFML::Audio SFML::System
    imgui::imgui
    ${Boost_LIBRARIES}
    nuklear
    BT::behaviortree_cpp
)
target_compile_definitions(CPPModule PRIVATE nsel_CONFIG_SELECT_EXPECTED=1)
target_link_libraries(${PROJECT_NAME} PRIVATE OpenCL::OpenCL)

find_package(glad CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE glad::glad)


find_package(SFML COMPONENTS Network Graphics Window Audio System CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE SFML::Network SFML::Graphics SFML::Window SFML::Audio SFML::System)

find_package(imgui CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE imgui::imgui)

find_package(Boost CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE ${Boost_LIBRARIES})

find_package(unofficial-nuklear CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE nuklear)

find_package(behaviortree_cpp CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE BT::behaviortree_cpp)

find_package(unofficial-lua CONFIG REQUIRED)
find_package(sol2 CONFIG REQUIRED)

target_compile_definitions(${PROJECT_NAME} PRIVATE SOL_USING_CXX_LUA=1)
target_link_libraries(${PROJECT_NAME} PRIVATE unofficial::lua::lua-cpp sol2::sol2)

add_custom_target(copy_assets
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/assets ${CMAKE_CURRENT_BINARY_DIR}/assets
)
add_custom_target(copy_scripts
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/scripts ${CMAKE_CURRENT_BINARY_DIR}/scripts
)
add_custom_target(copy_behavior
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/BTreeXML ${CMAKE_CURRENT_BINARY_DIR}/BTreeXML
)
add_dependencies(${PROJECT_NAME} copy_assets)
add_dependencies(${PROJECT_NAME} copy_scripts)
add_dependencies(${PROJECT_NAME} copy_behavior)