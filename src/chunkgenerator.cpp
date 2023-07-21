#include "chunkgenerator.hpp"
#include "trees.h"
#include "compiler_settings.h"
#include <algorithm>
#include <chrono>
#include <sstream>
using namespace std::chrono_literals;

#ifdef TREES_DEBUG
 #define TREES_CHUNK_GENERATOR_DEBUG
#endif
#ifdef TREES_CHUNK_GENERATOR_DEBUG 
 #define D(x) std::cout << "[ChunkGenerator] " << x
 #define D_(x) x
 #define D_SPACER "-- chunkgenerator.cpp -------------------------------------------------\n" 
 #define D_SPACER_END "----------------------------------------------------------------\n"
#else 
 #define D(x)
 #define D_(x)
 #define D_SPACER ""
 #define D_SPACER_END ""
#endif


ChunkGenerator::ChunkGenerator(const PerlinSettings& perlin_settings, ChunkGrid* target) : settings(perlin_settings), _targetGrid(target) {
	D(__PRETTY_FUNCTION__ << "\n");

	this->_builderAmount = 0;
	this->_builders = nullptr;
	this->threadsReadyToBuildChunks = 0;
}

ChunkGenerator::~ChunkGenerator() {
	if (this->_builderAmount) {
		for (uint8_t i = 0; i < this->_builderAmount; i++)
			delete this->_builders[i];
		delete[] this->_builders;
	}
	this->joinBuilders();
}

// `amount - 1` builders and 1 updater
void	ChunkGenerator::initAllBuilders(uint8_t amount, Cam* cam, ChunkGrid* grid) {
	this->_builderAmount = amount;
	this->_builders = new std::thread* [this->_builderAmount + 1];
	for (uint8_t i = 0; i < this->_builderAmount - 1; i++) {
		//contexts[i] = glfwCreateWindow(500, 30, std::to_string(i).c_str(), NULL, m.glfw->_window);
		//if (!contexts[i]) {
			//D("Error when creating context " << i << "\n")
			//Misc::breakExit(5);
		//}
		//glfwSetWindowPos(contexts[i], 2000, 50 + 30 * i);
		//builders[i] = new std::thread(std::bind(&ChunkGenerator::th_builders, &generator, contexts[i]));
		this->_builders[i] = new std::thread(std::bind(&ChunkGenerator::th_builders, this, nullptr));
	}
	//helper: player pos, jobs, trash(need gl cntext?)
	this->_builders[this->_builderAmount - 1] = new std::thread(std::bind(&ChunkGenerator::th_updater, this, cam, grid));
	this->_builders[this->_builderAmount] = nullptr;

	std::this_thread::sleep_for(0.5s);
	D("Notifying threads to build data...\n");
	this->cv.notify_all();
}

// Assuming initBuilders() has been called before. 
void	ChunkGenerator::joinBuilders() {
	D("Notifying cv to wake up waiters, need to join " << int(this->_builderAmount) << " builders...\n");
	this->terminateThreads = true;
	this->cv.notify_all();
	for (size_t i = 0; i < this->_builderAmount; i++) {
		this->_builders[i]->join();
		D("joined builder " << i << "\n");
	}
}

void	ChunkGenerator::th_updater(Cam* cam, ChunkGrid* grid) {
	std::thread::id		thread_id = std::this_thread::get_id();
	std::stringstream	ss;
	ss << thread_id;
	std::string			d_prefix = std::string("[helper0 ") + ss.str() + "] ";
	Math::Vector3		playerPos;

	while (!this->terminateThreads) {//no mutex for reading?
		this->mutex_cam.lock();
		playerPos = cam->local.getPos();
		this->mutex_cam.unlock();

		if (grid->updateGrid(playerPos)) {//player changed chunk
			D(d_prefix << "grid updated with player pos, as he changed chunk\n");
		} else {
			//D(d_prefix << "grid unchanged, player in same grid\n")
		}
		this->cv.notify_all();
		this->updateJobsDone(*grid);//before updateJobsToDo()
		//D(d_prefix << "jobs done updated\n")
		this->updateJobsToDo(*grid);
		//D(d_prefix << "jobsToDo updated\n")

		D_(std::cout << "*");
		std::this_thread::sleep_for(100ms);
	}
	D(d_prefix << "exiting...\n");
}

