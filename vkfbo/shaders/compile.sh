#!/bin/sh
glslangValidator teapot.vert -V -S vert -o teapot.vert.spv
glslangValidator teapot.frag -V -S frag -o teapot.frag.spv

glslangValidator util_debugstr.vert -V -S vert -o util_debugstr.vert.spv
glslangValidator util_debugstr.frag -V -S frag -o util_debugstr.frag.spv

glslangValidator util_pmeter.vert -V -S vert -o util_pmeter.vert.spv
glslangValidator util_pmeter.frag -V -S frag -o util_pmeter.frag.spv
