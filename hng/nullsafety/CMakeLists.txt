cmake_minimum_required(VERSION 3.28)
project(nullsafety VERSION 1.0.1 LANGUAGES CXX)
add_library(nullsafety INTERFACE)
target_include_directories(nullsafety INTERFACE include)
