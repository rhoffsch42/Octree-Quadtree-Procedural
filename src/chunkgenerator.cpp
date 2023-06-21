#include "chunkgenerator.hpp"
#include "trees.h"
#include "compiler_settings.h"
#include <algorithm>
#include <chrono>
using namespace std::chrono_literals;

#ifdef TREES_DEBUG
 #define TREES_CHUNK_GENERATOR_DEBUG
#endif
#ifdef TREES_CHUNK_GENERATOR_DEBUG 
 #define D(x) std::cout << "[ChunkGenerator] " << x ;
 #define D_(x) x ;
 #define D_SPACER "-- chunkgenerator.cpp -------------------------------------------------\n"
 #define D_SPACER_END "----------------------------------------------------------------\n"
#else 
 #define D(x)
 #define D_(x)
 #define D_SPACER ""
 #define D_SPACER_END ""
#endif

#ifdef USE_OLD_GENERATOR

ChunkGenerator::ChunkGenerator(Math::Vector3 player_pos, const PerlinSettings& perlin_settings, Math::Vector3 chunk_size, Math::Vector3 grid_size, Math::Vector3 grid_size_displayed)
	: chunkSize(chunk_size),
	gridSize(grid_size),//better be odd for equal horizon on every cardinal points
	gridDisplaySize(grid_size_displayed),//better be odd for equal horizon on every cardinal points
	settings(perlin_settings)
{
	D(__PRETTY_FUNCTION__ << "\n")

	this->heightMaps = new HeightMap** [this->gridSize.z];
	for (unsigned int z = 0; z < this->gridSize.z; z++) {
		this->heightMaps[z] = new HeightMap* [this->gridSize.x];
		for (unsigned int x = 0; x < this->gridSize.x; x++) {
			this->heightMaps[z][x] = nullptr;
		}
	}

	this->grid = new Chunk*** [this->gridSize.z];
	for (unsigned int k = 0; k < this->gridSize.z; k++) {
		this->grid[k] = new Chunk* *[this->gridSize.y];
		for (unsigned int j = 0; j < this->gridSize.y; j++) {
			this->grid[k][j] = new Chunk*[this->gridSize.x];
			for (unsigned int i = 0; i < this->gridSize.x; i++) {
				this->grid[k][j][i] = nullptr;
			}
		}
	}

	this->builderAmount = 1;
	this->_chunksReadyForMeshes = true;
	this->playerChangedChunk = true;
	this->gridMemoryMoved = false;
	this->threadsReadyToBuildChunks = 0;

	this->gridIndex = Math::Vector3(0, 0, 0);
	this->gridDisplayIndex = Math::Vector3(0, 0, 0);

	//center the player in the grid display
	this->currentChunkWorldIndex = Math::Vector3(0, 0, 0);
	this->currentChunkWorldIndex += Math::Vector3(
		int(this->gridDisplaySize.x / 2),
		int(this->gridDisplaySize.y / 2),
		int(this->gridDisplaySize.z / 2));

	this->updateGrid(player_pos);
}

ChunkGenerator::~ChunkGenerator() {
	for (unsigned int k = 0; k < this->gridSize.z; k++) {
		for (unsigned int i = 0; i < this->gridSize.z; i++) {
			delete this->heightMaps[k][i];
		}
		delete[] this->heightMaps[k];
	}
	delete[] this->heightMaps;

	for (unsigned int k = 0; k < this->gridSize.z; k++) {
		for (unsigned int j = 0; j < this->gridSize.y; j++) {
			for (unsigned int i = 0; i < this->gridSize.x; i++) {
				delete this->grid[k][j][i];
			}
			delete[] this->grid[k][j];
		}
		delete[] this->grid[k];
	}
	delete[] this->grid;
}

void	ChunkGenerator::th_updater(Cam* cam) {
	Math::Vector3 playerPos;
	while (!this->terminateThreads) {//no mutex for reading?
		this->mutex_cam.lock();
		playerPos = cam->local.getPos();
		this->mutex_cam.unlock();

		if (this->updateGrid(playerPos)) {//player changed chunk
			D("[helper0] grid updated with player pos, as he changed chunk\n")
		}
		this->updateChunkJobsDone();//before updateChunkJobsToDo()
		//D("[helper0] jobs done updated\n")
		this->updateChunkJobsToDo();
		//D("[helper0] jobsToDo updated\n")

		D_(std::cout << ".")
		std::this_thread::sleep_for(100ms);
	}
	D("[helper0] exiting...\n")
}


