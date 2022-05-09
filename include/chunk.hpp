#pragma once

#include "math.hpp"
#include "heightmap.hpp"
#include "octree.hpp"
#include "obj3dBP.hpp"
#include "obj3d.hpp"
#include "texture.hpp"

#include <algorithm>

//#define OCTREE_OLD
#define OCTREE_THRESHOLD 0
#define TESSELATION_LVLS 6

class Chunk //could inherit from Object
{
public:
	static Obj3dBP *cubeBlueprint;
	static Obj3dPG *renderer;

	Chunk(const Math::Vector3 &chunk_index, const Math::Vector3 &chunk_size, PerlinSettings &perlinSettings, HeightMap *map = nullptr); //if a hmap is specified, its sizes must fit, undefined behavior if sizes are too small
	~Chunk();
	std::string toString() const;

	Math::Vector3 index;
	Math::Vector3 pos;
	Math::Vector3 size;
#ifdef OCTREE_OLD
	Octree_old *root;
#else
	Octree<Voxel> *root;
#endif

	//opengl
	Obj3dBP *meshBP[TESSELATION_LVLS];//1,2,4,8,16,32 : sizes of smallest voxel 
	Obj3d	*mesh[TESSELATION_LVLS];//same

	void	glth_buildMesh(); //with _vertexArray
	int		buildVertexArraysFromOctree(Octree<Voxel>* root, Math::Vector3 pos_offset = Math::Vector3(0, 0, 0), const uint8_t tessLevel = 0, const double* threshold = nullptr);
	void	clearMeshesData();
	void	clearOctreeData();
	//bool operator<(const Chunk& rhs) const//tmp
	//{
	//	return std::tie(this->index.x, this->index.y, this->index.z) < std::tie(rhs.index.x, rhs.index.y, rhs.index.z);
	//}
private:
// you should check for error: 0 means Chunk::cubebp is null, 1 means no error
#ifdef OCTREE_OLD
	int buildVertexArrayFromOctree(Octree_old *root, Math::Vector3 pos_offset = Math::Vector3(0, 0, 0));
#else
	int buildVertexArrayFromOctree_homogeneous(Octree<Voxel> *root, Math::Vector3 pos_offset = Math::Vector3(0, 0, 0));
#endif
	std::vector<SimpleVertex> _vertexArray[TESSELATION_LVLS];
	std::vector<unsigned int> _indices[TESSELATION_LVLS];//for TINY_VERTEX
};
