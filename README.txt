Milestone 1

Wang Ruipeng:

The terrain is generated in chunks, each encompassing a 16x256x16 section of blocks. These chunks are stored in a map keyed by coordinates, allowing for an effective world generation in the horizontal directions. Biomes are determined using a combination of 2D Perlin noise. The Perlin noise provides the base heightfield, while Worley noise determines variation, particularly in the plains biome to simulate hills.

Plains: Utilize Worley noise to create gentle, rolling hills.
Desert: Characterized by relatively flat terrain, achieved through a specific frequency and amplitude of Perlin noise.
Mountains: Use higher amplitude in the Perlin noise to create dramatic elevation changes.
Smooth Biome Transitions

To ensure smooth transitions between biomes, I employed a technique involving smoothstep functions and a third Perlin noise map with a larger grid size. This approach increases the contrast of the Perlin noise, allowing for distinct biome areas while also providing smooth transitions.

Challenges and Solutions:

The initial implementation of noise functions and terrain generation was computationally intensive, leading to performance issues. This was mitigated by optimizing noise calculations and only updating chunks within the player's vicinity. And creating natural-looking transitions between biomes was challenging. We experimented with various interpolation techniques and noise parameters to achieve a balance between distinct biome characteristics and seamless transitions.

Han Yang: 

I implemented the efficient terrain rendering and chunking. Specifically, in create() function of a chunk, I examine each face of each block. If the block is a boundary, then we add the block data into VBO data vectors. After setting chunk as the base element of rendering, in terrain.draw, we can instead loop over every chunks and call create() for each chunk that need to be sent to VBO, like modified chunks. Also, I add a function that check whether the player is near the boundary of the terrain. If so, add a new chunk and apply the same terrain generation function on the chunk.

Cindy:
Inputs/ mygl.cpp: I edited the key events to update m_inputs instead, then process the inputs through player.cpp. I also added mouse events, where when the mouse moves, the camera would rotate, and add blocks with right click and remove blocks with left click.

Tick/ mygl.cpp and player.cpp: invoked tick from player from tick from MyGL, and I calculated the delta time through the currentMSecsSinceEpoch() function.

Process inputs/player.cpp: I used m_inputs gotten from mygl.cpp to calculate the accerlation with different keys pressed. My jump however currently goes up but doesnt come down, and my flight mode would fly really high. The player moves based on the acceleration and the calculated velocity. For friction, I apply 0.95 of the current velocity everytime. For gravity, I did -100 on the y axis to mimic the gravity multiplied by the mass.

collisions/player.cpp: I used raycasting and gridmarch to check the collision on all three normals, assyming that the cube size of the player is the same as the cube of everything else

Add and remove blocks/player.cpp: I used ray casting to find the blcok that intersects within the radius of three, then for remove, I set the block to EMPTY, and for add, I set the block to DIRT.

Milestone 2

Wang Ruipeng:
I helped Cindy fix the bugs on milestone 1 first. The original code she submitted was unable to work properly in many situations, so I have to rewrite a lot of part of her code:

1. Her code calculated the delta time in the currentMSecsSinceEpoch() function through a wrong way which takes milliseconds as seconds, resulting in the player velocity being ridiculously small while acceleration being ridiculously large. This may answer her question above in the Process inputs parts. I helped improved her code by changing certain parameters.
2. The collision part is not working properly because she calculates the player position, player velocity and acceleration in wrong order. This makes the clamped velocity after collision will not be processed until next frame, resulting in a jittering collision when player hits a block. I fixed this problem by changing dynamic parameters processing order.
3. I also completely rewrote her ray casting function to match the functions on the slides given by Adam. Her original implementation of the ray casting function will result in mistakenly place the blocks somewhere randomly around the desired location. 
4. I also helped her fixed the code she has written for dealing with input which cannot run on the windows platform. The moveMouseToCenter() logic is not being called at the right time.
5. Finally, I corrected some spelling mistakes in her milestone1 readme.txt.

These problems could be addressed before the deadline of milestone1. However, Cindy was extremely busy that week because she had to rehearse for an acapella show and therefore unable to respond message in time. Despite this minor issue, the collaboration is overall quite a good experience. I would appreciate it more if the code passed down by Cindy was more polished and proofread so that it would save us more time to further prepare and develop for milestone 2.

