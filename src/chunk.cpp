#include "chunk.hpp"
#include <iostream>

Obj3dBP* Chunk::cubeBlueprint = nullptr;
Obj3dPG* Chunk::renderer = nullptr;

int	Chunk::buildVertexArrayFromOctree(Octree* root, Math::Vector3 pos_offset) {
	/*
		for each chunck, build a linear vertex array with concatened faces and corresponding attributes (texcoord, color, etc)
		it will use the chunk matrix so for the vertex position we simply add the chunk pos and the node pos to the vertex.position of the face

		this will be used with glDrawArray() later
		we could even concat all chunk array for a single glDrawArray
			cons: we need to remap data at every chunck change
				-> display list? check what it is
	*/

	/*
	# order with assimp: (probably because it trys to fan/strip tiangles)
	#   white red green blue purple black
	#   front right left bottom top back

	#define CUBE_FRONT_FACE		0
	#define CUBE_RIGHT_FACE		1
	#define CUBE_LEFT_FACE		2
	#define CUBE_BOTTOM_FACE	3
	#define CUBE_TOP_FACE		4
	#define CUBE_BACK_FACE		5

	012 023 456 467 8910 81011 121314 121415 161718 161819 202122 202223
	011 001 101 111 111 101 100 110 010 000 001 011 001 000 100 101 010 011 111 110 110 100 000 010 

	*/
	/*
	float	cube_array[] = { 0,1,1, 0,0,1, 1,0,1, 1,1,1, 1,1,1, 1,0,1, 1,0,0, 1,1,0, 0,1,0, 0,0,0, 0,0,1, 0,1,1, 0,0,1, 0,0,0, 1,0,0, 1,0,1, 0,1,0, 0,1,1, 1,1,1, 1,1,0, 1,1,0, 1,0,0, 0,0,0, 0,1,0 };
	int		indices_array[] = { 0,1,2, 0,2,3, 4,5,6, 4,6,7, 8,9,10, 8,10,11, 12,13,14, 12,14,15, 16,17,18, 16,18,19, 20,21,22, 20,22,23 };
	std::vector<Math::Vector3>	colors;
	colors.push_back(Math::Vector3(255, 255, 255));//white
	colors.push_back(Math::Vector3(255, 0, 0));//red
	colors.push_back(Math::Vector3(0, 255, 0));//green
	colors.push_back(Math::Vector3(0, 0, 255));//blue
	colors.push_back(Math::Vector3(255, 0, 255));//purple
	colors.push_back(Math::Vector3(0, 0, 0));//black

	std::vector<float>	linear_array;
	for (size_t i = 0; i < 6*2*3; i++) {
		linear_array.push_back(cube_array[indices_array[i]]);
	}

	std::vector<SimpleVertex>	vertex_array;
	for (size_t i = 0; i < 12; i++) {
		SimpleVertex	v;
		v.position.x = linear_array[i * 3 + 0];
		v.position.y = linear_array[i * 3 + 1];
		v.position.z = linear_array[i * 3 + 2];
		v.color = colors[i / 2];
		vertex_array.push_back(v);
	}
	*/
	if (!Chunk::cubeBlueprint) {
		std::cout << "Obj3dBP*	Chunk::cubeBlueprint is null" << std::endl;
		return 0;
	}
	std::vector<SimpleVertex>	vertices = Chunk::cubeBlueprint->getVertices();
	std::vector<SimpleVertex>	mesh_array;
	root->browse(0, [&pos_offset, &mesh_array, &vertices](Octree* node) {
		if ((node->pixel.r < VOXEL_EMPTY.r \
			|| node->pixel.g < VOXEL_EMPTY.g \
			|| node->pixel.b < VOXEL_EMPTY.b) \
			&& node->neighbors < NEIGHBOR_ALL)// should be < NEIGHBOR_ALL or (node->n & NEIGHBOR_ALL) != 0
		{
			Math::Vector3	cube_pos(pos_offset);//can be the root pos or (0,0,0)
			cube_pos.add(node->pos);//the position of the cube
			int neighbors_flags[] = { NEIGHBOR_FRONT, NEIGHBOR_RIGHT, NEIGHBOR_LEFT, NEIGHBOR_BOTTOM, NEIGHBOR_TOP, NEIGHBOR_BACK };
			for (size_t i = 0; i < 6; i++) {//6 faces
				if ((node->neighbors & neighbors_flags[i]) != neighbors_flags[i]) {
					for (size_t j = 0; j < 6; j++) {// push the 2 triangles = 2 * 3 vertex
						SimpleVertex	vertex = vertices[i * 6 + j];
						vertex.position.x *= node->size.x;
						vertex.position.y *= node->size.y;
						vertex.position.z *= node->size.z;
						vertex.position.add(node->pos);
						mesh_array.push_back(vertex);
					}
				}
			}
		}
	});
	if (mesh_array.size()) {//if there are some voxels in the chunk
		this->meshBP = new Obj3dBP(mesh_array);
		this->meshBP->freeData(BP_FREE_ALL);
		this->mesh = new Obj3d(*this->meshBP, *Chunk::renderer);
		this->mesh->local.setPos(this->pos);
	}
	return 1;
}


