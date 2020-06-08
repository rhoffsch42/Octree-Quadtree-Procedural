#pragma once

#include "math.hpp"
#include "chunk.hpp"
#include "uipanel.hpp"

#define CHUNK_GEN_DEBUG	0

//make a class for that
typedef uint8_t**	hmap;
typedef UIImage*	minimapTile;

class HeightMap//2D
{
public:
	HeightMap(PerlinSettings& perlin_settings, int posx, int posz, int sizex, int sizez)
		: posX(posx), posZ(posz), sizeX(sizex), sizeZ(sizez)
	{
		perlin_settings.genHeightMap(posx, posz, sizex, sizez);//this can be optimized to do it only once per Y (and even keep the generated heightmap in memory for later use)
		this->map = perlin_settings.map;
		perlin_settings.map = nullptr;

		//build map tiles
		this->texture = PerlinSettings::HeightmapToTexture(map, sizex, sizez);
		this->panel = new UIImage(this->texture);
		this->panel->setPos(0, 0);
		this->panel->setSize(sizex, sizez);
	}
	~HeightMap() {
		delete this->texture;
		delete this->panel;
		for (int k = 0; k < this->sizeZ; k++) {
			delete[] this->map[k];
		}
		delete[] this->map;
	}

	int			posX;
	int			posZ;
	int			sizeX;
	int			sizeZ;
	uint8_t**	map;
	Texture*	texture;
	UIImage*	panel;
private:
};

class ChunkGenerator
{
public:
	//grid_size_displayed will be clamped between 1 -> grid_size
	ChunkGenerator(Math::Vector3 player_pos, const PerlinSettings& perlin_settings, Math::Vector3 chunk_size, Math::Vector3 grid_size, Math::Vector3 grid_size_displayed);
	~ChunkGenerator();
	bool	updateGrid(Math::Vector3 player_pos);

	void	printData();

	Math::Vector3	chunkSize;

	Math::Vector3	gridSize;
	Math::Vector3	gridPos;//the center of the grid in game world coo
	Math::Vector3	gridSizeDisplay;
	Math::Vector3	gridDisplayIndex;

	Chunk* ***		grid;
	PerlinSettings	settings;
	Math::Vector3	currentChunk;

	HeightMap* **	heightMaps;
	Math::Vector3	playerPos;
private:

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
