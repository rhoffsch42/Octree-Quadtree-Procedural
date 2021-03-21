#pragma once

#include "math.hpp"
#include "chunk.hpp"
#include "heightmap.hpp"
#include <mutex>
#include <condition_variable>
#include <vector>

#define CHUNK_GEN_DEBUG	0

//make a class for that
typedef uint8_t**	hmap;
typedef UIImage*	minimapTile;


class ChunkGenerator
{
public:
	std::condition_variable	cv;
	std::mutex	chunks_mutex;

	//grid_size_displayed will be clamped between 1 -> grid_size
	ChunkGenerator(Math::Vector3 player_pos, const PerlinSettings& perlin_settings, Math::Vector3 chunk_size, Math::Vector3 grid_size, Math::Vector3 grid_size_displayed);
	~ChunkGenerator();
	bool	updateGrid_old(Math::Vector3 player_pos);
	bool	updateGrid(Math::Vector3 player_pos);
	void	th_builders();
	bool	buildMeshesAndMapTiles();
	bool	try_deleteUnusedData();

	void	printData();

	Math::Vector3	chunkSize;

	Math::Vector3	gridSize;
	Math::Vector3	gridPos;//the center of the grid in game world coo
	Math::Vector3	gridDisplaySize;
	Math::Vector3	gridDisplayIndex;//the chunck where the player is

	Chunk* ***		grid;
	PerlinSettings	settings;
	Math::Vector3	currentChunk;

	HeightMap* **	heightMaps;
	Math::Vector3	playerPos;
	bool			playerChangedChunk;
	std::mutex		mutex1;//? used in updatePlayerPos, usefull? maybe for events
	bool	_chunksReadyForMeshes;
	bool	gridMemoryMoved;
	uint8_t threadsReadyToBuildChunks;
private:
	void	updatePlayerPos(Math::Vector3 player_pos);
	ChunkGenerator();

	void	deleteUnusedData();
	std::mutex					trash_mutex;
	std::vector<HeightMap*>		trashHeightMaps;
	std::vector<Chunk*>			trashChunks;
};

/*
	avoir des chunk bien plus grand que 32, genre 500. (le temps de generation peut etre long)
	ajuster le threshold selon la distance avec le joueur
		si un des coins du cube de la node est dans le range, alors descendre le threshold

	on genere seulement les chunk voisins (3*3*3 - 1)
	on diplay seulement les nodes en dessous d'un certain range en adaptant les threshold
*/
