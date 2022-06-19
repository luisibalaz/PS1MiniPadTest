# PS1MiniPadTest

Hi, this is a small sized, controller tested app, currently is for NTSC consoles.  
It has only been tested on emulators and on some real hardware. More tests are required.  
This software comes with no warranty.

---

## Screenshots:

![Screenshot 1: Only digital pad in port 1](/screenshots/ss1.png)

![Screenshot 2: Digital in port 1, analog in port 2](/screenshots/ss2.png)

![Screenshot 3: Only analog in port 2](/screenshots/ss3.png)

---

## Features:
* Fully graphical with a clean and simple layout.
* Works with both Digital and Analog controllers, with support to switch bewteen Analog and Digital modes.
* Analog joysticks values are visible in decimal for precise readings, on the [-128,127] range.
* It is **28KB** compiled (less than the 122KB FreePSXBoot's limit) so it can work without the need to burn a CD or get an ODE. Just need a way the execute FreePSXBoot on your PS1, namely, an original Memory Card, a PS1 MemCardPro, or a cheap Pi Pico with the [PicoMemcard](https://github.com/dangiu/PicoMemcard) firmware, etc.
* (**Supposedly working after fix. More testing is recommended**) Should work "on the fly". If I disconnect the controller while testing, it should stop displaying it. If I reconnect it, it should display it again.

## Supported controllers:
* DualDigital
* Analog Flightstick (more testing needed)
* DualAnalog (Flightstick mode needs more testing)
* DualShock (no rumble support yet)

## To do:
* Add support for PAL.
* Support for every controller made for the PS1.
* Probably fix Flightstick button presses. However, I don't own one.
* Add rumble support.
* Automatically switch to analog Mode on analog pads.

---

## Build it yourself

Use PSn00bSDK. Add to ``PATH`` the folder where the PSn00bSDK binaries are, and create a new environment variable ``PSN00BSDK_LIBS`` as the steps 6 and 7 from the [installation instructions](https://github.com/Lameguy64/PSn00bSDK/blob/master/doc/installation.md) from PSn00bSDK say.

Use the CMake preset included, changing the ``"generator"`` to whatever yours is (``"Unix Makefiles"`` if using ``cmake`` from linux as I use, for example). Be sure to change the ``"PSN00BSDK_TC"`` entry to your MIPSEL binary files directory, as steps 2 and 3 from the [installation instructions](https://github.com/Lameguy64/PSn00bSDK/blob/master/doc/installation.md) from PSn00bSDK say

To configure presets, do (once):
```
cmake --preset default .
```
and to build:
```
cmake --build ./build
```

---

### Contact

Email: <luisibalaz@gmail.com>  
Github: <https://www.github.com/luisibalaz>
Discord: 