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

## September 25, 2025

Now that i managed to implement the input of the controller and a basic movement, we can move forward in two directions, starting to build an example level using LDtk and develop the logic to include it in my game or implement additional motions like a jump or double jump or a dash.

For tonigith I have decided to spend an hour in aseprite and add a couple more animations, maybe I will write a draft of gameplay document for a first level.

## September 26, 2025

Yesterday I worked out a jump animation and i think i feel ready to implement some collider related logic.

## September 27, 2025

In the end, I started looking some video tutorials about working with Tiled, in order to have a kind of level to work with.
So no coding for today.

## September 28, 2025

I am still working with tiled. I also reviewed the idea of using 16x16 pixel sprites, they are just too little, and decided to switch to a 32x32 sprite size.
I scaled all my animations to the new size and recreated the spritesheet.

## October 5, 2025

I had a tough week, with some personal issues that cause me a lot of stress. So i was not in the right mindset to proceed with the work.
Today I feel a bit more relaxed (even if tomorrow it is time to go back to work again). In any case, it is time to go back to work with a fresh mind.
I can proceed in two different directions:

- Import of a tiled map in the game and on screen rendering.
- Implementation of some collision logic.

Since it will be easier to implement the collision with some map elements to map to, I will start with the first point.
To import the map, I can either use an external library or implement it by myself.
I will start looking at some existing code to see how it is done, and then decide how to proceed (implementing my own version or using the existing logic).

A C library I was looking at for this task is a colleciton of headers known as "cute_headers": https://github.com/RandyGaul/cute_headers which includes both logic for collisions and tiled maps loding implementations.

## October 7, 2025

I have decided to use this library as it looks fairly complicated and it will save me a lot of time, also since it is a single header library, I can modify the code if necessary.

I need to put the loading logic of the map in some place, so for the moment I will use the Scene entity manager, and will test the loading logic in the CreateScene function. The render scene will instead take the responsibility to render the tiles based on the loaded map.

The map is loaded, tomorrow we can work on the rendering of the tiles on the screen.


## October 8, 2025

Today we will work on the map rendering. We will not implement any scrolling at least for the moment, the prototype will be in Rick Dangerous style, static screens. To render the scene we need basically to load the tileset in an atlas and create a tile for each of them by converting the tile index in the target tile by rendering only the required tile, indexed by some wrap-around logic.

I ended up adding a new entity to model the tiled map. It will support the creation, rendering and destruction of a tilemap.
THe renderer can be done tomorrow.


## October 9, 2025

Rendering of a simple tiled map has been completed. 

## October 11, 2025

Today I would like to focus on the collissions and in putting some additional information in my tiled map. I cna breakdown this part in three steps. 

- Add a start position object to the map, so that the main character will be rendered over there in the first place. [DONE]
- Add a collision shape to the tiles in the tileset and read them from the code [DONE]
- Add some gravity to the world, meaning the carachter will fall until he founds some ground (and, why not, a fall animation)
- Implementing the collision logic so that when the character hits the ground, the fall is stopped.

## October 14, 2025

### Today's Menu
Yesterday I spent some time looking at the ECS pattern, the Entity-Component-System, which looks particularly fit to the work I am currently doing. 
Before to apply the pattern though, I would like to test the collisions system.
I have my falling sprite and a map with a few collision shapes in place. 
I can breakdown this work in three steps:

- attache a collider to my player sprite [DONE]
- read these shapes while loading a tilemap and create possibly an array of colliders [DONE]
- test the character is not allowed to move if there is a collition hit condition.

### Wrap-up
I have completed the first two tasks, we will implement the collision detection tomorrow to test the logic.
Have a good night.

## October 16, 2025

### Todays' Menu
I am doing some small changes to adopt some naming convention in order to identify pointers from normal variables more easily. Nothing fancy. Before to proceed with the testing of the colliders, i decided to add a small function to render the collider shape on the screen, this will be helpful in troubleshooting the colliders implementation.

- Implement a draw function that can take a collider in input and render it in some color on the screen, as a set of lines.
- Enable the possibility to display or hide the debug info pressing a key on the keyboard.

## October 17, 2025

### Todays' Menu
I have only spent 20 minutes on the project as tonight I was pretty tired.
The debug drawing logic was completed by adding a switch to display/hide the debug info rendering.
I added this switch in a dedicatged struct inside of the appstate entity which is more or less available in any call.

So far, today, I completed the implementation of a basic collision logic between the actor and the tileset colliders, it looks to work as I expect for the moment.

Next step will be handling the reaction force from the obstacle in the evaluation of the next position (to stand on a platform, or for example to swim in a fluid or to float on a baloon...). This will be handled with a force.

Also I may think of porting this to use Box2D, but that is a possible task for next week.


## October 17, 2025

### Todays' Menu


## Later

- Implementation of a precise delta time logic which does not depend on the system framerate.
- [DONE] Controlling the sprite with the keyboard and the controller
- Mouse controls and projection on the screen
- Adding a background
- Move the init appstate logic in a different place as in main it is becoming too crowded, next to appState we may have other similar entities
  like gameState, playerState, rendererState, audioState. and so on. that may be useful to exchange data across different components. For example, the rendererState is a better place to hold the textures pool.
- We may also want to include a defines file esplicitly to create the constants, as is the GAMEPAD_DEADZONE
- Create a Scene descriptor file so we do not need to hardcode all the inputs needed to create a scene.
  - Eventually, it will have a reference to some Sprite Descriptor file where we can identify in a text format the sprite properties, animations and so forth.
- The texture pool is cleaned once when a scene is destroyed, maybe we can fine tune it to destory individual textures when needed, 
  to make better use of the memory. To do that, we may need to use another type of data structure so we can remove a texture from the list whatever is its position.
- rather than a linked list, for some data structures like arrays of colliders and in general for ECS approach it should be better to  
  use arrays of a given lenght.
