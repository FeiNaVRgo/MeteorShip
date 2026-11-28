cmake_minimum_required(VERSION 4.0.2)
project(MeteorShip LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# C++26 reflection currently needs GCC 16+ (on every platform).
# macOS: Apple's clang won't work, install GCC via Homebrew and configure with it,
#        e.g. -DCMAKE_CXX_COMPILER=g++-16
if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_VERSION VERSION_LESS 16)
    message(FATAL_ERROR "This project requires GCC 16+ for C++26 reflection, found ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
endif()

# ---- Dependencies ----
find_package(OpenCL REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(SFML COMPONENTS Network Graphics Window Audio System CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(boost_json CONFIG REQUIRED)
find_package(boost_signals2 CONFIG REQUIRED)
find_package(unofficial-nuklear CONFIG REQUIRED)
find_package(behaviortree_cpp CONFIG REQUIRED)
find_package(unofficial-lua CONFIG REQUIRED)
find_package(sol2 CONFIG REQUIRED)

# ---- Sources ----
file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp")
file(GLOB_RECURSE PROJECT_INCLUDE CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/src/*.h")

add_executable(${PROJECT_NAME}
        "${CMAKE_CURRENT_LIST_DIR}/main.cpp"
        ${PROJECT_INCLUDE}
        ${PROJECT_SOURCES}
)

target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_options(${PROJECT_NAME} PRIVATE -freflection)

target_compile_definitions(${PROJECT_NAME} PRIVATE
        # Match BehaviorTree.CPP's expected-lite ABI when its headers are used with C++26.
        nsel_CONFIG_SELECT_EXPECTED=1
)

target_link_libraries(${PROJECT_NAME} PRIVATE
        OpenCL::OpenCL
        glad::glad
        SFML::Network SFML::Graphics SFML::Window SFML::Audio SFML::System
        imgui::imgui
        Boost::json
        Boost::signals2
        nuklear
        BT::behaviortree_cpp
        unofficial::lua::lua
        sol2::sol2
)

# ---- Runtime libraries: make the exe runnable outside the IDE ----
if(WIN32)
    # 1) Copy every dependent DLL (SFML, Lua, Boost, ...) next to the exe.
    #    The IF() skips the command when there is nothing to copy.
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E
            $<IF:$<BOOL:$<TARGET_RUNTIME_DLLS:${PROJECT_NAME}>>,copy,true>
            $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}>
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
            COMMAND_EXPAND_LISTS
    )

    # 2) Copy the MinGW runtime DLLs. The vcpkg DLLs depend on them too, so they
    #    must stay shared (do NOT link the runtime statically with a dynamic triplet).
    get_filename_component(MINGW_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
    foreach(RUNTIME_DLL libstdc++-6.dll libgcc_s_seh-1.dll libwinpthread-1.dll)
        if(EXISTS "${MINGW_BIN_DIR}/${RUNTIME_DLL}")
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${MINGW_BIN_DIR}/${RUNTIME_DLL}"
                    $<TARGET_FILE_DIR:${PROJECT_NAME}>
            )
        endif()
    endforeach()
elseif(APPLE)
    # Look for bundled .dylib files next to the executable.
    set_target_properties(${PROJECT_NAME} PROPERTIES
            BUILD_RPATH "@executable_path"
            INSTALL_RPATH "@executable_path"
    )
else()
    # Linux: look for bundled .so files next to the executable.
    set_target_properties(${PROJECT_NAME} PROPERTIES
            BUILD_RPATH_USE_ORIGIN ON
            INSTALL_RPATH "$ORIGIN"
    )
endif()

# ---- Asset copying ----
add_custom_target(copy_assets
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/assets ${CMAKE_CURRENT_BINARY_DIR}/assets
)
add_custom_target(copy_scripts
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/scripts ${CMAKE_CURRENT_BINARY_DIR}/scripts
)
add_custom_target(copy_behavior
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/BTreeXML ${CMAKE_CURRENT_BINARY_DIR}/BTreeXML
)
add_dependencies(${PROJECT_NAME} copy_assets copy_scripts copy_behavior)