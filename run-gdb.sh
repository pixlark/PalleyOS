#!/usr/bin/env bash

gdb build/palleyos.bin -ex 'target remote localhost:1234'
