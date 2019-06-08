cd cmpbin
echo cmpbin
gcc -Wall -static -O2 -s main.c -llzma -o cmpbin
cd ../emu
make clean
make
cd ../decmp
make clean
make
cd../dolinject
echo dolinject
gcc -Wall -static -O2 -s main.c -o dolinject
pause