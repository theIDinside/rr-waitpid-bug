#!/bin/bash
clang++ -std=c++20 -g3 main.cpp -o tracer
clang++ -std=c++20 -g3 tracee.cpp -o tracee