#pragma once

#include <cstdlib>
#include <iostream>
#include <stdlib.h>

#include "math.hpp"
#include "quadtree.hpp"

//#define OC_DEBUG_LEAF
//#define OC_DEBUG_NODE
#define OC_DEBUG_LEAF_AREA	1
#define OC_DEBUG_FILL_TOO	false
#define OC_DEBUG_BUILD_TOO	true

#define	CHILDREN	8

//neighbors
#define NEIGHBOR_ALL	0b00111111
#define NEIGHBOR_FRONT	0b00100000
#define NEIGHBOR_RIGHT	0b00010000
#define NEIGHBOR_LEFT	0b00001000
#define NEIGHBOR_BOTTOM	0b00000100
#define NEIGHBOR_TOP	0b00000010
#define NEIGHBOR_BACK	0b00000001

//#define USE_TEMPLATE // and accumulators with about 15% performence loss
#ifdef USE_TEMPLATE
class Voxel;

template <typename T>
class IAccumulator {
public:
	virtual T		getAverage() const = 0;
	virtual void	reset() = 0;
protected:
	double _count = 0;
};

template <typename T>
class IValueAccumulator : public IAccumulator<T> {
public:
	//IValueAccumulator() : IAccumulator() {}
	virtual void	add(const T&) = 0;
};

template <typename T>
class IDistanceAccumulator : public IAccumulator<T> {
public:
	//IDistanceAccumulator() : IAccumulator() {}
	virtual void	add(const T& vox1, const T& vox2) = 0;
	virtual double	getDetail() const = 0;
};

class VoxelValueAccumulator2 : public IValueAccumulator<Voxel> {
public:
	VoxelValueAccumulator2();
	virtual void	add(const Voxel& vox);
	virtual Voxel	getAverage() const;
	virtual void	reset();
private:
	double	_v1 = 0;
};

class VoxelDistanceAccumulator2 : public IDistanceAccumulator<Voxel> {
public:
	VoxelDistanceAccumulator2();
	virtual void	add(const Voxel& vox1, const Voxel& vox2);
	virtual Voxel	getAverage() const;
	virtual double	getDetail() const;
	virtual void	reset();
private:
	double	_v1 = 0;
};
#endif


/*
	https://www.youtube.com/watch?v=bGN445_2NSw
	chunk 32x32x32 :
		3 x 11bits per pos
		6 possible normals: 3bits

	buildVertexArrayFromOctree in GPU:
		1 voxel id containing:
			textureID,
				2 texCoo per 1 vertex computed on GPU
		neighbors_flag: determining if each faces are drawn
		voxel pos & size: allowing to compute each vertex for drawn faces
*/

class Voxel {
public:
#ifdef USE_TEMPLATE
	static VoxelValueAccumulator2* getValueAccumulator();
	static VoxelDistanceAccumulator2* getDistanceAccumulator();
#else
	static Voxel	getAverage(Voxel*** arr, const Math::Vector3& pos, const Math::Vector3& size);
	static double	measureDetail(Voxel*** arr, const Math::Vector3& pos, const Math::Vector3& size, const Voxel& average);
#endif

	Voxel();
	Voxel(uint8_t& v);
	Voxel(uint8_t&& v);
	Voxel& operator=(const Voxel& rhs);
	~Voxel();

	bool	operator==(const Voxel& rhs) const;
	bool	operator!=(const Voxel& rhs) const;

	//uint8_t		get() const { return this->_value; }
	uint8_t		_value = 0;
private:
};


/*
	less data:
		store material ID (uint8_t) instead of RGB, limited to 256 different mats?
		why Pixel? store voxel ID (uint8_t)
*/

/*
_________
| A | B |
|---|---|	lower layer
| C | D |
`````````
_________
| E | F |
|---|---|	higher layer
| G | H |
`````````
*/

// interface vs template https://stackoverflow.com/questions/16586489/c-interface-vs-template

template <typename T>
class Octree
{
public:
	Octree(T*** arr, Math::Vector3 corner_pos, Math::Vector3 tree_size, unsigned int threshold);
	~Octree();
	bool	isLeaf() const;

	/*
		- Size must correspond to a valid root depending on the pos, will return NULL if not.
		- if returned root is bigger (ie contains the requested root), it means it is a leaf,
			the user should check the size, and be aware of this
		? should the empty node be null instead of storing them in the octree?
		? should the average ignore empty nodes?
	*/
	Octree<T>*		getNode(const Math::Vector3& target_pos, const Math::Vector3& target_size);
	bool			contains(const T& elem, const Math::Vector3 pos, const Math::Vector3 size);//	need operator==()
	void			verifyNeighbors(const T& filter);
	//virtual T		getAverage(T*** arr, Math::Vector3 pos, Math::Vector3 size) = 0;
	//virtual double	measureDetail(T*** arr, Math::Vector3 pos, Math::Vector3 size, T average) = 0;

	template<typename T, typename U>
	void	browse_until(T condition, U func) {
		func(this);//return a bool to stop the browse? is it possible?
		if (condition(this)) {
			if (this->size.x == 0 || this->size.y == 0 || this->size.z == 0) {
				std::cout << this << ": error with tree data\n";
				std::exit(1);
			}
		}
		else if (this->children) {
			for (size_t i = 0; i < CHILDREN; i++) {
				if (this->children[i])
					this->children[i]->browse_until(condition, func);
			}
		}
	}

	//equivalent to browse_until node.isleaf() == true
	template<typename U>
	void	browse(U func) {
		func(this);
		if (this->children) {
			for (size_t i = 0; i < CHILDREN; i++) {
				if (this->children[i])
					this->children[i]->browse(func);
			}
		}
	}

	Octree**		children;
	T				element;
	double			detail;//average difference between the averaged (if not leaf) element and all elements contained in children that are leaves
	Math::Vector3	pos;
	Math::Vector3	size;
	Math::Vector3	summit;//the opposite summit of the cube
	uint8_t			neighbors;
private:
};
