# compiles and executes for windows with MinGW
# run from project root directory
param([bool]$recompile=$true)
if ($recompile) {
    if (Test-Path -Path './build/bin/CMakeSFMLProject.exe') {
        Remove-Item ./build/bin/CMakeSFMLProject.exe
    }
    cmake -S . -B build -G "MinGW Makefiles"
    cmake --build build --config Release
}
if (Test-Path -Path './build/bin/CMakeSFMLProject.exe') {
    ./build/bin/CMakeSFMLProject.exe
}