bool			ChunkGenerator::updateGrid(Math::Vector3 player_pos) {
	std::unique_lock<std::mutex> chunks_lock(this->chunks_mutex);

	Math::Vector3	old_chunk = this->currentChunkWorldIndex;
	this->_updatePlayerPos(player_pos);
	Math::Vector3	diff = this->currentChunkWorldIndex - old_chunk;
	if (diff.len2() == 0)
		return false;

	this->playerChangedChunk = true;
	Math::Vector3	diffGrid = this->_calculateGridDiff(diff);
	if (diffGrid.len() == 0)
		return true;

	std::vector<Chunk*>		chunksToDelete;
	std::vector<HeightMap*>	hmapsToDelete;
	this->_translateGrid(diffGrid, &chunksToDelete, &hmapsToDelete);

	//debug checks before rebuild
	D("checks...\n" << this->getGridChecks())
	this->_chunksReadyForMeshes = false;
	chunks_lock.unlock();
	this->cv.notify_all();

	//send useless data to dustbins
	this->trash_mutex.lock();
	this->trashHeightMaps.insert(this->trashHeightMaps.end(), hmapsToDelete.begin(), hmapsToDelete.end());
	this->trashChunks.insert(this->trashChunks.end(), chunksToDelete.begin(), chunksToDelete.end());
	this->trash_mutex.unlock();
	return true;
}
void			ChunkGenerator::_updatePlayerPos(const Math::Vector3& player_pos) {
	/*
		update chain :
		player_pos > currentChunkWorldIndex > gridDisplayIndex > gridIndex
	*/
	this->playerPos = player_pos;
	this->currentChunkWorldIndex.x = int(this->playerPos.x / this->chunkSize.x);
	this->currentChunkWorldIndex.y = int(this->playerPos.y / this->chunkSize.y);
	this->currentChunkWorldIndex.z = int(this->playerPos.z / this->chunkSize.z);
	if (this->playerPos.x < 0) // cauze if x=-32..+32 ----> x / 32 = 0; 
		this->currentChunkWorldIndex.x--;
	if (this->playerPos.y < 0) // same
		this->currentChunkWorldIndex.y--;
	if (this->playerPos.z < 0) // same
		this->currentChunkWorldIndex.z--;
}
Math::Vector3	ChunkGenerator::_calculateGridDiff(Math::Vector3 playerdiff) {
	//calculate if we need to move the memory grid and load new chunks
	Math::Vector3	gridDiff;
	Math::Vector3	new_gridIndex;
	Math::Vector3	new_worldDisplayIndex = this->gridIndex + this->gridDisplayIndex + playerdiff;
	Math::Vector3	relDisplayIndex = new_worldDisplayIndex - this->gridIndex;
	/*
		moving the display grid in the world, and adapt the memory grid after it, independently from the original player offset
		if for any reason the player offset changes, this will not catch it and will continue moving grids based on above diff
	*/

	if (relDisplayIndex.x < 0)
		new_gridIndex.x = std::min(new_worldDisplayIndex.x, this->gridIndex.x);
	else
		new_gridIndex.x = std::max(new_worldDisplayIndex.x + this->gridDisplaySize.x - this->gridSize.x, this->gridIndex.x);
	if (relDisplayIndex.y < 0)
		new_gridIndex.y = std::min(this->gridIndex.y, new_worldDisplayIndex.y);
	else
		new_gridIndex.y = std::max(new_worldDisplayIndex.y + this->gridDisplaySize.y - this->gridSize.y, this->gridIndex.y);
	if (relDisplayIndex.z < 0)
		new_gridIndex.z = std::min(this->gridIndex.z, new_worldDisplayIndex.z);
	else
		new_gridIndex.z = std::max(new_worldDisplayIndex.z + this->gridDisplaySize.z - this->gridSize.z, this->gridIndex.z);
	gridDiff = new_gridIndex - this->gridIndex;
	this->gridDisplayIndex = new_worldDisplayIndex - new_gridIndex;
	this->gridIndex = new_gridIndex;

	//D("\t--gridSize       \t" << this->gridSize << "\n")
	//D("\t--gridDisplaySize\t" << this->gridDisplaySize << "\n")
	D("\t--playerdiff            \t" << playerdiff << "\n")
	D("\t--gridDiff              \t" << gridDiff << "\n")
	D("\t--gridWorldIndex       \t" << this->gridIndex << "\n")
	D("\t--currentChunkWorldIndex\t" << this->currentChunkWorldIndex << "\n")
	D("\t--gridDisplayIndex     \t" << this->gridDisplayIndex << "\n")

	return gridDiff;
}
void			ChunkGenerator::_translateGrid(Math::Vector3 gridDiff, std::vector<Chunk*>* chunksToDelete, std::vector<HeightMap*>* hmapsToDelete) {
	//grid (memory)
	std::vector<int>		indexZ;
	std::vector<int>		indexY;
	std::vector<int>		indexX;
	std::vector<Chunk*>		chunks;
	std::vector<HeightMap*>	hmaps;

	//store the chunks and heightmaps with their new index 3D
	for (int z = 0; z < this->gridSize.z; z++) {
		for (int y = 0; y < this->gridSize.y; y++) {
			for (int x = 0; x < this->gridSize.x; x++) {
				indexZ.push_back(z - gridDiff.z);//each chunk goes in the opposite way of the player movement
				indexY.push_back(y - gridDiff.y);
				indexX.push_back(x - gridDiff.x);
				chunks.push_back(this->grid[z][y][x]);
				this->grid[z][y][x] = nullptr; // if the main RENDER thread reads here, he is fucked
				if (y == 0) {// there is only one hmap for an entire column of chunks
					hmaps.push_back(this->heightMaps[z][x]);
					this->heightMaps[z][x] = nullptr;// if the main RENDER thread reads here, he is fucked
				}
				else {
					hmaps.push_back(nullptr);	// to match the chunks index for the next hmap
				}
			}
		}
	}
	D("chunks referenced: " << chunks.size() << "\n")
	D("hmaps referenced (with nulls): " << hmaps.size() << "\n")

	//reassign the chunks
	D("reassigning chunks...\n")
	for (size_t n = 0; n < chunks.size(); n++) {
		if (indexZ[n] < this->gridSize.z && indexY[n] < this->gridSize.y && indexX[n] < this->gridSize.x \
			&& indexZ[n] >= 0 && indexY[n] >= 0 && indexX[n] >= 0) {
			this->grid[indexZ[n]][indexY[n]][indexX[n]] = chunks[n];
		}
		else {//is outside of the memory grid
			chunksToDelete->push_back(chunks[n]);
			//meaning the opposite side has to be nulled
			//int x = indexX[n] + this->gridSize.x * (indexX[n] < 0 ? 1 : -1)
			//int y = indexY[n] + this->gridSize.y * (indexY[n] < 0 ? 1 : -1)
			//int iz = indexZ[n] + this->gridSize.z * (indexZ[n] < 0 ? 1 : -1)
			//int x = int(indexX[n] + this->gridSize.x) % int(this->gridSize.x);
			//int y = int(indexY[n] + this->gridSize.y) % int(this->gridSize.y);
			//int z = int(indexZ[n] + this->gridSize.z) % int(this->gridSize.z);
			//this->grid[z][y][x] = nullptr;
		}
	}

	//reassign the heightmaps
	D("reassigning heightmaps...\n")
	for (size_t n = 0; n < hmaps.size(); n = n + 4) {
		if (hmaps[n]) {
			if (indexZ[n] < this->gridSize.z && indexX[n] < this->gridSize.x \
				&& indexZ[n] >= 0 && indexX[n] >= 0) {
				this->heightMaps[indexZ[n]][indexX[n]] = hmaps[n];
			}
			else {//is outside of the memory grid
				hmapsToDelete->push_back(hmaps[n]);
				//meaning the opposite side has to be nulled
				//int x = indexX[n] + this->gridSize.x * (indexX[n] < 0 ? 1 : -1)
				//int z = indexZ[n] + this->gridSize.z * (indexZ[n] < 0 ? 1 : -1)
				/*
				int x = int(indexX[n] + this->gridSize.x) % int(this->gridSize.x);
				int z = int(indexZ[n] + this->gridSize.z) % int(this->gridSize.z);
				this->heightMaps[z][x] = nullptr;
				*/
			}
		}
	}
}

