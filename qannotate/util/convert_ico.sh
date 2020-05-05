#!/bin/bash
#
# Convert Program-Logo from .svg to .ico, to be used as Windows Program Icon

inkscape -z -e appicon.png -w 64 -h 64 ../images/logo_icon.svg
convert -background transparent appicon.png -define icon:auto-resize=16,32,48,64,256 ../appicon.ico