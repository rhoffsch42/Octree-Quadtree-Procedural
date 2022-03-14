#pragma once

#include "math.hpp"
#include "chunk.hpp"
#include "heightmap.hpp"
#include "job.hpp"
class JobBuildGenerator;

#include <mutex>
#include <condition_variable>
#include <vector>
#include <GLFW/glfw3.h> //window context


#define CHUNK_GEN_DEBUG	0

//make a class for that
typedef uint8_t**	hmap;
typedef UIImage*	minimapTile;

/*
	chunk sizes:
		16x16x16 = 4 096
		32x32x32 = 32 768
*/

/*
	object.users:
		increment value each time a pointer/ref is create on the object
		cant delete as long as there is at least 2 users (the first one beeing the pointer trying to delete it)
	solutions:
		- use smart pointers
		- void operator delete( void * ) {} // make a custom bool object.tryDelete(bool forceDelete = false)
*/

// why there is #include "chunk.hpp" in Obj3dPG.cpp ??
// generator.gridIter([]) hmapIter([])
// class tab with iter xyzwabcdef...uv 24 dimensions
//		or indexes as a vector[], with .size() beeing the # of dimensions


class ChunkGenerator
{
public:
	//grid_size_displayed will be clamped between 1 -> grid_size
	ChunkGenerator(Math::Vector3 player_pos, const PerlinSettings& perlin_settings, Math::Vector3 chunk_size, Math::Vector3 grid_size, Math::Vector3 grid_size_displayed);
	~ChunkGenerator();
	void			th_updater(Cam* cam);
	std::string		getGridChecks() const;
	bool			updateGrid(Math::Vector3 player_pos);
	void			updateChunkJobsToDo();
	void			updateChunkJobsDone();
	void			build(PerlinSettings& perlinSettings, std::string& threadIDstr);
	void			th_builders(GLFWwindow* context);
	bool			buildMeshesAndMapTiles();

	// build the chunks meshes and load them to the GPU
	void			glth_loadChunks();

	void			pushDisplayedChunks(std::list<Object*>* dst) const;
	void			pushDisplayedChunks(Object** dst) const;
	Math::Vector3	getGridDisplayStart() const;

	std::string		toString() const;
	Math::Vector3	worldToGrid(const Math::Vector3& index) const;
	Math::Vector3	gridToWorld(const Math::Vector3& index) const;

	bool			try_deleteUnusedData();

	Math::Vector3	chunkSize;
	Math::Vector3	gridSize;
	Math::Vector3	gridDisplaySize;// must be <= gridSize

	/*
		world index of the start of the grid (0:0:0)
	*/
	Math::Vector3	gridIndex;
	/*
		index in grid
	*/
	Math::Vector3	gridDisplayIndex;
	/*
		player chunk in world
	*/
	Math::Vector3	currentChunkWorldIndex;

	HeightMap***	heightMaps;//2d grid
	Chunk* ***		grid;//3d grid
	Math::Vector3	playerPos;

	PerlinSettings	settings;
	uint8_t			builderAmount;
	bool			playerChangedChunk;

	std::condition_variable	cv;
	std::mutex		chunks_mutex;
	std::mutex		job_mutex;
	std::mutex		trash_mutex;
	std::mutex		terminateBuilders;//old code
	std::mutex		mutex_cam;// cam mutex
	// lock/unlock helper0 (player pos thread)
	// lock_guard main() -> render loop before calling cam.events()
	// //lock_guard ChunkGenerator::updateGrid() -> ChunkGenerator::updatePlayerPos() 

	bool			terminateThreads = false;
	bool			chunksChanged;//related to: job_mutex
	bool			_chunksReadyForMeshes;
	bool			gridMemoryMoved;//not used for now
	uint8_t			threadsReadyToBuildChunks;

	std::map<Math::Vector3, bool>	map_jobsHmap;//store directly the jobs ptr?
	std::map<Math::Vector3, bool>	map_jobsChunk;
	std::list<JobBuildGenerator*>	jobsToDo;
	std::list<JobBuildGenerator*>	jobsDone;
	std::vector<HeightMap*>			trashHeightMaps;
	std::vector<Chunk*>				trashChunks;
private:
	ChunkGenerator();
	void	_updatePlayerPos(const Math::Vector3& player_pos);
	void	_deleteUnusedData();
};

/*
	avoir des chunk bien plus grand que 32, genre 500. (le temps de generation peut etre long)
	ajuster le threshold selon la distance avec le joueur
		si un des coins du cube de la node est dans le range, alors descendre le threshold

	on genere seulement les chunk voisins (3*3*3 - 1)
	on diplay seulement les nodes en dessous d'un certain range en adaptant les threshold
*/
