#include "octree.hpp"
#include "compiler_settings.h"
#include "utils.hpp"

#ifdef TREES_DEBUG
#define TREES_OCTREE_DEBUG
#endif
#define TREES_OCTREE_DEBUG
#ifdef TREES_OCTREE_DEBUG 
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

template class Octree<Voxel>;

Voxel	Voxel::getAverage(Voxel*** arr, const Math::Vector3& pos, const Math::Vector3& size) {
	//std::cout << "__getAverage.. " << width << "x" << height << " at " << x << ":" << y << "\n";
	//Add up values
	double sum = 0;
	for (int z = pos.z; z < pos.z + size.z; z++) {
		for (int y = pos.y; y < pos.y + size.y; y++) {
			for (int x = pos.x; x < pos.x + size.x; x++) {
				sum += arr[z][y][x]._value;
			}
		}
	}
	return Voxel(sum / (size.x * size.y * size.z));//care overflow
}

double	Voxel::measureDetail(Voxel*** arr, const Math::Vector3& pos, const Math::Vector3& size, const Voxel& average) {
	// Calculates the distance between every voxel in the region
	// and the average color. The Manhattan distance is used, and
	// all the distances are added.
	double sum = 0;
	for (int k = pos.z; k < pos.z + size.z; k++) {
		for (int j = pos.y; j < pos.y + size.y; j++) {
			for (int i = pos.x; i < pos.x + size.x; i++) {
				sum += std::abs(int(average._value) - int(arr[k][j][i]._value));
			}
		}
	}
	// Calculates the average distance, and returns the result.
	// We mult by 1, because we are averaging over 1 value.
	return (sum / double(1 * size.x * size.y * size.z));
}

Voxel::Voxel() { this->_value = 0; }
Voxel::Voxel(uint8_t& v) : _value(v) {}
Voxel::Voxel(uint8_t&& v) : _value(v) {}
bool	Voxel::operator==(const Voxel& rhs) const { return this->_value == rhs._value; }
bool	Voxel::operator!=(const Voxel& rhs) const { return this->_value != rhs._value; }
Voxel&	Voxel::operator=(const Voxel& rhs) { this->_value = rhs._value; return *this; }
Voxel::~Voxel() {}

