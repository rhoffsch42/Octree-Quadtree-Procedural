#pragma once

#include "math.hpp"
#include "chunk.hpp"
#include "heightmap.hpp"
#include <mutex>
#include <condition_variable>
#include <vector>
#include <GLFW/glfw3.h> //window context

#define CHUNK_GEN_DEBUG	0

//make a class for that
typedef uint8_t**	hmap;
typedef UIImage*	minimapTile;


/*
	object.users:
		increment value each time a pointer/ref is create on the object
		cant delete as long as there is at least 2 users (the first one beeing the pointer trying to delete it)
	solutions:
		- use smart pointers
		- void operator delete( void * ) {} // make a custom bool object.tryDelete(bool forceDelete = false)
*/

// generator.gridIter([]) hmapIter
// class tab with iter xyzwabcdef...uv 24 dimensions
//		or indexes as a vector[], with .size() beeing the # of dimensions

class Job
{
public:
	Job() : assigned(false), done(false) {}
	//virtual bool	execute() = 0;
	//args could differ, make a struct with args and a dyn_cast in execute()?
	//can lambda expressions [] work?

	bool	assigned;
	bool	done;
};

class JobBuildChunk : public Job
{
public:
	JobBuildChunk(Math::Vector3 chunk_index, HeightMap* hmap_ptr)
		: index(chunk_index), hmap(hmap_ptr)
	{
	}
	bool execute(PerlinSettings& perlinSettings, Math::Vector3 gridPos, Math::Vector3 halfGrid, Math::Vector3 chunk_size) {
		Math::Vector3	chunkWorldIndex(
			gridPos.x - halfGrid.x + this->index.x,
			gridPos.y - halfGrid.y + this->index.y,
			gridPos.z - halfGrid.z + this->index.z);
		if (!this->hmap) {//for now it create hmap here, leading to multiple identical hmaps for the same y coordinate
			this->hmap = new HeightMap(perlinSettings,
				chunkWorldIndex.x * chunk_size.x, chunkWorldIndex.z * chunk_size.z,
				chunk_size.x, chunk_size.z);
			this->hmap->glth_buildPanel();
		}
		this->chunk = new Chunk(chunkWorldIndex, chunk_size, perlinSettings, this->hmap);
		this->chunk->buildMesh();

		this->done = true;
		return this->done;
	}

	Math::Vector3	index;
	HeightMap*		hmap = nullptr;//do a different job for this, repush back jobsToDo if hmap not ready
	Chunk*			chunk = nullptr;
};

class ChunkGenerator
{
public:
	std::condition_variable	cv;
	std::mutex	chunks_mutex;
	std::mutex	job_mutex;
	std::mutex	mutex_cam;// cam mutex
	// lock/unlock helper0 (player pos thread)
	// lock_guard main() -> render loop before calling cam.events()
	// //lock_guard ChunkGenerator::updateGrid() -> ChunkGenerator::updatePlayerPos() 

	//grid_size_displayed will be clamped between 1 -> grid_size
	ChunkGenerator(Math::Vector3 player_pos, const PerlinSettings& perlin_settings, Math::Vector3 chunk_size, Math::Vector3 grid_size, Math::Vector3 grid_size_displayed);
	~ChunkGenerator();
	bool	updateGrid_old(Math::Vector3 player_pos);
	bool	updateGrid(Math::Vector3 player_pos);
	void	updateChunkJobsToDo();
	void	updateChunkJobsDone();
	void	th_builders(GLFWwindow* context);
	void	th_builders_old(GLFWwindow* context);
	bool	buildMeshesAndMapTiles();
	bool	try_deleteUnusedData();

	void	printData();

	Math::Vector3	chunkSize;

	Math::Vector3	gridSize;
	Math::Vector3	gridPos;//the center of the grid in game world chunk coo
	Math::Vector3	gridDisplaySize;
	Math::Vector3	gridDisplayIndex;//the chunk where the player is, index inside the grid

	Chunk* ***		grid;//3d grid
	PerlinSettings	settings;
	Math::Vector3	currentChunk;

	HeightMap* **	heightMaps;//2d grid
	Math::Vector3	playerPos;

	uint8_t			builderAmount;
	bool			playerChangedChunk;



	bool			chunksChanged;//related to: job_mutex
	bool			_chunksReadyForMeshes;
	bool			gridMemoryMoved;//not used for now
	uint8_t			threadsReadyToBuildChunks;
	std::mutex		terminateBuilders;
private:
	void	updatePlayerPos(Math::Vector3 player_pos);
	ChunkGenerator();

	void	deleteUnusedData();
	std::mutex					trash_mutex;
	std::vector<HeightMap*>		trashHeightMaps;
	std::vector<Chunk*>			trashChunks;

	std::list<JobBuildChunk*>	jobsToDo;
	std::list<JobBuildChunk*>	jobsDone;
};

/*
	avoir des chunk bien plus grand que 32, genre 500. (le temps de generation peut etre long)
	ajuster le threshold selon la distance avec le joueur
		si un des coins du cube de la node est dans le range, alors descendre le threshold

	on genere seulement les chunk voisins (3*3*3 - 1)
	on diplay seulement les nodes en dessous d'un certain range en adaptant les threshold
*/
