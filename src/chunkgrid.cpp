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
	//todo : make a trash system
	//todo : use unique_ptr in the vectors
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

	this->_heightMaps = new HMapPtr* [this->_gridSize.z];
	for (unsigned int z = 0; z < this->_gridSize.z; z++) {
		this->_heightMaps[z] = new HMapPtr [this->_gridSize.x];
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
			delete this->_heightMaps[k][i];
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

	if (this->_fullMesh)
		delete this->_fullMesh;
	if (this->_fullMeshBP)
		delete this->_fullMeshBP;
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
	D("checks...\n" << this->getGridChecks());
	std::vector<Chunk*>		chunksToDelete;
	std::vector<HeightMap*>	hmapsToDelete;
	this->_translateGrid(diffGrid, &chunksToDelete, &hmapsToDelete);

	//debug checks before rebuild
	D("checks...\n" << this->getGridChecks());
	D("removed hmaps  \t" << hmapsToDelete.size() << "\n");
	D("removed chunks \t" << chunksToDelete.size() << "\n");
	this->_chunksReadyForMeshes = false;
	chunks_lock.unlock();

	D("Deleting...\n");
	_deleteChunksAndHeightmaps(&chunksToDelete, &hmapsToDelete);
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
void			ChunkGrid::_translateGrid(Math::Vector3 gridDiff, std::vector<Chunk*>* chunksToDelete, std::vector<HeightMap*>* hmapsToDelete) {
	//grid (memory)
	std::vector<int>		indexes_Z;
	std::vector<int>		indexes_Y;
	std::vector<int>		indexes_X;
	std::vector<ChunkShPtr>	chunks;
	std::vector<HeightMap*>	hmaps;
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
				this->_grid[z][y][x] = nullptr; // if the main RENDER thread reads here, he is fucked
				if (y == 0) {// there is only one hmap for an entire column of chunks
					hmaps.push_back(this->_heightMaps[z][x]);
					this->_heightMaps[z][x] = nullptr;// if the main RENDER thread reads here, he is fucked
				}
				//else { // useless as all hmaps have already been pushed (Y=0 loop was done first)
				//	hmaps.push_back(nullptr);	// to match the chunks index for the next hmap 
				//}
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
		}
		else {//is outside of the memory grid
			//chunksToDelete->push_back(chunks[n]); // not required anymore with shared_ptr
		}
	}

	D("reassigning heightmaps...\n");
	//unsigned int hmap_not_assigned = 0;
	//unsigned int hmap_null_not_assigned = 0;
	size_t step = this->_gridSize.x * this->_gridSize.z;
	for (size_t n = 0; n < step; n++) {
		if (hmaps[n]) {
			if (indexes_Z[n] < this->_gridSize.z && indexes_X[n] < this->_gridSize.x \
				&& indexes_Z[n] >= 0 && indexes_X[n] >= 0) {
				this->_heightMaps[indexes_Z[n]][indexes_X[n]] = hmaps[n];
			}
			else {//is outside of the memory grid
				hmapsToDelete->push_back(hmaps[n]);
				//hmap_not_assigned++;
			}
		}
		//else {
		//	hmap_null_not_assigned++;
		//}
	}
	//D("hmap_not_assigned : " << hmap_not_assigned << "\n");
	//D("hmap_null_not_assigned : " << hmap_null_not_assigned << "\n");
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
	Chunk* c = nullptr;
	for (unsigned int k = 0; k < this->_gridSize.z; k++) {
		for (unsigned int j = 0; j < this->_gridSize.y; j++) {
			for (unsigned int i = 0; i < this->_gridSize.x; i++) {
				c = this->_grid[k][j][i].get();
				if (c) {
					c->glth_buildAllMeshes();
				}
			}
		}
	}
}
void			ChunkGrid::pushRenderedChunks(std::vector<Object*>* dst, unsigned int lod) const {
	/*
	same as in glth_loadAllChunksToGPU() but more complicated
	we would need to select the chunks to remove (change the render vector with a map?),
	and push only the new chunks
	*/
	//D("Pushing rendered chunks to a vector<Object*> ...\n");
	//if (dst->size()) {
	//	D("Warning: Pushing rendered chunks to a vector<Object*> that is not empty\n");
	//}
	Chunk* c = nullptr;
	Math::Vector3	renderedGridEndIndex = this->_renderedGridIndex + this->_renderedGridSize;
	for (unsigned int k = this->_renderedGridIndex.z; k < renderedGridEndIndex.z; k++) {
		for (unsigned int j = this->_renderedGridIndex.y; j < renderedGridEndIndex.y; j++) {
			for (unsigned int i = this->_renderedGridIndex.x; i < renderedGridEndIndex.x; i++) {
				c = this->_grid[k][j][i].get();
				if (c && c->mesh[lod]) {//mesh can be null if the chunk is empty (see glth_buildAllMeshes())
					dst->push_back(c->mesh[lod]);
					//D("list : pushing chunck [" << k << "][" << j << "][" << i << "] " << c << " lod " << lod << "\n")
				}
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
	HeightMap* old = this->_heightMaps[z][x];
	this->_heightMaps[z][x] = new_hmap;
	if (old) {
		D("Deleting Heightmap[" << z << "][" << x << "]" << old << ", replaced by " << new_hmap << "\n");
		delete old;//todo: should send to trash and not delete right away. need IDisposable
	}
}
void			ChunkGrid::replaceChunk(Chunk* new_chunk, Math::Vector3 index) {
	unsigned int	x = index.x;
	unsigned int	y = index.y;
	unsigned int	z = index.z;
	if (x < 0 || y < 0 || z < 0 || x > this->_gridSize.x - 1 || y > this->_gridSize.y - 1 || z > this->_gridSize.z - 1) {
		D("ChunkGrid::replaceChunk() Error : supplied index is invalid");
		return;
	}
	//Chunk* old = this->_grid[z][y][x];
	this->_grid[z][y][x] = std::make_shared<Chunk>(*new_chunk);
	this->chunksChanged = true;
	//if (old) {
	//	D("Deleting Chunk in grid[" << z << "][" << y << "][" << x << "]" << old << ", replaced by " << new_chunk << "\n");
	//	D(" >> " << old->toString() << "\n");
	//	D(" >> " << new_chunk->toString() << "\n");
	//	delete old;//todo: should send to trash and not delete right away. need IDisposable
	//}
}

HMapPtr**		ChunkGrid::getHeightMaps() const			{ return this->_heightMaps; }
ChunkShPtr***	ChunkGrid::getGrid() const					{ return this->_grid; }
Obj3dBP*		ChunkGrid::getFullMeshBP() const			{ return this->_fullMeshBP; }
Obj3d*			ChunkGrid::getFullMesh() const				{ return this->_fullMesh; }
Math::Vector3	ChunkGrid::getSize() const					{ return this->_gridSize; }
Math::Vector3	ChunkGrid::getRenderedSize() const			{ return this->_renderedGridSize; }
Math::Vector3	ChunkGrid::getChunkSize() const				{ return this->_chunkSize; }
Math::Vector3	ChunkGrid::getRenderedGridIndex() const		{ return this->_renderedGridIndex; }
Math::Vector3	ChunkGrid::getWorldIndex() const			{ return this->_gridWorldIndex; }
Math::Vector3	ChunkGrid::getPlayerChunkWorldIndex() const	{ return this->_playerChunkWorldIndex; }
GridGeometry	ChunkGrid::getGeometry() const {
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
	for (int k = 0; k < this->_gridSize.z; k++) {
		for (int j = 0; j < this->_gridSize.y; j++) {
			for (int i = 0; i < this->_gridSize.x; i++) {
				ss << "[" << k << "]";
				ss << "[" << j << "]";
				ss << "[" << i << "] " << this->_grid[k][j][i] << "\t";
				if (this->_grid[k][j][i])
					ss << this->_grid[k][j][i]->toString() << "\n";
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
