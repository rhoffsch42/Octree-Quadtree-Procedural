#pragma once

#include "uipanel.hpp"
#include "perlin.hpp"
#include "math.hpp"
#include "dispose.hpp"


#define HMAP_BUILD_TEXTUREDATA_IN_CTOR	1 // this is usually done in builder threads, while the UIPanel has to be done on the gl thread
#define PERLIN_GENERATION				1
#define OCTREE_OPTIMISATION				1
#define DISPLAY_BLACK					1

#define PERLIN_NORMALIZER			500
#define PERLIN_DEFAULT_OCTAVES		8
#define PERLIN_DEFAULT_FREQUENCY	8
#define PERLIN_DEFAULT_FLATTERING	1
#define PERLIN_DEFAULT_HEIGHTCOEF	1
#define PERLIN_DEFAULT_ISLAND		0
#define	CHUNK_DEFAULT_SIZE			32
#define VOXEL_EMPTY					Pixel(255, 255, 255) //todo: change this to 0,0,0 and adapt octree

#include "compiler_settings.h"
class PerlinSettings
{
public:
	static uint8_t* HeightmapToTextureData(uint8_t** map, int sizex, int sizez) {
		uint8_t* data = new uint8_t[sizex * sizez * 3];//care overflow, a chunk shouldn't be that big tho
		for (int z = 0; z < sizez; z++) {
			for (int x = 0; x < sizex; x++) {
				int index = 3 * (z * sizex + x);
				data[index + 0] = map[z][x];
				data[index + 1] = map[z][x];
				data[index + 2] = map[z][x];
			}
		}
		return data;
	}

	uint8_t** genHeightMap(int posx, int posz, int sizex, int sizez) {
		//std::cout << "_ " << __PRETTY_FUNCTION__ << std::endl;
		//std::cout << posx << ":" << posz << std::endl;
		this->posX = posx;
		this->posZ = posz;
		this->sizeX = sizex;
		this->sizeZ = sizez;

		uint8_t** map = new uint8_t * [this->sizeZ];
		for (int z = 0; z < this->sizeZ; z++)
			map[z] = new uint8_t[this->sizeX];

		for (int z = 0; z < this->sizeZ; z++) {
			for (int x = 0; x < this->sizeX; x++) {
				if (z < 0) {
					std::cout << z << " WTF" << std::endl;
				}
				Math::Vector3	currentPos(this->posX + x, 0, this->posZ + z);//we dont care of y
				double value = perlin.accumulatedOctaveNoise2D_0_1(
					double(currentPos.x) / double(PERLIN_NORMALIZER) * this->frequency,
					double(currentPos.z) / double(PERLIN_NORMALIZER) * this->frequency,
					this->octaves);
				value = std::pow(value, this->flattering);
				//double dist = (double(currentPos.magnitude()) / double(PERLIN_NORMALIZER / 2));//normalized 0..1
				//value = std::clamp(value + this->island * (0.5 - dist), 0.0, 1.0);
				value *= this->heightCoef;

				//manual
				value = std::clamp(value, 0.15, 1.0);
				//final value
				map[z][x] = uint8_t(value * 255.0);
			}
		}
		return map;
	}

	PerlinSettings(siv::PerlinNoise& perlin_ref)
		: perlin(perlin_ref)
	{
		this->octaves = PERLIN_DEFAULT_OCTAVES;
		this->frequency = PERLIN_DEFAULT_FREQUENCY;
		this->flattering = PERLIN_DEFAULT_FLATTERING;
		this->heightCoef = PERLIN_DEFAULT_HEIGHTCOEF;
		this->island = PERLIN_DEFAULT_ISLAND;

		//this->flattering = 2;
		this->heightCoef = 1.0 / 5.0;

		this->posX = 0;
		this->posZ = 0;
		this->sizeX = CHUNK_DEFAULT_SIZE;
		this->sizeZ = CHUNK_DEFAULT_SIZE;
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
		//this->map = nullptr;//doesnt cpy map!;
		this->posX = src.posX;
		this->posZ = src.posZ;
		this->sizeX = src.sizeX;
		this->sizeZ = src.sizeZ;
		return *this;
	}
	~PerlinSettings() {}

	siv::PerlinNoise& perlin;
	unsigned int		octaves;
	double				frequency;
	double				flattering;
	double				heightCoef;
	double				island;
	int					posX;
	int					posZ;
	int					sizeX;
	int					sizeZ;
private:
};

class HeightMap;
typedef std::shared_ptr<HeightMap>	HeightMapShPtr;

class HeightMap : public IDisposable//2D
{
public:
	static std::mutex	m;
	static int count;
	HeightMap(PerlinSettings& perlin_settings, Math::Vector3 chunk_index, Math::Vector3 chunk_size);
	HeightMap(PerlinSettings& perlin_settings, int posx, int posz, int sizex, int sizez);
	~HeightMap();

	//opengl thread
	void			glth_buildPanel();
	Math::Vector3	getIndex() const;
	//bool operator<(const HeightMap& rhs) const//tmp
	//{
	//	return std::tie(this->tile.x, this->tile.y, this->tile.z) < std::tie(rhs.tile.x, rhs.tile.y, rhs.pos.z);
	//}


	uint8_t**	map = nullptr;//used by Chunk::

	//used to display the map tile on screen
	uint8_t* textureData = nullptr;
	Texture* texture = nullptr;
	UIImage* panel = nullptr;
private:
	Math::Vector3	_index;
	int				_posX;//not tile
	int				_posZ;
	unsigned int	_sizeX;
	unsigned int	_sizeZ;

	void	buildMap(PerlinSettings& perlin_settings);
};