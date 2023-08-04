#include "chunk.hpp"
#include "trees.h"
//#include <iostream>
#include <sstream>
#include <set>

#ifdef TREES_DEBUG
#define TREES_CHUNK_DEBUG
#endif
#ifdef TREES_CHUNK_DEBUG 
#define D(x) std::cout << "[Chunk] " << x
#define D_(x) x
#define D_SPACER "-- chunk.cpp -------------------------------------------------\n"
#define D_SPACER_END "----------------------------------------------------------------\n"
#else 
#define D(x)
#define D_(x)
#define D_SPACER ""
#define D_SPACER_END ""
#endif

int Chunk::count = 0;
std::mutex	Chunk::m;
static void	_countAdd(int n) {
	Chunk::m.lock();
	Chunk::count += n;
	Chunk::m.unlock();
}

Obj3dBP* Chunk::cubeBlueprint = nullptr;
Obj3dPG* Chunk::renderer = nullptr;

Chunk::Chunk(const Math::Vector3& chunk_index, const Math::Vector3& chunk_size, PerlinSettings& perlinSettings, HeightMap* hmap) {
	_countAdd(1);
	//D(__PRETTY_FUNCTION__ << "\n");
	this->index = chunk_index;
	this->pos = chunk_index;
	this->pos.x *= chunk_size.x;
	this->pos.y *= chunk_size.y;
	this->pos.z *= chunk_size.z;
	this->size = chunk_size;

	this->meshBP = nullptr;
	this->mesh = nullptr;
	for (int i = 0; i < LODS_AMOUNT; i++) {
		this->_generatedLod[i] = false;
	}

	uint8_t*** data;
	data = new uint8_t * *[this->size.z];
	for (unsigned int k = 0; k < this->size.z; k++) {
		data[k] = new uint8_t * [this->size.y];
		for (unsigned int j = 0; j < this->size.y; j++) {
			data[k][j] = new uint8_t[this->size.x];
			for (unsigned int i = 0; i < this->size.x; i++) {
				//std::cout << "pos.y + j = " << (pos.y + j) << "  is > to " << int(map[k][i]) << "\t?" << std::endl;
				if (!hmap || !hmap->map)
					std::cout << "dd>>> " << hmap << "\t" << hmap->map << "\n";
				if (hmap && pos.y + j > int(hmap->map[k][i])) {//procedural: surface with heightmap
					data[k][j][i] = VOXEL_EMPTY.r;
				}
				else {
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
					}
					else {
						data[k][j][i] = 75;
					}
				}
			}
		}
	}

#ifdef OCTREE_OLD
	Pixel*** pix = new Pixel * *[this->size.z];
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
}

Chunk::~Chunk() {
	_countAdd(-1);
	//D(__PRETTY_FUNCTION__ << "\n");
	std::thread::id thread_id = std::this_thread::get_id();
	if (Glfw::thread_id != thread_id) {
		//D("Error: Chunk " << this << " dtor called in the wrong thread. " << "Glfw::thread_id: " << Glfw::thread_id << ", != " << thread_id << "\n");
	}

	if (this->root) { delete this->root; }
	if (this->meshBP) {
		delete this->meshBP;
		if (Glfw::thread_id != thread_id) {
			D("Error: Chunk meshBP " << this << " dtor called in the wrong thread. " << "Glfw::thread_id: " << Glfw::thread_id << ", != " << thread_id << "\n");
		}
		//D_(else {
		//	D("Good: Chunk meshBP " << this << " dtor called in the right thread. " << "Glfw::thread_id: " << Glfw::thread_id << ", == " << thread_id << "\n");
		//});
	}

	if (this->mesh) {
		delete this->mesh;
		if (Glfw::thread_id != thread_id) {
			D("Error: Chunk mesh " << this << " dtor called in the wrong thread. " << "Glfw::thread_id: " << Glfw::thread_id << ", != " << thread_id << "\n");
		}
		//D_(else {
		//	D("Good: Chunk meshBP " << this << " dtor called in the right thread. " << "Glfw::thread_id: " << Glfw::thread_id << ", == " << thread_id << "\n");
		//});
	}

	//D("Chunk destroyed " << this->index.toString() << "\n");
}

