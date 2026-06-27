# ESP32-The-Index-Prescript

### NOTE THIS PROJECT IS STILL UNFINISHED! 
![License: MIT](https://img.shields.io/badge/License-MIT-green)

A physical device aiming to recreate the pager/beeper device rien used to recive prescript.
Feel free to do anything about this project but please give credits/attributions and if you want to feel free to tag me if u make it lol @quantdrent on tiktok

>  this is my 3rd ever arduino project please bare with my okay ish code, if you want to fork and make a better one feel free!
>  and some knowledge of programming a arduino/esp and soldering is needed.
## TODO lIST
>  I recommend not starting on anything here until most stuff are finished

- [x] Beeper/Pager case
- [x] Drawn Schematics
- [x] Prescript send via wifi
- [ ] Preloaded prescripts
- [ ] save preloaded prescripts via wifi send
- [ ] idk

## Required Materials
* ESP32C3 Super Mini [[AliExpress](https://www.aliexpress.com/item/1005007941259180.html)]
* 1.3 Inch OLED Screen SH1106 [[AliExpress](https://www.aliexpress.com/item/1005006862867338.html)]
* 2x Touch capacitive switches TTP-223 [[AliExpress](https://www.aliexpress.com/item/32964219843.html)]
* DF Player Mini [[AliExpress](https://www.aliexpress.com/item/1005006166800318.html)]
* Any small speakers that can connnect to the DF Player
* Micro SD card
* AWG 26 wires

>  You need a soldering iron, solder, and maybe some flux to connect them all.

I recommend using the 3MF file for printing, The results i got were by using a bambulab A1 0.4 nozzle with SUNLU pla+ 2.0.

## Required Libraries

- `Adafruit SH110X` (with their dependency)
- `Preinstalled ESP libraries`

`lookk at .ino for the full required libraries.`

## Instructions
1. Click the code button and click download ZIP
2. Unzip and open .ino file and upload it to esp32 c3
3. Wire it just like the schematic in pictures folder
4. Transfer all audio from audio folder to a sd card and hook it up to the DF Player
6. Assemble everything and there you go!

If you want to send prescript via wifi connect to the wifi larpmachine from ur phone/laptop/pc and type http://192.168.4.1/ on ur browser.

## Images
wip.
![alt text](https://raw.githubusercontent.com/quantdrent/ESP32-Prescript-Beeper/refs/heads/main/Images/wiring.png)

this project was inspired by Kritzkingvoid Prescript web project

https://kritzkingvoid.github.io/Prescripts/

SFX are from project moon for limbus company.
