#!/bin/bash

if [[ $# -ne 2 ]]
then
    echo "App needs exactly 2 arguments"
    exit 1
fi

if [[ -d $(dirname $1) ]]
then
    touch $1
else
    mkdir -p $(dirname $1) && touch $1
fi

echo $2 > $1