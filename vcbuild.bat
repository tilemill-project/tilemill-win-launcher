rd /q /s Default
del build.sln
del common.sln
del TileMill.vcxproj
del TileMill.vcxproj.user
c:\Python27\python.exe gyp/gyp build.gyp --depth=. -f msvs -G msvs_version=2010
msbuild build.sln
copy Default\TileMill.exe ..\TileMill.exe
@rem copy Debug\TileMill.exe ..\TileMill.exe
