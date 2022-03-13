#pragma once

#include "math.hpp"
#include "heightmap.hpp"
#include "octree.hpp"
#include "obj3dBP.hpp"
#include "obj3d.hpp"
#include "texture.hpp"

#include <algorithm>

class Chunk //could inherit from Object
{
public:
	static Obj3dBP* cubeBlueprint;
	static Obj3dPG* renderer;

	Chunk(const Math::Vector3& chunk_index, const Math::Vector3& chunk_size, PerlinSettings& perlinSettings, HeightMap* map = nullptr);//if a hmap is specified, its sizes must fit, undefined behavior if sizes are too small
	~Chunk();
	std::string	toString() const;

	Math::Vector3	index;
	Math::Vector3	pos;
	Math::Vector3	size;
	Octree*			root;

	//opengl 
	Obj3dBP*		meshBP;
	Obj3d*			mesh;

	void		glth_buildMesh();//with _vertexArray
	//bool operator<(const Chunk& rhs) const//tmp
	//{
	//	return std::tie(this->index.x, this->index.y, this->index.z) < std::tie(rhs.index.x, rhs.index.y, rhs.pos.z);
	//}
private:
	// you should check for error: 0 means Chunk::cubebp is null, 1 means no error
	int		buildVertexArrayFromOctree(Octree* root, Math::Vector3 pos_offset = Math::Vector3(0, 0, 0));
	std::vector<SimpleVertex>	_vertexArray;
};
