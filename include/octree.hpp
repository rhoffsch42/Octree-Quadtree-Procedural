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
	*/
	Octree*		getRoot(Math::Vector3 target_pos, Math::Vector3 target_size);
	void		verifyNeighbors(Pixel filter, Math::Vector3 pos_offset);

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
	/*
		could be encoded in 6bits
		LEFT	32	XX100000
		RIGHT	16	XX010000
		UP		8	XX001000
		DOWN	4	XX000100
		FRONT	2	XX000010
		BACK	1	XX000001
	*/

	//unsigned int	x;
	//unsigned int	y;
	//unsigned int	z;
	//unsigned int	width;//X axis
	//unsigned int	height;//Y axis
	//unsigned int	depth;//Z axis
private:
};