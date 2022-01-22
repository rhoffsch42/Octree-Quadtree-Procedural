#include "chunkgenerator.hpp"
#include "compiler_settings.h"
#include <algorithm>

ChunkGenerator::ChunkGenerator(Math::Vector3 player_pos, const PerlinSettings& perlin_settings, Math::Vector3 chunk_size, Math::Vector3 grid_size, Math::Vector3 grid_size_displayed)
	: chunkSize(chunk_size),
	gridSize(grid_size),//better be odd for equal horizon on every cardinal points
	gridDisplaySize(grid_size_displayed),//better be odd for equal horizon on every cardinal points
	settings(perlin_settings)
{
	std::cout << "_ " << __PRETTY_FUNCTION__ << "\n";
/*
	retfacto grid with 3d list
	std::list< std::list< std::list<Chunk*> > >	gr;
	//move forward +Z
	gr.push_back
	gr.pop_front
	//move backward -Z
	gr.pop_back
	gr.push_front
	//move up +Y
	gr[z].push_back
	gr[z].pop_front
	//move down -Y
	gr[z].pop_back
	gr[z].push_front
	//move left -X
	gr[z][y].pop_back
	gr[z][y].push_front
	//move right +X
	gr[z][y].push_back
	gr[z][y].pop_front
*/

	// current tile index
	this->updatePlayerPos(player_pos);
	this->gridPos = this->currentChunk;
	this->gridDisplayIndex = Math::Vector3(
		int(this->gridSize.x / 2),
		int(this->gridSize.y / 2),
		int(this->gridSize.z / 2));
	//clamp grid display
	this->gridDisplaySize.x = std::clamp(this->gridDisplaySize.x, 1.0f, this->gridSize.x);
	this->gridDisplaySize.y = std::clamp(this->gridDisplaySize.y, 1.0f, this->gridSize.y);
	this->gridDisplaySize.z = std::clamp(this->gridDisplaySize.z, 1.0f, this->gridSize.z);

	//init grid
	Math::Vector3 v(this->gridSize);
	v.div(2);
	Math::Vector3	smallestChunk(this->currentChunk);
	//smallestChunk.sub(v);
	smallestChunk.sub((int)v.x, (int)v.y, (int)v.z);//a Vector3i would be better

	this->heightMaps = new HeightMap * *[this->gridSize.z];
	for (unsigned int z = 0; z < this->gridSize.z; z++) {
		this->heightMaps[z] = new HeightMap * [this->gridSize.x];
		for (unsigned int x = 0; x < this->gridSize.x; x++) {
			this->heightMaps[z][x] = nullptr;
		}
	}

	this->grid = new Chunk * **[this->gridSize.z];
	for (unsigned int k = 0; k < this->gridSize.z; k++) {
		this->grid[k] = new Chunk * *[this->gridSize.y];
		for (unsigned int j = 0; j < this->gridSize.y; j++) {
			this->grid[k][j] = new Chunk * [this->gridSize.x];
			for (unsigned int i = 0; i < this->gridSize.x; i++) {
				this->grid[k][j][i] = nullptr;
			}
		}
	}
	this->builderAmount = 0;
	this->_chunksReadyForMeshes = true;
	this->playerChangedChunk = true;
	this->gridMemoryMoved = false;
	this->threadsReadyToBuildChunks = 0;
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

static void		initValues(float diff, float& inc, float& start, float& end, float& endShift, float intersection, float size) {
	if (diff >= 0) {
		inc = 1;
		start = 0;//inclusive, we start here
		end = size;//exclusive cause we will use `!=` for the comparison
	}
	else {
		inc = -1;
		start = size - 1;//inclusive, we start here
		end = 0 - 1;//exclusive cause we will use `!=` for the comparison
	}
	endShift = start + inc * intersection;//exclusive
}

static int	calcExceed(int posA, int sizeA, int posB, int sizeB) {
	/*
		A========.========A
			B----.----B

		sizeB <= sizeA
	*/
	if (sizeB > sizeA) {
		std::cout << "Error: Grid Displayed is larger than the total Grid in memory\n";
		exit(4);
	}
	int sign = posA <= posB ? 1 : -1;							// set up the sign to mirror the diff if it is negative to handle only positive part
	int diffpos = sign * (posB - posA);							// mirrored if negative
	int diffside = std::max((sizeB - sizeA) / 2 + diffpos, 0);	// std::max(diff,0) : if the diff is negative, nothing exceeds the bounds
	return sign * diffside;										// mirrored back if sign was negative
	/*
		.========A
		  .----B		//B does not exceed A
			  .----C    //C exceeds A
	*/
}

//return true if player step in another chunk
bool	ChunkGenerator::updateGrid_old(Math::Vector3 player_pos) {
	std::unique_lock<std::mutex> chunks_lock(this->chunks_mutex);
	//more direct but more complicated than the new updateGrid()
	Math::Vector3	old_tile = this->currentChunk;
	this->updatePlayerPos(player_pos);
	Math::Vector3	diff(this->currentChunk);
	diff.sub(old_tile);

	if (diff.len()) {//gridDisplayed
		std::cout << "diff: " << diff.toString() << "\n";

		//calculate if we need to move the memory grid and load new chunks
		Math::Vector3	diffGrid;
		diffGrid.x = calcExceed(this->gridPos.x, this->gridSize.x, this->currentChunk.x, this->gridDisplaySize.x);
		diffGrid.y = calcExceed(this->gridPos.y, this->gridSize.y, this->currentChunk.y, this->gridDisplaySize.y);
		diffGrid.z = calcExceed(this->gridPos.z, this->gridSize.z, this->currentChunk.z, this->gridDisplaySize.z);
		this->gridPos.add(diffGrid);
		std::cout << "\t--diff        \t" << diff.toString() << "\n";
		std::cout << "\t--diffGrid    \t" << diffGrid.toString() << "\n";
		std::cout << "\t--gridPos     \t" << this->gridPos.toString() << "\n";
		std::cout << "\t--currentChunk\t" << this->currentChunk.toString() << "\n";

		this->gridDisplayIndex.add(diff);
		this->gridDisplayIndex.sub(diffGrid);
		if (diffGrid.len()) {//grid (memory)

			Math::Vector3	halfGrid(int(this->gridSize.x / 2), int(this->gridSize.y / 2), int(this->gridSize.z / 2));//a Vector3i would be better
			//Math::Vector3	smallestChunk(this->currentChunk); smallestChunk.sub(halfGrid);
			Math::Vector3	smallestChunk(this->gridPos); smallestChunk.sub(halfGrid);

			Math::Vector3	absdiff(std::abs(diffGrid.x), std::abs(diffGrid.y), std::abs(diffGrid.z));
			Math::Vector3	intersection(this->gridSize);
			intersection.sub(absdiff);

			std::cout << "size of intersection cube dimension:\n" << intersection.toString() << "\n";

			//now we define the incrementers, start points and end points to shift the intersection cube and delete unused data
			Math::Vector3	inc, start, end, endShift;
			initValues(diffGrid.x, inc.x, start.x, end.x, endShift.x, intersection.x, this->gridSize.x);
			initValues(diffGrid.y, inc.y, start.y, end.y, endShift.y, intersection.y, this->gridSize.y);
			initValues(diffGrid.z, inc.z, start.z, end.z, endShift.z, intersection.z, this->gridSize.z);

			Math::Vector3	progress(0, 0, 0);
			for (int k = start.z; k != end.z; k += inc.z) {
				progress.y = 0;
				for (int j = start.y; j != end.y; j += inc.y) {
					progress.x = 0;
					for (int i = start.x; i != end.x; i += inc.x) {
						if (progress.z < absdiff.z || progress.y < absdiff.y || progress.x < absdiff.x) {//if we override a chunk with an existing one
							if (CHUNK_GEN_DEBUG) { std::cout << "delete " << i << ":" << j << ":" << k << "\n"; }
							delete this->grid[k][j][i];
							if (progress.y == 0 && (absdiff.x != 0 || absdiff.z != 0)) {//do it only once, for all the Y, only if we go up/down
								delete this->heightMaps[k][i];
								if (CHUNK_GEN_DEBUG) { std::cout << "deleting hmap " << i << ":" << k << "\n"; }
							}
						}
						if (progress.z < intersection.z && progress.y < intersection.y && progress.x < intersection.x) {
							if (CHUNK_GEN_DEBUG) { std::cout << i << ":" << j << ":" << k << " becomes " << i + diffGrid.x << ":" << j + diffGrid.y << ":" << k + diffGrid.z << "\n"; }
							this->grid[k][j][i] = this->grid[k + int(diffGrid.z)][j + int(diffGrid.y)][i + int(diffGrid.x)];
							this->grid[k + int(diffGrid.z)][j + int(diffGrid.y)][i + int(diffGrid.x)] = nullptr;//useless but cleaner, it will become another chunk on next pass (generated or shifted)
							if (progress.y == 0) {//do it only once, for all the Y
								this->heightMaps[k][i] = this->heightMaps[k + int(diffGrid.z)][i + int(diffGrid.x)];
								//this->heightMaps[k + int(diff.z)][i + int(diff.x)] = nullptr;//dont do that, if diff.z and .x are 0, it leeks the previously created heightmap
							}
						}
						else {
							if (CHUNK_GEN_DEBUG) { std::cout << "gen new chunk: " << i << ":" << j << ":" << k << "\n"; }
							if (progress.y == 0) {//do it only once, for all the Y
								this->heightMaps[k][i] = new HeightMap(this->settings,
									(smallestChunk.x + i) * this->chunkSize.x, (smallestChunk.z + k) * this->chunkSize.z,
									this->chunkSize.x, this->chunkSize.z);
							}
							this->grid[k][j][i] = new Chunk(
								Math::Vector3(smallestChunk.x + i, smallestChunk.y + j, smallestChunk.z + k),//what happen when we send that to a ref?
								this->chunkSize, this->settings, this->heightMaps[k][i]);
						}
						progress.x++;
					}
					progress.y++;
				}
				progress.z++;
			}

			this->_chunksReadyForMeshes = true;
			chunks_lock.unlock();
		}//end grid memory
		this->playerChangedChunk = true;
		return true;
	}//end gridDisplayed
	return false;
}

//tmp debug
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

//return true if player step in another chunk
bool	ChunkGenerator::updateGrid(Math::Vector3 player_pos) {
	std::unique_lock<std::mutex> chunks_lock(this->chunks_mutex);

	Math::Vector3	old_tile = this->currentChunk;
	this->updatePlayerPos(player_pos);
	Math::Vector3	diff(this->currentChunk);
	diff.sub(old_tile);

	if (diff.len() == 0)
		return false;
	//gridDisplayed
	this->playerChangedChunk = true;
	std::cout << "diff: " << diff.toString() << "\n";

	//calculate if we need to move the memory grid and load new chunks
	Math::Vector3	diffGrid;
	diffGrid.x = calcExceed(this->gridPos.x, this->gridSize.x, this->currentChunk.x, this->gridDisplaySize.x);
	diffGrid.y = calcExceed(this->gridPos.y, this->gridSize.y, this->currentChunk.y, this->gridDisplaySize.y);
	diffGrid.z = calcExceed(this->gridPos.z, this->gridSize.z, this->currentChunk.z, this->gridDisplaySize.z);
	this->gridPos.add(diffGrid);
	//std::cout << "\t--gridSize       \t" << this->gridSize.toString() << "\n";
	//std::cout << "\t--gridDisplaySize\t" << this->gridDisplaySize.toString() << "\n";
	std::cout << "\t--diff           \t" << diff.toString() << "\n";
	std::cout << "\t--diffGrid       \t" << diffGrid.toString() << "\n";
	std::cout << "\t--gridPos        \t" << this->gridPos.toString() << "\n";
	std::cout << "\t--currentChunk   \t" << this->currentChunk.toString() << "\n";

	this->gridDisplayIndex.add(diff);
	this->gridDisplayIndex.sub(diffGrid); //should be inside the next block? yes, test it after the refacto
	std::cout << "\t--gridDisplayIndex        \t" << this->gridDisplayIndex.toString() << "\n";

	if (diffGrid.len() == 0)
		return true;
	this->gridMemoryMoved = true;

	//grid (memory)
	std::vector<int>		indexZ;
	std::vector<int>		indexY;
	std::vector<int>		indexX;
	std::vector<Chunk*>		chunks;
	std::vector<HeightMap*>	hmaps;
	std::vector<Chunk*>		chunksToDelete;
	std::vector<HeightMap*>	hmapsToDelete;

	//store the chunks and heightmaps with their new index 3D
	for (int z = 0; z < this->gridSize.z; z++) {
		for (int y = 0; y < this->gridSize.y; y++) {
			for (int x = 0; x < this->gridSize.x; x++) {
				indexZ.push_back(z - diffGrid.z);//each tile goes in the opposite way of the player movement
				indexY.push_back(y - diffGrid.y);
				indexX.push_back(x - diffGrid.x);
				chunks.push_back(this->grid[z][y][x]);
				//this->grid[z][y][x] = nullptr; // if the main RENDER thread reads here, he is fucked
				//if (y == 0) {// there is only one hmap for an entire column of chunks
					hmaps.push_back(this->heightMaps[z][x]);
					//this->heightMaps[z][x] = nullptr;// if the main RENDER thread reads here, he is fucked
				//}
				//else {
					//hmaps.push_back(nullptr);	// to match the chunks index for the next hmap
				//}
			}
		}
	}
	std::cout << "chunks referenced: " << chunks.size() << "\n";
	std::cout << "hmaps referenced (with nulls): " << hmaps.size() << "\n";

	//reassign the chunks
	std::cout << "reassigning chunks...";
	for (size_t n = 0; n < chunks.size(); n++) {
		if (indexZ[n] < this->gridSize.z && indexY[n] < this->gridSize.y && indexX[n] < this->gridSize.x \
			&& indexZ[n] >= 0 && indexY[n] >= 0 && indexX[n] >= 0) {
			this->grid[indexZ[n]][indexY[n]][indexX[n]] = chunks[n];
		}
		else {//is outside of the memory grid
			chunksToDelete.push_back(chunks[n]);
			//meaning the opposite side has to be nulled
			//int x = indexX[n] + this->gridSize.x * (indexX[n] < 0 ? 1 : -1)
			//int y = indexY[n] + this->gridSize.y * (indexY[n] < 0 ? 1 : -1)
			//int iz = indexZ[n] + this->gridSize.z * (indexZ[n] < 0 ? 1 : -1)
			int x = int(indexX[n] + this->gridSize.x) % int(this->gridSize.x);
			int y = int(indexY[n] + this->gridSize.y) % int(this->gridSize.y);
			int z = int(indexZ[n] + this->gridSize.z) % int(this->gridSize.z);
			this->grid[z][y][x] = nullptr;
		}
	}
	std::cout << " OK\n";

	//reassign the heightmaps
	std::cout << "reassigning heightmaps...";
	for (size_t n = 0; n < hmaps.size(); n = n + 4) {
		if (hmaps[n]) {
			if (indexZ[n] < this->gridSize.z && indexX[n] < this->gridSize.x \
				&& indexZ[n] >= 0 && indexX[n] >= 0) {
				this->heightMaps[indexZ[n]][indexX[n]] = hmaps[n];
			}
			else {//is outside of the memory grid
				hmapsToDelete.push_back(hmaps[n]);
				//meaning the opposite side has to be nulled
				//int x = indexX[n] + this->gridSize.x * (indexX[n] < 0 ? 1 : -1)
				//int z = indexZ[n] + this->gridSize.z * (indexZ[n] < 0 ? 1 : -1)
				int x = int(indexX[n] + this->gridSize.x) % int(this->gridSize.x);
				int z = int(indexZ[n] + this->gridSize.z) % int(this->gridSize.z);
				this->heightMaps[z][x] = nullptr;
			}
		}
	}
	std::cout << " OK\n";

	//debug checks before rebuild
	std::cout << "checks...\n" << this->getGridChecks();
	std::cout << " OK\n";

	//send useless data to bins
	this->trash_mutex.lock();
	this->trashHeightMaps.insert(this->trashHeightMaps.end(), hmapsToDelete.begin(), hmapsToDelete.end());
	this->trashChunks.insert(this->trashChunks.end(), chunksToDelete.begin(), chunksToDelete.end());
	this->trash_mutex.unlock();

	this->_chunksReadyForMeshes = false;
	chunks_lock.unlock();
	this->cv.notify_all();
	return true;
}

void	ChunkGenerator::updateChunkJobsToDo() {
	//build jobs
	unsigned int cn = 0;
	unsigned int hn = 0;
	std::thread::id threadID = std::this_thread::get_id();
	this->job_mutex.lock();
	this->chunks_mutex.lock();
	for (int z = 0; z < this->gridSize.z; z++) {
		for (int x = 0; x < this->gridSize.x; x++) {
			if (!this->heightMaps[z][x]) {
				JobBuildHeighMap* job = new JobBuildHeighMap(Math::Vector3(x, 0, z));
				this->jobsToDo.push_back(job);
				hn++;
				std::cout << "new JobBuildHeighMap: " << x << ":" << z << " " << this->heightMaps[z][x] << "\n";
			}
			for (int y = 0; y < this->gridSize.y; y++) {
				if (!this->grid[z][y][x] && this->heightMaps[z][x]) {
					JobBuildChunk* job = new JobBuildChunk(Math::Vector3(x, y, z), this->heightMaps[z][x]);
					this->jobsToDo.push_back(job);
					cn++;
					std::cout << "new JobBuildChunk: " << x << ":" << y << ":" << z << " hmap: " << this->heightMaps[z][x] << "\n";
				}
			}
		}
	}
	std::cout << "[helper0 " << threadID << "] new jobs: " << cn << " chunks, " << hn << " hmaps, total: " << this->jobsToDo.size() << "\n";
	this->chunks_mutex.unlock();
	this->job_mutex.unlock();
	if (this->jobsToDo.size()) {
		std::cout << "[helper0 " << threadID << "] notifying all...\n";
		this->cv.notify_all();
	}
}

void	ChunkGenerator::updateChunkJobsDone() {
	this->chunks_mutex.lock();
	this->job_mutex.lock();
	//process jobs done
	if (this->jobsDone.empty()) {
		std::cout << "No jobs to dispatch...\n";
		this->chunks_mutex.unlock();
		this->job_mutex.unlock();
		return ;
	}
	JobBuildGenerator* job = nullptr;

	while (this->jobsDone.size()) {
		job = this->jobsDone.front();
		this->jobsDone.pop_front();

		if (!job->done) {
			std::cout << "job not done but is in the jobsDone list: " << job << job->index.toString() << "\n";
			std::cout << "exiting...\n";
			std::exit(-14);
		}

		job->dispatch(*this);
	}

	//all the list should be erased, as all jobs should be done, but we check...
	//this->jobsDone.erase(std::remove_if(this->jobsDone.begin(), this->jobsDone.end(),
		//[](JobBuildGenerator* elem) { return (elem->done); }),
		//this->jobsDone.end());
	if (!this->jobsDone.empty()) {
		std::cout << "jobsDone not empty, wth? : " << this->jobsDone.size() << "\nexiting...\n";
		std::exit(-14);
	}
	this->chunksChanged = true;
	this->chunks_mutex.unlock();
	this->job_mutex.unlock();
}

void	ChunkGenerator::th_builders(GLFWwindow* context) {
	std::cout << __PRETTY_FUNCTION__ << "\n";
	glfwMakeContextCurrent(context);
	Glfw::initDefaultState();
	std::thread::id threadID = std::this_thread::get_id();
	std::stringstream ss;
	ss << threadID;
	std::string	threadIDstr = "[" + ss.str() + "] ";
	glfwSetWindowTitle(context, threadIDstr.c_str());
	std::unique_lock<std::mutex> job_lock(this->job_mutex);

	//build job variables
	PerlinSettings		perlinSettings(this->settings);//if they change later, we have to update them, cpy them when finding a job?
	Math::Vector3		halfGrid = Math::Vector3(int(this->gridSize.x / 2), int(this->gridSize.y / 2), int(this->gridSize.z / 2)); // same
	Math::Vector3		chunk_size(this->chunkSize);//same
	JobBuildGenerator*	job = nullptr;


	while (!this->terminateThreads) {
		if (!this->jobsToDo.empty()) {
			// grab job
			std::cout << threadIDstr << "grabbing job\n";
			job = this->jobsToDo.front();
			job->assigned = true;
			this->jobsToDo.pop_front();

			job_lock.unlock();
			this->chunks_mutex.lock();
			job->execute(perlinSettings, this->gridPos, halfGrid, chunk_size);
			this->chunks_mutex.unlock();
			job_lock.lock();

			if (job->done)
				this->jobsDone.push_back(job);
			else
				this->jobsToDo.push_back(job);
			job->assigned = false;

			std::cout << threadIDstr << "job done " << job->index.toString() << "\n";
			glfwSetWindowTitle(context, (threadIDstr + "job done: chunk built").c_str());
			job = nullptr;
		} else {
			std::cout << threadIDstr << "no job found... \n";
		}

		std::cout << threadIDstr << "waiting \n";
			glfwSetWindowTitle(context, (threadIDstr + "waiting").c_str());
		this->cv.wait(job_lock, [this] { return (!this->jobsToDo.empty() || this->terminateThreads); });//wait until jobs list in not empty
		std::cout << threadIDstr << "waking up from cv.wait() \n";
	}
	job_lock.unlock();

	std::cout << threadIDstr << " exiting...\n";
	glfwSetWindowShouldClose(context, GLFW_TRUE);
	glfwMakeContextCurrent(nullptr);
}

void	ChunkGenerator::th_builders_old(GLFWwindow* context) {
	std::cout << __PRETTY_FUNCTION__ << "\n";
	glfwMakeContextCurrent(context);
	Glfw::initDefaultState();
	std::thread::id threadID = std::this_thread::get_id();
	std::stringstream ss;
	ss << threadID;
	std::string	threadIDstr = "[" + ss.str() + "] ";
	glfwSetWindowTitle(context, threadIDstr.c_str());
	Chunk* fakeChunk = reinterpret_cast<Chunk*>(2);
	HeightMap* fakeHeightmap = reinterpret_cast<HeightMap*>(3);
	std::unique_lock<std::mutex> chunks_lock(this->chunks_mutex);

	//build job variables
	PerlinSettings	perlinSettings(this->settings);//if they change later, we have to update them, cpy them when finding a job?
	Math::Vector3	halfGrid = Math::Vector3(int(this->gridSize.x / 2), int(this->gridSize.y / 2), int(this->gridSize.z / 2)); // same
	Math::Vector3	chunk_size(this->chunkSize);//same
	Chunk* newChunk = nullptr;
	HeightMap* newHmap = nullptr;

	while (!this->terminateBuilders.try_lock()) {
		std::cout << "[" << threadID << "] waiting \n";
		glfwSetWindowTitle(context, (threadIDstr + "waiting").c_str());
		this->cv.wait(chunks_lock, [this] { return !this->_chunksReadyForMeshes; });//unlock and wait
		if (this->terminateBuilders.try_lock()) {
			break;
		}
		for (size_t z = 0; z < this->gridSize.z; z++) {
			for (size_t x = 0; x < this->gridSize.x; x++) {
				if (!this->heightMaps[z][x]) {
					this->heightMaps[z][x] = fakeHeightmap;//reserve the slot: assign whatever to the pointer so it's not nullptr anymore
					chunks_lock.unlock(); // now we can build the data
					Math::Vector3	chunkWorldIndex(
						this->gridPos.x - halfGrid.x + x,
						0,//useless here
						this->gridPos.z - halfGrid.z + z);
					newHmap = new HeightMap(perlinSettings,
						chunkWorldIndex.x * chunk_size.x, chunkWorldIndex.z * chunk_size.z,
						chunk_size.x, chunk_size.z);

					std::cout << "[" << threadID << "] gen HeightMap: [" << z << "][" << x << "]";
					std::cout << newHmap->map << " ";
					std::cout << (chunkWorldIndex.x * chunk_size.x) << ":" << (chunkWorldIndex.z * chunk_size.z) << " ";
					std::cout << chunk_size.x << "x" << chunk_size.z << " ";
					std::cout << "\n";

					newHmap->glth_buildPanel();
					chunks_lock.lock(); //to assign the new data
					//std::cout << "[thread " << threadID << "] built HeightMap " << newHmap << "\n";
					this->heightMaps[z][x] = newHmap;
					newHmap = nullptr;
				}
			}
		}
		this->threadsReadyToBuildChunks++;
		std::cout << "[" << threadID << "] threadsReadyToBuildChunks : " << (int)threadsReadyToBuildChunks << "\n";
		glfwSetWindowTitle(context, (threadIDstr + "thread ready to build Chunks").c_str());
		if (this->threadsReadyToBuildChunks != this->builderAmount) {
			chunks_lock.unlock();
			this->cv.notify_all();//wakes up other threads
			glfwSetWindowTitle(context, (threadIDstr + "thread ready to build Chunks, waiting other threads").c_str());
			this->cv.wait(chunks_lock, [this] { return (this->threadsReadyToBuildChunks == this->builderAmount); });//unlock and wait
		}
		chunks_lock.unlock();
		this->cv.notify_all();//wakes up other threads
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
		chunks_lock.lock();

		std::cout << "[" << threadID << "] building chunks... \n";
		glfwSetWindowTitle(context, (threadIDstr + "building chunks...").c_str());
		for (size_t z = 0; z < this->gridSize.z; z++) {
			for (size_t y = 0; y < this->gridSize.y; y++) {
				for (size_t x = 0; x < this->gridSize.x; x++) {
					if (!this->grid[z][y][x]) {
						this->grid[z][y][x] = fakeChunk;//reserve the slot: assign whatever to the pointer so it's not nullptr anymore
						newHmap = this->heightMaps[z][x];
						Math::Vector3	chunkWorldIndex(
							this->gridPos.x - halfGrid.x + x,
							this->gridPos.y - halfGrid.y + y,
							this->gridPos.z - halfGrid.z + z);
						chunks_lock.unlock();//now we can build a real chunk and heightmap
						newChunk = new Chunk(chunkWorldIndex, chunk_size, perlinSettings, newHmap);
						newChunk->buildMesh();
						chunks_lock.lock(); //to assign the new data
						this->grid[z][y][x] = newChunk;
						std::cout << "[" << threadID << "] built chunk " << z << " " << y << " " << x << " " << this->grid[z][y][x];
						if (newChunk->meshBP)
							std::cout << " meshBP " << newChunk->meshBP << " vao " << newChunk->meshBP->getVao();
						std::cout << "\n";

						newChunk = nullptr;
						newHmap = nullptr;
					}
				}
			}
		}
		//this->playerChangedChunk = false;
		this->_chunksReadyForMeshes = true;
		this->threadsReadyToBuildChunks = 0;
		std::cout << "[" << threadID << "] nomore chunks to build, _chunksReadyForMeshes true\n";
		glfwSetWindowTitle(context, (threadIDstr + "nomore chunks to build, _chunksReadyForMeshes true").c_str());
		chunks_lock.unlock();
	}
	std::cout << "[" << threadID << "] exiting...\n";
	glfwSetWindowShouldClose(context, GLFW_TRUE);
	glfwMakeContextCurrent(nullptr);
	this->terminateBuilders.unlock();
}

bool	ChunkGenerator::buildMeshesAndMapTiles() {
	std::cout << __PRETTY_FUNCTION__ << "\n";
	if (this->_chunksReadyForMeshes) {
		for (unsigned int k = 0; k < this->gridSize.z; k++) {
				for (unsigned int i = 0; i < this->gridSize.x; i++) {
					this->heightMaps[k][i]->glth_buildPanel();
					for (unsigned int j = 0; j < this->gridSize.y; j++) {
						this->grid[k][j][i]->buildMesh();
						std::cout << "[" << k << "-" << j << "-" << i << "] mesh built:" << this->grid[k][j][i];
					}
			}
		}
		this->_chunksReadyForMeshes = false;
		std::cout << "ChunkGenerator meshes built\n";
		return true;
	} else {
		//std::cout << "Chunks are not ready yet\n";
		return false;
	}
}

void	ChunkGenerator::updatePlayerPos(Math::Vector3 player_pos) {
	//std::lock_guard<std::mutex> guard(this->mutex_cam);
	this->playerPos = player_pos;
	this->currentChunk.x = int(this->playerPos.x / this->chunkSize.x);
	this->currentChunk.y = int(this->playerPos.y / this->chunkSize.y);
	this->currentChunk.z = int(this->playerPos.z / this->chunkSize.z);
	if (this->playerPos.x < 0) // cauze if x=-32..+32 ----> x / 32 = 0; 
		this->currentChunk.x--;
	if (this->playerPos.y < 0) // same
		this->currentChunk.y--;
	if (this->playerPos.z < 0) // same
		this->currentChunk.z--;
}

std::string	ChunkGenerator::toString() const {
	std::stringstream ss;
	ss << "currentChunk: " << this->currentChunk.toString() << "\n";
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

bool	ChunkGenerator::try_deleteUnusedData() {
	if (this->trash_mutex.try_lock()) {
		this->deleteUnusedData();
		this->trash_mutex.unlock();
		return true;
	}
	return false;
}

void	ChunkGenerator::deleteUnusedData() {
	//todo : use unique_ptr in the vectors
	for (auto& pointer : this->trashHeightMaps)	{
			delete pointer;
			pointer = nullptr;
	}
	this->trashHeightMaps.erase(std::remove(this->trashHeightMaps.begin(), this->trashHeightMaps.end(), nullptr), this->trashHeightMaps.end());
	for (auto& pointer : this->trashChunks) {
		delete pointer;
		pointer = nullptr;
	}
	this->trashChunks.erase(std::remove(this->trashChunks.begin(), this->trashChunks.end(), nullptr), this->trashChunks.end());
}