Then, for multithreading terrain generation, I rearranged Han’s code to make it better suited for multithreading purpose. The VBO calculation part and the VBO binding was then separated into 2 processes since the binding process cannot be multithreaded. Here is the major update:

1. In MyGL::tick() function, the chunk update function is changed into m_terrain.multithreadedTerrainUpdate(). The original check_edge() is not used now. People may switch back to original check_edge() rendering function though, just change the line in the MyGL::tick() and remember to call buff_data() after createVBOdata() for each chunk.
2. The BlockGenerateWorker and VBOWorker class is stored in the chunkworker.cpp file, which stores each thread for running. The terrain generation code is now moved into the chunk class because the setBlock is done through the unordered_map m_chunk in terrain class, which is not thread safe. I try to add lock when accessing m_chunk, however, this does not work out fine. There was no dead lock and the data was simply unable to sent to VBO.
3. The thread is based on Qt runnable I tried C++ mutex first but considering it is rather unpolished so I decided to switch to Qt multithreading class instead.
4. As per instructed, the VBO data is now stored as a struct in the chunk class. The struct contains two separated VBO data, one for opaque object and the other one is for transparent. The transparent is reserved for the people who will be doing texturing part. However, it is uninitialized, so you might need to fill them accordingly.
5. I also implemented the cave system for procedural generation using 3D Perlin noise.

However, after the new rendering pipeline, the destroy and place block is not functioning. I will try to fix this problem. I am pushing this version at 11/17 now wishing to give other teammates who not yet start on milestone 2 a good place to start and test their code efficiently. The problem should be in the rendering as far as I am concerned. I will update as soon as the bug got fixed.

Cindy:

I implemented the post-process pipeline for overlay effects when the player's head is under water or under the lava. I added two shader files, WLoverlay.frag.glsl and WLoverlay.vert.glsl for the blue tint effect underwater and the red tint effect under lava. I also added functions in player.cpp like isonground, isinwater, and isinlava to check whether the player is standing on land, water, or lava. if the player is standing on water or lava, then it slowly sinks, with the velocity reduced to 2/3 of the original speed. and the player could move up with a constant speed when the space bar is pressed. I also fixed the jumping issue, where jumping by pressing the space bar only works when the player is standing on the ground. I also implemented the quad.cpp and quad.h where it creates a quad so we could apply the post process shader on the quad under water and under lava.

Challenges: My computer had issues loading the texture files so I couldn't render the screen properly. When the player's head is underneath the water, it would render a complete blue screen instead of a blue tint over the screen. When I try to get the original colors of the screen it would just render a dark screen so I will try to fix that before milestone 3's deadline. 


Clarence Yang:

I implemented the texturing part. Specifically, I followed the code in ShaderFun to finish most part of it. The different thing is that, I replace the previous vec4 Col into UV. So in my implementation, UV is a vec4, with the first two coordinates representing the uv coordinate, and the third coordinate representing whether the texture is animating. Also, I modified drawable so that now it contains two index buffers and two data buffers, for both opaque and transparent objects. One of the issues that tortures me the most is that, I used a reference to either the transparent vector or the opaque vector, and then push data in to the refered vector. Initially, I first init the reference to opaque vector, then conditionally change it to transparent vector. This caused strange problem... Later I changed to init with (condition) ? a : b, and the problem fixed.

The main challenge is that we don't know where the bug is... To difficult to debug, and I had to go through everywhere to find the problem. So during debugging, I even found minor problems in multithreading...

Futher works:
1, For texturing, I feel that all the basic requirements are fulfilled.
2, For multi-threading, in Terrain::spawnBlockTypeWorker, currently the main thread waits for the child that generates block type to finish. We need to remove this wait, and move m_generatedTerrain.insert(zone) inside child thread codes, so that the rendering can be much more efficient. It is the same case in line 230 of waiting all VBO workers to finish. I added this waiting to ensure correctness, but it turns out that this waiting is unnecessary.
3, For post-processing, the work is still very problematic. First, for detecting whether the player is inside the water, the decision boundary should be in the same height as the camera. Second, currently the post process pipeline is still incorrect. When the player is not in water or lava, the shader directly render the results to screen. The framebuffer is only used when player is in water or lava. But the pipeline of writing surface shader output to framebuffer, loading from framebuffer and rendering to screen is still not corrent. I don't have enough time to fix it before deadline though.