void	ChunkGenerator::th_builders(GLFWwindow* context) {
	//D(__PRETTY_FUNCTION__ << "\n");
	//glfwMakeContextCurrent(context);
	std::thread::id threadID = std::this_thread::get_id();
	std::stringstream ss;
	ss << threadID;
	std::string	threadIDstr = "[" + ss.str() + "]\t";
	D("Builder started " << threadIDstr << "\n");

	//build job variables

	// [Checklist] make sure every builders use a different PerlinSettings, or generation will be fucked
	PerlinSettings		perlinSettings(this->settings);//if they change later, we have to update them, cpy them when finding a job?
	JobBuildGenerator* job = nullptr;
	unsigned int		thread_jobsdone = 0;
	std::unique_lock<std::mutex> job_lock(this->job_mutex);

	while (!this->terminateThreads) {
		if (!this->jobsToDo.empty()) {
			// grab job
			//D(threadIDstr << "grabbing job\n")
			job = this->jobsToDo.front();
			this->jobsToDo.pop_front();
			job->assigned = true;

			job_lock.unlock();
			job->prepare(&perlinSettings);
			job->execute();
			job_lock.lock();

			job->assigned = false;
			if (job->done) {
				this->jobsDone.push_back(job);
				thread_jobsdone++;
			}
			else
				this->jobsToDo.push_back(job);

			//D(threadIDstr << "job done " << job->index << "\n")
			job = nullptr;
		}
		//D_(else { D(threadIDstr << "no job found... \n"); })
		//else {
		//	D(threadIDstr << "no job found... \n")
		//}

		//D(threadIDstr << "waiting \n")
		this->cv.wait(job_lock, [this] { return (!this->jobsToDo.empty() || this->terminateThreads); });//wait until jobs list in not empty
		//D(threadIDstr << "waking up from cv.wait() \n")
	}
	job_lock.unlock();

	//D(threadIDstr << "yyy Jobs done : " << thread_jobsdone << ". Exiting...\n")
	//glfwSetWindowShouldClose(context, GLFW_TRUE);
	//glfwMakeContextCurrent(nullptr);
}

bool	ChunkGenerator::createHeightmapJob(Math::Vector3 hmapWorldIndex, Math::Vector3 chunkSize) {
	if (!this->map_jobsHmap[hmapWorldIndex]) {
		this->jobsToDo.push_back(new JobBuildHeightMap(this, this->_targetGrid, hmapWorldIndex, chunkSize));
		this->map_jobsHmap[hmapWorldIndex] = true;

		#ifdef CHUNK_GEN_DEBUG
		D("new JobBuildHeighMap: WorldIndex : " << hmapWorldIndex << "\n");
		#endif
		return true;
	}

	#ifdef CHUNK_GEN_DEBUG
	D("already created JobBuildHeighMap: WorldIndex : " << hmapWorldIndex << "\n");
	#endif
	return false;
}

bool	ChunkGenerator::createChunkJob(const Math::Vector3 chunkWorldIndex, const Math::Vector3 chunkSize, HeightMap* heightmap) {
	if (!this->map_jobsChunk[chunkWorldIndex] && heightmap->dispose()) {
		this->jobsToDo.push_back(new JobBuildChunk(this, this->_targetGrid, chunkWorldIndex, chunkSize, heightmap));
		this->map_jobsChunk[chunkWorldIndex] = true;

		#ifdef CHUNK_GEN_DEBUG
		D("new JobBuildChunk: WorldIndex : " << chunkWorldIndex << " hmap: " << heightmap << "\n");
		#endif
		return true;
	}

	#ifdef CHUNK_GEN_DEBUG
	D("duplicate JobBuildChunk: WorldIndex : " << chunkWorldIndex << " hmap: " << heightmap << "\n");
	#endif
	return false;
}

