cmake_minimum_required(VERSION 3.19)

project(Game)

find_package(Vulkan REQUIRED)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_subdirectory("${PROJECT_SOURCE_DIR}/dependencies/glfw")

aux_source_directory(src SOURCE_FILES)
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_include_directories(Game PUBLIC "${PROJECT_BINARY_DIR}" "${PROJECT_SOURCE_DIR}/include" "${PROJECT_SOURCE_DIR}/dependencies/glfw/include" )
target_link_libraries( Game ${VULKAN_LIBRARIES} glfw)