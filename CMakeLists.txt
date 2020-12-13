cmake_minimum_required(VERSION 3.17)
project(ncmdump)

set(CMAKE_CXX_STANDARD 17)

include_directories(.)
set(taglib_DIR /usr/local/Cellar/taglib/1.11.1/)

include_directories(${taglib_DIR}/include)
link_directories(${taglib_DIR}/lib)

add_executable(
        ncmdump
        aes.cpp
        aes.h
        base64.h
        cJSON.cpp
        cJSON.h
        main.cpp
        ncmcrypt.cpp
        ncmcrypt.h
        argparse.hpp
)
target_link_libraries(ncmdump tag)
