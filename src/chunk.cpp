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

	if (!Chunk::cubeBlueprint) {
		std::cout << "Obj3dBP*	Chunk::cubeBlueprint is null" << std::endl;
		return 0;
	}
	std::vector<SimpleVertex>	vertices = Chunk::cubeBlueprint->getVertices();
	std::vector<SimpleVertex>*	ptr_vertex_array = &this->_vertexArray;
	root->browse(0, [&pos_offset, ptr_vertex_array, &vertices](Octree* node) {
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
						ptr_vertex_array->push_back(vertex);
					}
				}
			}
		}
	});
	std::cout << "\t> vertex array ready: " << this->_vertexArray.size() << std::endl;
	return 1;
}


void	Chunk::buildMesh() {
	if (this->_vertexArray.size()) {//if there are some voxels in the chunk
		this->meshBP = new Obj3dBP(this->_vertexArray, BP_DONT_NORMALIZE);
		this->meshBP->freeData(BP_FREE_ALL);
		this->mesh = new Obj3d(*this->meshBP, *Chunk::renderer);
		this->mesh->local.setPos(this->pos);

		//delete vertex array of the mesh so we don't build it more than once
		this->_vertexArray.clear();
	}
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

	bool	delmap = false;
	if (perlinSettings.map == nullptr) {//when we didnt pregenerate the heightmap
		perlinSettings.genHeightMap(this->pos.x, this->pos.z, this->size.x, this->size.z);
		delmap = true;//to delete it after
	}

	uint8_t***		data;
	data = new uint8_t * *[this->size.z];
	for (unsigned int k = 0; k < this->size.z; k++) {
		data[k] = new uint8_t * [this->size.y];
		for (unsigned int j = 0; j < this->size.y; j++) {
			data[k][j] = new uint8_t[this->size.x];
			for (unsigned int i = 0; i < this->size.x; i++) {
				//std::cout << "pos.y + j = " << (pos.y + j) << "  is > to " << int(perlinSettings.map[k][i]) << "\t?" << std::endl;
				if (pos.y + j > int(perlinSettings.map[k][i])) {//procedural: surface with heightmap
					data[k][j][i] = VOXEL_EMPTY.r;
				} else {
					if (0) {//procedural: which dirt, 3d noise
						double value = perlinSettings.perlin.accumulatedOctaveNoise3D_0_1(
							double(pos.x + double(i)) / double(PERLIN_NORMALIZER) * perlinSettings.frequency,
							double(pos.y + double(j)) / double(PERLIN_NORMALIZER) * perlinSettings.frequency,
							double(pos.z + double(k)) / double(PERLIN_NORMALIZER) * perlinSettings.frequency,
							perlinSettings.octaves);
						uint8_t v = uint8_t(double(255.0) * value);
						//v = v / 128 * 128;
						if (v >= 128)
							v = 150;
						else
							v = 75;
						//std::cout << value << " ";
						//std::cout << (int)v << " ";
					} else {
						data[k][j][i] = 75;
					}
				}
			}
		}
	}

	if (delmap)
		perlinSettings.deleteMap();

	//tmp
	Pixel*** pix = new Pixel**[this->size.z];
	for (unsigned int k = 0; k < this->size.z; k++) {
		pix[k] = new Pixel * [this->size.y];
		for (unsigned int j = 0; j < this->size.y; j++) {
			pix[k][j] = new Pixel[this->size.x];
			for (unsigned int i = 0; i < this->size.x; i++) {
				pix[k][j][i].r = data[k][j][i];
				pix[k][j][i].g = data[k][j][i];
				pix[k][j][i].b = data[k][j][i];
			}
		}
	}

	// important note: all chunks have their octree starting at pos 0 0 0. 
	this->root = new Octree(pix, Math::Vector3(0,0,0), this->size, 0);//octree(T data,...) template?  || classe abstraite pour def average() etc
	this->root->verifyNeighbors(VOXEL_EMPTY);//white
	if (this->buildVertexArrayFromOctree(this->root, Math::Vector3(0, 0, 0)) == 0) {
		std::cout << "Error: failed to build vertex array for the chuck " << this << std::endl;
		std::exit(23);
	}

	//delete data
	for (unsigned int k = 0; k < this->size.z; k++) {
		for (unsigned int j = 0; j < this->size.y; j++) {
			delete[] data[k][j];
		}
		delete[] data[k];
	}
	delete[] data;
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
}
