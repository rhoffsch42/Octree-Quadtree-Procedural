#include "chunkgrid.hpp"
#include "trees.h"
#include "compiler_settings.h"

#ifdef TREES_DEBUG
 #define TREES_CHUNK_GRID_DEBUG
#endif
#ifdef TREES_CHUNK_GRID_DEBUG 
 #define D(x) std::cout << "[ChunkGrid] " << x
 #define D_(x) x
 #define D_SPACER "-- chunkgrid.cpp -------------------------------------------------\n"
 #define D_SPACER_END "----------------------------------------------------------------\n"
#else 
 #define D(x)
 #define D_(x)
 #define D_SPACER ""
 #define D_SPACER_END ""
#endif

static void	_deleteChunksAndHeightmaps(std::vector<Chunk*>* chunk_trash, std::vector<HeightMap*>* hmap_trash) {
	unsigned int	c = 0;
	int del = 0; // manually count deletion because vectors can have nullptr (which can be safely sent to 'delete' as argument). 

	std::for_each(hmap_trash->begin(), hmap_trash->end(),
		[&del, &c](auto hmap) {
			if (hmap) {
				c = hmap->getdisposedCount();
				if (c == 0) {
					delete hmap;
					del++;
				} else {
					D("Warning: couldn't delete disposed (" << c << ") heightmap: " << hmap << "\n");
				}
			}
		});
	D("deleted hmaps: " << del << "\n");

	del = 0;
	std::for_each(chunk_trash->begin(), chunk_trash->end(),
		[&del](auto chunk) {
			if (chunk) { del++; }
			delete chunk;
		});
	D("deleted chunks: " << del << "\n");
}

ChunkGrid::ChunkGrid(Math::Vector3 chunk_size, Math::Vector3 grid_size, Math::Vector3 rendered_grid_size)
	: _chunkSize(chunk_size), _gridSize(grid_size), _renderedGridSize(rendered_grid_size)
{
	D(__PRETTY_FUNCTION__ << "\n");
	D("gridSize : " << this->_gridSize << "\n");
	D("renderedGridSize : " << this->_renderedGridSize << "\n");
	D("chunkSize : " << this->_chunkSize << "\n");

	this->_heightMaps = new HeightMapShPtr * [this->_gridSize.z];
	for (unsigned int z = 0; z < this->_gridSize.z; z++) {
		this->_heightMaps[z] = new HeightMapShPtr[this->_gridSize.x];
		for (unsigned int x = 0; x < this->_gridSize.x; x++) {
			this->_heightMaps[z][x] = nullptr;
		}
	}

	this->_grid = new ChunkShPtr ** [this->_gridSize.z];
	for (unsigned int k = 0; k < this->_gridSize.z; k++) {
		this->_grid[k] = new ChunkShPtr * [this->_gridSize.y];
		for (unsigned int j = 0; j < this->_gridSize.y; j++) {
			this->_grid[k][j] = new ChunkShPtr [this->_gridSize.x];
			for (unsigned int i = 0; i < this->_gridSize.x; i++) {
				this->_grid[k][j][i] = nullptr;
			}
		}
	}

	this->chunksChanged = true;
	this->gridShifted = true;
	this->playerChangedChunk = true;
	this->_gridWorldIndex = Math::Vector3(0, 0, 0);
	this->_renderedGridIndex = Math::Vector3(0, 0, 0);

	//center the player in the rendered grid
	this->_playerChunkWorldIndex = Math::Vector3(0, 0, 0);
	this->_playerChunkWorldIndex += Math::Vector3(
		int(this->_renderedGridSize.x / 2),
		int(this->_renderedGridSize.y / 2),
		int(this->_renderedGridSize.z / 2));
}
ChunkGrid::~ChunkGrid() {
	for (unsigned int k = 0; k < this->_gridSize.z; k++) {
		for (unsigned int i = 0; i < this->_gridSize.z; i++) {
			//delete this->_heightMaps[k][i];// done by the shared_ptr
		}
		delete[] this->_heightMaps[k];
	}
	delete[] this->_heightMaps;

	for (unsigned int k = 0; k < this->_gridSize.z; k++) {
		for (unsigned int j = 0; j < this->_gridSize.y; j++) {
			for (unsigned int i = 0; i < this->_gridSize.x; i++) {
				//delete this->_grid[k][j][i]; // done by the shared_ptr
			}
			delete[] this->_grid[k][j];
		}
		delete[] this->_grid[k];
	}
	delete[] this->_grid;
}

