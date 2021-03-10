cmake_minimum_required(VERSION 3.10)

include(GNUInstallDirs)

project(s3pdb)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

find_package(AWSSDK REQUIRED)

set(SRC_FILES
	src/s3pdb.cpp)

add_executable(s3pdb ${SRC_FILES})

if(NOT MSVC)
	target_compile_options(s3pdb PUBLIC -Wall -Werror=cast-function-type -Wno-expansion-to-defined -Wunused-parameter -Wtype-limits -Wextra -Wconversion)
endif()

target_link_libraries(s3pdb aws-cpp-sdk-s3 aws-cpp-sdk-core)

install(TARGETS s3pdb
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)
