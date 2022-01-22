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

	Chunk(const Math::Vector3& tile_number, Math::Vector3 chunk_size, PerlinSettings& perlinSettings, HeightMap* map = nullptr);//if a hmap is specified, its sizes must fit, undefined behavior if sizes are too small
	~Chunk();
	std::string	toString() const;

	Math::Vector3	tile;
	Math::Vector3	pos;
	Math::Vector3	size;
	Octree*			root;

	//opengl 
	Obj3dBP*		meshBP;
	Obj3d*			mesh;

	void		buildMesh();//with _vertexArray
private:
	// you should check for error: 0 means Chunk::cubebp is null, 1 means no error
	int		buildVertexArrayFromOctree(Octree* root, Math::Vector3 pos_offset = Math::Vector3(0, 0, 0));
	std::vector<SimpleVertex>	_vertexArray;
};
