## TODO LIST

### Code TODOs (from source comments)

- **Implement a scene descriptor file** - The scene descriptor will provide the list of spritesshets, the reference to the tilemap, the actors with information required to load each actor, and other scene specific data. It will be implemented using json format (similarly to the one we use for sprite animations)
- **Initialize the scene using the scene descriptor** - Initialize scene, tilemnap, actors, spritesheets from the scene descriptor file.
- **Fill the gpuSpritesInfo array on update** - The array must be filled in order to upload significant data to the GPU
- **Move actor SSBO upload out of render** — The storage buffer upload in `SOT_GPU_RenderActors` should be moved to the update phase. ([sot_scene.c:246](src/sot_scene.c#L246))
- [DONE] **Upload atlas to GPU** — Atlas upload step needs to be handled properly. ([sot_actor.c:33](src/sot_actor.c#L33)) - 
- [DONE]  **Initialize sprite rendering pipeline separately** — Move sprite pipeline init out of actor creation, probably into its own file. ([sot_actor.c:32](src/sot_actor.c#L32))
- **Remove Direction field from actor** — Make direction a property of the animation/action instead of the actor struct. ([sot_actor.c:19](src/sot_actor.c#L19))
- **Extract animation update logic** — Move animation update logic to a dedicated `UpdateAnimation` function, invoked from `UpdateActor`; rendering should happen after sprite state is updated. ([sot_actor.c:170](src/sot_actor.c#L170))
- **Load other actors in scene** — Scene loading currently only handles the player; support loading additional actors from scene data. ([sot_scene.c:54](src/sot_scene.c#L54))
- **Implement debug info for sprites** — The debug overlay flag is checked but no sprite debug info is drawn. ([sot_scene.c:299](src/sot_scene.c#L299))

### Architecture TODOs

- **Implement `SDL_AppQuit` cleanup** — The quit callback is empty (`return;`). GPU resources, textures, buffers, and scenes are never freed, causing memory leaks. ([main.c:127](src/main.c#L127))
- **Integrate Box2D** — Box2D is fetched but not used. Replace or augment the cute_c2 collision system with Box2D for rigid body dynamics, joints, etc.
- **Scene management** — Only a single hardcoded scene exists. Build a scene manager to support multiple levels and transitions.
- **Entity management / ECS** — Only a single player actor is used. Move toward a more general entity system for data-driven game object creation.
- **UI system** — No dedicated UI system exists. Add menus, HUD elements, etc. using the overlay render pipeline.
- **Audio** — No audio system at all. Integrate SDL3 audio for sound effects and music.
- **Error handling** — Add robust error handling throughout (GPU calls, file loading, JSON parsing, etc.).
