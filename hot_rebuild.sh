#! /usr/bin/bash

gcc -fPIC -shared -o build/libspaceinv.so src/spaceinv.c -Ilib/grv/include -Iinclude
