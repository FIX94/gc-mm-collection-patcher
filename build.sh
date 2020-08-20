#!/bin/sh

set -e

DEVKITPPC_BIN_PATH=${1:-/opt/devkitpro/devkitPPC/bin}
COL_GREEN="\e[32m"
COL_DEFAULT="\e[0m"

build_cmpbin() {
cd cmpbin
printf "$COL_GREEN%s$COL_DEFAULT\n" "building cmpbin"
# TODO: add better check
if [[ -e /usr/lib/liblzma.a ]];
    then
        gcc -Wall -static -O2 -s main.c -llzma -o cmpbin
    else
        # not static lzma lib
        gcc -Wall -O2 -s main.c -llzma -o cmpbin
fi
cd $PROJDIR
}

build_emu() {
cd emu
printf "$COL_GREEN%s$COL_DEFAULT\n" "building emu"
make clean && make
cd $PROJDIR
}

build_decmp() {
cd decmp
printf "$COL_GREEN%s$COL_DEFAULT\n" "building decmp"
make clean && make
cd $PROJDIR
}

build_dolinject() {
cd dolinject
printf "$COL_GREEN%s$COL_DEFAULT\n"  "building dolinject"
gcc -Wall -static -O2 -s main.c -o dolinject
}


PROJDIR=$(pwd)
export PATH=$PATH:$DEVKITPPC_BIN_PATH
build_cmpbin
build_emu
build_decmp
build_dolinject
exit 0