bool			ChunkGrid::updateGrid(Math::Vector3 player_pos) {
	std::unique_lock<std::mutex> chunks_lock(this->chunks_mutex);

	Math::Vector3	playerDiff = this->_playerChunkWorldIndex;
	this->_updatePlayerPosition(player_pos);
	playerDiff = this->_playerChunkWorldIndex - playerDiff;
	if (playerDiff.len2() == 0)
		return false;

	this->playerChangedChunk = true;
	Math::Vector3	diffGrid = this->_calculateGridDiff(playerDiff);
	if (diffGrid.len() == 0)
		return true;

	D("Grid must be shifted, translation...\n");
	D("Grid checks...\n" << this->getGridChecks());
	this->_translateGrid(diffGrid);

	D("Grid checks...\n" << this->getGridChecks());
	this->_chunksReadyForMeshes = false;
	chunks_lock.unlock();

	return true;
}

void			ChunkGrid::_updatePlayerPosition(const Math::Vector3& player_pos) {
	/*
		update chain :
		player_pos > playerChunkWorldIndex > renderedGridIndex > gridIndex
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
	Math::Vector3	gridDiff;
	Math::Vector3	new_gridIndex;
	Math::Vector3	new_renderedWorldIndex = this->_gridWorldIndex + this->_renderedGridIndex + playerdiff;
	Math::Vector3	relative_renderedIndex = new_renderedWorldIndex - this->_gridWorldIndex;
	/*
		moving the rendered grid in the world, and adapt the memory grid after it, independently from the original player offset
		if for any reason the player offset changes, this will not catch it and will continue moving grids based on above diff
	*/

	if (relative_renderedIndex.x < 0)
		new_gridIndex.x = std::min(new_renderedWorldIndex.x, this->_gridWorldIndex.x);
	else
		new_gridIndex.x = std::max(new_renderedWorldIndex.x + this->_renderedGridSize.x - this->_gridSize.x, this->_gridWorldIndex.x);
	if (relative_renderedIndex.y < 0)
		new_gridIndex.y = std::min(this->_gridWorldIndex.y, new_renderedWorldIndex.y);
	else
		new_gridIndex.y = std::max(new_renderedWorldIndex.y + this->_renderedGridSize.y - this->_gridSize.y, this->_gridWorldIndex.y);
	if (relative_renderedIndex.z < 0)
		new_gridIndex.z = std::min(this->_gridWorldIndex.z, new_renderedWorldIndex.z);
	else
		new_gridIndex.z = std::max(new_renderedWorldIndex.z + this->_renderedGridSize.z - this->_gridSize.z, this->_gridWorldIndex.z);
	gridDiff = new_gridIndex - this->_gridWorldIndex;
	this->_renderedGridIndex = new_renderedWorldIndex - new_gridIndex;
	this->_gridWorldIndex = new_gridIndex;

	//D("\t--_gridSize             \t" << this->_gridSize << "\n");
	//D("\t--renderedGridSize      \t" << this->renderedGridSize << "\n");
	D("\t--playerdiff            \t" << playerdiff << "\n");
	D("\t--gridDiff              \t" << gridDiff << "\n");
	D("\t--_gridWorldIndex       \t" << this->_gridWorldIndex << "\n");
	D("\t--_playerChunkWorldIndex\t" << this->_playerChunkWorldIndex << "\n");
	D("\t--_renderedGridIndex    \t" << this->_renderedGridIndex << "\n");

	return gridDiff;
}
void			ChunkGrid::_translateGrid(Math::Vector3 gridDiff) {
	//grid (memory)
	std::vector<int>		indexes_Z;
	std::vector<int>		indexes_Y;
	std::vector<int>		indexes_X;
	std::vector<ChunkShPtr>	chunks;
	std::vector<HeightMapShPtr>	hmaps;
	chunks.reserve(this->_gridSize.x * this->_gridSize.y * this->_gridSize.z);
	hmaps.reserve(this->_gridSize.x * this->_gridSize.z);

	//store the chunks and heightmaps with their new index 3D
	for (int y = 0; y < this->_gridSize.y; y++) {//important that Y is the first loop, so the first z*x indexes of hmaps array are all valid
		for (int z = 0; z < this->_gridSize.z; z++) {
			for (int x = 0; x < this->_gridSize.x; x++) {
				indexes_Z.push_back(z - gridDiff.z);//each chunk goes in the opposite way of the player movement
				indexes_Y.push_back(y - gridDiff.y);
				indexes_X.push_back(x - gridDiff.x);
				chunks.push_back(this->_grid[z][y][x]);
				this->_grid[z][y][x].reset();
				if (y == 0) {// there is only one hmap for an entire column of chunks
					hmaps.push_back(this->_heightMaps[z][x]);
					this->_heightMaps[z][x].reset();
				}
			}
		}
	}
	D("chunks referenced: " << chunks.size() << "\n");
	D("hmaps referenced (with nulls): " << hmaps.size() << "\n");

	D("reassigning chunks...\n");
	for (size_t n = 0; n < chunks.size(); n++) {
		if (indexes_Z[n] < this->_gridSize.z && indexes_Y[n] < this->_gridSize.y && indexes_X[n] < this->_gridSize.x \
			&& indexes_Z[n] >= 0 && indexes_Y[n] >= 0 && indexes_X[n] >= 0) {
			this->_grid[indexes_Z[n]][indexes_Y[n]][indexes_X[n]] = chunks[n];
		} else {//is outside of the memory grid
			this->garbageMutex.lock();
			this->_garbageChunks.push_back(chunks[n]);
			this->garbageMutex.unlock();
		}
	}

	D("reassigning heightmaps...\n");
	size_t step = this->_gridSize.x * this->_gridSize.z;
	for (size_t n = 0; n < step; n++) {
		if (hmaps[n]) {
			if (indexes_Z[n] < this->_gridSize.z && indexes_X[n] < this->_gridSize.x \
				&& indexes_Z[n] >= 0 && indexes_X[n] >= 0) {
				this->_heightMaps[indexes_Z[n]][indexes_X[n]] = hmaps[n];
			} else {//is outside of the memory grid
				this->garbageMutex.lock();
				this->_garbageHeightmaps.push_back(hmaps[n]);
				this->garbageMutex.unlock();
			}
		}
	}

	this->gridShifted = true;
}

