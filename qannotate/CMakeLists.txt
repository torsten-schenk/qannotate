cmake_minimum_required(VERSION 2.8.11)

set(MXEPREFIX /opt/mxe/usr/i686-w64-mingw32.static)
set(ROOTDIR ..)

#show stacktraces on exceptions
#add_definitions(-DDUMP_EXCEPTION_CTOR)

add_subdirectory(${ROOTDIR}/qtfe qtfe)
if(UNIX)
	#add_subdirectory(${ROOTDIR}/3rdparty/backward-cpp backward-cpp)
endif()

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake" "/usr/local/opt/qt5/lib/cmake")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter -std=c++14 -g")
include_directories(${ROOTDIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR}/qtfe)

project(QAnnotate)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
find_package(Qt5Core)
find_package(Qt5Widgets)
find_package(Qt5Network)
find_package(Qt5Svg)
find_package(Qt5Xml)

set(QAnnotate_SRCS
	../common/base32.c
	src/main.cpp
	src/mainwindow.cpp
	src/converter.cpp
	src/centralwidget.cpp
	src/editform.cpp
	src/editwidgets.cpp
	src/treedata.cpp
	src/searchform.cpp
	src/settings.cpp
	src/util.cpp
)

set(QAnnotate_UIS
        #	src/mainwindow.ui
)

set(QAnnotate_RESOURCES
	resources.qrc
)

set(QAnnotate_MOC_HEADERS
        #	src/mainwindow.h
)

add_custom_target(gitversion
	COMMAND ${CMAKE_COMMAND}
		-Dlocal_dir="${CMAKE_CURRENT_SOURCE_DIR}"
		-Doutput_dir="${CMAKE_CURRENT_BINARY_DIR}"
		-P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/gitversion.cmake"
)

qt5_add_resources(QAnnotate_RES_SOURCES ${QAnnotate_RESOURCES})

if(WIN32)
	set(GUI_TYPE WIN32)
	set(PLATFORM_SUPPORT -lQt5Gui ${MXEPREFIX}/qt5/plugins/platforms/libqwindows.a -lQt5FontDatabaseSupport -lfreetype -ldwmapi -lbz2)

	include_directories(/opt/mxe/usr/i686-w64-mingw32.static/qt5/include/QtCore /opt/mxe/usr/i686-w64-mingw32.static/qt5/include/QtXml /opt/mxe/usr/i686-w64-mingw32.static/qt5/include/QtWidgets /opt/mxe/usr/i686-w64-mingw32.static/qt5/include/QtGui /opt/mxe/usr/i686-w64-mingw32.static/qt5/include/QtNetwork /opt/mxe/usr/i686-w64-mingw32.static/qt5/include/QtCore -isystem /opt/mxe/usr/i686-w64-mingw32.static/qt5/mkspecs/default -isystem /opt/mxe/usr/i686-w64-mingw32.static/qt5/include)
	add_executable(QAnnotate WIN32 ${QAnnotate_SRCS} ${QAnnotate_MOC_SRCS} ${QAnnotate_RES_SOURCES} ${QAnnotate_UI_HEADERS} appicon.rc)
	target_link_libraries(QAnnotate qtfe -L${MXEPREFIX}/qt5/lib/ -lQt5Xml -lQt5Widgets -lQt5FontDatabaseSupport -lQt5EventDispatcherSupport -lQt5ThemeSupport -lQt5Gui -lQt5Core ${MXEPREFIX}/qt5/plugins/platforms/libqwindows.a -lQt5WindowsUIAutomationSupport ${MXEPREFIX}/qt5/plugins/iconengines/libqsvgicon.a ${MXEPREFIX}/qt5/plugins/imageformats/libqsvg.a -lQt5Svg ${MXEPREFIX}/qt5/plugins/platforms/libqminimal.a -lsodium -lssl -lbtree -luv -lcrypto -lz -ltiff -lpcre2-16 -lharfbuzz -lpng -ljpeg -lmng -llcms2 -llzma -limm32 -lws2_32 -lwsock32 -lnetapi32 -luserenv -lwinmm -lz -lversion -lkernel32 -lpsapi -liphlpapi ${PLATFORM_SUPPORT})
endif(WIN32)
if(UNIX)
	add_executable(QAnnotate ${QAnnotate_SRCS} ${QAnnotate_MOC_SRCS} ${QAnnotate_RES_SOURCES} ${QAnnotate_UI_HEADERS} ${BACKWARD_ENABLE})
	#	add_backward(QAnnotate)
	target_link_libraries(QAnnotate qtfe Qt5::Widgets Qt5::Network Qt5::Xml -lsodium -luv -lbtree)
endif(UNIX)
target_include_directories(QAnnotate PRIVATE ${CMAKE_BINARY_DIR})

#OSX: cmake .. -DCMAKE_PREFIX_PATH=$(brew --prefix qt5)