void	ChunkGenerator::updateChunkJobsToDo() {
	std::unique_lock<std::mutex> chunks_lock(this->chunks_mutex);
	std::unique_lock<std::mutex> job_lock(this->job_mutex);
	if (this->jobsToDo.size() || this->jobsDone.size()) {
		// wait all jobs to be done and delivered before creating new ones
		return;
	}

	Math::Vector3		index;
	JobBuildChunk*		job_chunk = nullptr;
	JobBuildHeighMap*	job_hmap = nullptr;
	unsigned int		cn = 0;
	unsigned int		hn = 0;
	std::thread::id		threadID = std::this_thread::get_id();

	for (int z = 0; z < this->gridSize.z; z++) {
		for (int x = 0; x < this->gridSize.x; x++) {

			if (!this->heightMaps[z][x]) {
				index = this->gridToWorld(Math::Vector3(x, 0, z));
				index.y = 0;//same hmap for all Y, Y is ignored anyway
				if (!this->map_jobsHmap[index]) {
					job_hmap = new JobBuildHeighMap(index, this->chunkSize);
					this->jobsToDo.push_back(job_hmap);
					this->map_jobsHmap[index] = true;
					#ifdef CHUNK_GEN_DEBUG
					D("new JobBuildHeighMap: " << x << ":" << z << " world: " << index << "\n")
					#endif
					hn++;
				} else {// meaning the job has already been created in a previous update, but is not done yet
					#ifdef CHUNK_GEN_DEBUG
					D("already created JobBuildHeighMap: " << x << ":" << z << " world: " << index << "\n")
					#endif
				}
			}

			for (int y = 0; y < this->gridSize.y; y++) {

				if (!this->grid[z][y][x] && this->heightMaps[z][x]) {
					index = this->gridToWorld(Math::Vector3(x, y, z));
					if (!this->map_jobsChunk[index] && this->heightMaps[z][x]->dispose()) {
						job_chunk = new JobBuildChunk(index, this->chunkSize, this->heightMaps[z][x]);
						this->jobsToDo.push_back(job_chunk);
						this->map_jobsChunk[index] = true;
						#ifdef CHUNK_GEN_DEBUG
						D("new JobBuildChunk: " << x << ":" << y << ":" << z << " world: " << index << " hmap: " << this->heightMaps[z][x] << "\n")
						#endif
						cn++;
					} else {
						#ifdef CHUNK_GEN_DEBUG
						D("duplicate JobBuildChunk: " << x << ":" << y << ":" << z << " world: " << index << " hmap: " << this->heightMaps[z][x] << "\n")
						#endif
					}
				}

			}
		}
	}

	job_lock.unlock();
	chunks_lock.unlock();
	if (this->jobsToDo.size()) {
		D("[helper0 " << threadID << "]\tnew jobs: " << cn << " chunks, " << hn << " hmaps, total: " << this->jobsToDo.size() << "\n")
		D("[helper0 " << threadID << "]\tnotifying all...\n")
		this->cv.notify_all();
	}
}

