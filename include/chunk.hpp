#pragma once

#include "math.hpp"
#include "heightmap.hpp"
#include "octree.hpp"
#include "obj3dBP.hpp"
#include "obj3d.hpp"
#include "texture.hpp"

#include <algorithm>

#define OCTREE_THRESHOLD 0
/*
	0 = no LOD, taking smallest voxels (size = 1)
	5 = log2(32), max level, ie the size of a chunk
*/
#define LODS_AMOUNT 1
#define LOD_MIN_VERTEX_ARRAY_SIZE	1000

#define CHUNK_TREE_AMOUNT 10

#define	PRINT_INDEX		0b10000000
#define	PRINT_POS		0b01000000
#define	PRINT_SIZE		0b00100000
#define	PRINT_ROOT		0b00010000
#define	PRINT_MESHBP	0b00001000
#define	PRINT_MESH		0b00000100
#define PRINT_VERTEX	0b00000010
#define PRINT_INDICES	0b00000001
#define	PRINT_ALL		0b11111111

class Chunk;
typedef std::shared_ptr<Chunk>	ChunkShPtr;

class Chunk //could inherit from Object
{
public:
	static int			count;
	static std::mutex	m;
	static Obj3dBP*	cubeBlueprint;
	static Obj3dPG*	renderer;

	// If a hmap is specified, its sizes must fit, undefined behavior if sizes are too small
	Chunk(const Math::Vector3& chunk_index, const Math::Vector3& chunk_size, PerlinSettings& perlinSettings, const HeightMap* map = nullptr);
	~Chunk();
	std::string toString(uint8_t flags = PRINT_ALL) const;

	Math::Vector3	index;
	Math::Vector3	pos;
	Math::Vector3	size;
	Octree<Voxel>*	root = nullptr;

	//opengl
	Obj3dBP*	meshBP = nullptr;
	Obj3d*		mesh = nullptr;

	void	glth_buildAllMeshes(); //with _vertexArray
	size_t	buildVertexArray(Math::Vector3 pos_offset = Math::Vector3(0, 0, 0), const uint8_t desiredLod = 0, const double threshold = 0);
	void	glth_clearMeshesData();
	void	clearOctreeData();
	void	addTrees(Voxel*** voxels, const HeightMap* hmap, int treeAmount) const;

	//bool operator<(const Chunk& rhs) const//tmp
	//{
	//	return std::tie(this->index.x, this->index.y, this->index.z) < std::tie(rhs.index.x, rhs.index.y, rhs.index.z);
	//}
private:
	// you should check for error: 0 means Chunk::cubebp is null, 1 means no error
	void	_build(PerlinSettings& perlinSettings, const HeightMap* hmap);
	int		_buildVertexArrayFromOctree_homogeneous(Octree<Voxel>* root, Math::Vector3 pos_offset = Math::Vector3(0, 0, 0));
	bool						_generatedLod[LODS_AMOUNT];//if the lod has been generated or not. if not: a job will be created when needed (care duplicate).
	std::vector<SimpleVertex>	_vertexArray[LODS_AMOUNT];
	std::vector<unsigned int>	_indices[LODS_AMOUNT];
};
