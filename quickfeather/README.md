# polymath-iot (Quickfeather)
This repo contains the source code for polymath-iot (Quickfeather controller portion).

## Directory Structure
- source -- Contains the source code for this project
- source/app -- Contains the application specific code created by us.
- source/qorc-sdk -- Contains the SDK source the application is built on top of. Copied from here: https://github.com/QuickLogic-Corp/qorc-sdk

## Computer Setup (Command Line)
This project requires the following tools:
- Windows 10 workstation
- WSL (Windows Subsystem for Linux) installed. Note that Linux is only used for building the project with Make -- otherwise we will just use regular Windows 10 (e.g. Python, pip, etc).
  - The GCC-ARM toolchain must be installed to WSL like below.
  
    1. Download `"gcc-arm-none-eabi-9-2020-q2-update-x86_64-linux.tar.bz2"` tarball from: https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads

    2. Extract the tarball to /usr/share.

        `sudo tar xvjf gcc-arm-none-eabi-9-2020-q2-update-x86_64-linux.tar.bz2 -C /usr/share/`

- Python 3.6 or higher is need to reprogram the application (install a Windows version). 
    - TinyFPGA must be installed to the system. See the instructions here: https://github.com/QuickLogic-Corp/TinyFPGA-Programmer-Application

- Visual Studio Code (VSCode) is used as the IDE: https://code.visualstudio.com/
  - After installing VSCode, make sure to install the "C/C++" Extension from Microsoft.

## Build (Command Line)

In order to build the project, issue the following from a windows command line.

   `wsl sh build.sh`

In order to clean the project, issue the following from a windows command line.

  `wsl sh build.sh clean`

## Eclipse

In order to set up the project to build and Debug using Eclipse, refer to the documentation in **source/app/GCC_Project/EclipseReadme.txt**. Note that:

1. When instructed to enter `make -C ${workspace_loc:/${ProjName}}/../` as the build command for Eclipse, you can instead enter `${workspace_loc:/${ProjName}}/../eclipse_build.bat ${workspace_loc:/${ProjName}}`. This was done to workaround build failures due to dependency files.

2. When debugging, the app.elf file is used (not a .bin file).

3. When debugging an application with JLink, the bootstrap mode should not be enabled (i.e. the shunts for J1 and J7 should not be installed).

## Useful References

* [Quickfeather Board User Guide](https://github.com/QuickLogic-Corp/quick-feather-dev-board/blob/master/doc/QuickFeather_UserGuide.pdf)
* [Quickfeather Board Schematic](https://github.com/QuickLogic-Corp/quick-feather-dev-board/blob/master/doc/quickfeather-board.pdf)
* [EOS S3 Datasheet](https://www.quicklogic.com/wp-content/uploads/2020/12/QL-EOS-S3-Ultra-Low-Power-multicore-MCU-Datasheet-2020.pdf)
* [SensiML + Quickfeather](https://sensiml.com/documentation/firmware/quicklogic-quickfeather/quicklogic-quickfeather.html)
