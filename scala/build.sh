#!/bin/bash
set -e

mkdir -p bin
#scalac -d bin -g:vars `find src/ -name "*.scala" -printf "%p "`
scalac -d bin -g:none -optimise `find src/ -name "*.scala" -printf "%p "`