void			ChunkGrid::glth_loadAllChunksToGPU() {
	/*
		not ideal. each time there are new chunks on 1 dimension, it rechecks the entire grid
		ex: grid 1000x1000x1000. 1000x1000 new chunks to load, but 1000x1000x1000 iterations

		refacto: new job: change the chunk job deliver to create a new job for the main thread
		if the chunk isn't empty ofc.
	*/
	D("Loading chunks to the GPU...\n");
	FOR(z, 0, this->_gridSize.z) {
		FOR(y, 0, this->_gridSize.y) {
			FOR(x, 0, this->_gridSize.x) {
				if (this->_grid[z][y][x]) {
					this->_grid[z][y][x]->glth_buildAllMeshes();
				}
			}
		}
	}
}

//	Pushed heightmaps might have not been generated yet
void			ChunkGrid::pushAllHeightmaps(std::vector<HeightMapShPtr>* dst) const {
	FOR(z, 0, this->_gridSize.z) {
		FOR(x, 0, this->_gridSize.x) {
			dst->push_back(this->_heightMaps[z][x]);
			//D("ChunkGrid::pushAllHeightmaps() : hmap [" << z << "][" << x << "] " << this->_grid[z][x].get() << "\n")
		}
	}
}

//	Pushed chunks meshes might have not been generated yet
void			ChunkGrid::pushAllChunks(std::vector<ChunkShPtr>* dst) const {
	FOR(z, 0, this->_gridSize.z) {
		FOR(y, 0, this->_gridSize.y) {
			FOR(x, 0, this->_gridSize.x) {
				dst->push_back(this->_grid[z][y][x]);
				//D("ChunkGrid::pushAllChunks() : chunck [" << z << "][" << y << "][" << x << "] " << this->_grid[z][y][x].get() << "\n")
			}
		}
	}
}
//	Pushed chunks meshes might have not been generated yet
void			ChunkGrid::pushRenderedChunks(std::vector<ChunkShPtr>* dst) const {
	/*
	same as in glth_loadAllChunksToGPU() but more complicated
	we would need to select the chunks to remove (change the render vector with a map?),
	and push only the new chunks
	*/
	//D("Pushing rendered chunks to a vector<Object*> ...\n");
	//if (dst->size()) {
	//	D("Warning: Pushing rendered chunks to a vector<ChunkShPtr> that is not empty\n");
	//}
	Math::Vector3	renderedGridEndIndex = this->_renderedGridIndex + this->_renderedGridSize;
	for (unsigned int k = this->_renderedGridIndex.z; k < renderedGridEndIndex.z; k++) {
		for (unsigned int j = this->_renderedGridIndex.y; j < renderedGridEndIndex.y; j++) {
			for (unsigned int i = this->_renderedGridIndex.x; i < renderedGridEndIndex.x; i++) {
				dst->push_back(this->_grid[k][j][i]);
				//D("ChunkGrid::pushRenderedChunks() : chunck [" << k << "][" << j << "][" << i << "] " << this->_grid[k][j][i].get() << "\n")
			}
		}
	}
}

