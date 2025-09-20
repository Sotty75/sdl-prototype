# Development Diary

## September 3, 2025

I am looking for some lemmings animation style look.
Spent some time in replicating original lemmings sprites in aseprite and ended up with a pixel-perfect replica of the walk animation.

I got some better feeling on how they got that result with so few pixels, and tried myself to animate a character on my own.

I made a quick animation cycle of a monkey, which I plan to animate inside of this project in one of the next days.

## September 6, 2025

In the last two days i spent some time to refine the build procedure with CMake and did some first experiments in loading textures and rendering them on a screen.

The first obstacle I encontered was the complexity of working with cmake stuff, and setting it up in VSCode. Thankfully we can rely on some help from AI. I asked Gemini to provide myself a quick 20 minutes long tutgorial of how to setup a project in VSCode using cmake. Obviously I asked him not just to provide me the steps, but also to give me some background theory in the process.

In about 20 minutes I was good to go.

I faced a few more issues when I needed to load a an image in PNG format, as I needed obviolusly to work with images with an alpha channel. I needed an additional module for SDL, called SDL_Image. I went the "hard" way, downleaded the repository compiled it on my local machine. It was not that hard since i already had a decent understanding of the basics of working with cmakelists.
I linked the new library to the project and also this part was resolved.

I finally reached the stage where i can display a 2D texture on the screen, following some examples from the SDL website itself.

## September 16, 2025

Today I would like to spend some time animating the simple monkey spritesheet. 

Before to write any code, let's see more or less what should be the approach:
By looking into the documentation I can use a function to blit the spritesheet surface (which is a buffer of pixels in memory) inside a target surface (representing a frame). 
The function is the following one [SDL_BlitSurface]https://wiki.libsdl.org/SDL3/SDL_BlitSurface

With this surface I can update a texture that i use to render the character using the SDL_LockTexture and SDL_UnlockTexture functions.
I therefore can maintain a list of surfaces, one per each frame, and update the texture on each iteration frame.

## September 18, 2025

Today, I spent some time refininf the sprites module. Finally implemented the loading of the sprite animation data from a spritesheet with generic size. The indexing of each sprite in the spritesheet is made using a simple "wrap-around" technique mased on a module and division operations.

Also I did setup the basic logic for the update sprite animation, where we actually animate the sprite on the screen given a specific time step, which we can set at about 125ms. Next time i will implement this logic where the surface of the textrue will be udated using the lock texture function.

## September 19, 2025

Today, I worked out the first sprite animation routine. I had to modify the initial strategy of blitting different frame surfaces on the target texture 
of the GPU as Gemini recommended instead, for performance reasons, to create a texture per frame.
Once done that, it was simply a matter of presenting a different texture on the screen at each frame.
The frame rate in this case is passed to the CreateSprite function and so it can be adjusted easily.

Next time I would like to on implementing some movement, for example using the Joypad as an input, and eventually start working on a collision detection routine.

## September 20, 2025

Let's move on in writing some code to finally move the sprite on the screen. 
I will implement a struct to bring to a higher level the information related to the position of an actor and eventually the speed, and obviously this struct will be called actor.

We need a file specifically to handle this data. So the actor will represent a moving object on the screen.
Since I will use some vector calculations to handle the position and velocity of objects on the screen, I looked for a library for this particular purpose and
decided to select the cglm library, providing me a set of data types optimized for vector and matrix calculations.
I am going to consider also including the Chipmunk2D physics engine that looks particularly useful in adding physics for the game.

In the end I spent most of my time implementing and testing the Gamepad support for the game, as I need some input to drive the movements of the character.


## Later

- Controlling the sprite with the keyboard and the controller
- Mouse controls and projection on the screen
- Adding a background


