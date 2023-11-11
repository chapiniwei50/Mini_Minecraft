This is the final project for CIS460.

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

Tick/ mygl.coo and player.cpp: invoked tick from player from tick from MyGL, and I calculated the delta time through the currentMSecsSinceEpoch() function.

Process inputs/player.cpp: I used m_inputs gotten from mygl.cpp to calculate the accerlation with different keys pressed. My jump however currently goes up but doesnt come down, and my flight mode would fly really high. The player moves based on the acceleration and the calculated velocity. For friction, I apply 0.95 of the current velocity everytime. For gravity, I did -100 on the y axis to mimic the gravity multiplied by the mass.

collisions/player.cpp: I used raycasting and gridmarch to check the collision on all three normals, assyming that the cube size of the player is the same as the cube of everything else

Add and remove blocks/player.cpp: I used ray casting to find the blcok that intersects within the radius of three, then for remove, I set the block to EMPTY, and for add, I set the block to DIRT.
