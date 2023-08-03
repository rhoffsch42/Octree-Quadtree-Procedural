#pragma once

#include "chunk.hpp"
#include "heightmap.hpp"
#include "chunkgenerator.hpp"

#include <vector>

#define GRID_GARBAGE_DELETION_STEP			10
#define GRID_GARBAGE_DELETION_RECOMMENDED	1000

typedef struct s_grid_gemoetry
{
	Math::Vector3	chunkSize;
	Math::Vector3	gridSize;
	Math::Vector3	renderedGridSize;
	Math::Vector3	gridWorldIndex;
	Math::Vector3	renderedGridIndex;
	Math::Vector3	playerChunkWorldIndex;
} GridGeometry;

class ChunkGrid
{
public:
	ChunkGrid(Math::Vector3 chunk_size, Math::Vector3 grid_size, Math::Vector3 rendered_grid_size);
	~ChunkGrid();

	bool			updateGrid(Math::Vector3 player_pos);
	// Build the chunks meshes and load them to the GPU. Must be executed in the OpenGL thread
	void			glth_loadAllChunksToGPU();
	void			pushAllHeightmaps(std::vector<HeightMapShPtr>* dst) const;
	void			pushAllChunks(std::vector<ChunkShPtr>* dst) const;
	void			pushRenderedChunks(std::vector<ChunkShPtr>* dst) const;
	void			replaceHeightMap(HeightMap* new_hmap, Math::Vector3 index);
	void			replaceChunk(Chunk* new_chunk, Math::Vector3 index);

	void			glth_try_emptyGarbage();
	bool			glth_try_deleteUnusedHeightmaps(size_t amount);
	bool			glth_try_deleteUnusedChunks(size_t amount);
	size_t			getGarbageSize() const;
	void			glth_testGarbage(size_t amount);

	HeightMapShPtr**	getHeightMaps() const;
	ChunkShPtr***		getGrid() const;
	Math::Vector3		getSize() const;
	Math::Vector3		getRenderedSize() const;
	Math::Vector3		getChunkSize() const;
	Math::Vector3		getWorldIndex() const;
	Math::Vector3		getRenderedGridIndex() const;
	Math::Vector3		getPlayerChunkWorldIndex() const;
	GridGeometry		getGeometry() const;

	Math::Vector3	worldToGrid(const Math::Vector3& index) const;
	Math::Vector3	gridToWorld(const Math::Vector3& index) const;
	std::string		toString() const;
	std::string		getGridChecks() const;

	std::mutex		garbageMutex;
	std::mutex		chunks_mutex;
	bool			chunksChanged;
	bool			gridShifted;
	bool			playerChangedChunk;
private:
	HeightMapShPtr**	_heightMaps;//2d grid
	ChunkShPtr***		_grid;//3d grid

	bool			_chunksReadyForMeshes = false;

	// Grid geometry
	Math::Vector3	_chunkSize;// needed only to determine the playerChunkWorldIndex
	Math::Vector3	_gridSize;
	Math::Vector3	_renderedGridSize;// must be <= gridSize
	Math::Vector3	_gridWorldIndex;// world index of the start of the grid (0:0:0)
	Math::Vector3	_renderedGridIndex;// index in grid
	Math::Vector3	_playerChunkWorldIndex;

	Math::Vector3	_playerWorldPos;

	// Garbage
	std::vector<ChunkShPtr>		_garbageChunks;
	std::vector<HeightMapShPtr>	_garbageHeightmaps;

	ChunkGrid();
	void			_updatePlayerPosition(const Math::Vector3& pos);
	//calculate if we need to move the memory grid and if so, generate new chunks
	Math::Vector3	_calculateGridDiff(Math::Vector3 playerdiff);
	void			_translateGrid(Math::Vector3 gridDiff);
};
