#pragma once

#include "math.hpp"
#include "chunk.hpp"
#include "uipanel.hpp"
#include <mutex>

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

		//map tiles
		//could be done in buildPanel, but it is done here for parallelized computation
		this->textureData = PerlinSettings::HeightmapToTextureData(map, this->sizeX, this->sizeZ);
		this->texture = nullptr;
		this->panel = nullptr;
	}
	~HeightMap() {
		if (this->texture)
			delete this->texture;
		if (this->panel)
			delete this->panel;

		//should be deleted only here, it is used by Chunk::
		for (int k = 0; k < this->sizeZ; k++) {
			delete[] this->map[k];
		}
		delete[] this->map;
		this->map = nullptr;
	}

	//opengl thread
	void	glth_buildPanel() {
		Glfw::glThreadSafety();
		if (!this->panel) {//if it's not already done
			this->texture = new Texture(this->textureData, this->sizeX, this->sizeZ);
			this->panel = new UIImage(this->texture);
			this->panel->setPos(0, 0);
			this->panel->setSize(this->sizeX, this->sizeZ);

			delete[] this->textureData;
		}
	}

	int			posX;
	int			posZ;
	int			sizeX;
	int			sizeZ;
	uint8_t**	map;//used by Chunk::

	//used to display the map tile
	uint8_t*	textureData;
	Texture*	texture;
	UIImage*	panel;
private:
};

class ChunkGenerator
{
public:
	static std::mutex	chunks_mutex;

	//grid_size_displayed will be clamped between 1 -> grid_size
	ChunkGenerator(Math::Vector3 player_pos, const PerlinSettings& perlin_settings, Math::Vector3 chunk_size, Math::Vector3 grid_size, Math::Vector3 grid_size_displayed);
	~ChunkGenerator();
	bool	updateGrid_old(Math::Vector3 player_pos);
	bool	updateGrid(Math::Vector3 player_pos);
	bool	buildMeshesAndMapTiles();

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
	std::mutex		mutex1;
private:
	void	updatePlayerPos(Math::Vector3 player_pos);
	bool	_chunksReadyForMeshes;
	ChunkGenerator();
};

/*
	avoir des chunk bien plus grand que 32, genre 500. (le temps de generation peut etre long)
	ajuster le threshold selon la distance avec le joueur
		si un des coins du cube de la node est dans le range, alors descendre le threshold

	on genere seulement les chunk voisins (3*3*3 - 1)
	on diplay seulement les nodes en dessous d'un certain range en adaptant les threshold
*/
