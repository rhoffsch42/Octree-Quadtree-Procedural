#include "chunkgrid.hpp"
#include "trees.h"
#include "compiler_settings.h"

#ifdef TREES_DEBUG
 #define TREES_CHUNK_GRID_DEBUG
#endif
#ifdef TREES_CHUNK_GRID_DEBUG 
 #define D(x) std::cout << "[ChunkGrid] " << x ;
 #define D_(x) x ;
 #define D_SPACER "-- chunkgrid.cpp -------------------------------------------------\n"
 #define D_SPACER_END "----------------------------------------------------------------\n"
#else 
 #define D(x)
 #define D_(x)
 #define D_SPACER ""
 #define D_SPACER_END ""
#endif

ChunkGrid::ChunkGrid(Math::Vector3 player_pos) {
	D(__PRETTY_FUNCTION__ << "\n")

	this->_heightMaps = new HMapPtr* [this->_gridSize.z];
	for (unsigned int z = 0; z < this->_gridSize.z; z++) {
		this->_heightMaps[z] = new HMapPtr [this->_gridSize.x];
		for (unsigned int x = 0; x < this->_gridSize.x; x++) {
			this->_heightMaps[z][x] = nullptr;
		}
	}

	this->_grid = new ChunkPtr** [this->_gridSize.z];
	for (unsigned int k = 0; k < this->_gridSize.z; k++) {
		this->_grid[k] = new ChunkPtr* [this->_gridSize.y];
		for (unsigned int j = 0; j < this->_gridSize.y; j++) {
			this->_grid[k][j] = new ChunkPtr [this->_gridSize.x];
			for (unsigned int i = 0; i < this->_gridSize.x; i++) {
				this->_grid[k][j][i] = nullptr;
			}
		}
	}

	this->_playerChangedChunk = true;
	this->_gridWorldIndex = Math::Vector3(0, 0, 0);
	this->_gridDisplayIndex = Math::Vector3(0, 0, 0);

	//center the player in the grid display
	this->_playerChunkWorldIndex = Math::Vector3(0, 0, 0);
	this->_playerChunkWorldIndex += Math::Vector3(
		int(this->_gridDisplaySize.x / 2),
		int(this->_gridDisplaySize.y / 2),
		int(this->_gridDisplaySize.z / 2));

	this->updateGrid(player_pos);
}
ChunkGrid::~ChunkGrid() {
	for (unsigned int k = 0; k < this->_gridSize.z; k++) {
		for (unsigned int i = 0; i < this->_gridSize.z; i++) {
			delete this->_heightMaps[k][i];
		}
		delete[] this->_heightMaps[k];
	}
	delete[] this->_heightMaps;

	for (unsigned int k = 0; k < this->_gridSize.z; k++) {
		for (unsigned int j = 0; j < this->_gridSize.y; j++) {
			for (unsigned int i = 0; i < this->_gridSize.x; i++) {
				delete this->_grid[k][j][i];
			}
			delete[] this->_grid[k][j];
		}
		delete[] this->_grid[k];
	}
	delete[] this->_grid;

	if (this->_fullMesh)
		delete this->_fullMesh;
	if (this->_fullMeshBP)
		delete this->_fullMeshBP;
}

void			ChunkGrid::th_updater(Cam* cam) {
	Math::Vector3 playerPos;
	/*
	while (!this->terminateThreads) {//no mutex for reading?
		this->mutex_cam.lock();//do we really need it ? only reading
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
	*/

	D("[helper0] exiting...\n")
}

bool			ChunkGrid::updateGrid(Math::Vector3 player_pos) {
	std::unique_lock<std::mutex> chunks_lock(this->_chunks_mutex);

	Math::Vector3	playerDiff = this->_playerChunkWorldIndex;
	this->_updatePlayerPosition(player_pos);
	playerDiff = this->_playerChunkWorldIndex - playerDiff;
	if (playerDiff.len2() == 0)
		return false;

	this->_playerChangedChunk = true;
	Math::Vector3	diffGrid = this->_calculateGridDiff(playerDiff);
	if (diffGrid.len() == 0)
		return true;

	std::vector<Chunk*>		chunksToDelete;
	std::vector<HeightMap*>	hmapsToDelete;
	this->_translateGrid(diffGrid, &chunksToDelete, &hmapsToDelete);

	#ifdef FFFF_TMP
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
	#endif

	return true;
}

