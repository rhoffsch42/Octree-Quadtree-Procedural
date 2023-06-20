#pragma once

#include "chunk.hpp"
#include "heightmap.hpp"
#include "chunkgenerator.hpp"

typedef Chunk* ChunkPtr;
typedef HeightMap* HMapPtr;

class ChunkGrid
{
public:
	ChunkGrid(Math::Vector3 player_pos);
	~ChunkGrid();

	void			th_updater(Cam* cam);
	bool			updateGrid(Math::Vector3 player_pos);

	// Build the chunks meshes and load them to the GPU. Must be executed in the OpenGL thread
	void			glth_loadChunks();
	//push chunks with the asked tesselation level, inside the list dst.
	void			pushDisplayedChunks(std::list<Object*>* dst, unsigned int tesselation_lvl = 0) const;
	//push chunks with the asked tesselation level, inside the array dst. Then returns the next index (last chunk added + 1)
	unsigned int	pushDisplayedChunks(Object** dst, unsigned int tesselation_lvl = 0, unsigned int starting_index = 0) const;

	Math::Vector3	worldToGrid(const Math::Vector3& index) const;
	Math::Vector3	gridToWorld(const Math::Vector3& index) const;
	Math::Vector3	getGridDisplayStart() const;
	std::string		toString() const;
	std::string		getGridChecks() const;

private:
	std::mutex		_chunks_mutex;

	Math::Vector3	_chunkSize;// needed only to determine the playerChunkWorldIndex
	Math::Vector3	_gridSize;
	Math::Vector3	_gridDisplaySize;// must be <= gridSize
	Math::Vector3	_gridWorldIndex;// world index of the start of the grid (0:0:0)
	Math::Vector3	_gridDisplayIndex;// index in grid
	Math::Vector3	_playerWorldPos;// world position
	Math::Vector3	_playerChunkWorldIndex;

	HMapPtr**		_heightMaps;//2d grid
	ChunkPtr***		_grid;//3d grid
	bool			_playerChangedChunk;

	Obj3dBP*		_fullMeshBP = nullptr;//all chunks merged
	Obj3d*			_fullMesh = nullptr;

	ChunkGrid();
	void			_updatePlayerPosition(const Math::Vector3& pos);
	//calculate if we need to move the memory grid and if so, generate new chunks
	Math::Vector3	_calculateGridDiff(Math::Vector3 playerdiff);
	void			_translateGrid(Math::Vector3 gridDiff, std::vector<Chunk*>* chunksToDelete, std::vector<HeightMap*>* hmapsToDelete);
};
