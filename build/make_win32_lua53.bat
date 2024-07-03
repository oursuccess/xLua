mkdir build32 & pushd build32
cmake -G "Visual Studio 17 2022" -A Win32 ..
popd
cmake --build build32 --config Release
md plugin_lua53\Plugins\x86
copy /Y build32\Release\xlua.dll plugin_lua53\Plugins\x86\xlua.dll

REM copy /Y build32\lua-debugger-hook\Release\cpl_hook.dll plugin_lua53\Plugins\x86\cpl_hook.dll
pause