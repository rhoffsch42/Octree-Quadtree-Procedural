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
	for (int i = 0; i < TESSELATION_LVLS; i++) {
		this->meshBP[i] = nullptr;
		this->mesh[i] = nullptr;
	}
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
	this->root = new Octree<Voxel>(voxels, Math::Vector3(0, 0, 0), this->size, OCTREE_THRESHOLD);
	this->root->verifyNeighbors(Voxel(255));

	//if (this->buildVertexArraysFromOctree(this->root, Math::Vector3(0, 0, 0)) == 0) {
	//	std::cout << "Error: failed to build vertex array for the chuck " << this << std::endl;
	//	Misc::breakExit(23);
	//}
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
	//delete this->root;
	//this->root = nullptr;
}

Chunk::~Chunk() {
	if (this->root) { delete this->root; }
	for (int i = 0; i < TESSELATION_LVLS; i++) {
		if (this->meshBP[i]) { delete this->meshBP[i]; }
		if (this->mesh[i]) { delete this->mesh[i]; }
	}
}


/*
	Before calling this function, empty vertices means empty chunk.
	If there are vertices, it uses them to build a mesh, then it deletes the vertices.

	tess lvls: it actually builds all tess lvl meshes
*/
void	Chunk::glth_buildMesh() {
	for (int i = 0; i < TESSELATION_LVLS; i++) {
		//std::cout << "Chunk::_vertexArray size: " << this->_vertexArray.size() << "\n";
		if (!this->_vertexArray[i].empty()) {//if there are some voxels in the chunk
			if (this->meshBP[i]) {
				std::cout << "overriding mesh bp: " << this->meshBP[i] << " on chunk: " << this << "\n";
				Misc::breakExit(31);
			}
			if (this->mesh[i]) {
				std::cout << "overriding mesh: " << this->mesh[i] << " on chunk: " << this << "\n";
				Misc::breakExit(31);
			}

			this->meshBP[i] = new Obj3dBP(this->_vertexArray[i], this->_indices[i], BP_DONT_NORMALIZE);
			//this->meshBP[i]->freeData(BP_FREE_ALL);
			this->mesh[i] = new Obj3d(*this->meshBP[i], *Chunk::renderer);
			this->mesh[i]->local.setPos(this->pos);

			/*
				Delete vertex array of the mesh so we don't build it again later.
				To rebuild the mesh (for whatever reason), call buildVertexArrayFromOctree() first.
					pb: for now the root octree is deleted (in Chunk constructor)
			*/
			this->_vertexArray[i].clear();
			this->_indices[i].clear();
		}
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
	root->browse([&pos_offset, ptr_vertex_array, &vertices](Octree_old* node) {
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

//builds all tesselation levels, we could make another func: void Octree::getVertexArray(std::vector<SimpleVertex>& dst, uint8_t tesselationLevel)
int	Chunk::buildVertexArraysFromOctree(Octree<Voxel>* root, Math::Vector3 pos_offset, const uint8_t desiredTessLevel, const double* threshold) {
	/*
		for each chunck, build a linear vertex array with concatened faces and corresponding attributes (texcoord, color, etc)
		it will use the chunk matrix so for the vertex position we simply add the chunk pos and the node pos to the vertex.position of the face

		this will be used with glDrawArray() later
		we could even concat all chunk array for a single glDrawArray
			cons: we need to remap data at every chunck change
				-> display list? check what it is
	*/
	static const std::vector<SimpleVertex>	vertices = Chunk::cubeBlueprint->getVertices();//lambda can access it
	this->clearMeshesData();
	if (!this->root) {
		std::cout << "WARNING: attempting to build vertex arrays from a missing octree, it might have been deleted before.\n";
		//throw
	}

	std::vector<SimpleVertex>*	ptr_vertex_array = this->_vertexArray;
	uint8_t neighbors_flags[] = { NEIGHBOR_FRONT, NEIGHBOR_RIGHT, NEIGHBOR_LEFT, NEIGHBOR_BOTTOM, NEIGHBOR_TOP, NEIGHBOR_BACK };

	//#define EVERY_TESSELATION_LEVELS
	#define	TESSELATION_WITH_THRESHOLD
	/*
		0 = no tesselation, taking smallest voxels (size = 1)
		5 = log2(32), max level, ie the size of a chunk
	*/

	#ifdef EVERY_TESSELATION_LEVELS
	root->browse(
		[&pos_offset, ptr_vertex_array, &neighbors_flags](Octree<Voxel>* node) {
			//inverse of std::pow(2, x)
			int tesselation_lvl = std::log2(node->size.x); // todo: should be node->depth ?

			if (node->element._value != 255 && node->neighbors < NEIGHBOR_ALL) {// should be < NEIGHBOR_ALL or (node->n & NEIGHBOR_ALL) != 0
				for (size_t i = 0; i < 6; i++) {//6 faces
					if ((node->neighbors & neighbors_flags[i]) != neighbors_flags[i]) {
						for (size_t j = 0; j < 6; j++) {// push the 2 triangles = 2 * 3 vertex
							SimpleVertex	vertex = (vertices)[i * 6 + j];
							vertex.position.x *= node->size.x;
							vertex.position.y *= node->size.y;
							vertex.position.z *= node->size.z;
							vertex.position += node->pos;
							ptr_vertex_array[tesselation_lvl].push_back(vertex);
							if (node->isLeaf()) {//meaning it has to be pushed to lower tesselation lvls too
								for (int t = tesselation_lvl - 1; t >= 0; t--)
									ptr_vertex_array[t].push_back(vertex);
							}
						}
					}
				}
			}
		}
	);
	#endif
	#ifdef TESSELATION_WITH_THRESHOLD
	root->browse_until(
		[desiredTessLevel, threshold](Octree<Voxel>* node) {
			int tesselation_lvl = std::log2(node->size.x); // todo: should be node->depth ?
			double t = threshold ? *threshold : 0;
			return ((node->detail <= t && node->element._value < 200) || tesselation_lvl == desiredTessLevel); //should be % of empty voxel > 50
		},
		[&pos_offset, ptr_vertex_array, &neighbors_flags, desiredTessLevel, threshold](Octree<Voxel>* node) {
			//inverse of std::pow(2, x)
			int tesselation_lvl = std::log2(node->size.x); // todo: should be node->depth ?
			double t = threshold ? *threshold : 0;

			// recheck with the same condition because we want only the node at the desired tess level, or threshold, or leaf
			if ((node->detail <= t && node->element._value < 200) || tesselation_lvl == desiredTessLevel || node->isLeaf()) {
				if (node->element._value != 255 && node->neighbors < NEIGHBOR_ALL) {// should be < NEIGHBOR_ALL or (node->n & NEIGHBOR_ALL) != 0
					for (size_t i = 0; i < 6; i++) {//6 faces
						if ((node->neighbors & neighbors_flags[i]) != neighbors_flags[i]) {
							for (size_t j = 0; j < 6; j++) {// push the 2 triangles = 2 * 3 vertex
								SimpleVertex	vertex = (vertices)[i * 6 + j];
								vertex.position.x *= node->size.x;
								vertex.position.y *= node->size.y;
								vertex.position.z *= node->size.z;
								vertex.position += node->pos;
								ptr_vertex_array[desiredTessLevel].push_back(vertex);
							}
						}
					}
				}
			}

		}
	);
#endif

	//std::cout << "\t> vertex array ready: " << this->_vertexArray.size() << std::endl;
	return 1;
}

void	Chunk::clearMeshesData() {
	for (auto i = 0; i < TESSELATION_LVLS; i++) {
		this->_vertexArray[i].clear();
		this->_indices[i].clear();
		delete this->meshBP[i];
		delete this->mesh[i];
		this->meshBP[i] = nullptr;
		this->mesh[i] = nullptr;
	}
}

void	Chunk::clearOctreeData() {
	delete this->root;
	this->root = nullptr;
}

//#define TINY_VERTEX
#ifdef TINY_VERTEX
typedef TinyVertex VertexClass;
#else
typedef SimpleVertex VertexClass;
#endif
#ifdef TINY_VERTEX
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
	root->browse([&full_vertex_vec, &vertex_set](Octree<Voxel>* node) {
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
#endif

std::string		Chunk::toString() const {
	std::stringstream ss;
	ss << "index: " << this->index << "\n";
	ss << "pos: " << this->pos << "\n";
	ss << "size: " << this->size << "\n";
	ss << "root: " << this->root << "\n";
	return ss.str();
}
