#include "heightmap.hpp"

HeightMap::HeightMap(PerlinSettings& perlin_settings, int posx, int posz, int sizex, int sizez)
	: posX(posx), posZ(posz), sizeX(sizex), sizeZ(sizez)
{
	this->map = perlin_settings.genHeightMap(posx, posz, sizex, sizez);

	//map tiles
	//could be done in buildPanel, but it is done here for parallelized computation
	this->textureData = PerlinSettings::HeightmapToTextureData(map, this->sizeX, this->sizeZ);
	this->texture = nullptr;
	this->panel = nullptr;
}

HeightMap::~HeightMap() {
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
void	HeightMap::glth_buildPanel() {
	Glfw::glThreadSafety();
	if (!this->panel) {//if it's not already done
		this->texture = new Texture(this->textureData, this->sizeX, this->sizeZ);
		this->panel = new UIImage(this->texture);
		this->panel->setPos(0, 0);
		this->panel->setSize(this->sizeX, this->sizeZ);

		delete[] this->textureData;
	}
}
