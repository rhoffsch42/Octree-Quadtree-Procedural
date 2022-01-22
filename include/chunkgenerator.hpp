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

// why there is #include "chunk.hpp" in Obj3dPG.cpp ??
// generator.gridIter([]) hmapIter([])
// class tab with iter xyzwabcdef...uv 24 dimensions
//		or indexes as a vector[], with .size() beeing the # of dimensions

class JobBuildGenerator;

class ChunkGenerator
{
public:
	//grid_size_displayed will be clamped between 1 -> grid_size
	ChunkGenerator(Math::Vector3 player_pos, const PerlinSettings& perlin_settings, Math::Vector3 chunk_size, Math::Vector3 grid_size, Math::Vector3 grid_size_displayed);
	~ChunkGenerator();
	std::string getGridChecks() const;
	bool	updateGrid_old(Math::Vector3 player_pos);
	bool	updateGrid(Math::Vector3 player_pos);
	void	updateChunkJobsToDo();
	void	updateChunkJobsDone();
	void	th_builders(GLFWwindow* context);
	void	th_builders_old(GLFWwindow* context);
	bool	buildMeshesAndMapTiles();
	bool	try_deleteUnusedData();

	std::string	toString() const;

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

	std::condition_variable	cv;
	std::mutex		chunks_mutex;
	std::mutex		job_mutex;
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
	std::list<JobBuildGenerator*>	jobsToDo;
	std::list<JobBuildGenerator*>	jobsDone;
private:
	void	updatePlayerPos(Math::Vector3 player_pos);
	ChunkGenerator();

	void	deleteUnusedData();
	std::mutex					trash_mutex;
	std::vector<HeightMap*>		trashHeightMaps;
	std::vector<Chunk*>			trashChunks;
};

/*
	avoir des chunk bien plus grand que 32, genre 500. (le temps de generation peut etre long)
	ajuster le threshold selon la distance avec le joueur
		si un des coins du cube de la node est dans le range, alors descendre le threshold

	on genere seulement les chunk voisins (3*3*3 - 1)
	on diplay seulement les nodes en dessous d'un certain range en adaptant les threshold
*/


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

class JobBuildGenerator : public Job
{
public:
	Math::Vector3	index;
	virtual bool	execute(PerlinSettings& perlinSettings, Math::Vector3 gridPos, Math::Vector3 halfGrid, Math::Vector3 chunk_size) = 0;
	virtual void	dispatch(ChunkGenerator& generator) const = 0;
protected:
	JobBuildGenerator(Math::Vector3 ind) : index(ind) {}
};

class JobBuildHeighMap : public JobBuildGenerator
{
public:
	JobBuildHeighMap(Math::Vector3 chunk_index)
		: JobBuildGenerator(chunk_index) {}
	virtual bool execute(PerlinSettings& perlinSettings, Math::Vector3 gridPos, Math::Vector3 halfGrid, Math::Vector3 chunk_size) {
		Math::Vector3	worldIndex(
			gridPos.x - halfGrid.x + this->index.x,
			0,//dont need it actually
			gridPos.z - halfGrid.z + this->index.z);
		this->hmap = new HeightMap(perlinSettings,
			worldIndex.x * chunk_size.x, worldIndex.z * chunk_size.z,
			chunk_size.x, chunk_size.z);
		this->hmap->glth_buildPanel();

		this->done = true;
		return this->done;
	}
	virtual void	dispatch(ChunkGenerator& generator) const {
		int x = this->index.x;
		int z = this->index.z;
		if (generator.heightMaps[z][x]) {
			std::cout << "Heightmap " << this->hmap << " overriding " << generator.heightMaps[z][x] << "\n";
			std::cout << this->index.toString() << "\n";
			std::cout << "This shouldn't happen, exiting...\n";
			std::exit(-14);
		}
		generator.heightMaps[z][x] = this->hmap;
		std::cout << "hmap plugged in x:z " << x << ":" << z << " " << this->hmap << "\n";
	}

	HeightMap* hmap = nullptr;
};

class JobBuildChunk : public JobBuildGenerator
{
public:
	JobBuildChunk(Math::Vector3 chunk_index, HeightMap* hmap_ptr)
		: JobBuildGenerator(chunk_index), hmap(hmap_ptr) {}
	virtual bool execute(PerlinSettings& perlinSettings, Math::Vector3 gridPos, Math::Vector3 halfGrid, Math::Vector3 chunk_size) {
		if (!this->hmap) {// should not be possible
			std::cout << "JobBuildChunk error: HeightMap nullptr, for index:\n";
			std::cout << this->index.toString() << "\n";
			std::exit(-14);
			this->done = false;//should already be false, better be sure
			return this->done;
		}
		Math::Vector3	worldIndex(
			gridPos.x - halfGrid.x + this->index.x,
			gridPos.y - halfGrid.y + this->index.y,
			gridPos.z - halfGrid.z + this->index.z);
		this->chunk = new Chunk(worldIndex, chunk_size, perlinSettings, hmap);
		this->chunk->buildMesh();
		this->done = true;
		return this->done;
	}
	virtual void	dispatch(ChunkGenerator& generator) const {
		int x = this->index.x;
		int y = this->index.y;
		int z = this->index.z;
		if (generator.grid[z][y][x]) {
			std::cout << "Chunk " << this->chunk << " overriding " << generator.grid[z][y][x] << "\n";
			std::cout << this->index.toString() << "\n";
			std::cout << "This shouldn't happen, exiting...\n";
			std::exit(-14);
		}
		generator.grid[z][y][x] = this->chunk;
		std::cout << "chunk plugged in x:y:z " << x << ":" << y << ":" << z << " " << this->chunk << "\n";
	}

	HeightMap* hmap = nullptr;
	Chunk* chunk = nullptr;
};
