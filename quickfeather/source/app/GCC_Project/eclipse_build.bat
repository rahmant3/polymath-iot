:: Created to workaround build failures due to dependency files in Windows.

set WORKSPACE=%~1

call del /S /Q "%WORKSPACE%\..\output\depend\*.d"
make -C "%WORKSPACE%/../" %2 %3 %4