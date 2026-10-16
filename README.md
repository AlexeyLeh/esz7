# ESZ7 Ground Unit

## 1. Project Purpose

Replacement for a ground unit equipped with pressure and weight sensors.
This project runs on Xilinx Zynq 2023.2 Linux (ARMv7).
(Theoretically, it can run on any Linux or Windows system.)

## 2. esz7-siu

```
ESZ7-SIU Ver. 2.1.1 Oct 16 2025 (built Oct 24 2025 11:28:22)

Usage: esz7-siu [options] [-d <max_npnts>] [-v] [-h]
Where options are:
--mode <type>       - operating mode (supported types: Atlas, ADS, SureShot)
--dump <type>       - save objects to files
``` type: 1..3
-d <1-5>            - debug printout
-h                  - help
-v                  - version

See esz7-siu.cfg to set up the working configuration.

Examples:

```
$ ./esz7-siu
```

#### Operational Ports

Ports for interfacing with APS ADS (decoder) and SureShot:

* 8220 - ADS commands
* 8110 - data



* 7111 - SureShot commands
* 8111 - data



* 7110 - Depth Tracker commands
* 8210 - data

## 3. Unfinished Features (Current Limitations)

SureShot menu items supported by esz7-siu:

#### MWD - MWD Selection

* ADS	- works correctly
* SUI 1 - not supported
* SIU 2 - connection is handled, but no data is transmitted (the data format is currently unknown)
* WITS  - connection is handled, but transmitted data is ignored (requires further investigation)

#### Depth - Depth Selection

* APS SIU2 - works correctly
* WITS from APS SIU2 - the operational logic is currently unclear

#### WITS

* The operational logic is unclear. It receives data and displays it in the "SHOW WITS" window,
but the parsing logic remains inactive; no responses are observed.


## 4. Build Options

The project is developed in the CLion environment.

Builds are performed using CMake.

Since the program is intended for use on the Xilinx XME0724 module (ARMv7 Linux),
it is built on a Linux machine.
It can also be built on Windows and executed via a terminal. * On Linux, GCC compilers can be installed as follows (variations are possible):

```
sudo apt install -y flex bison
sudo apt install -y make
sudo apt install -y gcc g++
sudo apt install -y gcc-arm-linux-gnueabi   g++-arm-linux-gnueabi   binutils-arm-linux-gnueabi
sudo apt install -y gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf binutils-arm-linux-gnueabihf
```

* For Windows, the GCC compiler is installed differently:

* ```
search online for instructions...
:)

## 5. Build Procedure

* CMake
* Makefile

Note: The build procedure is also described in the **HowToBuild.txt** file for both CMake and Makefile.

#### Using CMake

* /usr/bin/cmake
* cmake.exe

1. Preparing the build

`cd /.../esz7-siu/`
`cmake.exe -S .` -B build`

2. Building the project

```
cmake.exe --build debug -v --target esz7-siu -j4
```

3. Cleaning the project

`cmake.exe --build debug -v --target clean -j4`

#### When using Makefile

* /usr/bin/make
* mingw64-make.exe

1. Preparing the build

`$ mkdir build $ cmake -G "MinGW Makefiles" -B build .`

or

`$ mkdir build $ cd build $ cmake -G "MinGW Makefiles" ..`

2. Building the project

`$ make -C build`

or

`$ cd build $ make`

3. Cleaning the project

`$ make -C build clean`

## 6. Location of the compiled program

```
$ ls -l build\esz7-siu\*
```
