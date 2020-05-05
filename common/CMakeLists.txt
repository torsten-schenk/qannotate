#cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr
cmake_minimum_required(VERSION 2.8.11)

set(ROOTDIR ../)
include_directories(${ROOTDIR})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/../cmake")
set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -g")

#include_directories(../lib/include)

function(make_tests)
	foreach(arg ${ARGN})
		add_executable(test_${arg} src/${arg}.c)
		target_link_libraries(test_${arg} common -lcunit)
		add_test(NAME ${arg} COMMAND test_${arg})
		set_target_properties(test_${arg} PROPERTIES COMPILE_FLAGS "-DTESTING")
	endforeach()
endfunction()

project(common)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories("${CMAKE_SOURCE_DIR}")

set(common_SRC
	base32.c
)

set(common_HDR
)

if(WIN32)
	add_library(common STATIC ${common_SRC})
	#	include_directories(/home/humanist/mxe/usr/i686-w64-mingw32.static/qt/include/QtGui /home/humanist/mxe/usr/i686-w64-mingw32.static/qt/include/QtCore -isystem /home/humanist/mxe/usr/i686-w64-mingw32.static/qt/mkspecs/default -isystem /home/humanist/mxe/usr/i686-w64-mingw32.static/qt/include)
	#add_executable(common main.cpp textlex.cpp annotlex.cpp treedata.cpp mainwindow.cpp dispatch.cpp dialogs.cpp log.cpp widgets.cpp ${annotate_MOC_SRCS})
	#add_backward(annotate)
	#target_link_libraries(annotate -L/home/humanist/mxe/usr/i686-w64-mingw32.static/qt/lib/ -lQtGui -lQtCore -lz -ltiff -lpng -ljpeg -lmng -llcms2 -llzma -limm32 -lws2_32 -lwsock32 -lwinmm -lz)
	#target_link_libraries(annotate Qt4::QtGui -ltiff -lpng -ljpeg -lmng -llcms2 -llzma -lz -limm32 -lws2_32 -lwsock32 -lwinmm -lz)
endif(WIN32)
if(UNIX)
	add_library(common STATIC ${common_SRC})

	#	add_definitions(-DTESTING)
	#enable_testing()
	#	make_tests(buffer)
endif(UNIX)
#target_include_directories(common PRIVATE ${CMAKE_BINARY_DIR})

#set_target_properties(common PROPERTIES PUBLIC_HEADER "${common_HDR}")
#install(TARGETS common
#	PUBLIC_HEADER DESTINATION include/common
#	ARCHIVE DESTINATION lib
#	LIBRARY DESTINATION lib)

