cmake_minimum_required(VERSION 3.28)
project(nullsafety VERSION 1.0 LANGUAGES CXX)
add_library(nullsafety STATIC)
target_compile_features(nullsafety PUBLIC cxx_std_20)
target_include_directories(nullsafety PRIVATE include)

target_sources(nullsafety 
    PRIVATE src/hng/nullsafety/nullsafety.cpp)

target_include_directories(nullsafety PUBLIC "include")

# #set_target_properties(nullsafety PROPERTIES PUBLIC_HEADER "nullsafety.h")
# INSTALL(TARGETS nullsafety
#     FILE_SET HEADERS)