void	ChunkGenerator::updateChunkJobsDone() {
	std::unique_lock<std::mutex> chunks_lock(this->chunks_mutex);
	std::unique_lock<std::mutex> job_lock(this->job_mutex);
	if (this->jobsDone.empty()) {
		//D(<< "No jobs to deliver...\n")
		return;
	}
	JobBuildGenerator* job = nullptr;

	while (this->jobsDone.size()) {
		job = this->jobsDone.front();
		this->jobsDone.pop_front();

		if (!job->done) {
			std::cerr << "job not done but is in the jobsDone list: " << job << job->index << "\n";
			std::cerr << "exiting...\n";
			Misc::breakExit(-14);
		}
		job->deliver(*this);
		delete job;//no custom destructor: nothing to delete manually for now
	}
	if (!this->jobsDone.empty()) {
		std::cerr << "jobsDone not empty, wth? : " << this->jobsDone.size() << "\nexiting...\n";
		Misc::breakExit(-14);
	}
	//this->chunksChanged = true;
}

void	ChunkGenerator::executeAllJobs(PerlinSettings& perlinSettings, std::string& threadIDstr) {
	JobBuildGenerator* job = nullptr;

	while (!this->jobsToDo.empty()) {
		// grab job
		//D(<< threadIDstr << "grabbing job\n")
		job = this->jobsToDo.front();
		this->jobsToDo.pop_front();
		job->assigned = true;

		//job_lock.unlock();
		this->chunks_mutex.lock();
		job->execute(perlinSettings);
		this->chunks_mutex.unlock();
		//job_lock.lock();

		job->assigned = false;
		if (job->done)
			this->jobsDone.push_back(job);
		else
			this->jobsToDo.push_back(job);

		//D(<< threadIDstr << "job done " << job->index << "\n")
		job = nullptr;
	}
	D(threadIDstr << "nomore job found... \n")
}

