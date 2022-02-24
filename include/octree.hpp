#pragma once

#include <cstdlib>
#include <iostream>
#include <stdlib.h>

#include "math.hpp"
#include "quadtree.hpp"

#define OC_DEBUG_LEAF		0
#define OC_DEBUG_LEAF_AREA	1
#define OC_DEBUG_FILL_TOO	false
#define OC_DEBUG_BUILD_TOO	true

#define	CHILDREN	8

//neighbors
#define NEIGHBOR_ALL	63	//00111111
#define NEIGHBOR_FRONT	32	//00100000
#define NEIGHBOR_RIGHT	16	//00010000
#define NEIGHBOR_LEFT	8	//00001000
#define NEIGHBOR_BOTTOM	4	//00000100
#define NEIGHBOR_TOP	2	//00000010
#define NEIGHBOR_BACK	1	//00000001


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

class Octree
{
public:
	Octree(Pixel*** arr, Math::Vector3 corner_pos, Math::Vector3 tree_size, unsigned int threshold);
	~Octree();
	bool		isLeaf() const;

	/*
		- Size must correspond to a valid root depending on the pos, will return NULL if not.
		- if returned root is bigger (ie contains the requested root), it means it is a leaf,
			the user should check the size, and be aware of this
		? should the empty node be null instead of storing them in the octree?
		? should the average ignore empty nodes?
	*/
	Octree*		getRoot(Math::Vector3 target_pos, Math::Vector3 target_size);
	bool		contain(Pixel pix, Math::Vector3 pos, Math::Vector3 size);
	void		verifyNeighbors(Pixel filter);

	/*
		Octree::browse is declared in the hpp, because of linkage problems
		https://stackoverflow.com/questions/36880799/how-i-can-pass-lambda-expression-to-c-template-as-parameter
		https://stackoverflow.com/questions/3575901/can-lambda-functions-be-templated
	*/

	//faire un browse vers un root target?
	template<class UnaryPredicate>
	void	browse(int threshold, UnaryPredicate p) {
		if (this->detail <= threshold || this->isLeaf()) {
			if (this->size.x == 0 || this->size.y == 0 || this->size.z == 0) {
				std::cout << "error with tree data\n";
				exit(1);
			}
			p(this);// lamda-expr
			//return a bool to stop the browse? is it possible?
			/*
				func ptr works too, but args cant be transfered like in lambda-expr
				create a lambda expression using the func and args instead
			*/
		}
		else if (this->children) {
			for (size_t i = 0; i < CHILDREN; i++) {
				if (this->children[i])
					this->children[i]->browse(threshold, p);
			}
		}
	}

	Octree**		children;
	Pixel			pixel;
	double			detail;//average difference between the average pixel and all pixel
	Math::Vector3	pos;
	Math::Vector3	size;
	Math::Vector3	summit;//the opposite summit of the cube
	uint8_t			neighbors;

	//unsigned int	x;
	//unsigned int	y;
	//unsigned int	z;
	//unsigned int	width;//X axis
	//unsigned int	height;//Y axis
	//unsigned int	depth;//Z axis
private:
};