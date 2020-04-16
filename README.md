# Linux Bootloader Application
This is an application that allows interacting with the MPLABX harmony USB HID bootloader from linux. It's functionality has been verified using a PIC32MZ1024EFG064. 

It is based on the PIC32UBL.exe Microchip PC Application. Copyright (c) 2013 Microchip Technology Inc. All rights reserved.

Upon running the bootloader application, the following sequence of events will occur automatically, if a step fails the program is aborted:
* Initialization: connect to USB HID device & load in hex file
* Erase: erases firmware user application flash memory
* Program: programs firmware users application flash memory using hex file
* Verify: CRC check is done on firmware application flash memory
* Jump to application: firmware bootloader stops running and jumps to execute the firmware user applicaiton

## Dependencies
* libusb
* udev
* pthread
* hidapi (contained in source tree)

## Build
```
make
```

## Install
Replace DESTDIR as you see fit:
```
sudo make install DESTDIR=/opt/pic32ubl
```

## Uninstall
Replace DESTDIR as you see fit:
```
sudo make uninstall DESTDIR=/opt/pic32ubl
```

## Issues
If you are having issues getting the program to work, check the following:
* Ensure USB HID device is connected and recognized by system
* Permissions set accordingly
* Hex file is using unix EOL