template <typename T>
Octree<T>::Octree(T*** arr, Math::Vector3 corner_pos, Math::Vector3 tree_size, unsigned int threshold) {
	//D("New octree : corner_pos " << corner_pos << " tree_size " << tree_size << " threshold " << threshold << LF);
	//std::cout << "_ " << __PRETTY_FUNCTION__ << "\n";

	this->children = nullptr;
	this->detail = 0;
	this->element = T();
	this->pos = corner_pos;
	this->size = tree_size;
	this->summit = corner_pos + tree_size - Math::Vector3(1, 1, 1);
	this->neighbors = 0;//all sides are not empty by default (could count corners too, so 26)
	if (this->size.x == 1 && this->size.y == 1 && this->size.z == 1) {
		this->element._value = arr[(int)this->pos.z][(int)this->pos.y][(int)this->pos.x]._value;//use Vector3i to avoid casting
		#ifdef OC_DEBUG_LEAF
		if (size.x * size.y * size.z >= OC_DEBUG_LEAF_AREA) {
			std::cout << "new leaf: " << size.x << "x" << size.y << "x" << size.z << " at ";
			std::cout << pos.x << ":" << pos.y << ":" << pos.z << "\t";
			std::cout << (int)this->pixel.r << "  \t" << (int)this->pixel.g << "  \t" << (int)this->pixel.b;
			std::cout << "\t" << this->detail << "\n";
		}
		#endif
		return;
	}

	this->element = T::getAverage(arr, pos, size);
	this->detail = T::measureDetail(arr, pos, size, this->element);

	#ifdef OC_DEBUG_NODE
	std::cout << "pos: " << pos << "\t";
	std::cout << "size: " << size << "\t";
	std::cout << "average: " << int(this->element._value) << "\t";
	std::cout << "detail: " << this->detail << "\t";
	std::cout << "\n---------------\n";
	#endif
	if (this->detail <= double(threshold)) { // not that much details in the area, this is the end leaf
		#ifdef OC_DEBUG_LEAF
		if (size.x * size.y * size.z >= OC_DEBUG_LEAF_AREA) {//simple debug
			std::cout << "new leaf: ";
			std::cout << "pos: " << pos << "\t";
			std::cout << "size: " << size << "\t";
			std::cout << "average: " << int(this->element._value) << "\t";
			std::cout << "detail: " << this->detail << "\t";
			std::cout << "\n---------------\n";
		}
		#endif
		return;
	}

	//enough detail to split
	this->children = new Octree<T> * [CHILDREN];
	for (size_t i = 0; i < 8; i++) {
		this->children[i] = nullptr;
	}

	/*

	o________x
	| A | B |
	|---|---|		lower layer seen from top
	| C | D |
	z¨¨¨¨¨¨¨¨

	o________x
	| E | F |
	|---|---|		higher layer seen from top
	| G | H |
	z¨¨¨¨¨¨¨¨
	   ________
	  /___/___/|
	y/___/___/ |
	 | G | H | |
	 |---|---| /		both layers seen from front
	 | C | D |/
	 o¨¨¨¨¨¨¨¨x

	*/
	/*
		FIX: choose if we force the size to be even, and all dimensions to be the same.
		All of the following could be avoided if the size is forced to be even(even better : power of 2), and all dimensions are the same. ie a node is always a cube.
		At least, it currently handles odd and different sizes, but it is more complex.
	*/
	/*
		Sizes of subnodes.When current node size is odd, sides closest to the origin are prioritized for the split.
		Farthest sides are crushed by the cast to int: 5/2 = 2, 1/2 = 0
	*/
	int	xBDFH = this->size.x / 2; // smaller when width is odd
	int yEFGH = this->size.y / 2; // smaller when height is odd
	int	zCDGH = this->size.z / 2; // smaller when depth is odd
	int xACEG = this->size.x - xBDFH;
	int yABCD = this->size.y - yEFGH;
	int zABEF = this->size.z - zCDGH;

	Math::Vector3	posA(pos.x, pos.y, pos.z);
	Math::Vector3	posB(pos.x + xACEG, pos.y, pos.z);
	Math::Vector3	posC(pos.x, pos.y, pos.z + zABEF);
	Math::Vector3	posD(pos.x + xACEG, pos.y, pos.z + zABEF);

	Math::Vector3	posE(pos.x, pos.y + yABCD, pos.z);
	Math::Vector3	posF(pos.x + xACEG, pos.y + yABCD, pos.z);
	Math::Vector3	posG(pos.x, pos.y + yABCD, pos.z + zABEF);
	Math::Vector3	posH(pos.x + xACEG, pos.y + yABCD, pos.z + zABEF);

	//A0 B1 C2 D3 E4 F5 G6 H7

	this->children[0] = new Octree<T>(arr, posA, Math::Vector3(xACEG, yABCD, zABEF), threshold);
	if (xBDFH)
		this->children[1] = new Octree<T>(arr, posB, Math::Vector3(xBDFH, yABCD, zABEF), threshold);
	if (zCDGH)
		this->children[2] = new Octree<T>(arr, posC, Math::Vector3(xACEG, yABCD, zCDGH), threshold);
	if (xBDFH && zCDGH)
		this->children[3] = new Octree<T>(arr, posD, Math::Vector3(xBDFH, yABCD, zCDGH), threshold);

	if (yEFGH)
		this->children[4] = new Octree<T>(arr, posE, Math::Vector3(xACEG, yEFGH, zABEF), threshold);
	if (yEFGH && xBDFH)
		this->children[5] = new Octree<T>(arr, posF, Math::Vector3(xBDFH, yEFGH, zABEF), threshold);
	if (yEFGH && zCDGH)
		this->children[6] = new Octree<T>(arr, posG, Math::Vector3(xACEG, yEFGH, zCDGH), threshold);
	if (yEFGH && xBDFH && zCDGH)
		this->children[7] = new Octree<T>(arr, posH, Math::Vector3(xBDFH, yEFGH, zCDGH), threshold);
}

template <typename T>
Octree<T>::~Octree() {
	if (this->children) {
		for (size_t i = 0; i < CHILDREN; i++) {
			if (this->children[i])
				delete this->children[i];
		}
		delete[] this->children;
	}
}

template <typename T>
bool		Octree<T>::isLeaf() const {
	return (this->children == nullptr);
}

