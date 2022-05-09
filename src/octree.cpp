#include "octree.hpp"
#include "compiler_settings.h"

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
Voxel	Voxel::speGetAverage(Voxel*** arr, const Math::Vector3& pos, const Math::Vector3& size) {
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
double	Voxel::speMeasureDetail(Voxel*** arr, const Math::Vector3& pos, const Math::Vector3& size, const Voxel& average) {
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
	//std::cout << "_ " << __PRETTY_FUNCTION__ << "\n";

	this->children = nullptr;
	this->detail = 0;
	//pixel is the data contained in the octree, it can be anything, here it is a voxel.
	//make a template for Octree_old? Octree_old<Pixel>(). ->> need getAverage() and measureDetail() defined
	//make Octree_old abstract with these 2 funcs pure. then create the corresponding Octree_old class, here VoxelOctree_old
	this->element = T();
	this->pos = corner_pos;
	this->size = tree_size;
	this->summit = corner_pos + tree_size;
	this->neighbors = 0;//all sides are not empty by default (could count corners too, so 26)
	if (size.x == 1 && size.y == 1 && size.z == 1) {// or x*y*z==1
		//this->element = arr[(int)this->pos.z][(int)this->pos.y][(int)this->pos.x];//use Vector3i to avoid casting
		this->element._value = arr[(int)this->pos.z][(int)this->pos.y][(int)this->pos.x]._value;//use Vector3i to avoid casting
#if 0 // checks not needed, the tree has only 1 voxel of size 1
		this->detail = measureDetail(arr, pos, size, this->pixel);//should be 0
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
	this->element = Voxel::speGetAverage(arr, pos, size);
	this->detail = Voxel::speMeasureDetail(arr, pos, size, this->element);
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
	z``front`
	o________x
	| E | F |
	|---|---|		higher layer seen from top
	| G | H |
	z``front`
	   _________
	  /___/___/|
	y/___/___/ |
	| G | H |  |
	|---|---| /		both layers seen from front
	| C | D |/
	o````````x

	*/

	//sizes
	int	xBDFH = this->size.x / 2;//is rounded down, so smaller when width is odd
	int yEFGH = this->size.y / 2;//is rounded down, so smaller when height is odd
	int	zCDGH = this->size.z / 2;//is rounded down, so smaller when depth is odd
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
			exit(0);
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
//#define USE_SAMESIZE_SEARCH
#define USE_DETAILLED_SEARCH
//#define USE_BACKTRACKING_SEARCH

#ifdef USE_SAMESIZE_SEARCH
			/*
				this requires that the entire node should be non empty
				check isLeaf: as long as all non-empty voxels are the same, we're sure there is no empty voxels here
					cf average method
			*/
		Math::Vector3	left = node->pos - Math::Vector3(node->size.x, 0, 0);
		Math::Vector3	right = node->pos + Math::Vector3(node->size.x, 0, 0);
		Math::Vector3	bot = node->pos - Math::Vector3(0, node->size.y, 0);
		Math::Vector3	top = node->pos + Math::Vector3(0, node->size.y, 0);
		Math::Vector3	back = node->pos - Math::Vector3(0, 0, node->size.z);
		Math::Vector3	front = node->pos + Math::Vector3(0, 0, node->size.z);
		Math::Vector3*	sides[6] = { &left, &right, &bot, &top, &back, &front };
		uint8_t			tags[6] = { NEIGHBOR_LEFT, NEIGHBOR_RIGHT, NEIGHBOR_BOTTOM, NEIGHBOR_TOP, NEIGHBOR_BACK, NEIGHBOR_FRONT };

		for (size_t i = 0; i < 6; i++) {
			Octree* subnode = root->getNode(*sides[i], node->size);
			if (subnode	&& subnode->element._value != filter._value) {
				subnode->neighbors |= tags[i];
			}
		}
#endif
#ifdef USE_DETAILLED_SEARCH
		/*
			getNode() with neighbors size of 1, so depending of node size:
				size.x*size.y*2 amount of getNode() for back/front
				size.y*size.z*2 amount of getNode() for left/right
				size.x*size.z*2 amount of getNode() for top/down
		*/

		Math::Vector3	posleft = node->pos - Math::Vector3(1, 0, 0);
		Math::Vector3	posright = node->pos + Math::Vector3(node->size.x, 0, 0);
		Math::Vector3	posdown = node->pos - Math::Vector3(0, 1, 0);
		Math::Vector3	posup = node->pos + Math::Vector3(0, node->size.y, 0);
		Math::Vector3	posback = node->pos - Math::Vector3(0, 0, 1);
		Math::Vector3	posfront = node->pos + Math::Vector3(0, 0, node->size.z);

		Math::Vector3	sizeXaxis(1, node->size.y, node->size.z);
		Math::Vector3	sizeYaxis(node->size.x, 1, node->size.z);
		Math::Vector3	sizeZaxis(node->size.x, node->size.y, 1);

		if (!root->contains(filter, posleft, sizeXaxis)) { node->neighbors |= NEIGHBOR_LEFT; }
		if (!root->contains(filter, posright, sizeXaxis)) { node->neighbors |= NEIGHBOR_RIGHT; }
		if (!root->contains(filter, posdown, sizeYaxis)) { node->neighbors |= NEIGHBOR_BOTTOM; }
		if (!root->contains(filter, posup, sizeYaxis)) { node->neighbors |= NEIGHBOR_TOP; }
		if (!root->contains(filter, posback, sizeZaxis)) { node->neighbors |= NEIGHBOR_BACK; }
		if (!root->contains(filter, posfront, sizeZaxis)) { node->neighbors |= NEIGHBOR_FRONT; }

#endif
#ifdef USE_BACKTRACKING_SEARCH
		// getNode() on size/2 until we find all adjacent leafs, and check if empty
		// backtracking
#endif
	});
}


