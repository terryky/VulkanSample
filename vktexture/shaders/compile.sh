#!/bin/sh
glslangValidator shader.vert -V -S vert -o shader.vert.spv
glslangValidator shader.frag -V -S frag -o shader.frag.spv