void			ChunkGrid::replaceHeightMap(HeightMap* new_hmap, Math::Vector3 index) {
	unsigned int	x = index.x;
	unsigned int	z = index.z;
	if (x < 0 || z < 0 || x > this->_gridSize.x - 1 || z > this->_gridSize.z - 1) {
		D("ChunkGrid::replaceHeightMap() Error : supplied index is invalid");
		return ;
	}
	if (this->_heightMaps[z][x]) {
		this->garbageMutex.lock();
		this->_garbageHeightmaps.push_back(this->_heightMaps[z][x]);
		this->garbageMutex.unlock();
	}
	this->_heightMaps[z][x].reset(new_hmap);
}
void			ChunkGrid::replaceChunk(Chunk* new_chunk, Math::Vector3 index) {
	unsigned int	x = index.x;
	unsigned int	y = index.y;
	unsigned int	z = index.z;
	if (x < 0 || y < 0 || z < 0 || x > this->_gridSize.x - 1 || y > this->_gridSize.y - 1 || z > this->_gridSize.z - 1) {
		D("ChunkGrid::replaceChunk() Error : supplied index is invalid");
		return;
	}
	if (this->_grid[z][y][x]) {
		this->garbageMutex.lock();
		this->_garbageChunks.push_back(this->_grid[z][y][x]);
		this->garbageMutex.unlock();
	}
	this->_grid[z][y][x].reset(new_chunk);
	this->chunksChanged = true;
}

void			ChunkGrid::glth_testGarbage(size_t amount) {
	D("Warning: glth_testGarbage() THIS IS A TEST FUNCTION, IT SHOULDN'T BE CALLED CASUALLY\n");
	this->garbageMutex.lock();
	size_t	size = 20;
	siv::PerlinNoise	perlin;
	PerlinSettings	ps(perlin);
	FOR(i, 0, size) {
		HeightMap*		hmap = new HeightMap(ps, Math::Vector3(1,1,1), Math::Vector3(32,32,32));
		if (i % 2 == 0) {
			hmap->dispose();
		}
		HeightMapShPtr	hmapSptr(hmap);
		this->_garbageHeightmaps.push_back(hmapSptr);
	}
	for (auto& hmap : this->_garbageHeightmaps) {
		D(hmap << " disp" << hmap->getdisposedCount() << LF);
	}
	D("garbage size: " << this->_garbageHeightmaps.size() << LF);
	D("deleting " << amount << LF);
	this->garbageMutex.unlock();
	this->glth_try_deleteUnusedHeightmaps(amount);
	this->garbageMutex.lock();
	D("garbage size: " << this->_garbageHeightmaps.size() << LF);
	for (auto& hmap : this->_garbageHeightmaps) {
		D(hmap << " disp" << hmap->getdisposedCount() << LF);
	}
	this->garbageMutex.unlock();
}

// it tries to lock the garbageMutex
bool			ChunkGrid::glth_try_deleteUnusedHeightmaps(size_t amount) {
	//this->_garbageHeightmaps.resize(std::max(size_t(0), this->_garbageHeightmaps.size() - amount));
	if (amount && this->garbageMutex.try_lock()) {
		if (this->_garbageHeightmaps.size()) {
			amount = std::min(amount, this->_garbageHeightmaps.size());
			this->_garbageHeightmaps.erase(std::remove_if(this->_garbageHeightmaps.begin(), this->_garbageHeightmaps.begin() + amount,
				[](HeightMapShPtr& hmap) {
					return hmap->getdisposedCount() == 0;
				}), this->_garbageHeightmaps.begin() + amount);
		}
		this->garbageMutex.unlock();
		return true;
	}
	else {
		return false;
	}
}

