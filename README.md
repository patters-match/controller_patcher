[![Build Status](https://travis-ci.org/Maschell/controller_patcher.svg?branch=wut)](https://travis-ci.org/Maschell/controller_patcher)  

# What is in this controller_patcher repository
These files are the magic behind tools like HID to VPAD and can used to use your USB HID Device on your WiiU console.

# How to create config files
Detailed information about creating config files and adding support for more controller can be found in the [wiki](https://github.com/Maschell/controller_patcher/wiki)

# Where can I find config files
Configfiles for all controllers are collection in [this repository](https://github.com/Maschell/controller_patch_configs)

## Logging usage
To able to use the logging change the "DO_LOGGING" parameter in the Makefile.

# Compiling
You need to install all dependencies first!

Install this static library into your wut folder via: 

```
make install
```

Link the application with

```
-lcontrollerpatcher
```

You also need to add the include path to your Makefile. Example:

```
export INCLUDE	:= [...] -I$(WUT_ROOT)/usr/include
```

# Dependencies
- [wut](https://github.com/decaf-emu/wut) (WUT branch) for common functions.

# Example implementation

### How to "install" it
TODO!

```
ControllerPatcher::Init(NULL); //No custom configuration
ControllerPatcher::disableControllerMapping();
ControllerPatcher::startNetworkServer();
```

```
ControllerPatcher::DeInit();
ControllerPatcher::stopNetworkServer();
```

# Credits:
- Maschell  
- FIX94 - huge thanks to him and his initally created gc-to-vpad. Was a motivation and base to start all this
