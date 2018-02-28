# xodometer
Track the total distance of your pointing device and cursor travel. The distance
can be displayed in various units.

Slightly updated to be compatible with modern compilers.

## Build
Use the provided Makefile to build the source code:
```
$ make
```
Or run your favourite C compiler directly:
```
$ gcc8 -DP_EVAP_MM_PATH=\"/usr/local/bin\" -c evap/evap.c
$ gcc8 -I/usr/local/include -L/usr/local/lib -o xodo xodo.c evap.o -lX11 -lm
```
Or use imake:
```
$ setenv IMAKECPP cpp8
$ xmkmf
$ make
```
You have to use GCC, as imake doesn’t work well with Clang.

## Copyright
Copyright (c) 1996, Stephen O. Lidie and Lehigh University. Inspired by the
Macintosh Mouse Odometer by Sean P. Nolan.
