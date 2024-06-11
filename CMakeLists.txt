## #################################################################
## Root CMakeLists.txt
## #################################################################

cmake_minimum_required(VERSION 3.0)


project(BabyX)

if(CMAKE_SYSTEM_NAME MATCHES Windows)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

# Build the Baby X library 

file( GLOB BBX_COMMON src/common/*.c )
file( GLOB BBX_COMMONH src/common/*.h )
file( GLOB BBX_WINDOWS src/windows/*.c )
file( GLOB BBX_WINDOWSH src/windows/*.h )
file( GLOB BBX_LINUX src/linux/*.c )
file( GLOB BBX_LINUXH src/linux/*.h ) 
file( GLOB BBX_WIDGETS src/widgets/*.c )
file( GLOB BBX_WIDGETSH src/widgets/*.h )

get_filename_component(full_path ${CMAKE_CURRENT_SOURCE_DIR}/src/windows/testbed.c ABSOLUTE)
list( REMOVE_ITEM BBX_WINDOWS "${full_path}" )
get_filename_component(full_path ${CMAKE_CURRENT_SOURCE_DIR}/src/windows/main.c ABSOLUTE)
list( REMOVE_ITEM BBX_WINDOWS "${full_path}" )

set( bbx_includes "")
set( bbx_sources "")
list(APPEND bbx_sources ${BBX_COMMON} )
list(APPEND bbx_sources ${BBX_COMMONH} )
list(APPEND bbx_includes "src/common")

if(CMAKE_SYSTEM_NAME MATCHES Windows)
	list(APPEND bbx_sources ${BBX_WINDOWS} )
	list(APPEND bbx_sources ${BBX_WINDOWSH} )
	list(APPEND bbx_includes "src/windows") 
endif()

if(CMAKE_SYSTEM_NAME MATCHES Linux)
	list(APPEND bbx_sources ${BBX_LINUX})
	list(APPEND bbx_sources ${BBX_LINUXH})
        list(APPEND bbx_includes "src/linux")
endif()

if(CMAKE_SYSTEM_NAME MATCHES Darwin)
    list(APPEND bbx_sources ${BBX_LINUX})
    list(APPEND bbx_sources ${BBX_LINUXH})
    list(APPEND bbx_includes "src/linux")
    list(APPEND bbx_includes "/usr/X11/include")
endif()

list(APPEND bbx_sources ${BBX_WIDGETS})
list(APPEND bbx_sources ${BBX_WIDGETSH})
list(APPEND bbx_includes "src/widgets")

add_library( bbx ${bbx_sources} )
target_include_directories(bbx SYSTEM PRIVATE ${bbx_includes})

set(libs bbx)
if(CMAKE_SYSTEM_NAME MATCHES Linux)
	list(APPEND libs X11)
        list(APPEND libs m)
endif()
if(CMAKE_SYSTEM_NAME MATCHES Darwin)
    find_package("X11")
    list(APPEND libs ${X11_LIBRARIES})
        list(APPEND libs m)
endif()


# Baby Bridge example program

file( GLOB BRIDGE_SOURCES BabyPrograms/BabyBridge/*.c )
file( GLOB BRIDGE_HEADERS BabyPrograms/BabyBridge/*.h )


add_executable( "BabyBridge" ${BRIDGE_SOURCES} ${BRIDGE_HEADERS} )
target_include_directories("BabyBridge" SYSTEM PRIVATE ${bbx_includes})
target_link_libraries("BabyBridge" ${libs} )

# Baby Hello World example program

file( GLOB HELLOWORLD_SOURCES BabyPrograms/BabyHelloWorld/*.c )
file( GLOB HELLOWORLD_HEADERS BabyPrograms/BabyHelloWorld/*.h )


add_executable( "BabyHelloWorld" ${HELLOWORLD_SOURCES} ${HELLOWORLD_HEADERS} )
target_include_directories("BabyHelloWorld" SYSTEM PRIVATE ${bbx_includes})
target_link_libraries("BabyHelloWorld" ${libs} )

# Baby Perlin example program

file( GLOB PERLIN_SOURCES BabyPrograms/BabyPerlin/*.c )
file( GLOB PERLIN_HEADERS BabyPrograms/BabyPerlin/*.h )


add_executable( "BabyPerlin" ${PERLIN_SOURCES} ${PERLIN_HEADERS} )
target_include_directories("BabyPerlin" SYSTEM PRIVATE ${bbx_includes})
target_link_libraries("BabyPerlin" ${libs} )

# Graphics Context Test example program

file( GLOB GCTEST_SOURCES BabyPrograms/GraphicsContextTest/*.c )
file( GLOB GCTEST_HEADERS BabyPrograms/GraphicsContextTest/*.h )


add_executable( "GraphicsContextTest" ${GCTEST_SOURCES} ${GCTEST_HEADERS} )
target_include_directories("GraphicsContextTest" SYSTEM PRIVATE ${bbx_includes})
target_link_libraries("GraphicsContextTest" bbx ${libs})

# Widget Test example program

file( GLOB WIDGETTEST_SOURCES BabyPrograms/WidgetTest/*.c )
file( GLOB WIDGETTEST_HEADERS BabyPrograms/WidgetTest/*.h )


add_executable( "WidgetTest" ${WIDGETTEST_SOURCES} ${WIDGETTEST_HEADERS} )
target_include_directories("WidgetTest" SYSTEM PRIVATE ${bbx_includes})
target_link_libraries("WidgetTest" ${libs} )

# WordCatch example program

# We've got audio. Not yet implemented for Linux
if( CMAKE_SYSTEM_NAME MATCHES Windows )
	file( GLOB WORDCATCH_SOURCES BabyPrograms/WordCatch/*.c )
	file( GLOB WORDCATCH_HEADERS BabyPrograms/WordCatch/*.h )


	add_executable( "WordCatch" ${WORDCATCH_SOURCES} ${WORDCATCH_HEADERS} )
	target_include_directories("WordCatch" SYSTEM PRIVATE ${bbx_includes})

	target_link_libraries("WordCatch" bbx Winmm )
endif()
