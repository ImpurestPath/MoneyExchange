cmake_minimum_required(VERSION 3.10)
project(MoneyExchange)

set(CMAKE_CXX_STANDARD 11)
#add_definitions (-D_GLIBCXX_USE_CXX11_ABI=0)
add_executable(MoneyExchange main.cpp)
#set(LIBXML2_LIBRARY "-lxml2")

find_package(PkgConfig)

pkg_search_module(libxml++ REQUIRED libxml++-2.6)

target_include_directories(MoneyExchange PUBLIC ${libxml++_INCLUDE_DIRS})
target_link_libraries(MoneyExchange PUBLIC ${libxml++_LIBRARIES})
target_compile_options(MoneyExchange PUBLIC ${libxml++_CFLAGS})

set(CURL_LIBRARY "-lcurl")
find_package(CURL)
if (NOT CURL_FOUND)
    message(FATAL_ERROR "curl is required")
else(NOT CURL_FOUND)
    include_directories(${CURL_INCLUDE_DIRS})
    target_link_libraries(MoneyExchange PUBLIC ${CURL_LIBRARIES})
endif(NOT CURL_FOUND)

include_directories("-I/usr/lib/x86_64-linux-gnu/libxml++-2.6/include")


