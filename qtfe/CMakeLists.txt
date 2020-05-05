cmake_minimum_required(VERSION 2.8.11)

set(ROOTDIR ${CMAKE_CURRENT_SOURCE_DIR}/..)
#set(CMAKE_PREFIX_PATH "/home/christoph/Qt/5.11.1/gcc_64/bin/qmake")

#add_subdirectory(${ROOTDIR}/qtbe qtbe)
#add_subdirectory(${ROOTDIR}/3rdparty/backward-cpp backward-cpp)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter -g")
include_directories(${ROOTDIR})

project(qtfe)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

if(CMAKE_SYSTEM STREQUAL "Windows")
	set(MXE_TARGET_ROOT "/opt/mxe/usr/i686-w64-mingw32.static")
else()
endif()

#find_package(Qt4 REQUIRED QtGui QtNetwork)
find_package(Qt5Xml)
find_package(Qt5Core)
find_package(Qt5Widgets)
find_package(Qt5Network)
#find_package(QScintilla REQUIRED)
#include_directories(${QSCINTILLA_INCLUDE_DIRS})

set(qtfe_SRCS
	src/spec.cpp src/key.cpp src/frontend.cpp src/frontend-dir.cpp src/xml.cpp src/editor.cpp src/text.cpp

	spec/spec-1.0.cpp
)

set(qtfe_UIS
)

set(qtfe_RESOURCES
)

set(qtfe_MOC_HEADERS
)

set(qtfe_SPEC_HEADERS
	${CMAKE_CURRENT_BINARY_DIR}/spec-1.0.gen.h
)

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/spec-1.0.gen.h DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/spec/spec-1.0.xml ${ROOTDIR}/util/specgen/specgen COMMAND ${ROOTDIR}/util/specgen/specgen ${CMAKE_CURRENT_SOURCE_DIR}/spec/spec-1.0.xml ${CMAKE_CURRENT_BINARY_DIR}/spec-1.0.gen.h)
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/spec-global.gen.h DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/spec-1.0.gen.h ${CMAKE_CURRENT_SOURCE_DIR}/spec/spec-1.0.h ${ROOTDIR}/util/globalgen/globalgen COMMAND ${ROOTDIR}/util/globalgen/globalgen ${CMAKE_CURRENT_BINARY_DIR}/spec-global.gen.h ${CMAKE_CURRENT_SOURCE_DIR}/spec/spec-1.0.h ${CMAKE_CURRENT_BINARY_DIR}/spec-1.0.gen.h)
add_custom_target(specs DEPENDS spec-1.0.gen.h spec-global.gen.h)
#add_dependencies(spec-1.0.h ${ROOTDIR}/util/specgen/specgen)

qt5_add_resources(qtfe_RES_SOURCES ${qtfe_RESOURCES})
#qt4_wrap_cpp(qtfe_MOC_SRCS ${qtfe_MOC_HEADERS})
#qt4_wrap_ui(qtfe_UI_HEADERS ${qtfe_UIS})

if(WIN32)
	include_directories(${MXE_TARGET_ROOT}/qt5/include/QtXml ${MXE_TARGET_ROOT}/qt5/include/QtWidgets ${MXE_TARGET_ROOT}/qt5/include/QtGui ${MXE_TARGET_ROOT}/qt5/include/QtNetwork ${MXE_TARGET_ROOT}/qt5/include/QtCore -isystem ${MXE_TARGET_ROOT}/qt5/mkspecs/default -isystem ${MXE_TARGET_ROOT}/qt5/include)
	add_library(qtfe ${qtfe_SRCS} ${qtfe_MOC_SRCS} ${qtfe_RES_SOURCES} ${qtfe_UI_HEADERS})
endif(WIN32)
if(UNIX)
	add_library(qtfe ${qtfe_SRCS} ${qtfe_MOC_SRCS} ${qtfe_SPEC_HEADERS} ${qtfe_RES_SOURCES} ${qtfe_UI_HEADERS})
	#	add_backward(qtfe)
	target_link_libraries(qtfe Qt5::Widgets Qt5::Network Qt5::Xml)
	#target_link_libraries(qtfe client common Qt4::QtGui Qt4::QtNetwork -lsodium -luv -lbtree)
endif(UNIX)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
add_dependencies(qtfe specs)

