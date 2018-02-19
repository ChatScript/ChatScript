This folder contains binaries Linux*  that run on Linux.  These are all release builds.
All OTHER files are for windows.

ChatScript.exe is the basic windows executable.  Currently always a debug build (using the pdb file).
ChatScriptpg.exe is one that can talk with postgres, while ChatScriptMongo talks to mongodb.
Currently always a debug build.
Loebner.exe runs the Loebner protocol if you are trying to make a Loebner contest entry.

While you may not need to install any more dll's (they should be here), if you find 
you need a microsoft msxxxx.dll installed, the vcredist2013 executables will install
them for 64-bit and 32-bit machines.

The libpq, ssleay32, libintl, libeay32 dlls are used if you run a postgres build  ChatScriptpg.exe.
The libcurl(release) or libcurld(debug) dlls are used in either build when #define JSON is on (default).

For mysql, to rebuild the mysqlclient.lib see https://github.com/PyMySQL/mysqlclient-python/wiki/Building-mysqlclient.lib-on-Windows-with-VS2015