void	ChunkGenerator::th_builders(GLFWwindow* context) {
	D(__PRETTY_FUNCTION__ << "\n")
	//glfwMakeContextCurrent(context);
	std::thread::id threadID = std::this_thread::get_id();
	std::stringstream ss;
	ss << threadID;
	std::string	threadIDstr = "[" + ss.str() + "]\t";
	D(threadIDstr << " started\n")

	//build job variables
	PerlinSettings		perlinSettings(this->settings);//if they change later, we have to update them, cpy them when finding a job?
	JobBuildGenerator*	job = nullptr;
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
			job->execute(perlinSettings);
			job_lock.lock();

			job->assigned = false;
			if (job->done) {
				this->jobsDone.push_back(job);
				thread_jobsdone++;
			} else
				this->jobsToDo.push_back(job);

			//D(threadIDstr << "job done " << job->index << "\n")
			job = nullptr;
		}
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

bool	ChunkGenerator::glth_buildMeshesAndMapTiles() {
	D(__PRETTY_FUNCTION__ << "\n")
	if (this->_chunksReadyForMeshes) {
		for (unsigned int k = 0; k < this->gridSize.z; k++) {
			for (unsigned int i = 0; i < this->gridSize.x; i++) {
				this->heightMaps[k][i]->glth_buildPanel();
				for (unsigned int j = 0; j < this->gridSize.y; j++) {
					this->grid[k][j][i]->glth_buildMesh();
					D_(std::cout << "[" << k << "-" << j << "-" << i << "] mesh built:" << this->grid[k][j][i])
				}
			}
		}
		this->_chunksReadyForMeshes = false;
		D("ChunkGenerator meshes built\n")
		return true;
	}
	else {
		//D("Chunks are not ready yet\n")
		return false;
	}
}

void	ChunkGenerator::glth_loadChunksToGPU() {
	/*
		not ideal. each time there are new chunks on 1 dimension, it rechecks the entire grid
		ex: grid 1000x1000x1000. 1000x1000 new chunks to load, but 1000x1000x1000 iterations

		refacto: new job: change the chunk job deliver to create a new job for the main thread
		if the chunk isn't empty ofc.
	*/
	D("Loading chunks to the GPU... ")
	Chunk* c = nullptr;
	for (unsigned int k = 0; k < gridSize.z; k++) {
		for (unsigned int j = 0; j < gridSize.y; j++) {
			for (unsigned int i = 0; i < gridSize.x; i++) {
				c = grid[k][j][i];
				//at this point, the chunk might not be generated yet
				if (c && !c->mesh[0]) {//why 0!
					c->glth_buildMesh();
				}
			}
		}
	}
	D_(std::cout << "Done\n")
}

void	ChunkGenerator::pushDisplayedChunks(std::list<Object*>* dst, unsigned int tesselation_lvl) const {
	/*
		same as in glth_loadChunks() but more complicated
		we would need to select the chunks to remove (change the render list with a map?),
		and push only the new chunks
	*/
	D("Pushing chunks to a list<Object*> ... ")
	Chunk* c = nullptr;
	Math::Vector3	endDisplay = gridDisplayIndex + gridDisplaySize;
	for (unsigned int k = gridDisplayIndex.z; k < endDisplay.z; k++) {
		for (unsigned int j = gridDisplayIndex.y; j < endDisplay.y; j++) {
			for (unsigned int i = gridDisplayIndex.x; i < endDisplay.x; i++) {
				c = grid[k][j][i];
				//at this point, the chunk might not be generated yet
				if (c && c->mesh[tesselation_lvl]) {//mesh can be null if the chunk is empty (see glth_buildMesh())
					dst->push_back(c->mesh[tesselation_lvl]);
					//D("list : pushing chunck [" << k << "][" << j << "][" << i << "] " << c << " tessLvl " << tesselation_lvl << "\n")
				}
			}
		}
	}
	D_(std::cout << "Done\n")
}

