mkdir build32 & pushd build32
cmake -G "Visual Studio 17 2022" -A Win32 ..
popd
cmake --build build32 --config Debug
md plugin_lua53\Plugins\x86
copy /Y build32\Debug\xlua.dll plugin_lua53\Plugins\x86\xlua.dll
copy /Y build32\Debug\xlua.pdb plugin_lua53\Plugins\x86\xlua.pdb
pause