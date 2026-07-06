# ESP32-The-Index-Prescript

### DISCONTINUED CHECK [THIS](https://github.com/quantdrent/Rien-Prescript-Beeper) INSTEAD
https://github.com/quantdrent/Rien-Prescript-Beeper

### NOTE THIS PROJECT MAY HAVE BUGS! 
![License: MIT](https://img.shields.io/badge/License-MIT-green)

A physical device aiming to recreate the pager/beeper device rien used to recive prescript from limbus company canto 9.

The main goal for this project was to make a budget index proxy / rien device. This project is under the MIT license so you are free to do anything with this project (this includes selling these stuff) but dont try to gatekeep lol and also if you decided to make this feel free to show me lol i wana see it. @quantdrent on TikTok, Twitter, and Discord.

>  this is my 3rd ever arduino project please bare with my okay ish code, if you want to fork and make a better one feel free!

>  **some knowledge of programming a arduino/esp and soldering is needed.**
## TODO lIST

- [x] Beeper/Pager case
- [x] Drawn Schematics
- [x] Prescript send via wifi
- [X] Preloaded prescripts
- [X] save preloaded prescripts via wifi send
- [ ] Rework the case to make it easier to assemble (maybe)

## Required Materials
* ESP32C3 Super Mini [[AliExpress](https://www.aliexpress.com/item/1005007941259180.html)]
* 1.3 Inch OLED Screen SH1106 [[AliExpress](https://www.aliexpress.com/item/1005006862867338.html)]
* 2x Touch capacitive switches TTP-223 [[AliExpress](https://www.aliexpress.com/item/32964219843.html)]
* AWG 26 wires

>  You need a soldering iron, solder, and maybe some flux to connect them all.

I recommend using the 3MF file for printing, The results i got were by using a bambulab A1 0.4 nozzle with SUNLU pla+ 2.0.

(this is basically less than 8 bucks bro)

## Required Libraries

- `Adafruit SH110X` (with their dependency)
- `Preinstalled ESP libraries`

`lookk at .ino for the full required libraries.`

## Instructions
1. CLick the code button and download zip. 
2. Solder the components like the provided schematics inside the images folder or scroll down. (you need to desolder the 1.3 oled screen goodluck)
3. Open the IndexProxy folder and open  the IndexProxy.ino
4. you can edit the prescripts.h to add more preloaded prescripts or check step 7
5. Connect your esp32c3 and upload the code
6. check if everything works and put everything in the case
7. Connect your device (Phone/Laptop/Tablet) to the wifi **'LarpMachine'** or what ever you named it and open your browser and open **http://192.168.4.1/** and there should be a gui to send live prescripts or add more saved prescripts

## Images
wip.
![alt text](https://raw.githubusercontent.com/quantdrent/ESP32-Prescript-Beeper/refs/heads/main/Images/wiring.png)

this project was inspired by Kritzkingvoid Prescript web project

https://kritzkingvoid.github.io/Prescripts/
