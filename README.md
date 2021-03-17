# KitchenTimer
Nice Kitchen Timer using LilyGO TTGO T-Display

You can set hours/minutes or minutes/seconds.
In the Configuration.h you can choose to use a rotary enconder (recommended) or 4 external Buttons, set colors, timing, language, ...

In the first version I had no separat power switch and just set the ESP32 to deepsleep.
The display draws 300µA in deepsleep which is pretty much for the battery but a simple solution. 
But the main problem is that the connected audion amplifier draws another 4-5mA. Hence the battery is empty after few days or less.
So at least the amplifier must be switched off, e.g. by a transistor.
I decided to install the full solution: a small additional circuit switches off the battery completely.
Charging via USB is still possible when timer is switched on.

Usage:

Switch on by pressing the rotary button.
After power on the timer is in stopped state.
Here you can set the time by rotaty encoder. Select setting left or right part of colon with the upper button of the TTGO display.
Select between Minutes/Seconds or Hours/Minutes with the lower button. The selection is shown in the header and the color of the time digits.
- Minutes/Seconds mode: 0 to 99 minutes can be selected, the time digits are green
- Hours/Minutes mode: 0 to 99 hours can be selected, the time digits are cyan

Start the timer by pressing the rotary button.
Time is counting down until 0:00 anf the progress bar gets smaller.
At the end it rings. 
If you don't press any button it rings again every minute until a button is pressed.
Pressing a button resets the time to start.

The timer can be paused by pressing the rotary button.
Pressing the button again continues.
The buttons of the display reset the timer.

There is no power off button.
Power is switches off automatically after 30 seconds of inactivity, except during countdown.

The following parts are needed:
- LilyGO TTGO T-Display
- rotary button (2-step or 4-step type is possible, see configuration.h)
- a small audio amplifier for 3-5V supply
- a small speaker
- a chargable Lithium battery 3.7V, e.g. 500-1000mAh
- a small DIY printed board an electrinic parts according the attached schematic

Case is provided for 3D printout.

![Kitchen_Timer_4_small](https://user-images.githubusercontent.com/76703830/111516757-8d25cf00-8754-11eb-9f6a-f4aeaa255936.JPG)

The 3D print exploded:

![Kitchen_Timer_1](https://user-images.githubusercontent.com/76703830/111516946-ccecb680-8754-11eb-95d3-bf93fe14d79f.png)
![Kitchen_Timer_2](https://user-images.githubusercontent.com/76703830/111516956-d0803d80-8754-11eb-8cf8-802b473a5812.png)
![Kitchen_Timer_3](https://user-images.githubusercontent.com/76703830/111516958-d24a0100-8754-11eb-9978-d97ee7e021d2.png)
