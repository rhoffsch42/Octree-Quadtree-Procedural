#pragma once

#include "math.hpp"
#include "cam.hpp"
#include "chunk.hpp"
#include "heightmap.hpp"
#include "job.hpp"
class JobBuildGenerator;

#include <mutex>
#include <condition_variable>
#include <vector>
#include <thread>

//#include <GLFW/glfw3.h> //window context


//#define CHUNK_GEN_DEBUG

//make a class for that
//typedef uint8_t**	hmap;
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



/*
	avoir des chunk bien plus grand que 32, genre 256. (le temps de generation peut etre long)
	ajuster le threshold selon la distance avec le joueur
		si un des coins du cube de la node est dans le range, alors descendre le threshold

	on genere seulement les chunk voisins (3*3*3 - 1)
	on diplay seulement les nodes en dessous d'un certain range en adaptant les threshold
*/

#include "chunkgrid.hpp"
class ChunkGrid;

class ChunkGenerator
{
public:
	//grid_size_displayed will be clamped between 1 -> grid_size
	ChunkGenerator(const PerlinSettings& perlin_settings);
	~ChunkGenerator();
	void			th_updater(Cam* cam, ChunkGrid* grid);
	//cam is the player
	void			initAllBuilders(uint8_t amount, Cam* cam, ChunkGrid* grid);
	void			joinBuilders();

	// returns true if the job was successfully created
	bool			createHeightmapJob(Math::Vector3 chunkWorldIndex, Math::Vector3 chunkSize);
	// returns true if the job was successfully created
	bool			createChunkJob(Math::Vector3 chunkWorldIndex, Math::Vector3 chunkSize, HeightMap* heightmap);
	void			updateJobsToDo(ChunkGrid& grid);
	void			updateJobsDone(ChunkGrid& grid);
	void			executeAllJobs(PerlinSettings& perlinSettings, std::string& threadIDstr);
	void			th_builders(GLFWwindow* context);

	bool			try_deleteUnusedData();

	uint8_t			getBuildersAmount() const;
	std::thread**	getBuilders() const;

	PerlinSettings	settings;

	std::condition_variable	cv;
	std::mutex		job_mutex;
	std::mutex		trash_mutex;
	std::mutex		terminateBuilders;//old code
	std::mutex		mutex_cam;// cam mutex
	// lock/unlock helper0 (player pos thread)
	// lock_guard main() -> render loop before calling cam.events()
	// //lock_guard ChunkGenerator::updateGrid() -> ChunkGenerator::updatePlayerPos() 

	bool			terminateThreads = false;
	uint8_t			threadsReadyToBuildChunks;

	std::map<Math::Vector3, bool>	map_jobsHmap;//store directly the jobs ptr?
	std::map<Math::Vector3, bool>	map_jobsChunk;
	std::list<JobBuildGenerator*>	jobsToDo;
	std::list<JobBuildGenerator*>	jobsDone;
	std::vector<HeightMap*>			trashHeightMaps;
	std::vector<Chunk*>				trashChunks;
private:
	uint8_t			_builderAmount;
	std::thread**	_builders;
	ChunkGenerator();
	void			_deleteUnusedData();
};

//class GridManager
//{
//public:
//	GridManager();
//	~GridManager();
//
//	ChunkGrid		grid;
//	ChunkGenerator generator;
//private:
//};

