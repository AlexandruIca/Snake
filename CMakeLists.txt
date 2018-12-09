cmake_minimum_required( VERSION 3.5 )

project( ConsoleSnake )

find_package( SDL2 REQUIRED )

set( CMAKE_CXX_STANDARD 17 )
set( SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp )

add_executable( ioana ${SRC_FILES} )

target_include_directories( ioana PUBLIC ${SDL2_INCLUDE_DIRS} )
target_link_libraries( ioana ${SDL2_LIBRARIES} )