////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

#if 0
Pixel   getAverage(Pixel*** arr, Math::Vector3 pos, Math::Vector3 size) {
	//std::cout << "__getA.. " << width << "x" << height << " at " << x << ":" << y << "\n";
	unsigned int r = 0;
	unsigned int g = 0;
	unsigned int b = 0;
	//Adds the color values for each channel.
	unsigned int counter = 0;
	for (int k = pos.z; k < pos.z + size.z; k++) {
		for (int j = pos.y; j < pos.y + size.y; j++) {
			for (int i = pos.x; i < pos.x + size.x; i++) {
				//std::cout << arr[j][i].r << "_" << arr[j][i].g << "_" << arr[j][i].b << "\n";
				r += (unsigned int)(arr[k][j][i].r);
				g += (unsigned int)(arr[k][j][i].g);
				b += (unsigned int)(arr[k][j][i].b);
				//this->cumulateAverage(arr[][][]) = 0;
				counter++;
			}
		}
	}
	//number of pixels evaluated
	unsigned int area = size.x * size.y * size.z;
	if (area != counter) {
		std::cout << "fuck calcul" << "\n";
		exit(1);
	}
	r = r / area;
	g = g / area;
	b = b / area;
	//std::cout << "average:\t" << r << "\t" << g << "\t" << b << "\n";
	// Returns the color that represent the average.
	//return this->genAverage(area) = 0;
	return Pixel(r, g, b);
}

double	measureDetail(Pixel*** arr, Math::Vector3 pos, Math::Vector3 size, Pixel average) {
	double	colorSum = 0;

	// Calculates the distance between every pixel in the region
	// and the average color. The Manhattan distance is used, and
	// all the distances are added.
	for (int k = pos.z; k < pos.z + size.z; k++) {
		for (int j = pos.y; j < pos.y + size.y; j++) {
			for (int i = pos.x; i < pos.x + size.x; i++) {
				colorSum += std::abs(average.r - arr[k][j][i].r);
				colorSum += std::abs(average.g - arr[k][j][i].g);
				colorSum += std::abs(average.b - arr[k][j][i].b);
				//this->cumulateDetail(arr[][][]) = 0;
			}
		}
	}

	// Calculates the average distance, and returns the result.
	// We divide by three, because we are averaging over 3 channels.
	return (colorSum / double(3 * size.x * size.y * size.z));
	//return this->genDetail(area) = 0;
}

