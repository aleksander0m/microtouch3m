cmake_minimum_required(VERSION 3.5.1)

option(IMX51 "Make IMX51 build" false)

if (IMX51)
    set(CMAKE_PROGRAM_PATH ${CMAKE_PROGRAM_PATH} /opt/timesys/i_MX51/toolchain/bin)
    set(CMAKE_SYSROOT /opt/timesys/i_MX51/toolchain)
    set(CMAKE_TOOLCHAIN_FILE /opt/timesys/i_MX51/toolchain/toolchain.cmake)
endif()

project(microtouch3m-scope)

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

find_package(SDL_net 1.2.8 REQUIRED)

if (NOT SDL_NET_FOUND)
    message(FATAL_ERROR "SDL_net Not found")
endif()

set(SOURCE_FILES
        main.cpp
        SDLUtils.cpp
        SDLApp.cpp
        Microtouch3MScopeApp.cpp
        BitmapFontRenderer.cpp
        BitmapFontRenderer.hpp
        M3MDevice.cpp M3MDevice.hpp
        Thread.cpp Thread.hpp
        Mutex.cpp Mutex.hpp)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

include_directories(${SDL_INCLUDE_DIR} ${SDL_IMAGE_INCLUDE_DIR} ${SDL_NET_INCLUDE_DIR})

target_link_libraries(${PROJECT_NAME} ${SDL_LIBRARY} ${SDL_IMAGE_LIBRARY} ${SDL_NET_LIBRARY} ${MICROTOUCH3M_LIBRARY})
