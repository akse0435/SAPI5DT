# SAPI5DT

SAPI5 interface for DECtalk 4.99

## What is this

This is a SAPI5 interface for DECtalk, making it possible to use the classic DECtalk speech synthesizer with all its voices in any Windows application that supports SAPI5. This code was originally written by [datajake1999](https://github.com/datajake1999/SAPI5DT), and a few small changes and improvements have been made since, along with making it possible to build the code for 64-bit Windows. A voice manager has also been added; a C port of the one by [masonasons](https://github.com/masonasons/DECTalkNVDA).

Please note: all changes made to the original code have been made with the help of AI, so not everything may be perfect. All contributions and feedback are very welcome.

## Building

### What you need

- Microsoft Visual Studio 2022 with the "Desktop development with C++" workload and the "C++ ATL for latest v143 build tools" component (some other versions of Visual Studio might work, but are not tested)
- A copy of [the DECtalk 4.99 GitHub repository](https://github.com/dectalk/dectalk)
- [NSIS](https://nsis.sourceforge.io/Download)

### How to build

1. Clone or download the DECtalk 4.99 repository.
2. Now clone this repository and place it in the root of the DECtalk repository, so that you have a `SAPI5DT` folder next to the `src` folder.
3. Run `build.bat` from the `SAPI5DT` folder.
4. Download and install NSIS if you haven't already.
5. Right-click on `installer.nsi` in the `SAPI5DT` folder and press "Compile NSIS Script". When it's done, press "Close".
6. You now have a file in the `SAPI5DT` folder called `dt499sapi5_installer.exe`. This is your complete SAPI5 installer.