template <typename T>
Octree<T>* Octree<T>::getNode(const Math::Vector3 targetPos, const Math::Vector3 nodeSize) {
	// This could be done by generating the index array to access the exact node: this->children[index[0]]->children[index[1]]->children[index[2]]...

	if (nodeSize.x <= 0 || nodeSize.y <= 0 || nodeSize.z <= 0) {
		std::cout << "Problem with nodeSize (negative or zero) in " << __PRETTY_FUNCTION__ << LF;
		std::cout << "\tnodeSize:\t" << nodeSize.toString() << LF;
		return nullptr;
	}

	if (targetPos.x < this->pos.x || targetPos.y < this->pos.y || targetPos.z < this->pos.z || \
		targetPos.x > this->summit.x || targetPos.y > this->summit.y || targetPos.z > this->summit.z) {
		return nullptr;
	} else if (this->isLeaf() || this->size == nodeSize) {
		return this;
	} else {
		// send the request to the right child!
		int index = 0;
		if (targetPos.x >= this->pos.x + this->size.x - int(this->size.x / 2)) // cf layer sizes, in the constructor
			index += 1;
		if (targetPos.y >= this->pos.y + this->size.y - int(this->size.y / 2)) // same
			index += 4;
		if (targetPos.z >= this->pos.z + this->size.z - int(this->size.z / 2)) // same
			index += 2;

		if (this->children[index]) { // if not, then it will return null, should be impossible (detected with summits)
			return this->children[index]->getNode(targetPos, nodeSize);
		} else {
			std::cout << "Problem with data in " << __PRETTY_FUNCTION__ << "\n";
			std::cout << "\ttarget pos: \t" << targetPos.toString() << "\n";
			std::cout << "\ttarget size:\t" << nodeSize.toString() << "\n";
			std::cout << "\tnode pos:   \t" << this->pos.toString() << "\n";
			std::cout << "\tnode size:  \t" << this->size.toString() << "\n";
			std::exit(1);
			return nullptr;
		}
	}
}


template <typename T>
Octree<T>*	Octree<T>::getNodeExact(const Math::Vector3 nodeOrigin, const Math::Vector3 nodeSize) {
	// This could be done by generating the index array to access the exact node: this->children[index[0]]->children[index[1]]->children[index[2]]...
	static int ii = 0;
	ii++;

	if (nodeSize.x <= 0 || nodeSize.y <= 0 || nodeSize.z <= 0) {
		std::cout << "Problem with nodeSize (negative or zero) in " << __PRETTY_FUNCTION__ << LF;
		std::cout << "\ttarget size:\t" << nodeSize.toString() << LF;
		return nullptr;
	}
	if (this->pos == nodeOrigin && this->size == nodeSize) {
		return this;
	} else if (this->isLeaf()) {
		return nullptr;
	} else {
		// send the request to the right child!
		int index = 0;
		if (nodeOrigin.x >= this->pos.x + this->size.x - int(this->size.x / 2))// cf layer sizes, in the constructor
			index += 1;
		if (nodeOrigin.y >= this->pos.y + this->size.y - int(this->size.y / 2))// same
			index += 4;
		if (nodeOrigin.z >= this->pos.z + this->size.z - int(this->size.z / 2))// same
			index += 2;

		if (this->children[index]) {//if not, then it will return null, should be impossible (detected with summits)
			return this->children[index]->getNodeExact(nodeOrigin, nodeSize);
			//std::cout << ">>4";
		} else {
			std::cout << "Problem with data in " << __PRETTY_FUNCTION__ << "\n";
			std::cout << "\ttarget pos: \t" << nodeOrigin.toString() << "\n";
			std::cout << "\ttarget size:\t" << nodeSize.toString() << "\n";
			std::cout << "\tnode pos:   \t" << this->pos.toString() << "\n";
			std::cout << "\tnode size:  \t" << this->size.toString() << "\n";
			std::exit(1);
			return nullptr;
		}
	}
}

template <typename T>
bool		Octree<T>::contains(const T& elem, const Math::Vector3 pos, const Math::Vector3 size) {
	Math::Vector3	unitSize(1, 1, 1);

	Math::Vector3 current = pos;
	Math::Vector3 max = pos + size;
	for (;current.x < max.x; current.x++) {
		current.z = pos.z;
		current.y = pos.y;
		for (;current.y < max.y; current.y++) {
			current.z = pos.z;
			for (;current.z < max.z; current.z++) {
				Octree<T>* subnode = this->getNode(current, unitSize);

				// if neighbor is out of the current octree (we're on the edge)// we could check with the right neighboring octree (chunk)
				// || equal to elem
				if (!subnode || subnode->element._value == elem._value) {
					return true;
				}
			}
		}
	}
	return false;
}

