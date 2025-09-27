#!/bin/bash

cmake --preset=DefaultDebug .
cmake --build cmake-build-debug -- -j 8
