#include "heightmap.hpp"



HeightMap::HeightMap(PerlinSettings& perlin_settings, Math::Vector3 chunk_index, Math::Vector3 chunk_size)
	: IDisposable(), _index(chunk_index)
{
	this->_posX = this->_index.x * chunk_size.x;
	this->_posZ = this->_index.z * chunk_size.z;
	this->_sizeX = chunk_size.x;
	this->_sizeZ = chunk_size.z;

	this->buildMap(perlin_settings);
}

HeightMap::HeightMap(PerlinSettings& perlin_settings, int posx, int posz, int sizex, int sizez)
	: IDisposable(), _posX(posx), _posZ(posz), _sizeX(sizex), _sizeZ(sizez)
{
	this->buildMap(perlin_settings);
}

void	HeightMap::buildMap(PerlinSettings& perlin_settings) {
	this->map = perlin_settings.genHeightMap(this->_posX, this->_posZ, this->_sizeX, this->_sizeZ);
	//hmap texture
	this->textureData = PerlinSettings::HeightmapToTextureData(map, this->_sizeX, this->_sizeZ);
	this->texture = nullptr;
	this->panel = nullptr;
}

HeightMap::~HeightMap() {
	//std::cout << __PRETTY_FUNCTION__ << this << "\n";
	if (this->texture)
		delete this->texture;
	if (this->panel)
		delete this->panel;

	//should be deleted only here, it is used by Chunk::
	for (int k = 0; k < this->_sizeZ; k++) {
		delete[] this->map[k];
	}
	delete[] this->map;
	this->map = nullptr;
}

//opengl thread
void	HeightMap::glth_buildPanel() {
	//Glfw::glThreadSafety();
	if (!this->panel) {//if it's not already done
		this->texture = new Texture(this->textureData, this->_sizeX, this->_sizeZ);
		this->panel = new UIImage(this->texture);
		this->panel->setPos(0, 0);
		this->panel->setSize(this->_sizeX, this->_sizeZ);

		delete[] this->textureData;
	}
}

Math::Vector3	HeightMap::getIndex() const { return this->_index; }
