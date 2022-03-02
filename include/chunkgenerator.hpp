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

class JobBuildGenerator;

class ChunkGenerator
{
public:
	static void		test_calcExceed();//unit tests
	static void		exceedTest();//unit tests
	//grid_size_displayed will be clamped between 1 -> grid_size
	ChunkGenerator(Math::Vector3 player_pos, const PerlinSettings& perlin_settings, Math::Vector3 chunk_size, Math::Vector3 grid_size, Math::Vector3 grid_size_displayed);
	~ChunkGenerator();
	void		th_updater(Cam* cam);
	std::string	getGridChecks() const;
	bool		updateGrid_old(Math::Vector3 player_pos);
	bool		updateGrid(Math::Vector3 player_pos);
	void		updateChunkJobsToDo();
	void		updateChunkJobsDone();
	void		build(PerlinSettings& perlinSettings, std::string& threadIDstr);
	void		th_builders(GLFWwindow* context);
	void		th_builders_old(GLFWwindow* context);
	bool		buildMeshesAndMapTiles();
	bool		try_deleteUnusedData();
	Math::Vector3	getGridDisplayStart() const;

	std::string	toString() const;
	Math::Vector3	ChunkGenerator::worldToGrid(const Math::Vector3& index) const;
	Math::Vector3	ChunkGenerator::gridToWorld(const Math::Vector3& index) const;

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

// nouveau check du helper qui voit que certains chunk sont pas crees, les jobs sont crees mais pas finis, donc il cree des doublons de job... qui sont repluggés apres...
// empecher le helper0 de creer des jobs doublons
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
	virtual bool	execute(PerlinSettings& perlinSettings) = 0;
	virtual void	deliver(ChunkGenerator& generator) const = 0;

	Math::Vector3	index;
	Math::Vector3	chunkSize;
protected:
	JobBuildGenerator(Math::Vector3 ind, Math::Vector3 chunk_size)
		: index(ind), chunkSize(chunk_size) {}
};

class JobBuildHeighMap : public JobBuildGenerator
{
public:
	JobBuildHeighMap(Math::Vector3 index, Math::Vector3 chunk_size)
		: JobBuildGenerator(index, chunk_size) {}
	virtual bool execute(PerlinSettings& perlinSettings) {
		this->hmap = new HeightMap(perlinSettings, this->index, this->chunkSize);
		this->hmap->glth_buildPanel();
		this->done = true;
		std::cout << "job done : new hmap : " << this->hmap << " " << this->index << "\n";
		return this->done;
	}
	virtual void	deliver(ChunkGenerator& generator) const {
		Math::Vector3	ind = generator.worldToGrid(this->index);
		ind.y = 0;//same hmap for all Y // not used...
		int x = ind.x;
		int z = ind.z;
		if (this->index != this->hmap->getIndex() || this->index.y != 0) {
			std::cerr << "wrong index for job " << this->index << " and hmap " << this->hmap << this->hmap->getIndex() << "\n";
			std::exit(-14);
		}
		if (x < 0 || z < 0 || x >= generator.gridSize.x || z >= generator.gridSize.z) {
			std::cerr << "out of memory hmap " << this->hmap << " index " << ind << "\n";
			generator.trash_mutex.lock();
			generator.trashHeightMaps.push_back(this->hmap);
			generator.trash_mutex.unlock();
			generator.map_jobsHmap[this->index] = false;
			return;
		}
		if (generator.heightMaps[z][x]) {
			HeightMap* h = generator.heightMaps[z][x];
			std::cerr << "Heightmap " << this->hmap << " overriding " << h << " " << ind << "\n";
			std::cerr << "This shouldn't happen, exiting...\n";
			//std::exit(-14);
		}

		generator.heightMaps[z][x] = this->hmap;
		generator.map_jobsHmap[this->index] = false;
		std::cout << this->hmap << " hmap " << this->hmap->getIndex() << " plugged in " << ind << "\n";
	}

	HeightMap* hmap = nullptr;
};

class JobBuildChunk : public JobBuildGenerator
{
public:
	JobBuildChunk(Math::Vector3 index, Math::Vector3 chunk_size, HeightMap* hmap_ptr)
		: JobBuildGenerator(index, chunk_size), hmap(hmap_ptr) {}
	virtual bool execute(PerlinSettings& perlinSettings) {
		if (!this->hmap) {// should not be possible
			std::cerr << "JobBuildChunk error: HeightMap nullptr, for index: " << this->index << "\n";
			//std::exit(-14);
			this->done = false;//should already be false, better be sure
			return this->done;
		}
		this->chunk = new Chunk(this->index, this->chunkSize, perlinSettings, hmap);
		this->chunk->buildMesh();
		this->done = true;
		std::cout << "job executed : new chunk : " << this->chunk << " " << this->index << "\n";
		return this->done;
	}
	virtual void	deliver(ChunkGenerator& generator) const {
		Math::Vector3	ind = generator.worldToGrid(this->index);
		int x = ind.x;
		int y = ind.y;
		int z = ind.z;

		if (this->index != this->chunk->index) {
			std::cerr << "wrong index for job "<< this->index << " and chunk " << this->chunk << this->chunk->index << "\n";
			std::exit(-14);
		}
		if (x < 0 || y < 0 || z < 0 || x >= generator.gridSize.x || y >= generator.gridSize.y || z >= generator.gridSize.z) {
			std::cerr << "out of memory chunk " << this->chunk << this->index << " on grid: " << ind << "\n";
			generator.trash_mutex.lock();
			generator.trashChunks.push_back(this->chunk);
			generator.trash_mutex.unlock();
			generator.map_jobsChunk[this->index] = false;
			return;
		}
		if (generator.grid[z][y][x]) {
			Chunk* c = generator.grid[z][y][x];
			std::cerr << "Chunk " << this->chunk << this->index << " overriding " << c << c->index << " on grid: " << ind << "\n";
			std::cerr << "This shouldn't happen, exiting...\n";
			//std::exit(-14);
		}
		generator.grid[z][y][x] = this->chunk;
		generator.map_jobsChunk[this->index] = false;
		std::cout << this->chunk << " chunk " << this->chunk->index << " plugged in " << ind << "\n";
		generator.chunksChanged = true;
	}

	HeightMap* hmap = nullptr;
	Chunk* chunk = nullptr;
};
