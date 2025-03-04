#!/bin/bash
clang++ -std=c++20 -g3 main.cpp -o tracer
clang++ -std=c++20 -g3 singlethread-main.cpp -o tracer-single
clang++ -std=c++20 -g3 tracee.cpp -o tracee