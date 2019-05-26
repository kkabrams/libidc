#!/bin/sh
grep --line-buffered ^_ \
  | stdbuf -oL tr 'a-z' 'n-za-m'
