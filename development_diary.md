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

After some additional reasarch, I have decided to ditch the idea of using the Chipmunk2D physics engine and go instead for Box2D.
Version 3 has been converted in C and should be farily simple to integrate in my application.
Before to proceed wioth the next steps in relation to sprite movements, I think it is time to take a quick tour about Box2D to understand how it can be integrated in the engine.

Depending on the outcome of this activity, we may take different decisions around the design of the physics system.


## September 21, 2025

Today i would like to finally imeplement the movement, even if i have a feeling i need to imlement first some other stuff to 
get to the point. 

In particular, I believe it would be time to start working on a scene, possibly loaded from a file generated by some tiledmap tool.
In the scene we can load the initial positions of each actor of the scene, background objects and so on. 

This will later also allow me to implement a camera movement in case we want to implement some scrolling algorithm and eventually 
parallax scrolling.

What do we need for a simple scene management implementation?
It will contain an array of actors, with one special actor which will be the player, i suppose.
We can therefore have one actor and then an array of tiles maybe to render the background.
We may have one array per parallax level or even a multidimensional array for the background.
We need therefore a datastructure to define a backgroun layer with the z-position and the scrolling speed.

Each layer will basically work as a group of textures rendered.

I also shared my project with gemini to get some recommendations and a code review.
He suggested me to refactor the sprite class to hanly have one texture in memory instead of many small ones (done) and revisit the logic for the time step to 
have a time delta.

I found out there is a problem in the texture creation and i will need to create a texture manager soon.

## September 22, 2025

I have completed the development of a simple texture manager component.
It is based on the introduction of a texture pool entity, which is nothing but a linked list of textures.
The resposibilities of this components will be:

- to maintain and store a linked list of texture pointers 
- return a texture by name or create a new one depending on the availability or not of the requested texture in the memory
- release the memory once the application is closed.

## September 23, 2025

Let's try to implement a basic controller support. 
The idea is to move my character to the left, to he right and keep it in the Idle position when no input is provided.

I need the Player to be accessible from the main loop, in order to push an event to it when an event is received.
TO do so, I have to add it to the AppState struct, at least for the moment.

The AppState is emerging as a struct used to hold static data, but it may become too large with time.
It is possible we need to add other structs similar to this one but specialized to hold some other type of information, like
level, game objects and so on. We can think about this later.

In the end i have decided to use, at least fo the moment, the player reference accessible from the scene.

## September 24, 2025 - 8.23 PM

Yesterday I have completed the implementation of the controller support. Needless to say, it does not work.
When moving the joypad stick, the character moves erratically, with no clear explanation.
I had to check with Gemini the reason of the issue, which looks rooted in a complete misunderstanding of the events cycle.

The error was in the inclusion of bot the event processing (move stick to the left for example), with the update of the characters position.

Why is that a problem? Looks like the update function being calle dalso for many other events caused the update of the position
to depend directly on the number of events in the queue, which is obviously not at all constant.

I can fix it by decoupling the reading of the controller input from the character update, which is expected to happen once per frame to have consistency.
Applying the equation of movement the character position will be updated as follows

   X' = X + (dx/dt) * dt
   Y' = Y + (dy/dt) * dt

so if we set a given constant velocity, the next position will be the previous one plus the velocity multiply by the delta time.
Today i will process the decoupling of the update function from the input polling method and see if this is fixing the issue.

...1 hour and a few Gemini prompts later...

I think that before to ask the AI i should better read the documentation, i had several issues and most of them where due to the fact that
while all my logi was essentially correct, i was not checking which one of the axis i was reading, so i was mixing up the X axis and the Y axis.

The motion is now correct and I was able also to handle the animations properly.


## Later

- Implementation of a precise delta time logic which does not depend on the system framerate.
- Controlling the sprite with the keyboard and the controller
- Mouse controls and projection on the screen
- Adding a background
- Move the init appstate logic in a different place as in main it is becoming too crowded, next to appState we may have other similar entities
  like gameState, playerState, rendererState, audioState. and so on. that may be useful to exchange data across different components. For example, the rendererState is a better place to hold the textures pool.
- We may also want to include a defines file esplicitly to create the constants, as is the GAMEPAD_DEADZONE

