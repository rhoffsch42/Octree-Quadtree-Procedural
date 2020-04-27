#include "octree.hpp"

Pixel   getAverage(Pixel*** arr, Math::Vector3 pos, Math::Vector3 size) {
	//std::cout << "__getA.. " << width << "x" << height << " at " << x << ":" << y << std::endl;
	unsigned int r = 0;
	unsigned int g = 0;
	unsigned int b = 0;
	//Adds the color values for each channel.
	unsigned int counter = 0;
	for (int k = pos.z; k < pos.z + size.z; k++) {
		for (int j = pos.y; j < pos.y + size.y; j++) {
			for (int i = pos.x; i < pos.x + size.x; i++) {
				//std::cout << arr[j][i].r << "_" << arr[j][i].g << "_" << arr[j][i].b << std::endl;
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
		std::cout << "fuck calcul" << std::endl;
		exit(1);
	}
	r = r / area;
	g = g / area;
	b = b / area;
	//std::cout << "average:\t" << r << "\t" << g << "\t" << b << std::endl;
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

Octree::Octree(Pixel *** arr, Math::Vector3 corner_pos, Math::Vector3 tree_size, unsigned int threshold) {
	//std::cout << "_ " << __PRETTY_FUNCTION__ << std::endl;

	this->children = nullptr;
	this->pixel.r = 0;
	this->pixel.g = 0;
	this->pixel.b = 0;
	this->pos = corner_pos;
	this->size = tree_size;
	this->summit = corner_pos;
	this->summit.add(tree_size);
	this->neighbors = 0;//all sides are not empty by default (could count corners too, so 26)
	if (size.x == 1 && size.y == 1 && size.z == 1) {// or x*y*z==1
		this->pixel = arr[(int)this->pos.z][(int)this->pos.y][(int)this->pos.x];//use Vector3i to avoid casting
		this->detail = measureDetail(arr, pos, size, this->pixel);//should be 0
		if (this->detail != 0) {
			std::cout << "1x1 area, detail: " << this->detail << std::endl;
			exit(10);
		}
		if (size.x * size.y * size.z >= OC_DEBUG_LEAF_AREA && OC_DEBUG_LEAF) {
			std::cout << "new leaf: " << size.x << "x" << size.y << "x" << size.z << " at ";
			std::cout << pos.x << ":" << pos.y << ":" << pos.z << "\t";
			std::cout << (int)this->pixel.r << "  \t" << (int)this->pixel.g << "  \t" << (int)this->pixel.b;
			std::cout << "\t" << this->detail << std::endl;
		}
		return;
	}
	this->pixel = getAverage(arr, pos, size);
	this->detail = measureDetail(arr, pos, size, this->pixel);
	if (this->detail <= double(threshold)) {//not that much details in the area
		if (size.x * size.y * size.z >= OC_DEBUG_LEAF_AREA && OC_DEBUG_LEAF) {
			std::cout << "new leaf: " << size.x << "x" << size.y << "x" << size.z << " at ";
			std::cout << pos.x << ":" << pos.y << ":" << pos.z << "\t";
			std::cout << (int)this->pixel.r << "  \t" << (int)this->pixel.g << "  \t" << (int)this->pixel.b;
			std::cout << "\tdetail: " << this->detail << std::endl;
		}
		//std::cout << this->pixel.r << " " << this->pixel.g << " " << this->pixel.b << std::endl;
	}
	else {

		//enough detail to split
		this->children = new Octree * [CHILDREN];
		//this->children = (Octree**)malloc(sizeof(Octree*) * 4);
		if (!this->children) {
			std::cout << "malloc failed\n"; exit(5);
		}

		for (size_t i = 0; i < 8; i++) {
			this->children[i] = nullptr;
		}

			/*

		   0_________x
			| A | B |
			|---|---|		lower layer seen from top
			| C | D |
		   z``front``
			_________
			| E | F |
			|---|---|		higher layer seen from top
			| G | H |
			``front``
			 _________
		   y/________/|
			| G | H | |
			|---|---| /		both layers seen from front
			| C | D |/
		   0`````````x

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

		this->children[0] = new Octree(arr, posA, Math::Vector3(xACEG, yABCD, zABEF), threshold);
		if (xBDFH)
			this->children[1] = new Octree(arr, posB, Math::Vector3(xBDFH, yABCD, zABEF), threshold);
		if (zCDGH)
			this->children[2] = new Octree(arr, posC, Math::Vector3(xACEG, yABCD, zCDGH), threshold);
		if (xBDFH && zCDGH)
			this->children[3] = new Octree(arr, posD, Math::Vector3(xBDFH, yABCD, zCDGH), threshold);

		if (yEFGH)
			this->children[4] = new Octree(arr, posE, Math::Vector3(xACEG, yEFGH, zABEF), threshold);
		if (yEFGH && xBDFH)
			this->children[5] = new Octree(arr, posF, Math::Vector3(xBDFH, yEFGH, zABEF), threshold);
		if (yEFGH && zCDGH)
			this->children[6] = new Octree(arr, posG, Math::Vector3(xACEG, yEFGH, zCDGH), threshold);
		if (yEFGH && xBDFH && zCDGH)
			this->children[7] = new Octree(arr, posH, Math::Vector3(xBDFH, yEFGH, zCDGH), threshold);
	}
}

Octree::~Octree() {
	if (this->children) {
		for (size_t i = 0; i < CHILDREN; i++) {
			if (this->children[i])
				delete this->children[i];
		}
		delete[] this->children;
	}
}

bool		Octree::isLeaf() const {
	return (this->children == nullptr);
}

Octree* Octree::getRoot(Math::Vector3 target_pos, Math::Vector3 target_size) {
	static int ii = 0;
	ii++;

	Math::Vector3	target_summit(target_pos);
	target_summit.add(target_size);//the highest summit of the root;

	if (target_pos.x < this->pos.x || target_pos.y < this->pos.y || target_pos.z < this->pos.z || \
		this->summit.x <= target_pos.x || this->summit.y <= target_pos.y || this->summit.z <= target_pos.z) {
		// ie the required root is fully or partially outside of the current root
		//std::cout << ">>1";
		return nullptr;
	} else if (this->pos.x == target_pos.x && this->pos.y == target_pos.y && this->pos.z == target_pos.z && \
		this->size.x == target_size.x && this->size.y == target_size.y && this->size.z == target_size.z) {
		//found the exact root (pos and size)
		//std::cout << ">>2";
		return this;
	} else if (this->isLeaf()) {
		//the exact root is contained in this current leaf root
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
			return this->children[index]->getRoot(target_pos, target_size);
			//std::cout << ">>4";
		} else {
			std::cout << "Problem with data in " << __PRETTY_FUNCTION__ << std::endl;
			std::cout << "\ttarget pos: \t"; target_pos.printData();
			std::cout << "\ttarget size:\t"; target_size.printData();
			std::cout << "\troot pos:   \t"; this->pos.printData();
			std::cout << "\troot size:  \t"; this->size.printData();
			exit(0);
			return nullptr;
		}
	}
}

bool		Octree::contain(Pixel pix, Math::Vector3 pos, Math::Vector3 size) {
	//the user has to send corrent pos and size
	Math::Vector3	unitSize(1, 1, 1);

	for (int x = 0; x < size.x; x++) {
		for (int y = 0; y < size.y; y++) {
			for (int z = 0; z < size.z; z++) {
				Math::Vector3	currentPos(pos);
				currentPos.add(x, y, z);
				Octree* subroot = this->getRoot(currentPos, unitSize);

				// if neighbor is out of the current octree (we're on the endge)// we could check with the right neighboring octree (chunk)
				// || equal to pix
				if (!subroot \
					|| (subroot->pixel.r == pix.r && subroot->pixel.g == pix.g && subroot->pixel.b == pix.b)) {
					return true;
				}
			}
		}
	}
	return false;
}

void		Octree::verifyNeighbors(Pixel filter) {
	Math::Vector3	maxSummit(this->summit);
	Math::Vector3	minSummit(this->pos);
	Octree*			mainRoot = this;
	this->browse(0, [mainRoot, &minSummit, &maxSummit, &filter](Octree* node) {
		node->neighbors = 0;
/*
	we need to know if there is EMPTY_VOXEL in the neighbour right next to us (more consuming),
	but how to differenciate an averaged voxel with EMPTY_VOXEL from one without EMPTY_VOXEL ?
		if all voxels are the same, there is no problem. we could use a first octree to map non empty voxels,
		then on each leaf, make another octree maping the different voxels
	even if we can, it is not optimal, we need to know if the adjacent nodes (at lowest threshold) are all not empty
	we could getRoot() with a size of 1, so depending of current size:
		size.x*size.y*2 amount of getRoot() for back/front
		size.y*size.z*2 amount of getRoot() for left/right
		size.x*size.z*2 amount of getRoot() for top/down
	or getRoot() on size/2 until we find all adjacent leafs, and check if empty

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
		Math::Vector3	left(node->pos); left.sub(node->size.x, 0, 0);
		Math::Vector3	right(node->pos); right.add(node->size.x, 0, 0);
		Math::Vector3	down(node->pos); down.sub(0, node->size.y, 0);
		Math::Vector3	up(node->pos); up.add(0, node->size.y, 0);
		Math::Vector3	back(node->pos); back.sub(0, 0, node->size.z);
		Math::Vector3	front(node->pos); front.add(0, 0, node->size.z);
		Math::Vector3*	sides[6] = { &left, &right, &down, &up, &back, &front };

		for (size_t i = 0; i < 6; i++) {
			Octree* root = mainRoot->getRoot(*sides[i], node->size);
			if (root && root->isLeaf() \
				&& (root->pixel.r != filter.r || root->pixel.g != filter.g || root->pixel.b != filter.b)) {
				node->neighbors++;
			}
		}
#endif
#ifdef USE_DETAILLED_SEARCH
		/*
			getRoot() with neighbors size of 1, so depending of node size:
				size.x*size.y*2 amount of getRoot() for back/front
				size.y*size.z*2 amount of getRoot() for left/right
				size.x*size.z*2 amount of getRoot() for top/down
		*/

			Math::Vector3	posleft(node->pos); posleft.sub(1, 0, 0);
			Math::Vector3	posright(node->pos); posright.add(node->size.x, 0, 0);
			Math::Vector3	posdown(node->pos); posdown.sub(0, 1, 0);
			Math::Vector3	posup(node->pos); posup.add(0, node->size.y, 0);
			Math::Vector3	posback(node->pos); posback.sub(0, 0, 1);
			Math::Vector3	posfront(node->pos); posfront.add(0, 0, node->size.z);

			Math::Vector3	sizeXaxis(1, node->size.y, node->size.z);
			Math::Vector3	sizeYaxis(node->size.x, 1, node->size.z);
			Math::Vector3	sizeZaxis(node->size.x, node->size.y, 1);

			if (!mainRoot->contain(filter, posleft, sizeXaxis)) { node->neighbors++; }
			if (!mainRoot->contain(filter, posright, sizeXaxis)) { node->neighbors++; }
			if (!mainRoot->contain(filter, posdown, sizeYaxis)) { node->neighbors++; }
			if (!mainRoot->contain(filter, posup, sizeYaxis)) { node->neighbors++; }
			if (!mainRoot->contain(filter, posback, sizeZaxis)) { node->neighbors++; }
			if (!mainRoot->contain(filter, posfront, sizeZaxis)) { node->neighbors++; }
	
#endif
#ifdef USE_BACKTRACKING_SEARCH
			// getRoot() on size/2 until we find all adjacent leafs, and check if empty
			// backtracking
#endif
	});
}