void	ChunkGenerator::updateJobsToDo(ChunkGrid& grid) {
	/*
		use condition variable ? it would need a dedicated thread and this func is used alongside grid.updateGrid(playerPos) that has to be called continuously
		move checks outside ?
	*/
	if (this->jobsToDo.size() || this->jobsDone.size()) {
		// wait all jobs to be done and delivered before creating new ones
		// maybe not necessary as there is the mapJob<index, bool>
		return;
	}
	if (!grid.gridShifted) {
		// wait that the grid shifted to search for new jobs
		return;
	}
	std::thread::id					threadID = std::this_thread::get_id();
	std::unique_lock<std::mutex>	chunks_lock(grid.chunks_mutex);
	std::unique_lock<std::mutex>	job_lock(this->job_mutex);

	Math::Vector3	gridSize = grid.getSize();
	Math::Vector3	chunkSize = grid.getChunkSize();
	HMapPtr**		hmaps = grid.getHeightMaps();
	ChunkShPtr***	chunks = grid.getGrid();

	Math::Vector3		index;
	unsigned int		newChunkJobs = 0;
	unsigned int		newHmapJobs = 0;

	D("updateChunkJobsToDo...\n");
	for (int z = 0; z < gridSize.z; z++) {
		for (int x = 0; x < gridSize.x; x++) {

			if (!hmaps[z][x]) {
				index = grid.gridToWorld(Math::Vector3(x, 0, z));
				index.y = 0;//important: even if it is ignored for the HeightMap itself, it is used in a map<Math::vector3, ...>
				if (createHeightmapJob(index, chunkSize))
					newHmapJobs++;
			}

			for (int y = 0; y < gridSize.y; y++) {
				//D("[" << z << "][" << x << "]\n")
				if (!chunks[z][y][x] && hmaps[z][x]) {
					index = grid.gridToWorld(Math::Vector3(x, y, z));
					if (createChunkJob(index, chunkSize, hmaps[z][x]))
						newChunkJobs++;
				}

			}
		}
	}
	
	if (!this->jobsToDo.size() || 
			( newChunkJobs && (newChunkJobs == this->jobsToDo.size()) )
		) {
		// (newChunkJobs == this->jobsToDo.size()) only works because we always wait jobs to finish before creating new ones
		// this is the last time we create job for this gridShift (what happen if there is another gridShift inbetween?)
		grid.gridShifted = false;
	}

	chunks_lock.unlock();
	D("[helper0 " << threadID << "]\tnew jobs: " << newChunkJobs << " chunks, " << newHmapJobs << " hmaps, total jobs: " << this->jobsToDo.size() << "\n");
	if (this->jobsToDo.size()) {
		D("[helper0 " << threadID << "]\tnotifying all...\n");
		this->cv.notify_all();
	}
	job_lock.unlock();// should be done automaticaly with unique_lock
}

void	ChunkGenerator::updateJobsDone(ChunkGrid& grid) {
	std::unique_lock<std::mutex> chunks_lock(grid.chunks_mutex);
	std::unique_lock<std::mutex> job_lock(this->job_mutex);
	if (this->jobsDone.empty()) {
		//D(<< "No jobs to deliver...\n")
		return;
	}
	JobBuildGenerator*	job = nullptr;
	size_t				size = this->jobsDone.size();

	D("updateChunkJobsDone (" << size << ")...\n");
	while (size) {
		job = this->jobsDone.front();
		this->jobsDone.pop_front();

		if (!job->done) {
			std::cerr << "job not done but is in the jobsDone list: " << job << job->getWorldIndex() << ". Exiting...\n";
			Misc::breakExit(-14);
		}
		job->deliver();
		delete job;//no custom destructor: nothing to delete manually for now
		size = this->jobsDone.size();
	}
	if (!this->jobsDone.empty()) {
		std::cerr << "jobsDone not empty, wth? : " << this->jobsDone.size() << "\nexiting...\n";
		Misc::breakExit(-14);
	}
}

void	ChunkGenerator::executeAllJobs(PerlinSettings& perlinSettings, std::string& threadIDstr) {
	JobBuildGenerator* job = nullptr;
	std::unique_lock<std::mutex> job_lock(this->job_mutex);

	while (!this->jobsToDo.empty()) {
		// grab job
		//D(<< threadIDstr << "grabbing job\n")
		job = this->jobsToDo.front();
		this->jobsToDo.pop_front();
		job->assigned = true;

		job_lock.unlock();
		job->execute();
		job_lock.lock();

		job->assigned = false;
		if (job->done)
			this->jobsDone.push_back(job);
		else
			this->jobsToDo.push_back(job);

		//D(<< threadIDstr << "job done " << job->index << "\n")
		job = nullptr;
	}
	D(threadIDstr << "nomore job found... \n");
}

uint8_t			ChunkGenerator::getBuildersAmount() const { return this->_builderAmount; }
std::thread**	ChunkGenerator::getBuilders() const { return this->_builders; }
