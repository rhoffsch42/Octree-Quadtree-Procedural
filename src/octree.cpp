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

#ifdef USE_TEMPLATE
VoxelValueAccumulator2::VoxelValueAccumulator2() : IValueAccumulator<Voxel>() {}
void	VoxelValueAccumulator2::add(const Voxel& vox) {
	_v1 += vox._value;
	_count++;
}
Voxel	VoxelValueAccumulator2::getAverage() const {
	return Voxel(_v1 / _count);
}
void	VoxelValueAccumulator2::reset() {
	_v1 = 0;
	_count = 0;
}

VoxelDistanceAccumulator2::VoxelDistanceAccumulator2() : IDistanceAccumulator<Voxel>() {}
void	VoxelDistanceAccumulator2::add(const Voxel& vox1, const Voxel& vox2)
{
	_v1 += std::abs(int(vox1._value) - int(vox2._value));
	_count++;
}
Voxel	VoxelDistanceAccumulator2::getAverage() const {
	return Voxel(_v1 / _count);
}
double	VoxelDistanceAccumulator2::getDetail() const {
	return (_v1 / (1 * _count)); // X * count, X beeing the number of values in the class (here Voxel::)
}
void	VoxelDistanceAccumulator2::reset() {
	_v1 = 0;
	_count = 0;
}

VoxelValueAccumulator2* Voxel::getValueAccumulator() { return new VoxelValueAccumulator2(); }
VoxelDistanceAccumulator2* Voxel::getDistanceAccumulator() { return new VoxelDistanceAccumulator2(); }
#else
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
#endif

Voxel::Voxel() {}
Voxel::Voxel(uint8_t& v) : _value(v) {}
Voxel::Voxel(uint8_t&& v) : _value(v) {}
bool	Voxel::operator==(const Voxel& rhs) const { return this->_value == rhs._value; }
bool	Voxel::operator!=(const Voxel& rhs) const { return this->_value != rhs._value; }
Voxel&	Voxel::operator=(const Voxel& rhs) { this->_value = rhs._value; return *this; }
Voxel::~Voxel() {}

#ifdef USE_TEMPLATE
template <typename T>
T	getAverage(T***arr, const Math::Vector3& pos, const Math::Vector3& size) {
	//std::cout << "__getAverage.. " << width << "x" << height << " at " << x << ":" << y << "\n";
	//Add up values
	static IValueAccumulator<T>* valueAcc = dynamic_cast<IValueAccumulator<T>*>(T::getValueAccumulator());
	valueAcc->reset();
	for (int z = pos.z; z < pos.z + size.z; z++) {
		for (int y = pos.y; y < pos.y + size.y; y++) {
			for (int x = pos.x; x < pos.x + size.x; x++) {
				valueAcc->add(arr[z][y][x]);
			}
		}
	}
	return valueAcc->getAverage();
}

template <typename T>
double	measureDetail(T*** arr, const Math::Vector3& pos, const Math::Vector3& size, const T& average) {
	// Calculates the distance between every T in the region
	// and the average T value. The Manhattan distance is used, and
	// all the distances are added.
	static IDistanceAccumulator<T>* distanceAcc = dynamic_cast<IDistanceAccumulator<T>*>(T::getDistanceAccumulator());
	distanceAcc->reset();
	for (int z = pos.z; z < pos.z + size.z; z++) {
		for (int y = pos.y; y < pos.y + size.y; y++) {
			for (int x = pos.x; x < pos.x + size.x; x++) {
				distanceAcc->add(average, arr[z][y][x]);
			}
		}
	}
	// Calculates the average distance, and returns the result.
	// Mult by T::n, because we are averaging over T::n values
	return distanceAcc->getDetail();
}
#endif

