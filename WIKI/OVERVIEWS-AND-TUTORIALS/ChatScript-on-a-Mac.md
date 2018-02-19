#Chatscript on a Mac

> © Bruce Wilcox, gowilcox@gmail.com brilligunderstanding.com

> Revision 1 - 7/16/2017 - todd@kuebler.org


## Installing

Life is not as easy for Mac users since Bruce doesn't have a mac - therefore a build of the mac binary is not part of every release.

However ChatScript builds and runs just fine on any version of OSX ( now called MacOs ) and there are many of us using ChatScript on macs with no problem.  The unix underpinnings of MacOS make it a great development environment and ChatScript generally works the same way as under Linux.

### Method 1 - build from command line

After unzipping into a folder or cloing from github directly, you need to compile the src in SRC. 

The example/default command line compile is located in NON-WINDOWS NON-C/MAC/MacCompile.sh.  Read the compile-on-osx.html in that directory for more instructions. 

### Method 2 - use outdated binary in the BINARIES folder
Alternatively you can use the binary in BINARIES/MacChatScript, but it is at least one version behind unless you are grabbing chatscript directly from github.   
 
### Method 3 - use the included Xcode project to build
There is also an Xcode project in the 'NON-WINDOWS NON-C/Xcode' directory  that you can launch and build your own binary with.  This is probably the prefered method for advanced users.

### Method 4 - cd to the SRC directory and type 'make'

Essentially this is identical to the linux compile instructions.  To do this you have to have XCode and the developer command line tools installed on your Mac.  This produces the higher performance EVSERVER binary for your architecture, but if there are errors it's largely unsupported.  


## Prerequisites for compiling via Xcode or 'make'

####(included) curl library
This is actually part of the default mac ecosystem and is already included in the Xcode project under 'TARGETS/MacChatScript/Build Phases/Link Binary with Libaries' ( navigation path in xcode editor )

#### (optional) mongodb libraries
These can be aquired via brew ( http://brew.sh ) The command is:

    brew install mongodb mongo-c-driver
#### (optional) postgress libraries
These can also be aquired via brew.  The command is:

    brew install postgresql

## brew errata

Follow the instructions at http://brew.sh to install and use brew.

In general you can find the packages talked about in the is document with:

    brew search mongo
    brew search postgres

You need the mongodb, mongo-c-driver and postgresql modules.

***Important:*** The version of MacOS/OSX you are running determines what version of brew you run and what packages are available.  The packages for your particular macos version might be different than above.

## Configuring Xcode for Mongo and/or Postgres

More details coming Soon - essentially it involves installing the libraries then adding them to the list of libs that get included.  The setting is under build phases.

## Running

Running Chatscript is as simple as executing the binary you have from the instructions above.  Follow the same instructions as the main manual similar to Linux.  It boils down to:

* Launch the `Terminal` application. It's found in /Applications/Utilities.  note: ⌘[space] launches spotlight, just type terminal in the window that pops up to find it and click on it to launch it.
* In `Terminal` `cd` to the directory that contains chatscript. example: `cd src/Chatscript`
* Run your binary using the linux instructions, ie

    BINARIES/MacChatScript local

## Getting Mac specific Help

The best place for getting help is probably http://chatbots.org in the Chatscript forum. There are a number of friendly and helpful users there that answer mac specific question.   The github project is primarily for people contributing to chatscript code, documentation or reporting bugs.