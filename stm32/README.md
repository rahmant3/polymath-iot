# polymath-IoT (stm32)
This repo contains the source code for polymath-iot (STM32 slave controller portion).

## Directory Structure
- PolymathSlave_STM32L476 -- Contains the source code for this project
- PolymathSlave_STM32L476/App -- Contains the application specific code created by us.

## Computer Setup (Command Line)
This project requires the following tools:

- Windows 10 workstation

- Users must download the BSEC library files from Bosch: https://www.bosch-sensortec.com/software-tools/software/bsec/ 
  - The following files must be placed into the “PolymathSlave_STM32L476\App\bme680” folder.
    - bsec_datatypes.h
    - bsec_interface.h
    - bsec_integration.h
    - bsec_integration.c
    - libalgobsec.a (from normal_version\bin\gcc\Cortex_M4)

- STM32CubeIDE is used as the IDE: https://www.st.com/en/development-tools/stm32cubeide.html

## Build

In order to build the project, import the project into STM32CubeIDE like so:

### Configure your workspace

1. Open STM32CubeIDE, selecting this stm32 folder as the root workspace.

2. Go to File -> Import.

3. On the Import window, select **General -> Existing Projects into Workspace.**

4. Add "PolymathSlave_STM32L476" to your workspace.

### Build and Upload

5. To build the project, go to **Project -> Build All.**

6. To upload the project, with the Nucleo Board connected, go to **Run -> Run.**


## Useful References
* [STM32 Nucleo-64 User Guide (UM1724)](https://www.st.com/resource/en/user_manual/dm00105823-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf)
* [STM32L47xxx Reference Manual (RM0351)](https://www.st.com/resource/en/reference_manual/dm00083560-stm32l47xxx-stm32l48xxx-stm32l49xxx-and-stm32l4axxx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