/*
	Builds the chunk Obj3d as well as all the LODs.

	Before calling this function, empty vertices means empty chunk.
	If there are vertices, it uses them to build a mesh, then it deletes the vertices.
*/
void	Chunk::glth_buildAllMeshes() {
	if (!Chunk::renderer) {
		D("Error: Chunk::renderer not found: " << Chunk::renderer << "\n");
		Misc::breakExit(521);
	}
	//D("Chunk::glth_buildAllMeshes() for chunk " << this->index << "\n");
	for (int i = 0; i < LODS_AMOUNT; i++) {
		if (!this->_vertexArray[i].empty()) {
			//D("Building LOD_" << i << "\tvertexArray size: " << this->_vertexArray[i].size() << "\n");
			if (i == 0 || this->_vertexArray[i].size() > LOD_MIN_VERTEX_ARRAY_SIZE) {
				Obj3dBP* bp = new Obj3dBP(this->_vertexArray[i], this->_indices[i], BP_DONT_NORMALIZE);
				if (i == 0) {
					if (this->meshBP) {
						std::cout << "Error: overriding mesh bp: " << this->meshBP << " on chunk: " << this << "\n";
						Misc::breakExit(31);
					}
					if (this->mesh) {
						std::cout << "Error: overriding mesh: " << this->mesh << " on chunk: " << this << "\n";
						Misc::breakExit(31);
					}
					this->meshBP = bp;
					this->meshBP->lodManager.deepDestruction = true;
					this->mesh = new Obj3d(*this->meshBP, *Chunk::renderer);
					this->mesh->local.setPos(this->pos);
				}
				else {
					//this->meshBP->lodManager.addLod(bp, this->size.x * (i+1));// 32 * (i+1);
					this->meshBP->lodManager.addLod(bp, this->size.x * std::pow(2, i + 1));// 32 * { 1, 2, 4, 8, 16, 32 };
				}
				bp->freeData(BP_FREE_ALL);
				this->mesh->getProgram()->linkBuffers(*bp);//we should not need to use a Obj3d to access the PG. todo: see obj3dPG::renderObject()
			}
			//else { D("Skipped LODS " << i << "+ for chunk " << this->index << "\n"); }
			this->_vertexArray[i].clear();
			this->_indices[i].clear();
		}
	}
	//if (this->meshBP && this->meshBP->lodManager.getLodCount() > 1)
		//D(this->meshBP->lodManager.toString() << "\n");

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
	std::vector<SimpleVertex>* ptr_vertex_array = &this->_vertexArray;
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

//we could make another func: void Octree::buildVertexArray(std::vector<SimpleVertex>& dst, uint8_t lod)
size_t	Chunk::buildVertexArray(Math::Vector3 pos_offset, const uint8_t desiredLod, const double threshold) {
	/*
		for each chunck, build a linear vertex array with concatened faces and corresponding attributes (texcoord, color, etc)
		it will use the chunk matrix so for the vertex position we simply add the chunk pos and the node pos to the vertex.position of the face

		this will be used with glDrawArray() later
		we could even concat all chunk array for a single glDrawArray
			cons: we need to remap data at every chunck change
				-> display list? check what it is
	*/
	if (!Chunk::cubeBlueprint) {
		D("Warning: attempting to build vertex arrays, but Chunk::cubeBlueprint is missing: " << Chunk::cubeBlueprint << "\n");
		Misc::breakExit(234);
	}
	if (!this->root) {
		D("Warning: attempting to build vertex arrays from a missing octree, it might have been deleted before.\n");
		Misc::breakExit(234);
	}

	static const std::vector<SimpleVertex>	vertices = Chunk::cubeBlueprint->getVertices();//lambda can access it
	std::vector<SimpleVertex>*	ptr_vertex_array = this->_vertexArray;
	uint8_t						neighbors_flags[] = { NEIGHBOR_FRONT, NEIGHBOR_RIGHT, NEIGHBOR_LEFT, NEIGHBOR_BOTTOM, NEIGHBOR_TOP, NEIGHBOR_BACK };
	this->_vertexArray[desiredLod].clear();
	this->_indices[desiredLod].clear();

//#define EVERY_LODS
#define	LODS_WITH_THRESHOLD
/*
	0 = best lod, taking smallest voxels (size = 1)
	5 = log2(32), worst lod, ie the size of a chunk
*/

#ifdef EVERY_LODS
	D("Warning: building every LODS, it is old code\n");//todo : check if it's still valid
	this->root->browse(
		[&pos_offset, ptr_vertex_array, &neighbors_flags](Octree<Voxel>* node) {
			//inverse of std::pow(2, x)
			int lod = std::log2(node->size.x); // todo: should be node->depth ?

			if (node->element._value != 255 && node->neighbors < NEIGHBOR_ALL) {// should be < NEIGHBOR_ALL or (node->n & NEIGHBOR_ALL) != 0
				for (size_t i = 0; i < 6; i++) {//6 faces
					if ((node->neighbors & neighbors_flags[i]) != neighbors_flags[i]) {
						for (size_t j = 0; j < 6; j++) {// push the 2 triangles = 2 * 3 vertex
							SimpleVertex	vertex = (vertices)[i * 6 + j];
							vertex.position.x *= node->size.x;
							vertex.position.y *= node->size.y;
							vertex.position.z *= node->size.z;
							vertex.position += node->pos;
							ptr_vertex_array[lod].push_back(vertex);
							if (node->isLeaf()) {//meaning it has to be pushed to lower LODs too
								for (int t = lod - 1; t >= 0; t--)
									ptr_vertex_array[t].push_back(vertex);
							}
						}
					}
				}
			}
		});
#endif
#ifdef LODS_WITH_THRESHOLD
	this->root->browse_until(
		[desiredLod, threshold](Octree<Voxel>* node) {
			int lod = std::log2(node->size.x); // todo: should be node->depth ?
			return ((node->detail <= threshold && node->element._value < 200) || lod == desiredLod); //should be % of empty voxel > 50
		},
		[&pos_offset, ptr_vertex_array, &neighbors_flags, desiredLod, threshold](Octree<Voxel>* node) {
			//inverse of std::pow(2, x)
			int lod = std::log2(node->size.x); // todo: should be node->depth ?

			// recheck with the same condition because we want only the node at the desired LOD, or threshold, or leaf
			if ((node->detail <= threshold && node->element._value < 200) || lod == desiredLod || node->isLeaf()) {
				if (node->element._value != 255 && node->neighbors < NEIGHBOR_ALL) {// should be < NEIGHBOR_ALL or (node->n & NEIGHBOR_ALL) != 0
					for (size_t i = 0; i < 6; i++) {//6 faces
						if ((node->neighbors & neighbors_flags[i]) != neighbors_flags[i]) {
							for (size_t j = 0; j < 6; j++) {// push the 2 triangles = 2 * 3 vertex
								SimpleVertex	vertex = (vertices)[i * 6 + j];
								vertex.position.x *= node->size.x;
								vertex.position.y *= node->size.y;
								vertex.position.z *= node->size.z;
								vertex.position += node->pos;
								ptr_vertex_array[desiredLod].push_back(vertex);
							}
						}
					}
				}
			}

		});
#endif

	//std::cout << "\t> vertex array ready: " << this->_vertexArray.size() << std::endl;
	return this->_vertexArray->size();
}

void	Chunk::glth_clearMeshesData() {
	for (auto i = 0; i < LODS_AMOUNT; i++) {
		this->_vertexArray[i].clear();
		this->_indices[i].clear();
	}
	delete this->meshBP;
	delete this->mesh;
	this->meshBP = nullptr;
	this->mesh = nullptr;
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
	std::vector<VertexClass>* vertex_array = &final_vertex_vec;
#else
	std::vector<VertexClass>* vertex_array = &this->_vertexArray;
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

std::string		Chunk::toString(uint8_t flags) const {
	std::stringstream ss;

	if (FLAG_HAS(flags, PRINT_INDEX))
		ss << "Chunk world index" << this->index << " ";
	if (FLAG_HAS(flags, PRINT_POS))
		ss << "pos" << this->pos << " ";
	if (FLAG_HAS(flags, PRINT_SIZE))
		ss << "size" << this->size << " ";
	if (FLAG_HAS(flags, PRINT_ROOT))
		ss << "root: " << this->root << " ";
	if (FLAG_HAS(flags, PRINT_MESHBP))
		ss << "meshBP: " << this->meshBP << " LODs: " << this->meshBP->lodManager.getLodCount();
	if (FLAG_HAS(flags, PRINT_MESH))
		ss << "mesh: " << this->mesh << " ";
	if (FLAG_HAS(flags, PRINT_VERTEX))
		ss << "vertexArray size: " << this->_vertexArray->size() << " ";
	if (FLAG_HAS(flags, PRINT_INDICES))
		ss << "indices size: " << this->_indices->size() << " ";

	return ss.str();
}
