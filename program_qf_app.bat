set PATH_TO_TINYFPGA=quickfeather\source\qorc-sdk\TinyFPGA-Programmer-Application
set PATH_TO_APP_BIN=quickfeather\source\app\GCC_Project\output\app.bin

if not "%~1"=="" set PATH_TO_APP_BIN=%~1

python "%PATH_TO_TINYFPGA%\tinyfpga-programmer-gui.py"  --mode m4 --m4app "%PATH_TO_APP_BIN%"

pause