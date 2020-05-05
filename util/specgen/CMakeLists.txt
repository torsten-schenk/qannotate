cmake_minimum_required(VERSION 2.8.11)

set(ROOTDIR ../..)

add_subdirectory(${ROOTDIR}/common common)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter -std=c++14 -g")
include_directories(${ROOTDIR})

project(specgen)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
find_package(Qt5Core)
find_package(Qt5Xml)

set(specgen_SRCS
	src/main.cpp
)

set(specgen_RESOURCES
)

set(specgen_MOC_HEADERS
)

qt5_add_resources(specgen_RES_SOURCES ${specgen_RESOURCES})

add_executable(specgen ${specgen_SRCS} ${specgen_MOC_SRCS} ${BACKWARD_ENABLE})
target_link_libraries(specgen Qt5::Xml)
target_include_directories(specgen PRIVATE ${CMAKE_BINARY_DIR})
install(TARGETS specgen RUNTIME DESTINATION ${CMAKE_SOURCE_DIR})

