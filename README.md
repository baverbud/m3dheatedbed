# m3dheatedbed
DIY M3D heated printer bed

Use at your own risk. Licensed under GPLv3.

Contents:
* app: Windows app to control temperature
* circuit: Schematic and PCB for the logic board. Use Kicad to open.
* firmware: Firmware for the atmega328p on the logic board. Uses atmel studio, but should be compatible with the Arduino toolchain.

Note that the firmware uses the Arduino PID Library at https://github.com/br3ttb/Arduino-PID-Library, which is licensed under the GPLv3 at the the time of this writing.

For a copy of the GPLv3, please see the LICENSE file. 
