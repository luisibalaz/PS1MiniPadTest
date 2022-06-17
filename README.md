# PS1MiniPadTest

Hi, this is a small sized, controller tested app, currently is for NTSC-U consoles.
It has only been tested on emulators and on some real hardware. More tests are required. This software comes with no warranty.

---

## Screenshots:

![Screenshot 1: Only digital pad in port 0](/screenshots/ss1.png)

![Screenshot 1: Digital in port 0, analog in port 1](/screenshots/ss2.png)

![Screenshot 1: Digital in port 0, analog in port 1, in action](/screenshots/ss3.png)

![Screenshot 1: Only Digital Pad in port 1](/screenshots/ss4.png)

---

## Features:
* Fully graphical (with analog joysticks values in decimal for precise readings, 0-255 range).
* Works with both Digital and Analog controllers, with support to switch bewteen Analog and Digital modes.
* It is 32KB compiled (less than the 122KB FreePSXBoot's limit) so it can work without the need to burn a CD or get an ODE. Just need a way the execute FreePSXBoot on your PS1, namely, an original Memory Card, a PS1 MemCardPro, or a cheap Pi Pico with the [PicoMemCard](https://github.com/dangiu/PicoMemcard) firmware, etc.
* (**Supposedly working after fix. More testing is recommended**) Should work "on the fly". If I disconnect the controller while testing, it should stop displaying it. If I reconnect it, it should display it again.

## To do:
* Fix weird texture colors (easy, I'm just lazy).
* Change joysticks ranges from [0,255] to [-128,128]. Also a better display layout.
* Add support for other regions. At least PAL.
* Probably fix button presses of the Analog Stick (Not the Analog Pad). However, I don't have one.
* Add rumble support.
* Automatically switch to Analog Mode on Analog Pads.
* Code rewrite to make it cleaner.

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