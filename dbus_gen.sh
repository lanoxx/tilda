#!/bin/bash

# Generate DBus interface
# Tutorial: https://aleksander.es/data/GNOMEASIA2014%20-%20Introduction%20to%20DBus.pdf

set -eu


readonly DEST=$([ -z "$1" ] && echo "" || echo "$1/")
readonly IFACE='toggle'
readonly DOMAIN='lanoxx.tilda'
readonly DEFINATION_FILE_NAME='lanoxx.tilda.Toggle.xml'

gdbus-codegen --interface-prefix "$DOMAIN" --generate-c-code "$IFACE" "$DEST/$DEFINATION_FILE_NAME"
\mv "./$IFACE.c" "./$IFACE.h" "${DEST}src/"

