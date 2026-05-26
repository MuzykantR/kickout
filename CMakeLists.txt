cmake_minimum_required(VERSION 3.20...4.1)
project(kickout)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SFML_DIR "${CMAKE_SOURCE_DIR}/libs/SFML-2.6.2/lib/cmake/SFML")

find_package(SFML 2.6 COMPONENTS graphics audio window system REQUIRED)

include(FetchContent)
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(kickout ${SOURCES})

target_include_directories(kickout PRIVATE include)

target_link_libraries(kickout PRIVATE
    nlohmann_json::nlohmann_json
    sfml-graphics
    sfml-audio
    sfml-window
    sfml-system
)

add_custom_command(TARGET kickout POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/assets"
    "$<TARGET_FILE_DIR:kickout>/assets"
)


add_custom_command(TARGET kickout POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/libs/SFML-2.6.2/bin"
    "$<TARGET_FILE_DIR:kickout>"
)