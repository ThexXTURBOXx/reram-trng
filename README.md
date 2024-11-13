# Raspberry Pi Measurement Kernel

Bare minimum Kernel for the Raspberry Pi to examine memory chips via SPI. Currently being used as a ReRAM-based TRNG.

## Setup

See [the setup documentation](doc/Setup.md)!

## Citing

The BibTeX snippet below is the recommended way to cite this project.

Further research on the TRNG itself has been made in several works. Many of them can be found [here](https://nmexis.me/#research-and-publications) (along with [BibTeX code](https://femtopedia.de/research/) to cite them).

```BibTeX
@inproceedings{arul2024investigationcommercialshelf,
  doi          = {10.1109/DSD64264.2024.00014},
  url          = {https://doi.org/10.1109/DSD64264.2024.00014},
  title        = {{Investigation Of Commercial Off-The-Shelf ReRAM Modules For Use As Runtime-Accessible TRNG}},
  author       = {Arul, Tolga and Mexis, Nico and George, Aleena Elsa and Frank, Florian and Anagnostopoulos, Nikolaos Athanasios and Katzenbeisser, Stefan},
  year         = 2024,
  month        = aug,
  booktitle    = {2024 27th Euromicro Conference on Digital System Design (DSD)},
  publisher    = {IEEE},
  address      = {Paris, France},
  pages        = {33--42},
  keywords     = {Temperature distribution;Protocols;Digital systems;Resistive RAM;NIST;Throughput;Entropy;Generators;Internet of Things;Security;Commercial Off-The-Shelf (COTS);Resistive Random Access Memory (ReRAM);runtime-accessible;True Random-Number Generator (TRNG)}
}
```

## Credits

- `boot/` is the target directory for all the files that will need to be copied to an SD card. This folder will contain files that are taken directly from [raspberrypi/firmware](https://github.com/raspberrypi/firmware)

- `circle/` is a submodule that includes [Rene Stange](https://github.com/rsta2)'s [circle](https://github.com/rsta2/circle) framework, which provides all the bare metal driver implementations we need
- `Makefile` is a modified version of the Makefile that [Rene Stange](https://github.com/rsta2)'s [circle](https://github.com/rsta2/circle) framework suggests
- `doc/Wiring*` files have been generated using [Fritzing](https://fritzing.org/)
- `mt19937ar*` files are modified versions of Takuji Nishimura's and Makoto Matsumoto's [Mersenne Twister implementation](http://www.math.sci.hiroshima-u.ac.jp/m-mat/MT/MT2002/emt19937ar.html)
- This work has been partially funded by the Interreg VI-A Programme Germany/Bavaria-Austria 2021-2027 - Programm INTERREG VI-A Bayern-Österreich 2021-2027, as part of Project BA0100016: "CySeReS-KMU: Cyber Security and Resilience in Supply Chains with focus on SMEs", co-funded by the European Union, and by the German Research Foundation - Deutsche Forschungsgemeinschaft (DFG), under Projects 440182124: "PUFMem: Intrinsic Physical Unclonable Functions from Emerging Non-Volatile Memories", and 439892735: "NANOSEC: Tamper-Evident PUFs Based on Nanostructures for Secure and Robust Hardware Security Primitives" of the Priority Program - SchwerPunktProgramme (SPP) 2253: "Nano Security: From Nano-Electronics to Secure Systems".
