#include "chunk.hpp"
#include "utils.hpp"
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

Chunk::Chunk(const Math::Vector3& chunk_index, const Math::Vector3& chunk_size, PerlinSettings& perlinSettings, const HeightMap* hmap) {
	_countAdd(1);
	//D(__PRETTY_FUNCTION__ << "\n");
	if (hmap  && !hmap->map) {
		D(" no hmap->map :" << hmap << "\t" << hmap->map << "\n");
		return;
	}
	if (int(chunk_size.x) <= 0 || int(chunk_size.y) <= 0 || int(chunk_size.z) <= 0) {
		D("Error: invalid chunk size: " << chunk_size << "\n");
		return;
	}
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
	this->_build(perlinSettings, hmap);
}

Chunk::~Chunk() {
	_countAdd(-1);
	//D(__PRETTY_FUNCTION__ << "\n");
	std::thread::id thread_id = std::this_thread::get_id();
	if (Glfw::thread_id != thread_id) {
		D("Error: Chunk " << this << " dtor called in the wrong thread. " << "Glfw::thread_id: " << Glfw::thread_id << ", != " << thread_id << "\n");
	}

	if (this->root) { delete this->root; }
	if (this->meshBP) { delete this->meshBP; }
	if (this->mesh) { delete this->mesh; }
}

void	Chunk::_build(PerlinSettings& perlinSettings, const HeightMap* hmap) {
	/*
		Important: Do not try to convert Octree Voxel data to a 1d array, it will be a mess.
		Octrees NEED a 3d array of Voxels, because they will extract cubes from it. Managing Voxels with a 1d array adds complexity.
		Two possible solutions to keep managing a 1d array in Octree:: :
			- send the 3d size of the first root, it would add another argument to the Octree constructor and build functions (getAverage, measureDetail).
			- rebuild a 1d array for each new Octree containing the right data. (Bad because the goal of a 1d array is to avoid this kind of operation)

		So what we're doing here is building a 1d array, then rebuilding a 3d array from it, avoiding multiple allocations for the X dimensions.
		For the last dimensions, instead of `size.y` allocations of `size.x` voxels, we have 1 single allocation of `len` voxels.
	*/
	size_t	highestPoint = this->pos.y + (hmap ? hmap->getMaxHeight() : this->size.y);
	size_t	len = this->size.x * this->size.y * this->size.z;
	auto	array1d = std::make_unique<Voxel[]>(len);
	auto	emptyRow = std::make_unique<Voxel[]>(this->size.x);
	auto	voxels = std::make_unique<Voxel**[]>(this->size.z);
	//std::unique_ptr<Voxel[]>	bedwallRow = std::make_unique<Voxel[]>(this->size.x);

	for (size_t i = 0; i < this->size.x; i++) {
		emptyRow[i] = VOXEL_EMPTY;
		//bedwallRow[i] = Voxel(75);
	}

	for (size_t k = 0; k < this->size.z; k++) {
		voxels[k] = new Voxel * [this->size.y]; // don't forget to delete them
		for (size_t j = 0; j < this->size.y; j++) {

			if (hmap && size_t(this->pos.y + j) > highestPoint) { // it avoids building identical voxels rows above the highestPoint
				voxels[k][j] = emptyRow.get();
			}
			/* no bedwall for now
			else if (hmap && this->pos.y + j <= BEDWALL) {
				voxels[k][j] = bedwallRow.get();
			}
			*/
			else {
				voxels[k][j] = &(array1d[size_t(k * this->size.y * this->size.x + j * this->size.x)]);
				for (size_t i = 0; i < this->size.x; i++) {
					size_t index = IND_3D_TO_1D(i, j, k, this->size.x, this->size.y);

					if (hmap && this->pos.y + j > int(hmap->map[k][i])) {
						array1d[index] = VOXEL_EMPTY;
					} else if (0) {//procedural: which dirt, 3d noise
						double value = perlinSettings.perlin.accumulatedOctaveNoise3D_0_1(
							double(this->pos.x + double(i)) / double(PERLIN_NORMALIZER) * perlinSettings.frequency,
							double(this->pos.y + double(j)) / double(PERLIN_NORMALIZER) * perlinSettings.frequency,
							double(this->pos.z + double(k)) / double(PERLIN_NORMALIZER) * perlinSettings.frequency,
							perlinSettings.octaves);
						uint8_t v = uint8_t(double(255.0) * value);
						//v = v / 128 * 128;
						v = (v >= 128) ? 150 : 75;
						//std::cout << value << " ";
						//std::cout << (int)v << " ";
					} else {
						array1d[index] = 75;
					}
				}
			}

		}
	}

	// important note: all chunks have their octree starting at pos 0 0 0.
	this->root = new Octree<Voxel>(&(voxels[0]), Math::Vector3(0, 0, 0), this->size, OCTREE_THRESHOLD);
	this->root->verifyNeighbors(VOXEL_EMPTY);

	for (size_t k = 0; k < this->size.z; k++) {
		delete[] voxels[k];
	}
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
		return;
	}
	//D("Chunk::glth_buildAllMeshes() for chunk " << this->index << "\n");
	for (int i = 0; i < LODS_AMOUNT; i++) {
		if (!this->_vertexArray[i].empty()) {
			//D("Building LOD_" << i << "\tvertexArray size: " << this->_vertexArray[i].size() << "\n");
			if (i == 0 || this->_vertexArray[i].size() > LOD_MIN_VERTEX_ARRAY_SIZE) {
				Obj3dBP* bp = new Obj3dBP(this->_vertexArray[i], this->_indices[i], BP_DONT_NORMALIZE);
				bp->freeData(BP_FREE_ALL);
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
					//this->meshBP->lodManager.addLod(bp, this->size.x * (i+1)); // 32 * (i+1);
					this->meshBP->lodManager.addLod(bp, this->size.x * std::pow(2, i + 1)); // 32 * { 1, 2, 4, 8, 16, 32 };
				}
				this->mesh->getProgram()->linkBuffers(*bp); // we should not need to use a Obj3d to access the PG. todo: see obj3dPG::renderObject()
			}
			//else { D("Skipped LODS " << i << "+ for chunk " << this->index << "\n"); }
			this->_vertexArray[i].clear();
			this->_indices[i].clear();
		}
	}
	//if (this->meshBP && this->meshBP->lodManager.getLodCount() > 1)
		//D(this->meshBP->lodManager.toString() << "\n");

}

