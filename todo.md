*
* V1:
*	current grid generation loop:
*		- [helper0] checks for player chunk change and shift grid if needed
*		- [helper0] builds jobs for missing hmaps and chunks
*		- [threads] execute jobs:
*			- build chunks by generating the octree with perlinsettings
*			- build the vertexArrays depending on LOD and octree_threshold, plug vertex in the corresponding vertex_array[lod]
*		- [helper0] delivers jobs:
*			- plugs hmap/chunk in the grid, put old hmap/chunk in garbage if needed
*		- [gl_main] build new meshes:
*			- for all LODs if vertex_array[lod] is not empty
*			- grabs all rendered chunks, selecting available meshes in all vertex_array[]
*
*	todo:
*		- check why there are some chunks in the garbage after the first loop, although the player didnt move.
*		- check every ctor by copy, they can access private members, useless to use accessors
*		- inconsistencies? with the use of ref or ptr on some pipeline
*		- generate the vertexArray[lod] and its BP only when needed (when close enough from the cam/player)
*	done:
*		- memory leaks in chunks generation : LODs were not deleted
*	bugs :
*		- when renderedGrid.size = grid.size, race between the renderer and grid.updater
*
*	[Checklist] all gl calls have to be done on the gl context (here main thread)
*
*-------------------------------------------------------------------------------------------
*	V2:
*	treeLOD != obj3d LOD
*	- start with a quadtree, then octree:
*		- start by generating the world with a crappy treeLOD. 1 single leaf or 8 leaves ?
*		- then, depending on the distance from the player, generate the next treeLODs, 1 per 1
*

- pre v2:
    - archive v1 on a new branch
    - abandon old ways of building the voxels data
    - cleanup useless code
- vUnreal ?
    - sub process to generate ? server ? https://www.reddit.com/r/VoxelGameDev/comments/13qsr80/voxel_engine_networking/
- v2 : fullWorld [DOUBLE_MIN:DOUBLE_MAX] in 1 octree, with precision based on distance.
    - ImGui https://github.com/ocornut/imgui
    - dev settings UI
    - Server/Client ? UDP/TCP ? https://www.reddit.com/r/VoxelGameDev/comments/13qsr80/voxel_engine_networking/
    - Game class: init engine, generator, and world settings
    - .env ?
    - load world settings from file.
    - design patterns assessment :
        - Observer
        - tasks/jobs in multithreads : create, todo queue, assign, run, done queue, deliver, etc
    - refacto chunk generation/engine for fullWorld
    - 1 OpenGL call to rule them all (glMultiDrawArraysIndirect / glMultiDrawElementsIndirect )
    - voxel data compression
    - new blocks
    - lights
    - physics
    - audio : ambiant, music, effects, etc.
    - proper log files 
    - shaders v2: customizable by user like in minecraft
    - voxel edition v1 : simple click to add or remove 1 block
    - voxel diff from base generation saved on disk
    - voxel edition v2 : tool for multiple voxels edition (sphere, cube, etc) check other engines like DualUniverse, Enshrouded, etc
    - chunk generation on GPU ?
    - better procedural generation : biomes, grotto, lakes, etc
    - RPG : NPC, quest, levels, weapons, etc
    - chunk cache on disk
    - creative mod. Save selected voxels as a .voxel/octree "object", it is in fact an octree.
    - VR ?
    - gamepad

- bugs :
    - when renderedGrid.size = grid.size, race between the renderer and grid.updater