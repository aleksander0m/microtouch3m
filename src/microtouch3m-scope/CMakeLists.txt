cmake_minimum_required(VERSION 3.5.1)

# must be before project(), or won't work
option(IMX51 "Make IMX51 build" off)

if (IMX51)
    set(CMAKE_PROGRAM_PATH ${CMAKE_PROGRAM_PATH} /opt/timesys/i_MX51/toolchain/bin)
    set(CMAKE_SYSROOT /opt/timesys/i_MX51/toolchain)
    set(CMAKE_TOOLCHAIN_FILE /opt/timesys/i_MX51/toolchain/toolchain.cmake)
endif()

project(microtouch3m-scope)

option(TEST_VALUES "Use test values instead of real microtouch3m device" off)

if (TEST_VALUES)
    add_definitions(-DTEST_VALUES)
endif()

if (IMX51)
    add_definitions(-DIMX51)

    include_directories(${PROJECT_SOURCE_DIR}/../../build/imx51/include)
    find_library(MICROTOUCH3M_LIBRARY microtouch3m ${PROJECT_SOURCE_DIR}/../../build/imx51/lib)
else()
    include_directories(${PROJECT_SOURCE_DIR}/../../build/pc/include)
    find_library(MICROTOUCH3M_LIBRARY microtouch3m ${PROJECT_SOURCE_DIR}/../../build/pc/lib)
endif()

if (NOT MICROTOUCH3M_LIBRARY)
    message(FATAL_ERROR "MICROTOUCH3M library Not found")
endif()

set(CMAKE_CXX_STANDARD 98)

find_package(SDL 1.2.15 REQUIRED)

if (NOT SDL_FOUND)
    message(FATAL_ERROR "SDL Not found")
endif()

find_package(SDL_image 1.2.12 REQUIRED)

if (NOT SDL_IMAGE_FOUND)
    message(FATAL_ERROR "SDL_image Not found")
endif()

set(SOURCE_FILES
        main.cpp
        LineChart.hpp
        Color.hpp
        SDLUtils.cpp
        SDLApp.cpp
        M3MScopeApp.cpp
        BitmapFontRenderer.cpp
        BitmapFontRenderer.hpp
        M3MDevice.cpp M3MDevice.hpp
        Thread.cpp Thread.hpp
        Mutex.cpp Mutex.hpp
        Utils.cpp Utils.hpp
        M3MLogger.cpp M3MLogger.hpp)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

include_directories(${SDL_INCLUDE_DIR} ${SDL_IMAGE_INCLUDE_DIR})
include_directories(../../)

target_link_libraries(${PROJECT_NAME} ${SDL_LIBRARY} ${SDL_IMAGE_LIBRARY} ${MICROTOUCH3M_LIBRARY})
