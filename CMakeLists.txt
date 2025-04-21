cmake_minimum_required(VERSION 3.28)
project(osi_hw2)

set(CMAKE_CXX_STANDARD 17)

add_executable(osi_hw2 main.cpp
        main7.cpp
        smokers.h
        agent7.cpp
        smoker1.cpp
        smoker2.cpp
        smoker3.cpp
        agent8.cpp
        smoker_tobacco.cpp
        init_shared_memory.cpp
        smoker_paper.cpp
        smoker_matches.cpp
)
