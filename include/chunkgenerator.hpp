#pragma once

#include "math.hpp"
#include "chunk.hpp"

#define CHUNK_GEN_DEBUG	1

typedef uint8_t** hmap;

class ChunkGenerator
{
public:
	ChunkGenerator(Math::Vector3 player_pos, PerlinSettings perlin_settings, Math::Vector3 chunk_size, Math::Vector3 grid_size);
	~ChunkGenerator();
	bool	updateGrid(Math::Vector3 player_pos);

	void	printData();

	Math::Vector3	chunkSize;
	Math::Vector3	gridSize;//3*3*3
	Chunk****		grid;//3*3*3 Chunk*
	PerlinSettings	settings;
private:
	Math::Vector3	playerPos;
	Math::Vector3	currentChunk;
	hmap**			heightMaps;

	void	updatePlayerPos(Math::Vector3 player_pos);
	void	initValues(float diff, float& inc, float& start, float& end, float& endShift, float intersectionDimension, float size);
	ChunkGenerator();
};

/*
	avoir des chunk bien plus grand que 32, genre 500. (le temps de generation peut etre long)
	ajuster le threshold selon la distance avec le joueur
		si un des coins du cube de la node est dans le range, alors descendre le threshold

	on genere seulement les chunk voisins (3*3*3 - 1)
	on diplay seulement les nodes en dessous d'un certain range en adaptant les threshold
*/
