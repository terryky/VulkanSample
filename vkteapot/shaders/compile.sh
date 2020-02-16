#!/bin/sh
glslangValidator teapot.vert -V -S vert -o teapot.vert.spv
glslangValidator teapot.frag -V -S frag -o teapot.frag.spv

