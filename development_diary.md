# Development Diary

## September 3, 2025

I am looking for some lemmings animation style look.
Spent some time in replicating original lemmings sprites in aseprite and ended up with a pixel-perfect replica of the walk animation.

I got some better feeling on how they got that result with so few pixels, and tried myself to animate a character on my own.

I made a quick animation cycle of a monkey, which I plan to animate inside of this project in one of the next days.

Just for fun.

## September 6, 2025

In the last two days i spent some time to refine the build procedure with CMake and did some first experiments in loading textures and rendering them on a screen.

The first obstacle I encontered was the complexity of working with cmake stuff, and setting it up in VSCode. Thankfully we can rely on some help from AI. I asked Gemini to provide myself a quick 20 minutes long tutgorial of how to setup a project in VSCode using cmake. Obviously I asked him not just to provide me the steps, but also to give me some background theory in the process.

In about 20 minutes I was good to go.

I faced a few more issues when I needed to load a an image in PNG format, as I needed obviolusly to work with images with an alpha channel. I needed an additional module for SDL, called SDL_Image. I went the "hard" way, downleaded the repository compiled it on my local machine. It was not that hard since i already had a decent understanding of the basics of working with cmakelists.
I linked the new library to the project and also this part was resolved.

I finally reached the stage where i can display a 2D texture on the screen, following some examples from the SDL website itself.

Just for fun
