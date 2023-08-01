#pragma once

#include "math.hpp"
#include "cam.hpp"
#include "chunk.hpp"
#include "heightmap.hpp" // also for PerlinSettings
#include "job.hpp"
class JobBuildGenerator;

#include <mutex>
#include <condition_variable>
#include <vector>
#include <thread>

//#include <GLFW/glfw3.h> //window context


//#define CHUNK_GEN_DEBUG


typedef UIImage*	minimapTile;

/*
	chunk sizes:
		16x16x16 = 4 096
		32x32x32 = 32 768
*/


#include "chunkgrid.hpp"
class ChunkGrid;

class ChunkGenerator
{
public:
	/*
		grid_size_displayed will be clamped between 1->grid_size
		target: the grid that the generator is working for
	*/
	ChunkGenerator(const PerlinSettings& perlin_settings);
	~ChunkGenerator();
	void			th_updater(Cam* cam, ChunkGrid* grid);
	void			th_builders(GLFWwindow* context);
	//cam is the player
	void			initAllBuilders(uint8_t amount, Cam* cam, ChunkGrid* grid);
	void			joinBuilders();

	// returns true if the job was successfully created
	bool			createHeightmapJob(ChunkGrid& grid, Math::Vector3 chunkWorldIndex, Math::Vector3 chunkSize);
	// returns true if the job was successfully created
	bool			createChunkJob(ChunkGrid& grid, Math::Vector3 chunkWorldIndex, Math::Vector3 chunkSize, HeightMap* heightmap);
	void			updateJobsToDo(ChunkGrid& grid);
	void			updateJobsDone(ChunkGrid& grid);
	void			executeAllJobs(PerlinSettings& perlinSettings, std::string& threadIDstr);

	uint8_t			getBuildersAmount() const;
	std::thread**	getBuilders() const;

	PerlinSettings	settings;

	std::condition_variable	cv;
	std::mutex		job_mutex;
	std::mutex		trash_mutex;
	std::mutex		terminateBuilders;//old code
	std::mutex		mutex_cam;// cam mutex

	bool			terminateThreads = false;
	uint8_t			threadsReadyToBuildChunks;

	std::map<Math::Vector3, bool>	map_jobsHmap;//store directly the jobs ptr instead of bool?
	std::map<Math::Vector3, bool>	map_jobsChunk;
	std::list<JobBuildGenerator*>	jobsToDo;
	std::list<JobBuildGenerator*>	jobsDone;
private:
	uint8_t			_builderAmount;
	std::thread**	_builders;
	ChunkGenerator();
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

