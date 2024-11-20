# Libhydrogen sig test

Simple test to invesitage why signature validation always works on Sonata

Contains test for both linux and sonata built form saem libhydrogen source and using pre-generated keys

## Linux:
```
gcc -I ./libhydrogen test.c ./libhydrogen/hydrogen.c -o test
./test
```

## Sonata
```
xmake f --sdk=/cheriot-tools/ -P .
xmake
xmake run
```