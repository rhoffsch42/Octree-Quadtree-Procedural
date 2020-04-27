#pragma once

#include "math.hpp"
#include "octree.hpp"
#include "perlin.hpp"

#include <algorithm>

#define PERLIN_GENERATION	1
#define OCTREE_OPTIMISATION	1
#define DISPLAY_BLACK		1

#define PERLIN_NORMALIZER		500
#define PERLIN_DEF_OCTAVES		8
#define PERLIN_DEF_FREQUENCY	8
#define PERLIN_DEF_FLATTERING	1
#define PERLIN_DEF_HEIGHTCOEF	1
#define PERLIN_DEF_ISLAND		1
#define	CHUNK_DEF_SIZE			32
#define VOXEL_EMPTY				Pixel(255, 255, 255)


class PerlinSettings
{
public:
	void	genHeightMap(int posx, int posz, int sizex, int sizez) {
		//std::cout << "_ " << __PRETTY_FUNCTION__ << std::endl;
		//std::cout << posx << ":" << posz << std::endl;
		this->posX = posx;
		this->posZ = posz;
		this->sizeX = sizex;
		this->sizeZ = sizez;

		if (!this->map) {//so that we can override the current map if it was already generated with different pos
			//std::cout << "new!" << std::endl;
			this->map = new uint8_t * [this->sizeZ];
			for (int z = 0; z < this->sizeZ; z++)
				this->map[z] = new uint8_t[this->sizeX];
		}
		for (int z = 0; z < this->sizeZ; z++) {
			for (int x = 0; x < this->sizeX; x++) {
				Math::Vector3	currentPos(this->posX + x, 0, this->posZ + z);//we dont care of y
				double value = perlin.accumulatedOctaveNoise2D_0_1(
					double(currentPos.x) / double(PERLIN_NORMALIZER) * this->frequency,
					double(currentPos.z) / double(PERLIN_NORMALIZER) * this->frequency,
					this->octaves);
				value = std::pow(value, this->flattering);
				double dist = (double(currentPos.magnitude()) / double(PERLIN_NORMALIZER / 2));//normalized 0..1
				value = std::clamp(value + this->island * (0.5 - dist), 0.0, 1.0);
				value *= this->heightCoef;
				this->map[z][x] = uint8_t(value * 255.0);
			}
		}
	}

	PerlinSettings(siv::PerlinNoise& perlin_ref)
		: perlin(perlin_ref)
	{
		this->octaves = PERLIN_DEF_OCTAVES;
		this->frequency = PERLIN_DEF_FREQUENCY;
		this->flattering = PERLIN_DEF_FLATTERING;
		this->heightCoef = PERLIN_DEF_HEIGHTCOEF;
		this->island = PERLIN_DEF_ISLAND;
		
		//this->flattering = 2;
		this->heightCoef = 1.0 / 5.0;

		this->map = nullptr;
		this->posX = 0;
		this->posZ = 0;
		this->sizeX = CHUNK_DEF_SIZE;
		this->sizeZ = CHUNK_DEF_SIZE;
	}
	PerlinSettings(const PerlinSettings& src) : perlin(src.perlin) {
		*this = src;
	}
	PerlinSettings& operator=(const PerlinSettings& src) {
		this->octaves = src.octaves;
		this->frequency = src.frequency;
		this->flattering = src.flattering;
		this->heightCoef = src.heightCoef;
		this->island = src.island;
		this->map = nullptr;//doesnt cpy map!;
		this->posX = src.posX;
		this->posZ = src.posZ;
		this->sizeX = src.sizeX;
		this->sizeZ = src.sizeZ;
		return *this;
	}
	~PerlinSettings() {
		std::cout << "_ " << __PRETTY_FUNCTION__ << std::endl;
		if (this->map) {
			std::cout << "delete map" << std::endl;
			for (unsigned int j = 0; j < this->sizeZ; j++) {
				delete[] this->map[j];
			}
			delete[] this->map;
		}
	}

	siv::PerlinNoise& perlin;
	unsigned int		octaves;
	double				frequency;
	double				flattering;
	double				heightCoef;
	double				island;
	uint8_t**			map;
	int					posX;
	int					posZ;
	int					sizeX;
	int					sizeZ;
private:
};

class Chunk //could inherit from Object
{
public:
	Chunk(const Math::Vector3& tile_number, unsigned int chunk_size, PerlinSettings& perlinSettings);
	~Chunk();

	void	printData();

	Math::Vector3	tile;
	Math::Vector3	pos;
	unsigned int	size;
	Octree*			root;
	uint8_t***		data;

private:
	Chunk();
};
