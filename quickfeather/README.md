# polymath-iot (Quickfeather)
This repo contains the source code for polymath-iot (Quickfeather controller portion).

## Directory Structure
- source -- Contains the source code for this project
- source/app -- Contains the application specific code created by us.
- source/qorc-sdk -- Contains the SDK source the application is built on top of. Copied from here: https://github.com/QuickLogic-Corp/qorc-sdk

## Computer Setup
This project requires the following tools:
- Windows 10 workstation
- WSL (Windows Subsystem for Linux) installed. Note that Linux is only used for building the project with Make -- otherwise we will just use regular Windows 10 (e.g. Python, pip, etc).
  - The GCC-ARM toolchain must be installed to WSL like below.
  
    1. Download `"gcc-arm-none-eabi-10-2020-q4-major-x86_64-linux.tar"` tarball from: https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads

    2. Extract the tarball to /usr/share.

        `sudo tar xvjf gcc-arm-none-eabi-9-2020-q2-update-x86_64-linux.tar.bz2 -C /usr/share/`

- Python 3.6 or higher is need to reprogram the application (install a Windows version). 
    - TinyFPGA must be installed to the system. See the instructions here: https://github.com/QuickLogic-Corp/TinyFPGA-Programmer-Application

- Visual Studio Code (VSCode) is used as the IDE: https://code.visualstudio.com/
  - After installing VSCode, make sure to install the "C/C++" Extension from Microsoft.

## Build

In order to build the project, issue the following from a windows command line.

   `wsl sh build.sh`

In order to clean the project, issue the following from a windows command line.

  `wsl sh build.sh clean`