unsigned int	ChunkGenerator::pushDisplayedChunks(Object** dst, unsigned int tesselation_lvl, unsigned int starting_index) const {
	/*
		same problem as in glth_loadChunks() but more complicated
		we would need to select the chunks to remove (change the render list with a map?),
		and push only the new chunks
	*/
	D("Pushing chunks to a Object* array ... ")
		Chunk* c = nullptr;
	Math::Vector3	endDisplay = gridDisplayIndex + gridDisplaySize;
	for (unsigned int k = gridDisplayIndex.z; k < endDisplay.z; k++) {
		for (unsigned int j = gridDisplayIndex.y; j < endDisplay.y; j++) {
			for (unsigned int i = gridDisplayIndex.x; i < endDisplay.x; i++) {
				c = grid[k][j][i];
				//at this point, the chunk might not be generated yet
				if (c && c->mesh[tesselation_lvl]) {//mesh can be null if the chunk is empty (see glth_buildMesh())
					dst[starting_index] = c->mesh[tesselation_lvl];
					//D("array : pushing chunck [" << k << "][" << j << "][" << i << "] " << c << " tessLvl " << tesselation_lvl << "\n")
					starting_index++;
				}
			}
		}
	}
	dst[starting_index] = nullptr;
	//D("pushDisplayedChunks() ret starting_index " << starting_index << "\n")
	D_(std::cout << "Done\n")
	return starting_index;
}

Math::Vector3	ChunkGenerator::getGridDisplayStart() const {
	return this->gridDisplayIndex;
	/*
		-Math::Vector3(
		int(this->gridDisplaySize.x / 2),
		int(this->gridDisplaySize.y / 2),
		int(this->gridDisplaySize.z / 2));
	*/
}

std::string		ChunkGenerator::toString() const {
	std::stringstream ss;
	ss << "currentChunkWorldIndex: " << this->currentChunkWorldIndex.toString() << "\n";
	for (int k = 0; k < this->gridSize.z; k++) {
		for (int j = 0; j < this->gridSize.y; j++) {
			for (int i = 0; i < this->gridSize.x; i++) {
				ss << "[" << k << "]";
				ss << "[" << j << "]";
				ss << "[" << i << "]\t" << this->grid[k][j][i]->toString() << "\n";
			}
		}
	}
	return ss.str();
}

std::string		ChunkGenerator::getGridChecks() const {
	std::stringstream ss;
	int		nonulls = 0;
	int		hmnonulls = 0;
	int nulls = 0;
	int hmnulls = 0;
	for (int k = 0; k < this->gridSize.z; k++) {
		for (int j = 0; j < this->gridSize.y; j++) {
			for (int i = 0; i < this->gridSize.x; i++) {
				if (grid[k][j][i])
					nonulls++;
				else
					nulls++;
				if (j == 0) {
					if (this->heightMaps[k][i])
						hmnonulls++;
					else {
						hmnulls++;
					}
				}
			}
		}
	}
	ss << " > grid checks\n";
	ss << " > nonull: " << nonulls << "\n";
	ss << " > nulls: " << nulls << "\n";
	ss << " >> total: " << nonulls + nulls << "\n";
	ss << " > hmap nonull: " << hmnonulls << "\n";
	ss << " > hmnulls: " << hmnulls << "\n";
	ss << " >> total: " << hmnonulls + hmnulls << "\n";
	return ss.str();
}

Math::Vector3	ChunkGenerator::worldToGrid(const Math::Vector3& index) const {
	return (index - this->gridIndex);
	//Math::Vector3	gridStart = this->gridIndex - Math::Vector3(
	//	int(this->gridSize.x / 2),
	//	int(this->gridSize.y / 2),
	//	int(this->gridSize.z / 2));
	//return (index - gridStart);
}

Math::Vector3	ChunkGenerator::gridToWorld(const Math::Vector3& index) const {
	return (this->gridIndex + index);
	//Math::Vector3	gridStart = this->gridIndex - Math::Vector3(
	//	int(this->gridSize.x / 2),
	//	int(this->gridSize.y / 2),
	//	int(this->gridSize.z / 2));
	//return (gridStart + index);
}

bool	ChunkGenerator::try_deleteUnusedData() {
	if (this->trash_mutex.try_lock()) {
		this->_deleteUnusedData();
		this->trash_mutex.unlock();
		return true;
	}
	return false;
}

