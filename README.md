# ESP32-The-Index-Prescript

### NOTE THIS PROJECT IS STILL UNFINISHED! 
![License: MIT](https://img.shields.io/badge/License-MIT-green)

A physical device aiming to recreate the pager/beeper device rien used to recive prescript.
Feel free to do anything about this project but please give credits/attributions and if you want to feel free to tag me if u make it lol @quantdrent on tiktok

>  this is my 3rd ever arduino project please bare with my okay ish code, if you want to fork and make a better one feel free!

## Pictures
wip.
![alt text](https://raw.githubusercontent.com/quantdrent/ESP32-Prescript-Beeper/refs/heads/main/Images/wiring.png)

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

`note: you need a sd card for mp3 audio`

print beepercase.stl with 0.16 with 70% infill for the case and 25% for the lid or just use the beepercase.3mf file

## Required Libraries

- `Adafruit SH110X` (with their dependency)
- `Preinstalled ESP libraries`

`lookk at .ino for the full required libraries.`

## Instructions
1. Click the code button and click download ZIP
2. unzip and open the IndexDevice Folder
3. Open .ino file and upload it to esp32 c3
4. wire it just like the schematic in pictures folder
5. load up ur micro sd card and put scramble.mp3 to the sd card
6. assemble everything and there you go! go to the wild and larp as the black silence

this project wouldnt be possible without Kritzkingvoid Prescript web project
https://github.com/Kritzkingvoid/Prescripts https://kritzkingvoid.github.io/Prescripts/
