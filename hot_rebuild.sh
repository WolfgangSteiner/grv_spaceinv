#! /usr/bin/bash

gcc -fPIC -shared -o build/libspaceinv.so -I. -Iinclude -Ilib/grv/include src/spaceinv.c
gcc -fPIC -shared -o build/libsynth.so -I. -Iinclude -Ilib/grv/include src/synth.c
