#!/usr/bin/bash
mkdir -p ../../build/Breakout.app/Contents/MacOS
pushd ../../build/Breakout.app/Contents/MacOS > /dev/null
gcc ../../../../src/mac/mac_main.m -o Breakout -framework Cocoa
popd > /dev/null