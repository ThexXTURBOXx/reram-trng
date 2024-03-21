# Raspberry Pi Measurement Kernel

Bare minimum Kernel for the Raspberry Pi to examine memory chips via SPI

## Setup

See [the setup documentation](doc/Setup.md)!


# Credits

 - `boot/` is the target directory for all the files that will need to be copied to a SD card. This folder will contain files which are taken directly from [raspberrypi/firmware](https://github.com/raspberrypi/firmware)
 - `circle/` is a submodule that includes [Rene Stange](https://github.com/rsta2)'s [circle](https://github.com/rsta2/circle) framework, which provides all the bare metal driver implementations we need
 - `Makefile` is a modified version of the Makefile that [Rene Stange](https://github.com/rsta2)'s [circle](https://github.com/rsta2/circle) framework suggests
 - `doc/Wiring*` files have been generated using [Fritzing](https://fritzing.org/)
 - `mt19937ar*` files are modified versions of Takuji Nishimura's and Makoto Matsumoto's [Mersenne Twister implementation](http://www.math.sci.hiroshima-u.ac.jp/m-mat/MT/MT2002/emt19937ar.html)