void			ChunkGrid::_updatePlayerPosition(const Math::Vector3& player_pos) {
	/*
		update chain :
		player_pos > playerChunkWorldIndex > gridDisplayIndex > gridIndex
	*/
	this->_playerWorldPos = player_pos;
	this->_playerChunkWorldIndex.x = int(this->_playerWorldPos.x / this->_chunkSize.x);
	this->_playerChunkWorldIndex.y = int(this->_playerWorldPos.y / this->_chunkSize.y);
	this->_playerChunkWorldIndex.z = int(this->_playerWorldPos.z / this->_chunkSize.z);
	if (this->_playerWorldPos.x < 0) // cauze if x=-32..+32 ----> x / 32 = 0; 
		this->_playerChunkWorldIndex.x--;
	if (this->_playerWorldPos.y < 0) // same
		this->_playerChunkWorldIndex.y--;
	if (this->_playerWorldPos.z < 0) // same
		this->_playerChunkWorldIndex.z--;
}
Math::Vector3	ChunkGrid::_calculateGridDiff(Math::Vector3 playerdiff) {
	Math::Vector3	diffGrid;
	Math::Vector3	new_gridIndex;
	Math::Vector3	new_worldDisplayIndex = this->_gridWorldIndex + this->_gridDisplayIndex + playerdiff;
	Math::Vector3	relDisplayIndex = new_worldDisplayIndex - this->_gridWorldIndex;
	/*
		moving the display grid in the world, and adapt the memory grid after it, independently from the original player offset
		if for any reason the player offset changes, this will not catch it and will continue moving grids based on above diff
	*/

	if (relDisplayIndex.x < 0)
		new_gridIndex.x = std::min(new_worldDisplayIndex.x, this->_gridWorldIndex.x);
	else
		new_gridIndex.x = std::max(new_worldDisplayIndex.x + this->_gridDisplaySize.x - this->_gridSize.x, this->_gridWorldIndex.x);
	if (relDisplayIndex.y < 0)
		new_gridIndex.y = std::min(this->_gridWorldIndex.y, new_worldDisplayIndex.y);
	else
		new_gridIndex.y = std::max(new_worldDisplayIndex.y + this->_gridDisplaySize.y - this->_gridSize.y, this->_gridWorldIndex.y);
	if (relDisplayIndex.z < 0)
		new_gridIndex.z = std::min(this->_gridWorldIndex.z, new_worldDisplayIndex.z);
	else
		new_gridIndex.z = std::max(new_worldDisplayIndex.z + this->_gridDisplaySize.z - this->_gridSize.z, this->_gridWorldIndex.z);
	diffGrid = new_gridIndex - this->_gridWorldIndex;
	this->_gridDisplayIndex = new_worldDisplayIndex - new_gridIndex;
	this->_gridWorldIndex = new_gridIndex;

	//D("\t--gridSize       \t" << this->gridSize << "\n")
	//D("\t--gridDisplaySize\t" << this->gridDisplaySize << "\n")
	D("\t--playerdiff            \t" << playerdiff << "\n")
	D("\t--diffGrid              \t" << diffGrid << "\n")
	D("\t--_gridWorldIndex       \t" << this->_gridWorldIndex << "\n")
	D("\t--_playerChunkWorldIndex\t" << this->_playerChunkWorldIndex << "\n")
	D("\t--_gridDisplayIndex     \t" << this->_gridDisplayIndex << "\n")

	return diffGrid;
}
void			ChunkGrid::_translateGrid(Math::Vector3 gridDiff, std::vector<Chunk*>* chunksToDelete, std::vector<HeightMap*>* hmapsToDelete) {
	//grid (memory)
	std::vector<int>		indexZ;
	std::vector<int>		indexY;
	std::vector<int>		indexX;
	std::vector<Chunk*>		chunks;
	std::vector<HeightMap*>	hmaps;

	//store the chunks and heightmaps with their new index 3D
	for (int z = 0; z < this->_gridSize.z; z++) {
		for (int y = 0; y < this->_gridSize.y; y++) {
			for (int x = 0; x < this->_gridSize.x; x++) {
				indexZ.push_back(z - gridDiff.z);//each chunk goes in the opposite way of the player movement
				indexY.push_back(y - gridDiff.y);
				indexX.push_back(x - gridDiff.x);
				chunks.push_back(this->_grid[z][y][x]);
				this->_grid[z][y][x] = nullptr; // if the main RENDER thread reads here, he is fucked
				if (y == 0) {// there is only one hmap for an entire column of chunks
					hmaps.push_back(this->_heightMaps[z][x]);
					this->_heightMaps[z][x] = nullptr;// if the main RENDER thread reads here, he is fucked
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
		if (indexZ[n] < this->_gridSize.z && indexY[n] < this->_gridSize.y && indexX[n] < this->_gridSize.x \
			&& indexZ[n] >= 0 && indexY[n] >= 0 && indexX[n] >= 0) {
			this->_grid[indexZ[n]][indexY[n]][indexX[n]] = chunks[n];
		}
		else {//is outside of the memory grid
			chunksToDelete->push_back(chunks[n]);
			//meaning the opposite side has to be nulled
			//int x = indexX[n] + this->_gridSize.x * (indexX[n] < 0 ? 1 : -1)
			//int y = indexY[n] + this->_gridSize.y * (indexY[n] < 0 ? 1 : -1)
			//int iz = indexZ[n] + this->_gridSize.z * (indexZ[n] < 0 ? 1 : -1)
			//int x = int(indexX[n] + this->_gridSize.x) % int(this->_gridSize.x);
			//int y = int(indexY[n] + this->_gridSize.y) % int(this->_gridSize.y);
			//int z = int(indexZ[n] + this->_gridSize.z) % int(this->_gridSize.z);
			//this->_grid[z][y][x] = nullptr;
		}
	}

	//reassign the heightmaps
	D("reassigning heightmaps...\n")
	for (size_t n = 0; n < hmaps.size(); n = n + 4) {
		if (hmaps[n]) {
			if (indexZ[n] < this->_gridSize.z && indexX[n] < this->_gridSize.x \
				&& indexZ[n] >= 0 && indexX[n] >= 0) {
				this->_heightMaps[indexZ[n]][indexX[n]] = hmaps[n];
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

void			ChunkGrid::glth_loadChunks() {}
void			ChunkGrid::pushDisplayedChunks(std::list<Object*>* dst, unsigned int tesselation_lvl) const {}
unsigned int	ChunkGrid::pushDisplayedChunks(Object** dst, unsigned int tesselation_lvl, unsigned int starting_index) const { return 0; }

Math::Vector3	ChunkGrid::getGridDisplayStart() const { return Math::Vector3(); }
Math::Vector3	ChunkGrid::worldToGrid(const Math::Vector3& index) const { return Math::Vector3(); }
Math::Vector3	ChunkGrid::gridToWorld(const Math::Vector3& index) const { return Math::Vector3(); }
std::string		ChunkGrid::toString() const {
	std::stringstream ss;
	ss << "playerChunkWorldIndex: " << this->_playerChunkWorldIndex.toString() << "\n";
	for (int k = 0; k < this->_gridSize.z; k++) {
		for (int j = 0; j < this->_gridSize.y; j++) {
			for (int i = 0; i < this->_gridSize.x; i++) {
				ss << "[" << k << "]";
				ss << "[" << j << "]";
				ss << "[" << i << "]\t" << this->_grid[k][j][i]->toString() << "\n";
			}
		}
	}
	return ss.str();
}
std::string		ChunkGrid::getGridChecks() const {
	std::stringstream ss;
	int		nonulls = 0;
	int		hmnonulls = 0;
	int		nulls = 0;
	int		hmnulls = 0;

	for (int k = 0; k < this->_gridSize.z; k++) {
		for (int j = 0; j < this->_gridSize.y; j++) {
			for (int i = 0; i < this->_gridSize.x; i++) {
				if (this->_grid[k][j][i])
					nonulls++;
				else
					nulls++;
				if (j == 0) {
					if (this->_heightMaps[k][i])
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