//we could make another func: void Octree::buildVertexArray(std::vector<SimpleVertex>& dst, uint8_t lod)
size_t	Chunk::buildVertexArray(Math::Vector3 pos_offset, const uint8_t desiredLod, const double threshold) {
	//D("Chunk::buildVertexArray() for chunk " << this->index << " desiredLod: " << int(desiredLod) << " threshold: " << threshold << "\n");

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

	static const std::vector<SimpleVertex>	vertices = Chunk::cubeBlueprint->getVertices(); // lambda can access it
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
			return ((node->detail <= threshold && node->element._value < 200) || lod == desiredLod); // should be % of empty voxel > 50
		},
		[&pos_offset, ptr_vertex_array, &neighbors_flags, desiredLod, threshold](Octree<Voxel>* node) {
			// inverse of std::pow(2, x)
			int lod = std::log2(node->size.x); // todo: should be node->depth ?

			// recheck with the same condition because we want only the node at the desired LOD, or threshold, or leaf
			if ((node->detail <= threshold && node->element._value < 200) || lod == desiredLod || node->isLeaf()) {
				if (node->element._value != 255 && node->neighbors < NEIGHBOR_ALL) { // should be < NEIGHBOR_ALL or (node->n & NEIGHBOR_ALL) != 0
					for (size_t i = 0; i < 6; i++) { // 6 faces
						if ((node->neighbors & neighbors_flags[i]) != neighbors_flags[i]) {
							for (size_t j = 0; j < 6; j++) { // push the 2 triangles = 2 * 3 vertex
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
int	Chunk::_buildVertexArrayFromOctree_homogeneous(Octree<Voxel>* root, Math::Vector3 pos_offset) {
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
	size_t i = 0;
	size_t size = vertex_set.size();
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
