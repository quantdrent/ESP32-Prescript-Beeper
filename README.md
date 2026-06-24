# ESP32-The-Index-Prescript

![License: MIT](https://img.shields.io/badge/License-MIT-green)

A physical device aiming to recreate the pager/beeper device rien used to recive prescript.
Feel free to do anything about this project but please give credits/attributions and if you want to feel free to tag me if u make it lol @quantdrent on tiktok

> This version is made for a 1.3 inch **(4 pins)** oled screen you will need to tweak the enclosure and the code if you wish to use other versions of the screen

## Pictures
not yet

## Required Materials
* **MCU**    : ESP32 C3
* **SCREEN** : 1.3 Inch oled screen SH110X
* **INPUT**  : 2x Touch capacitive switches (Standard micro switches work but case needs to be tweaked)
* **AUDIO**  : DF Player Mini & small speaker (optional)

`note: you need a sd card for mp3 audio`

3D files have been provided for the case of the beeper

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