template <typename T>
void		Octree<T>::verifyNeighbors(const T& filter) {
	Math::Vector3	maxSummit(this->summit);
	Math::Vector3	minSummit(this->pos);
	Octree<T>*		root = this;

	this->browse([root, &minSummit, &maxSummit, &filter](Octree<T>* node) {
		node->neighbors = 0;
		/*
			we need to know if there is EMPTY_VOXEL in the neighbor right next to us (more consuming),
			but how to differenciate an averaged voxel with EMPTY_VOXEL from one without EMPTY_VOXEL ?
				if all voxels are the same, there is no problem. we could use a first octree to map non empty voxels,
				then on each leaf, make another octree maping the different voxels
			even if we can, it is not optimal, we need to know if the adjacent nodes (at lowest threshold) are all not empty
			we could getNode() with a size of 1, so depending of current size:
				size.x*size.y*2 amount of getNode() for back/front
				size.y*size.z*2 amount of getNode() for left/right
				size.x*size.z*2 amount of getNode() for top/down
			or getNode() on size/2 until we find all adjacent leafs, and check if empty

		*/
		#define USE_SAMESIZE_SEARCH
		#define USE_DETAILLED_SEARCH
		//#define USE_BACKTRACKING_SEARCH

		#ifdef USE_SAMESIZE_SEARCH
		/*
			this requires that the entire node should be non empty
			check isLeaf: as long as all non-empty voxels are the same, we're sure there is no empty voxels here
				cf average method
		*/
		Math::Vector3	node_left = node->pos - Math::Vector3(node->size.x, 0, 0);
		Math::Vector3	node_right = node->pos + Math::Vector3(node->size.x, 0, 0);
		Math::Vector3	node_bot = node->pos - Math::Vector3(0, node->size.y, 0);
		Math::Vector3	node_top = node->pos + Math::Vector3(0, node->size.y, 0);
		Math::Vector3	node_back = node->pos - Math::Vector3(0, 0, node->size.z);
		Math::Vector3	node_front = node->pos + Math::Vector3(0, 0, node->size.z);
		Math::Vector3*	node_sides[6] = { &node_left, &node_right, &node_bot, &node_top, &node_back, &node_front };
		#endif USE_SAMESIZE_SEARCH

		#ifdef USE_DETAILLED_SEARCH
		/*
		              |         |
		              |ooooooooo|
		        ¨¨¨¨¨o|¨¨¨¨¨¨¨¨¨|o¨¨¨¨¨
		             o| current |o
		             o|  node   |o      every adjacent voxels (o) are checked, size of 1x1x1
		             o|         |o
		        _____o|_________|o_____
		              |ooooooooo|
		              |         |
			
			getNode() with neighbors size of 1, so depending of node size:
				size.x*size.y*2 amount of getNode() for back/front
				size.y*size.z*2 amount of getNode() for left/right
				size.x*size.z*2 amount of getNode() for top/down
		*/
		Math::Vector3	close_left = node->pos - Math::Vector3(1, 0, 0);
		Math::Vector3	close_right = node->pos + Math::Vector3(node->size.x, 0, 0);
		Math::Vector3	close_bot = node->pos - Math::Vector3(0, 1, 0);
		Math::Vector3	close_top = node->pos + Math::Vector3(0, node->size.y, 0);
		Math::Vector3	close_back = node->pos - Math::Vector3(0, 0, 1);
		Math::Vector3	close_front = node->pos + Math::Vector3(0, 0, node->size.z);

		Math::Vector3	sizeXaxis(1, node->size.y, node->size.z);
		Math::Vector3	sizeYaxis(node->size.x, 1, node->size.z);
		Math::Vector3	sizeZaxis(node->size.x, node->size.y, 1);

		Math::Vector3*	close_sides[6] = { &close_left, &close_right, &close_bot, &close_top, &close_back, &close_front };
		Math::Vector3*	sides_axis[6] = { &sizeXaxis, &sizeXaxis, &sizeYaxis, &sizeYaxis, &sizeZaxis, &sizeZaxis };
		#endif USE_DETAILLED_SEARCH

		uint8_t			tags[6] = { NEIGHBOR_LEFT, NEIGHBOR_RIGHT, NEIGHBOR_BOTTOM, NEIGHBOR_TOP, NEIGHBOR_BACK, NEIGHBOR_FRONT };

		for (size_t i = 0; i < 6; i++) {
			#ifdef USE_SAMESIZE_SEARCH
			Octree* subnode = root->getNode(*node_sides[i], node->size);
			if (subnode	&& subnode->element._value != filter._value && subnode->isLeaf()) {
				node->neighbors |= tags[i];
			}
			#endif
			#ifdef USE_DETAILLED_SEARCH
			if ((node->neighbors & tags[i]) != tags[i]) {
				if (!root->contains(filter, *close_sides[i], *sides_axis[i])) {
					node->neighbors |= tags[i];
				}
			}
			#endif
		}

		#ifdef USE_BACKTRACKING_SEARCH
		// getNodeExact() on size/2 until we find all adjacent leafs, and check if empty
		// backtracking
		#endif USE_BACKTRACKING_SEARCH
	});
}
