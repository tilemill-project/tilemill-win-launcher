# tilemill-win-launcher

Simple exe to launch TileMill.

Catches and displays errors in native Windows modal dialogs.

## Requires

 * gyp/python
 * Visual Studio 2010

## Usage

Create a top level directory to hold TileMill and this project.

    mkdir tilemill-build

Checkout Tilemill and build it:

    git clone https://github.com/mapbox/tilemill
	# see platforms/windows/README.md for instructions

Checkout this project into a subdirectory and compile:

    git clone https://github.com/mapbox/tilemill-win-launcher
	cd tilemill-win-launcher
	vcbuild

Test the launcher:

    cd ..\
	TileMill.exe