void	ChunkGenerator::_deleteUnusedData() {
	//todo : use unique_ptr in the vectors
	size_t size = this->trashHeightMaps.size();
	if (size) {
		for (auto it = this->trashHeightMaps.begin(); it != this->trashHeightMaps.end(); /*it++;*/) { //avoid it invalidation, inc manually when needed
			if ((*it)->getdisposedCount() == 0) {
				delete (*it);
				it = this->trashHeightMaps.erase(it);
			} else {
				it++;
			}
		}
		D(" X deleted hmaps: " << (size - this->trashHeightMaps.size()) << "\n")
	}
	size = this->trashChunks.size();
	if (size) {
		for (auto& ptr : this->trashChunks) {
			delete ptr;
			ptr = nullptr;
		}
		D(" X deleted chunks: " << (size - this->trashChunks.size()) << "\n")
		this->trashChunks.erase(std::remove(this->trashChunks.begin(), this->trashChunks.end(), nullptr), this->trashChunks.end());
	}
}

#else

ChunkGenerator::ChunkGenerator(const PerlinSettings& perlin_settings) : settings(perlin_settings) {
	D(__PRETTY_FUNCTION__ << "\n")

	this->chunksChanged = false;
	this->builderAmount = 1;
	this->threadsReadyToBuildChunks = 0;
}

ChunkGenerator::~ChunkGenerator() {}

void	ChunkGenerator::th_updater(Cam* cam, ChunkGrid* grid) {
	Math::Vector3 playerPos;
	while (!this->terminateThreads) {//no mutex for reading?
		this->mutex_cam.lock();
		playerPos = cam->local.getPos();
		this->mutex_cam.unlock();

		if (grid->updateGrid(playerPos)) {//player changed chunk
			D("[helper0] grid updated with player pos, as he changed chunk\n")
		}
		else {
			//D("[helper0] grid unchanged, player in same grid\n")
		}
		#ifdef USE_OLD_GENERATOR
		#else
		this->cv.notify_all();
		#endif
		this->updateChunkJobsDone(*grid);//before updateChunkJobsToDo()
		//D("[helper0] jobs done updated\n")
		this->updateChunkJobsToDo(*grid);
		//D("[helper0] jobsToDo updated\n")

		D_(std::cout << ".")
			std::this_thread::sleep_for(100ms);
	}
	D("[helper0] exiting...\n")
}

bool	ChunkGenerator::createHeightmapJob(Math::Vector3 hmapWorldIndex, Math::Vector3 chunkSize) {
	if (!this->map_jobsHmap[hmapWorldIndex]) {
		this->jobsToDo.push_back(new JobBuildHeighMap(hmapWorldIndex, chunkSize));
		this->map_jobsHmap[hmapWorldIndex] = true;

		#ifdef CHUNK_GEN_DEBUG
		D("new JobBuildHeighMap: WorldIndex : " << hmapWorldIndex << "\n")
		#endif
		return true;
	}

	#ifdef CHUNK_GEN_DEBUG
	D("already created JobBuildHeighMap: WorldIndex : " << hmapWorldIndex << "\n")
	#endif
	return false;
}

bool	ChunkGenerator::createChunkJob(const Math::Vector3 chunkWorldIndex, const Math::Vector3 chunkSize, HeightMap* heightmap) {
	if (!this->map_jobsChunk[chunkWorldIndex] && heightmap->dispose()) {
		this->jobsToDo.push_back(new JobBuildChunk(chunkWorldIndex, chunkSize, heightmap));
		this->map_jobsChunk[chunkWorldIndex] = true;

		#ifdef CHUNK_GEN_DEBUG
		D("new JobBuildChunk: WorldIndex : " << chunkWorldIndex << " hmap: " << heightmap << "\n")
		#endif
		return true;
	}

	#ifdef CHUNK_GEN_DEBUG
	D("duplicate JobBuildChunk: WorldIndex : " << chunkWorldIndex << " hmap: " << heightmap << "\n")
	#endif
	return false;
}