template <typename T>
Octree<T>::Octree(T*** arr, Math::Vector3 corner_pos, Math::Vector3 tree_size, unsigned int threshold) {
	//D("New octree : corner_pos " << corner_pos << " tree_size " << tree_size << " threshold " << threshold << LF);
	//std::cout << "_ " << __PRETTY_FUNCTION__ << "\n";

	this->children = nullptr;
	this->detail = 0;
	this->element = T();
	this->pos = corner_pos;
	this->size = tree_size;
	this->summit = corner_pos + tree_size;
	this->neighbors = 0;//all sides are not empty by default (could count corners too, so 26)
	if (size.x == 1 && size.y == 1 && size.z == 1) {// or x*y*z==1
		this->element._value = arr[(int)this->pos.z][(int)this->pos.y][(int)this->pos.x]._value;//use Vector3i to avoid casting
		#if 0 // checks not needed, the tree has only 1 voxel of size 1
		this->detail = measureDetail(arr, this->pos, this->size, this->pixel);//should be 0
		if (this->detail != 0) {
			std::cout << "error with 1x1 area, detail: " << this->detail << "\n";
			exit(10);
		}
		#endif
		/*		if (size.x * size.y * size.z >= OC_DEBUG_LEAF_AREA && OC_DEBUG_LEAF) {
					std::cout << "new leaf: " << size.x << "x" << size.y << "x" << size.z << " at ";
					std::cout << pos.x << ":" << pos.y << ":" << pos.z << "\t";
					std::cout << (int)this->pixel.r << "  \t" << (int)this->pixel.g << "  \t" << (int)this->pixel.b;
					std::cout << "\t" << this->detail << "\n";
				}
		*/
		return;
	}
	#ifdef USE_TEMPLATE
	this->element = getAverage<T>(arr, pos, size);
	this->detail = measureDetail<T>(arr, pos, size, this->element);
	#else
	this->element = T::getAverage(arr, pos, size);
	this->detail = T::measureDetail(arr, pos, size, this->element);
	#endif
	#ifdef OC_DEBUG_NODE
	std::cout << "pos: " << pos << "\t";
	std::cout << "size: " << size << "\t";
	std::cout << "average: " << int(this->element._value) << "\t";
	std::cout << "detail: " << this->detail << "\t";
	std::cout << "\n---------------\n";
	#endif
	if (this->detail <= double(threshold)) {//not that much details in the area, this is the end leaf
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

	//sizes
	int	xBDFH = this->size.x / 2; // is rounded down, so smaller when width is odd
	int yEFGH = this->size.y / 2; // is rounded down, so smaller when height is odd
	int	zCDGH = this->size.z / 2; // is rounded down, so smaller when depth is odd
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
Octree<T>*	Octree<T>::getNode(const Math::Vector3& target_pos, const Math::Vector3& target_size) {
	static int ii = 0;
	ii++;

	Math::Vector3	target_summit = target_pos + target_size;//the highest summit of the node;

	if (target_pos.x < this->pos.x || target_pos.y < this->pos.y || target_pos.z < this->pos.z || \
		this->summit.x <= target_pos.x || this->summit.y <= target_pos.y || this->summit.z <= target_pos.z) {
		// ie the required node is fully or partially outside of the current node
		//std::cout << ">>1";
		return nullptr;
	}
	else if (this->pos == target_pos && this->size == target_size) {
		//found the exact node (pos and size)
		//std::cout << ">>2";
		return this;
	}
	else if (this->isLeaf()) {
		//the exact node is contained in this current leaf node
		//	/!\ size could be invalid, from the current octree perspective
		//std::cout << ">>3";
		return this;
	}
	else {
		// send the request to the right child!
		int index = 0;
		if (target_pos.x >= this->pos.x + this->size.x - int(this->size.x / 2))// cf layer sizes, in the constructor
			index += 1;
		if (target_pos.y >= this->pos.y + this->size.y - int(this->size.y / 2))// same
			index += 4;
		if (target_pos.z >= this->pos.z + this->size.z - int(this->size.z / 2))// same
			index += 2;

		if (this->children[index]) {//if not, then it will return null, should be impossible (detected with summits)
			return this->children[index]->getNode(target_pos, target_size);
			//std::cout << ">>4";
		}
		else {
			std::cout << "Problem with data in " << __PRETTY_FUNCTION__ << "\n";
			std::cout << "\ttarget pos: \t" << target_pos.toString() << "\n";
			std::cout << "\ttarget size:\t" << target_size.toString() << "\n";
			std::cout << "\tnode pos:   \t" << this->pos.toString() << "\n";
			std::cout << "\tnode size:  \t" << this->size.toString() << "\n";
			exit(1);
			return nullptr;
		}
	}
}

template <typename T>
bool		Octree<T>::contains(const T& elem, const Math::Vector3& pos, const Math::Vector3& size) {
	//the user has to send corrent pos and size
	Math::Vector3	unitSize(1, 1, 1);

	for (int x = 0; x < size.x; x++) {
		for (int y = 0; y < size.y; y++) {
			for (int z = 0; z < size.z; z++) {
				Math::Vector3	currentPos = pos + Math::Vector3(x, y, z);
				Octree<T>* subnode = this->getNode(currentPos, unitSize);

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
			we need to know if there is EMPTY_VOXEL in the neighbour right next to us (more consuming),
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


		//if (!root->contains(filter, close_left, sizeXaxis)) { node->neighbors |= NEIGHBOR_LEFT; }
		//if (!root->contains(filter, close_right, sizeXaxis)) { node->neighbors |= NEIGHBOR_RIGHT; }
		//if (!root->contains(filter, close_bot, sizeYaxis)) { node->neighbors |= NEIGHBOR_BOTTOM; }
		//if (!root->contains(filter, close_top, sizeYaxis)) { node->neighbors |= NEIGHBOR_TOP; }
		//if (!root->contains(filter, close_back, sizeZaxis)) { node->neighbors |= NEIGHBOR_BACK; }
		//if (!root->contains(filter, close_front, sizeZaxis)) { node->neighbors |= NEIGHBOR_FRONT; }

		#ifdef USE_BACKTRACKING_SEARCH
		// getNode() on size/2 until we find all adjacent leafs, and check if empty
		// backtracking
		#endif USE_BACKTRACKING_SEARCH
	});
}