Chunk::Chunk() {}

Chunk::Chunk(const Math::Vector3& tile_number, Math::Vector3 chunk_size, PerlinSettings& perlinSettings) {
	this->tile = tile_number;
	this->pos = tile_number;
	this->pos.x *= chunk_size.x;
	this->pos.y *= chunk_size.y;
	this->pos.z *= chunk_size.z;
	this->size = chunk_size;
	this->meshBP = nullptr;
	this->mesh = nullptr;

	if (perlinSettings.map == nullptr) {//can this happen?
		perlinSettings.genHeightMap(this->pos.x, this->pos.z, this->size.z, this->size.x);
	}

	this->data = new uint8_t**[this->size.z];
	for (unsigned int k = 0; k < this->size.z; k++) {
		this->data[k] = new uint8_t * [this->size.y];
		for (unsigned int j = 0; j < this->size.y; j++) {
			this->data[k][j] = new uint8_t[this->size.x];
			for (unsigned int i = 0; i < this->size.x; i++) {
				//std::cout << "pos.y + j = " << (pos.y + j) << "  is > to " << int(perlinSettings.map[k][i]) << "\t?" << std::endl;
				if (pos.y + j > int(perlinSettings.map[k][i])) {
					this->data[k][j][i] = VOXEL_EMPTY.r;
				} else {
					if (0) {
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
						//std::cout << value << " ";
						//std::cout << (int)v << " ";
					} else { this->data[k][j][i] = 75; }
				}
			}
		}
	}

	//tmp
	Pixel*** pix = new Pixel**[this->size.z];
	for (unsigned int k = 0; k < this->size.z; k++) {
		pix[k] = new Pixel * [this->size.y];
		for (unsigned int j = 0; j < this->size.y; j++) {
			pix[k][j] = new Pixel[this->size.x];
			for (unsigned int i = 0; i < this->size.x; i++) {
				pix[k][j][i].r = this->data[k][j][i];
				pix[k][j][i].g = this->data[k][j][i];
				pix[k][j][i].b = this->data[k][j][i];
			}
		}
	}

	// important note: all chunks have their octree starting at pos 0 0 0. 
	this->root = new Octree(pix, Math::Vector3(0,0,0), this->size, 0);
	this->root->verifyNeighbors(VOXEL_EMPTY);//white
	this->buildVertexArrayFromOctree(this->root, Math::Vector3(0,0,0));

	//this->root = new Octree(this->data, this->pos, this->size, 0);//faire un class abstraite?

	//delete data
	for (unsigned int k = 0; k < this->size.z; k++) {
		for (unsigned int j = 0; j < this->size.y; j++) {
			delete[] this->data[k][j];
		}
		delete[] this->data[k];
	}
	delete[] this->data;
	//delete tmp pix
	for (unsigned int k = 0; k < this->size.z; k++) {
		for (unsigned int j = 0; j < this->size.y; j++) {
			delete[] pix[k][j];
		}
		delete[] pix[k];
	}
	delete[] pix;
	//delete root? not when we will edit it to destroy some voxels
	delete this->root;
}

Chunk::~Chunk() {
	//delete root? when we will edit it to destroy some voxels we will need to do it here
	//delete this->root;
	if (this->meshBP) { delete this->meshBP; }
	if (this->mesh) { delete this->mesh; }
}

void	Chunk::printData() {
	std::cout << "tile: "; this->tile.printData();
	std::cout << "pos: "; this->pos.printData();
	std::cout << "root: " << this->data << std::endl;
}
