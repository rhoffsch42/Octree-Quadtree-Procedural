#include "chunk.hpp"
#include <iostream>
#include <set>

Obj3dBP* Chunk::cubeBlueprint = nullptr;
Obj3dPG* Chunk::renderer = nullptr;

Chunk::Chunk(const Math::Vector3& chunk_index, const Math::Vector3& chunk_size, PerlinSettings& perlinSettings, HeightMap* hmap) {
	this->index = chunk_index;
	this->pos = chunk_index;
	this->pos.x *= chunk_size.x;
	this->pos.y *= chunk_size.y;
	this->pos.z *= chunk_size.z;
	this->size = chunk_size;
	this->meshBP = nullptr;
	this->mesh = nullptr;

	uint8_t***		data;
	data = new uint8_t * *[this->size.z];
	for (unsigned int k = 0; k < this->size.z; k++) {
		data[k] = new uint8_t * [this->size.y];
		for (unsigned int j = 0; j < this->size.y; j++) {
			data[k][j] = new uint8_t[this->size.x];
			for (unsigned int i = 0; i < this->size.x; i++) {
				//std::cout << "pos.y + j = " << (pos.y + j) << "  is > to " << int(map[k][i]) << "\t?" << std::endl;
				if (hmap && pos.y + j > int(hmap->map[k][i])) {//procedural: surface with heightmap
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

#ifdef OCTREE_OLD
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
	int threshold = 0;
	this->root = new Octree_old(pix, Math::Vector3(0, 0, 0), this->size, threshold);//octree(T data,...) template?  || classe abstraite pour def average() etc
	this->root->verifyNeighbors(VOXEL_EMPTY);//white
	if (this->buildVertexArrayFromOctree(this->root, Math::Vector3(0, 0, 0)) == 0) {
		std::cout << "Error: failed to build vertex array for the chuck " << this << std::endl;
		Misc::breakExit(23);
	}
	//delete tmp pix
	for (unsigned int k = 0; k < this->size.z; k++) {
		for (unsigned int j = 0; j < this->size.y; j++) {
			delete[] pix[k][j];
		}
		delete[] pix[k];
	}
	delete[] pix;
#else
	Voxel*** voxels = new Voxel * *[this->size.z];
	for (unsigned int k = 0; k < this->size.z; k++) {
		voxels[k] = new Voxel * [this->size.y];
		for (unsigned int j = 0; j < this->size.y; j++) {
			voxels[k][j] = new Voxel[this->size.x];
			for (unsigned int i = 0; i < this->size.x; i++) {
				voxels[k][j][i]._value = data[k][j][i];
			}
		}
	}
	// important note: all chunks have their octree starting at pos 0 0 0.
	int threshold = 0;
	this->root = new Octree<Voxel>(voxels, Math::Vector3(0, 0, 0), this->size, threshold);
	this->root->verifyNeighbors(Voxel(255));
	if (this->buildVertexArrayFromOctree(this->root, Math::Vector3(0, 0, 0)) == 0) {
		std::cout << "Error: failed to build vertex array for the chuck " << this << std::endl;
		Misc::breakExit(23);
	}
	//delete tmp pix
	for (unsigned int k = 0; k < this->size.z; k++) {
		for (unsigned int j = 0; j < this->size.y; j++) {
			delete[] voxels[k][j];
		}
		delete[] voxels[k];
	}
	delete[] voxels;
#endif OCTREE_OLD

	//delete data
	for (unsigned int k = 0; k < this->size.z; k++) {
		for (unsigned int j = 0; j < this->size.y; j++) {
			delete[] data[k][j];
		}
		delete[] data[k];
	}
	delete[] data;

	//delete root? not when we will edit it to destroy some voxels
	delete this->root;
	this->root = nullptr;
}

Chunk::~Chunk() {
	if (this->root) { delete this->root; }
	if (this->meshBP) { delete this->meshBP; }
	if (this->mesh) { delete this->mesh; }
}


/*
	Before calling this function, empty vertices means empty chunk.
	If there are vertices, it uses them to build a mesh, then it deletes the vertices.
*/
void	Chunk::glth_buildMesh() {
	//std::cout << "Chunk::_vertexArray size: " << this->_vertexArray.size() << "\n";
	if (!this->_vertexArray.empty()) {//if there are some voxels in the chunk
		if (this->meshBP) {
			std::cout << "overriding mesh bp: " << this->meshBP << " on chunk: " << this << "\n";
			Misc::breakExit(31);
		}
		if (this->mesh) {
			std::cout << "overriding mesh: " << this->mesh << " on chunk: " << this << "\n";
			Misc::breakExit(31);
		}

		this->meshBP = new Obj3dBP(this->_vertexArray, this->_indices, BP_DONT_NORMALIZE);
		//this->meshBP->freeData(BP_FREE_ALL);
		this->mesh = new Obj3d(*this->meshBP, *Chunk::renderer);
		this->mesh->local.setPos(this->pos);

		/*
			Delete vertex array of the mesh so we don't build it again later.
			To rebuild the mesh (for whatever reason), call buildVertexArrayFromOctree() first.
				pb: for now the root octree is deleted (in Chunk constructor)
		*/
		this->_vertexArray.clear();
		this->_indices.clear();
	}
}

#ifdef OCTREE_OLD
int	Chunk::buildVertexArrayFromOctree(Octree_old* root, Math::Vector3 pos_offset) {
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
	root->browse(0, [&pos_offset, ptr_vertex_array, &vertices](Octree_old* node) {
		if ((node->pixel.r < VOXEL_EMPTY.r \
			|| node->pixel.g < VOXEL_EMPTY.g \
			|| node->pixel.b < VOXEL_EMPTY.b) \
			&& node->neighbors < NEIGHBOR_ALL)// should be < NEIGHBOR_ALL or (node->n & NEIGHBOR_ALL) != 0
		{
			Math::Vector3	cube_pos(pos_offset);//can be the root pos or (0,0,0)
			cube_pos += node->pos;//the position of the cube
			int neighbors_flags[] = { NEIGHBOR_FRONT, NEIGHBOR_RIGHT, NEIGHBOR_LEFT, NEIGHBOR_BOTTOM, NEIGHBOR_TOP, NEIGHBOR_BACK };
			for (size_t i = 0; i < 6; i++) {//6 faces
				if ((node->neighbors & neighbors_flags[i]) != neighbors_flags[i]) {
					for (size_t j = 0; j < 6; j++) {// push the 2 triangles = 2 * 3 vertex
						SimpleVertex	vertex = vertices[i * 6 + j];
						vertex.position.x *= node->size.x;
						vertex.position.y *= node->size.y;
						vertex.position.z *= node->size.z;
						vertex.position += node->pos;
						ptr_vertex_array->push_back(vertex);
					}
				}
			}
		}
	});
	//std::cout << "\t> vertex array ready: " << this->_vertexArray.size() << std::endl;
	return 1;
}
#endif

int	Chunk::buildVertexArrayFromOctree(Octree<Voxel>* root, Math::Vector3 pos_offset) {
	/*
		for each chunck, build a linear vertex array with concatened faces and corresponding attributes (texcoord, color, etc)
		it will use the chunk matrix so for the vertex position we simply add the chunk pos and the node pos to the vertex.position of the face

		this will be used with glDrawArray() later
		we could even concat all chunk array for a single glDrawArray
			cons: we need to remap data at every chunck change
				-> display list? check what it is
	*/
	static std::vector<SimpleVertex>	vertices = Chunk::cubeBlueprint->getVertices();//lambda can access it

	std::vector<SimpleVertex>*	ptr_vertex_array = &this->_vertexArray;
	uint8_t neighbors_flags[] = { NEIGHBOR_FRONT, NEIGHBOR_RIGHT, NEIGHBOR_LEFT, NEIGHBOR_BOTTOM, NEIGHBOR_TOP, NEIGHBOR_BACK };
	root->browse(0, [&pos_offset, ptr_vertex_array, &neighbors_flags](Octree<Voxel>* node) {
		if (node->element != Voxel(255) && node->neighbors < NEIGHBOR_ALL) {// should be < NEIGHBOR_ALL or (node->n & NEIGHBOR_ALL) != 0
			for (size_t i = 0; i < 6; i++) {//6 faces
				if ((node->neighbors & neighbors_flags[i]) != neighbors_flags[i]) {
					for (size_t j = 0; j < 6; j++) {// push the 2 triangles = 2 * 3 vertex
						SimpleVertex	vertex = (vertices)[i * 6 + j];
						vertex.position.x *= node->size.x;
						vertex.position.y *= node->size.y;
						vertex.position.z *= node->size.z;
						vertex.position += node->pos;
						ptr_vertex_array->push_back(vertex);
					}
				}
			}
		}
	});
	//std::cout << "\t> vertex array ready: " << this->_vertexArray.size() << std::endl;
	return 1;
}

//#define TINY_VERTEX
#ifdef TINY_VERTEX
typedef TinyVertex VertexClass;
#else
typedef SimpleVertex VertexClass;
#endif
/*
	build the obj3d with indices and no duplicate vertices.
	as each vertex is common to 3 faces, each faces will have the same color for the said vertex
	same thing for normal, UV, ...

	This can work only with identical voxels, with identical texture on each faces.
	Lightning may be a problem 
*/
int	Chunk::buildVertexArrayFromOctree_homogeneous(Octree<Voxel>* root, Math::Vector3 pos_offset) {
	/*
		for each chunck, build a vertex array and a indices array with concatened faces and corresponding attributes (texcoord, color, etc)
		it will use the chunk matrix so for the vertex position we simply add the chunk pos and the node pos to the vertex.position of the face

		this will be used with glDrawElements() later
		we could even concat all chunk array for a single glDrawElements
			cons: we need to remap data at every chunck change
				-> display list? check what it is
	*/

	static std::vector<SimpleVertex>	cube_vertices = Chunk::cubeBlueprint->getVertices();//lambda can access it
	static uint8_t neighbors_flags[] = { NEIGHBOR_FRONT, NEIGHBOR_RIGHT, NEIGHBOR_LEFT, NEIGHBOR_BOTTOM, NEIGHBOR_TOP, NEIGHBOR_BACK };
	static Math::Vector3	face_color[] = {
		Math::Vector3(150,0,0),
		Math::Vector3(150,150,0),
		Math::Vector3(150,0,150),
		Math::Vector3(0,0,0),
		Math::Vector3(150,150,150),
		Math::Vector3(0,150,150),
	};

	//build the unique vertex array
	// fastest way : https://stackoverflow.com/questions/1041620/whats-the-most-efficient-way-to-erase-duplicates-and-sort-a-vector
	std::set<VertexClass>		vertex_set;
	std::vector<VertexClass>			full_vertex_vec;
	std::map<VertexClass, unsigned int>	vertex_map;
	std::vector<VertexClass>			final_vertex_vec;

	/*
		build a vec with all vertices
		build an set to have the list of vertices without duplicates
	*/
	root->browse(0, [&full_vertex_vec, &vertex_set](Octree<Voxel>* node) {
		if (node->element != Voxel(255) && node->neighbors < NEIGHBOR_ALL) {// should be < NEIGHBOR_ALL or (node->n & NEIGHBOR_ALL) != 0
			for (size_t i = 0; i < 6; i++) {//6 faces
				if ((node->neighbors & neighbors_flags[i]) != neighbors_flags[i]) {
					for (size_t j = 0; j < 6; j++) {// push the 2 triangles = 2 * 3 vertex
						SimpleVertex	vertex = cube_vertices[i * 6 + j];
						vertex.position.x *= node->size.x;
						vertex.position.y *= node->size.y;
						vertex.position.z *= node->size.z;
						vertex.position += node->pos;
						vertex.color = face_color[i];

						#ifdef TINY_VERTEX
						VertexClass	tiny_vertex{ vertex.position.x,vertex.position.y, vertex.position.z };
						full_vertex_vec.push_back(tiny_vertex);
						vertex_set.insert(tiny_vertex);
						#else
						full_vertex_vec.push_back(vertex);
						vertex_set.insert(vertex);
						#endif
					}
				}
			}
		}
	});
	unsigned int i = 0;
	unsigned int size = vertex_set.size();
	#ifdef TINY_VERTEX
	std::vector<VertexClass>*	vertex_array = &final_vertex_vec;
	#else
	std::vector<VertexClass>*	vertex_array = &this->_vertexArray;
	#endif
	//convert the set as a map<vertex,indice>, also convert the set as a vector
	for (const auto& v : vertex_set) {
		vertex_map[v] = i;
		vertex_array->push_back(v);
		i++;
	}
	//checks
	i = 0;
	while (i < size) {
		if (vertex_map[(*vertex_array)[i]] != i) {
			std::cout << "data error, map and vector indices should be equal\n";
			Misc::breakExit(55);
		}
		i++;
	}
	//checks
	size = full_vertex_vec.size();
	if (size % 6 != 0) {
		std::cout << "data error, full vertex array is not a multiple of 6 (2 triangles)\n";
		Misc::breakExit(55);
	}
	//if (size > 8)
		//std::cout << vertex_array->size() << " _ _ full: " << size << "\n";

	//build indices
	this->_indices.reserve(size);//avoid multiple realloc, we know the exact size of the final vector
	for (const auto& v : full_vertex_vec) {
		this->_indices.push_back(vertex_map[v]);
	}

	// if TINY_VERTEX save the array in the chunk
	return 1;
}

std::string		Chunk::toString() const {
	std::stringstream ss;
	ss << "index: " << this->index << "\n";
	ss << "pos: " << this->pos << "\n";
	ss << "size: " << this->size << "\n";
	ss << "root: " << this->root << "\n";
	return ss.str();
}