// it tries to lock the garbageMutex
bool			ChunkGrid::glth_try_deleteUnusedChunks(size_t amount) {
	if (this->garbageMutex.try_lock()) {
		if (this->_garbageChunks.size()) {
			amount = std::min(amount, this->_garbageChunks.size());
			//D("try delete some chunks... resizing to " << (this->_garbageChunks.size() - amount) << "\n");
			this->_garbageChunks.resize(this->_garbageChunks.size() - amount);
		}
		this->garbageMutex.unlock();
		return true;
	}
	else {
		return false;
	}
}
void			ChunkGrid::glth_try_emptyGarbage() {
	this->glth_try_deleteUnusedChunks(GRID_GARBAGE_DELETION_STEP);
	this->glth_try_deleteUnusedHeightmaps(GRID_GARBAGE_DELETION_STEP);
}
size_t			ChunkGrid::getGarbageSize() const { return this->_garbageChunks.size() + this->_garbageHeightmaps.size(); }

HeightMapShPtr**	ChunkGrid::getHeightMaps() const			{ return this->_heightMaps; }
ChunkShPtr***		ChunkGrid::getGrid() const					{ return this->_grid; }
Math::Vector3		ChunkGrid::getSize() const					{ return this->_gridSize; }
Math::Vector3		ChunkGrid::getRenderedSize() const			{ return this->_renderedGridSize; }
Math::Vector3		ChunkGrid::getChunkSize() const				{ return this->_chunkSize; }
Math::Vector3		ChunkGrid::getRenderedGridIndex() const		{ return this->_renderedGridIndex; }
Math::Vector3		ChunkGrid::getWorldIndex() const			{ return this->_gridWorldIndex; }
Math::Vector3		ChunkGrid::getPlayerChunkWorldIndex() const	{ return this->_playerChunkWorldIndex; }
GridGeometry		ChunkGrid::getGeometry() const {
	return GridGeometry{
		this->_chunkSize,
		this->_gridSize,
		this->_renderedGridSize,
		this->_gridWorldIndex,
		this->_renderedGridIndex,
		this->_playerChunkWorldIndex
	};
}

Math::Vector3	ChunkGrid::worldToGrid(const Math::Vector3& index) const { return (index - this->_gridWorldIndex); }
Math::Vector3	ChunkGrid::gridToWorld(const Math::Vector3& index) const { return (this->_gridWorldIndex + index); }
std::string		ChunkGrid::toString() const {
	std::stringstream ss;
	ss << "playerChunkWorldIndex: " << this->_playerChunkWorldIndex.toString() << "\n";
	FOR(z, 0, this->_gridSize.z) {
		FOR(y, 0, this->_gridSize.y) {
			FOR(x, 0, this->_gridSize.x) {
				ss << "[" << z << "]";
				ss << "[" << y << "]";
				ss << "[" << x << "] " << this->_grid[z][y][x] << "\t";
				if (this->_grid[z][y][x])
					ss << this->_grid[z][y][x]->toString() << "\n";
				else
					ss << "nullptr\n";
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

	FOR(z, 0, this->_gridSize.z) {
		FOR(y, 0, this->_gridSize.y) {
			FOR(x, 0, this->_gridSize.x) {
				if (this->_grid[z][y][x])
					nonulls++;
				else
					nulls++;
				if (j == 0) {
					if (this->_heightMaps[z][x])
						hmnonulls++;
					else {
						hmnulls++;
					}
				}
			}
		}
	}
	int barsize = 20;
	int total = hmnonulls + hmnulls;
	int bar = barsize * hmnonulls / total;
	ss << " > Grid checks: \n";
	ss << " > Heightmaps:\t[" << std::string(bar, '=') << std::string(barsize - bar, ' ') << "] ";
	ss << hmnonulls << " nonull, " << hmnulls << " nulls, " << hmnonulls + hmnulls << " total\n";
	total = nonulls + nulls;
	bar = barsize * nonulls / total;
	ss << " > Chunks:\t[" << std::string(bar, '=') << std::string(barsize - bar, ' ') << "] ";
	ss << nonulls << " nonull, " << nulls << " nulls, " << nonulls + nulls << " total\n";

	return ss.str();
}