void	ChunkGenerator::updateChunkJobsToDo(ChunkGrid& grid) {
	std::thread::id					threadID = std::this_thread::get_id();
	std::unique_lock<std::mutex>	chunks_lock(grid.chunks_mutex);
	std::unique_lock<std::mutex>	job_lock(this->job_mutex);

	if (this->jobsToDo.size() || this->jobsDone.size()) {
		// wait all jobs to be done and delivered before creating new ones
		return;
	}

	Math::Vector3	gridSize = grid.getSize();
	Math::Vector3	chunkSize = grid.getChunkSize();
	HMapPtr**		hmaps = grid.getHeightMaps();
	ChunkPtr***		chunks = grid.getGrid();

	Math::Vector3		index;
	unsigned int		newChunkJobs = 0;
	unsigned int		newHmapJobs = 0;

	for (int z = 0; z < gridSize.z; z++) {
		for (int x = 0; x < gridSize.x; x++) {

			if (!hmaps[z][x]) {
				index = grid.gridToWorld(Math::Vector3(x, 0, z));
				if (createHeightmapJob(index, chunkSize))
					newHmapJobs++;
			}

			for (int y = 0; y < gridSize.y; y++) {

				if (!chunks[z][y][x] && hmaps[z][x]) {
					index = grid.gridToWorld(Math::Vector3(x, y, z));
					if (createChunkJob(index, chunkSize, hmaps[z][x]))
						newChunkJobs++;
				}

			}
		}
	}

	job_lock.unlock();
	chunks_lock.unlock();
	if (this->jobsToDo.size()) {
		D("[helper0 " << threadID << "]\tnew jobs: " << newChunkJobs << " chunks, " << newHmapJobs << " hmaps, total: " << this->jobsToDo.size() << "\n")
		D("[helper0 " << threadID << "]\tnotifying all...\n")
		this->cv.notify_all();
	}
}

void	ChunkGenerator::updateChunkJobsDone(ChunkGrid& grid) {
	std::unique_lock<std::mutex> chunks_lock(grid.chunks_mutex);
	std::unique_lock<std::mutex> job_lock(this->job_mutex);
	if (this->jobsDone.empty()) {
		//D(<< "No jobs to deliver...\n")
		return;
	}
	JobBuildGenerator* job = nullptr;

	while (this->jobsDone.size()) {
		job = this->jobsDone.front();
		this->jobsDone.pop_front();

		if (!job->done) {
			std::cerr << "job not done but is in the jobsDone list: " << job << job->index << "\n";
			std::cerr << "exiting...\n";
			Misc::breakExit(-14);
		}
		#ifdef USE_OLD_GENERATOR
		job->deliver(*this);
		#else
		job->deliver(*this, grid);
		#endif
		delete job;//no custom destructor: nothing to delete manually for now
	}
	if (!this->jobsDone.empty()) {
		std::cerr << "jobsDone not empty, wth? : " << this->jobsDone.size() << "\nexiting...\n";
		Misc::breakExit(-14);
	}
	//this->chunksChanged = true;
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
		job->execute(perlinSettings);
		job_lock.lock();

		job->assigned = false;
		if (job->done)
			this->jobsDone.push_back(job);
		else
			this->jobsToDo.push_back(job);

		//D(<< threadIDstr << "job done " << job->index << "\n")
		job = nullptr;
	}
	D(threadIDstr << "nomore job found... \n")
}

void	ChunkGenerator::th_builders(GLFWwindow* context) {
	D(__PRETTY_FUNCTION__ << "\n")
		//glfwMakeContextCurrent(context);
		std::thread::id threadID = std::this_thread::get_id();
	std::stringstream ss;
	ss << threadID;
	std::string	threadIDstr = "[" + ss.str() + "]\t";
	D(threadIDstr << " started\n")

		//build job variables
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
			job->execute(perlinSettings);
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

bool	ChunkGenerator::try_deleteUnusedData() {
	if (this->trash_mutex.try_lock()) {
		this->_deleteUnusedData();
		this->trash_mutex.unlock();
		return true;
	}
	return false;
}

void	ChunkGenerator::_deleteUnusedData() {
	//todo : use unique_ptr in the vectors
	size_t size = this->trashHeightMaps.size();
	if (size) {
		for (auto it = this->trashHeightMaps.begin(); it != this->trashHeightMaps.end(); /*it++;*/) { //avoid it invalidation, inc manually when needed
			if ((*it)->getdisposedCount() == 0) {
				delete (*it);
				it = this->trashHeightMaps.erase(it);
			}
			else {
				it++;
			}
		}
		D(" X deleted hmaps: " << (size - this->trashHeightMaps.size()) << "\n")
	}
	size = this->trashChunks.size();
	if (size) {
		for (auto& ptr : this->trashChunks) {
			delete ptr;
			ptr = nullptr;
		}
		D(" X deleted chunks: " << (size - this->trashChunks.size()) << "\n")
			this->trashChunks.erase(std::remove(this->trashChunks.begin(), this->trashChunks.end(), nullptr), this->trashChunks.end());
	}
}

#endif