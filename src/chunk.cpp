#include "chunk.hpp"
#include <iostream>

Chunk::Chunk() {}

Chunk::Chunk(const Math::Vector3& tile_number, unsigned int chunk_size, PerlinSettings& perlinSettings) {
	this->tile = tile_number;
	this->pos = tile_number;
	this->pos.mult(chunk_size);
	this->size = chunk_size;

	if (perlinSettings.map == nullptr)
		perlinSettings.genHeightMap(this->pos.x, this->pos.z, this->size, this->size);

	this->data = new uint8_t**[this->size];
	for (unsigned int k = 0; k < this->size; k++) {
		this->data[k] = new uint8_t * [this->size];
		for (unsigned int j = 0; j < this->size; j++) {
			this->data[k][j] = new uint8_t[this->size];
			for (unsigned int i = 0; i < this->size; i++) {
				//std::cout << "pos.y + j = " << (pos.y + j) << "  is > to " << int(perlinSettings.map[k][i]) << "\t?" << std::endl;
				if (pos.y + j > int(perlinSettings.map[k][i])) {
					this->data[k][j][i] = VOXEL_EMPTY.r;
				} else {
					double value = perlinSettings.perlin.accumulatedOctaveNoise3D_0_1(
						double(pos.x + i) / double(PERLIN_NORMALIZER) * perlinSettings.frequency,
						double(pos.y + j) / double(PERLIN_NORMALIZER) * perlinSettings.frequency,
						double(pos.z + k) / double(PERLIN_NORMALIZER) * perlinSettings.frequency,
						perlinSettings.octaves);
					uint8_t v = uint8_t(double(255.0) * value);
					//v = v / 128 * 128;
					if (v >= 128)
						v = 150;
					else
						v = 75;
					v = 75;
					//std::cout << value << " ";
					//std::cout << (int)v << " ";
					this->data[k][j][i] = v;
				}
			}
		}
	}
	Math::Vector3	size_vec(this->size, this->size, this->size);

	//tmp
	Pixel*** pix = new Pixel**[this->size];
	for (unsigned int k = 0; k < this->size; k++) {
		pix[k] = new Pixel * [this->size];
		for (unsigned int j = 0; j < this->size; j++) {
			pix[k][j] = new Pixel[this->size];
			for (unsigned int i = 0; i < this->size; i++) {
				pix[k][j][i].r = this->data[k][j][i];
				pix[k][j][i].g = this->data[k][j][i];
				pix[k][j][i].b = this->data[k][j][i];
			}
		}
	}
	// important note: all chunks have their octree starting at pos 0 0 0. 
	this->root = new Octree(pix, Math::Vector3(0,0,0), size_vec, 0);
	this->root->verifyNeighbors(VOXEL_EMPTY, this->pos);//white

	/*
fustrum objects: 5649
forward objects: 8415   (not in fustrum)
total objects: 28859

fustrum objects: 5649
forward objects: 8380   (not in fustrum)
total objects: 28859
	*/

	//this->root = new Octree(this->data, this->pos, size_vec, 0);//faire un class abstraite?
	//delete this->data ?
}

Chunk::~Chunk() {
	delete this->root;
	for (unsigned int k = 0; k < this->size; k++) {
		for (unsigned int j = 0; j < this->size; j++) {
			delete[] this->data[k][j];
		}
		delete[] this->data[k];
	}
	delete[] this->data;
}

void	Chunk::printData() {
	std::cout << "tile: "; this->tile.printData();
	std::cout << "pos: "; this->pos.printData();
	std::cout << "root: " << this->data << std::endl;
}