Octree_old::Octree_old(Pixel*** arr, Math::Vector3 corner_pos, Math::Vector3 tree_size, unsigned int threshold) {
	//std::cout << "_ " << __PRETTY_FUNCTION__ << "\n";

	this->children = nullptr;
	this->detail = 0;
	//pixel is the data contained in the octree, it can be anything, here it is a voxel.
	//make a template for Octree_old? Octree_old<Pixel>(). ->> need getAverage() and measureDetail() defined
	//make Octree_old abstract with these 2 funcs pure. then create the corresponding Octree_old class, here VoxelOctree_old
	this->pixel.r = 0;
	this->pixel.g = 0;
	this->pixel.b = 0;
	this->pos = corner_pos;
	this->size = tree_size;
	this->summit = corner_pos + tree_size;
	this->neighbors = 0;//all sides are not empty by default (could count corners too, so 26)
	if (size.x == 1 && size.y == 1 && size.z == 1) {// or x*y*z==1
		this->pixel = arr[(int)this->pos.z][(int)this->pos.y][(int)this->pos.x];//use Vector3i to avoid casting
#if 0 // checks not needed
		this->detail = measureDetail(arr, pos, size, this->pixel);//should be 0
		if (this->detail != 0) {
			std::cout << "1x1 area, detail: " << this->detail << "\n";
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
	this->pixel = getAverage(arr, pos, size);
	this->detail = measureDetail(arr, pos, size, this->pixel);
	if (this->detail <= double(threshold)) {//not that much details in the area, this is the end leaf
		#ifdef OC_DEBUG_LEAF
		if (size.x * size.y * size.z >= OC_DEBUG_LEAF_AREA) {//simple debug
			std::cout << "new leaf: " << size.x << "x" << size.y << "x" << size.z << " at ";
			std::cout << pos.x << ":" << pos.y << ":" << pos.z << "\t";
			std::cout << (int)this->pixel.r << "  \t" << (int)this->pixel.g << "  \t" << (int)this->pixel.b;
			std::cout << "\tdetail: " << this->detail << "\n";
		}
		//std::cout << this->pixel.r << " " << this->pixel.g << " " << this->pixel.b << "\n";
		#endif
		return;
	}

	//enough detail to split
	this->children = new Octree_old * [CHILDREN];
	//this->children = (Octree_old**)malloc(sizeof(Octree_old*) * 4);
	if (!this->children) {
		std::cout << "malloc failed\n"; exit(5);
	}

	for (size_t i = 0; i < 8; i++) {
		this->children[i] = nullptr;
	}

		/*

		o________x
		| A | B |
		|---|---|		lower layer seen from top
		| C | D |
		z``front`
		o________x
		| E | F |
		|---|---|		higher layer seen from top
		| G | H |
		z``front`
		   _________
		  /___/___/|
		y/___/___/ |
		| G | H |  |
		|---|---| /		both layers seen from front
		| C | D |/
		o````````x

		*/

	//sizes
	int	xBDFH = this->size.x / 2;//is rounded down, so smaller when width is odd
	int yEFGH = this->size.y / 2;//is rounded down, so smaller when width is odd
	int	zCDGH = this->size.z / 2;//is rounded down, so smaller when height is odd
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

	this->children[0] = new Octree_old(arr, posA, Math::Vector3(xACEG, yABCD, zABEF), threshold);
	if (xBDFH)
		this->children[1] = new Octree_old(arr, posB, Math::Vector3(xBDFH, yABCD, zABEF), threshold);
	if (zCDGH)
		this->children[2] = new Octree_old(arr, posC, Math::Vector3(xACEG, yABCD, zCDGH), threshold);
	if (xBDFH && zCDGH)
		this->children[3] = new Octree_old(arr, posD, Math::Vector3(xBDFH, yABCD, zCDGH), threshold);

	if (yEFGH)
		this->children[4] = new Octree_old(arr, posE, Math::Vector3(xACEG, yEFGH, zABEF), threshold);
	if (yEFGH && xBDFH)
		this->children[5] = new Octree_old(arr, posF, Math::Vector3(xBDFH, yEFGH, zABEF), threshold);
	if (yEFGH && zCDGH)
		this->children[6] = new Octree_old(arr, posG, Math::Vector3(xACEG, yEFGH, zCDGH), threshold);
	if (yEFGH && xBDFH && zCDGH)
		this->children[7] = new Octree_old(arr, posH, Math::Vector3(xBDFH, yEFGH, zCDGH), threshold);

}

Octree_old::~Octree_old() {
	if (this->children) {
		for (size_t i = 0; i < CHILDREN; i++) {
			if (this->children[i])
				delete this->children[i];
		}
		delete[] this->children;
	}
}

bool		Octree_old::isLeaf() const {
	return (this->children == nullptr);
}

Octree_old* Octree_old::getNode(Math::Vector3 target_pos, Math::Vector3 target_size) {
	static int ii = 0;
	ii++;

	Math::Vector3	target_summit = target_pos + target_size;//the highest summit of the node;

	if (target_pos.x < this->pos.x || target_pos.y < this->pos.y || target_pos.z < this->pos.z || \
		this->summit.x <= target_pos.x || this->summit.y <= target_pos.y || this->summit.z <= target_pos.z) {
		// ie the required node is fully or partially outside of the current node
		//std::cout << ">>1";
		return nullptr;
	} else if (this->pos.x == target_pos.x && this->pos.y == target_pos.y && this->pos.z == target_pos.z && \
		this->size.x == target_size.x && this->size.y == target_size.y && this->size.z == target_size.z) {
		//found the exact node (pos and size)
		//std::cout << ">>2";
		return this;
	} else if (this->isLeaf()) {
		//the exact node is contained in this current leaf node
		//	/!\ size could be invalid, from the current octree perspective
		//std::cout << ">>3";
		return this;
	} else {
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
		} else {
			std::cout << "Problem with data in " << __PRETTY_FUNCTION__ << "\n";
			std::cout << "\ttarget pos: \t" << target_pos.toString() << "\n";
			std::cout << "\ttarget size:\t" << target_size.toString() << "\n";
			std::cout << "\tnode pos:   \t" << this->pos.toString() << "\n";
			std::cout << "\tnode size:  \t" << this->size.toString() << "\n";
			exit(0);
			return nullptr;
		}
	}
}

bool		Octree_old::contain(Pixel pix, Math::Vector3 pos, Math::Vector3 size) {
	//the user has to send corrent pos and size
	Math::Vector3	unitSize(1, 1, 1);

	for (int x = 0; x < size.x; x++) {
		for (int y = 0; y < size.y; y++) {
			for (int z = 0; z < size.z; z++) {
				Math::Vector3	currentPos = pos + Math::Vector3(x, y, z);
				Octree_old* subnode = this->getNode(currentPos, unitSize);

				// if neighbor is out of the current octree (we're on the edge)// we could check with the right neighboring octree (chunk)
				// || equal to pix
				if (!subnode \
					|| (subnode->pixel.r == pix.r && subnode->pixel.g == pix.g && subnode->pixel.b == pix.b)) {
					return true;
				}
			}
		}
	}
	return false;
}

void		Octree_old::verifyNeighbors(Pixel filter) {
	Math::Vector3	maxSummit(this->summit);
	Math::Vector3	minSummit(this->pos);
	Octree_old*			root = this;
	this->browse([root, &minSummit, &maxSummit, &filter](Octree_old* node) {
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
//#define USE_SAMESIZE_SEARCH
#define USE_DETAILLED_SEARCH
//#define USE_BACKTRACKING_SEARCH
#ifdef USE_SAMESIZE_SEARCH
			/*
				this requires that the entire node should be non empty
				check isLeaf: as long as all non-empty voxels are the same, we're sure there is no empty voxels here
					cf average method
			*/
		Math::Vector3	left = node->pos - Math::Vector3(node->size.x, 0, 0);
		Math::Vector3	right = node->pos + Math::Vector3(node->size.x, 0, 0);
		Math::Vector3	down = node->pos - Math::Vector3(0, node->size.y, 0);
		Math::Vector3	up = node->pos + Math::Vector3(0, node->size.y, 0);
		Math::Vector3	back = node->pos - Math::Vector3(0, 0, node->size.z);
		Math::Vector3	front = node->pos + Math::Vector3(0, 0, node->size.z);
		Math::Vector3*	sides[6] = { &left, &right, &down, &up, &back, &front };

		for (size_t i = 0; i < 6; i++) {
			Octree_old* node = root->getNode(*sides[i], node->size);
			if (node && node->isLeaf() \
				&& (node->pixel.r != filter.r || node->pixel.g != filter.g || node->pixel.b != filter.b)) {
				node->neighbors++;
			}
		}
#endif
#ifdef USE_DETAILLED_SEARCH
		/*
			getNode() with neighbors size of 1, so depending of node size:
				size.x*size.y*2 amount of getNode() for back/front
				size.y*size.z*2 amount of getNode() for left/right
				size.x*size.z*2 amount of getNode() for top/down
		*/

		Math::Vector3	posleft = node->pos - Math::Vector3(1, 0, 0);
		Math::Vector3	posright = node->pos + Math::Vector3(node->size.x, 0, 0);
		Math::Vector3	posdown = node->pos - Math::Vector3(0, 1, 0);
		Math::Vector3	posup = node->pos + Math::Vector3(0, node->size.y, 0);
		Math::Vector3	posback = node->pos - Math::Vector3(0, 0, 1);
		Math::Vector3	posfront = node->pos + Math::Vector3(0, 0, node->size.z);

		Math::Vector3	sizeXaxis(1, node->size.y, node->size.z);
		Math::Vector3	sizeYaxis(node->size.x, 1, node->size.z);
		Math::Vector3	sizeZaxis(node->size.x, node->size.y, 1);

		if (!root->contain(filter, posleft, sizeXaxis)) { node->neighbors |= NEIGHBOR_LEFT; }
		if (!root->contain(filter, posright, sizeXaxis)) { node->neighbors |= NEIGHBOR_RIGHT; }
		if (!root->contain(filter, posdown, sizeYaxis)) { node->neighbors |= NEIGHBOR_BOTTOM; }
		if (!root->contain(filter, posup, sizeYaxis)) { node->neighbors |= NEIGHBOR_TOP; }
		if (!root->contain(filter, posback, sizeZaxis)) { node->neighbors |= NEIGHBOR_BACK; }
		if (!root->contain(filter, posfront, sizeZaxis)) { node->neighbors |= NEIGHBOR_FRONT; }

#endif
#ifdef USE_BACKTRACKING_SEARCH
			// getNode() on size/2 until we find all adjacent leafs, and check if empty
			// backtracking
#endif
	});
}

#endif