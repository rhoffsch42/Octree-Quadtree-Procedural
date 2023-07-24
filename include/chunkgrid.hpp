#pragma once

#include "chunk.hpp"
#include "heightmap.hpp"
#include "chunkgenerator.hpp"

#include <vector>

typedef HeightMap*				HMapPtr;

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
	//push chunks with the asked LOD, inside the vector dst.
	void			pushRenderedChunks(std::vector<Object*>* dst, unsigned int lod = 0) const;
	//push chunks with the asked LOD, inside the array dst. Then returns the next index (last chunk added + 1)
	void			replaceHeightMap(HeightMap* new_hmap, Math::Vector3 index);
	void			replaceChunk(Chunk* new_chunk, Math::Vector3 index);

	HMapPtr**		getHeightMaps() const;
	ChunkShPtr***	getGrid() const;
	Obj3dBP*		getFullMeshBP() const;
	Obj3d*			getFullMesh() const;
	Math::Vector3	getSize() const;
	Math::Vector3	getRenderedSize() const;
	Math::Vector3	getChunkSize() const;
	Math::Vector3	getWorldIndex() const;
	Math::Vector3	getRenderedGridIndex() const;
	Math::Vector3	getPlayerChunkWorldIndex() const;
	GridGeometry	getGeometry() const;

	Math::Vector3	worldToGrid(const Math::Vector3& index) const;
	Math::Vector3	gridToWorld(const Math::Vector3& index) const;
	std::string		toString() const;
	std::string		getGridChecks() const;

	std::mutex		chunks_mutex;
	bool			chunksChanged;
	bool			gridShifted;
	bool			playerChangedChunk;
private:
	HMapPtr**		_heightMaps;//2d grid
	ChunkShPtr***	_grid;//3d grid

	bool			_chunksReadyForMeshes = false;
	Obj3dBP*		_fullMeshBP = nullptr;//all chunks merged
	Obj3d*			_fullMesh = nullptr;

	// Grid geometry
	Math::Vector3	_chunkSize;// needed only to determine the playerChunkWorldIndex
	Math::Vector3	_gridSize;
	Math::Vector3	_renderedGridSize;// must be <= gridSize
	Math::Vector3	_gridWorldIndex;// world index of the start of the grid (0:0:0)
	Math::Vector3	_renderedGridIndex;// index in grid
	Math::Vector3	_playerChunkWorldIndex;

	Math::Vector3	_playerWorldPos;

	ChunkGrid();
	void			_updatePlayerPosition(const Math::Vector3& pos);
	//calculate if we need to move the memory grid and if so, generate new chunks
	Math::Vector3	_calculateGridDiff(Math::Vector3 playerdiff);
	void			_translateGrid(Math::Vector3 gridDiff, std::vector<Chunk*>* chunksToDelete, std::vector<HeightMap*>* hmapsToDelete);
};
