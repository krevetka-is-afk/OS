cmake_minimum_required(VERSION 3.30)
project(ihw1 C)

set(CMAKE_C_STANDARD 23)

add_executable(ihw1 v_for_4/main.c
        v_for_5/main.c
        v_for_6/main.c
        v_for_7/main.c
        v_for_8/reader.c
        v_for_8/processor.c
        v_for_8/writer.c)
