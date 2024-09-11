# Mini Minecraft Project

## Description
This project is a simplified version of Minecraft implemented using C++ and OpenGL. The game features terrain generation, player controls, block manipulation, biomes, day and night cycles, and post-processing effects. It leverages procedural generation, multithreaded terrain rendering, and basic physics for an immersive experience. The project was developed as a collaborative effort, with contributions focusing on terrain creation, chunk rendering, player input handling, and visual effects.

## Features

### 1. **Procedural Terrain Generation**
   - **Biomes:** Procedurally generated biomes using Perlin and Worley noise.
     - **Plains:** Rolling hills generated using Worley noise.
     - **Desert:** Flat terrain with fine-tuned Perlin noise.
     - **Mountains:** Dramatic elevation changes via high amplitude Perlin noise.
     - **Smooth Transitions:** Between biomes using smoothstep functions and additional noise maps.
   - **Challenges Solved:** Optimized noise calculation for performance and smooth biome transitions.

### 2. **Efficient Chunk-Based Rendering**
   - Terrain is divided into 16x256x16 chunks for rendering efficiency.
   - Chunks are dynamically generated and rendered as the player moves, optimizing resource use.

### 3. **Player Movement and Physics**
   - **Player Movement:** Key and mouse inputs control movement, with functionality for walking, jumping, and flying.
   - **Collision Detection:** Raycasting and grid-marching algorithms handle player-block collisions.
   - **Block Manipulation:** Players can add or remove blocks (e.g., DIRT) by right-clicking or left-clicking.

### 4. **Post-Processing Effects**
   - **Underwater/Lava Effects:** Added shaders for underwater (blue tint) and lava (red tint) with ripple distortion using Worley noise.
   - **Challenges:** Ongoing issues with rendering proper tint over the screen.

### 5. **Day and Night Cycle**
   - A spherical skybox with procedural cloud generation using Worley noise.
   - The sun's position changes based on time, simulating a day-night cycle.
   - Skybox shaders adjust lighting and cloud movement as time progresses.

### 6. **Multithreading for Terrain Generation**
   - Multithreaded terrain generation, separating VBO calculation and binding to improve performance.
   - Procedural cave system generation using 3D Perlin noise.

### 7. **Shadow Mapping and Lighting**
   - Real-time shadows based on player height, using shadow maps and adjustable depth for higher resolution.
   - Dynamic lighting adjusts with the time of day, providing a realistic transition between daylight and night.
