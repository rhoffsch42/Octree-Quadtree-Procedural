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
			for (int i = pos.x; i < pos.x + size.z; i++) {
				//std::cout << arr[j][i].r << "_" << arr[j][i].g << "_" << arr[j][i].b << std::endl;
				r += (unsigned int)(arr[k][j][i].r);
				g += (unsigned int)(arr[k][j][i].g);
				b += (unsigned int)(arr[k][j][i].b);
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
	return Pixel(r, g, b);
}

double	measureDetail(Pixel*** arr, Math::Vector3 pos, Math::Vector3 size, Pixel average) {
	double	colorSum = 0;

	// Calculates the distance between every pixel in the region
	// and the average color. The Manhattan distance is used, and
	// all the distances are added.
	for (int k = pos.z; k < pos.z + size.z; k++) {
		for (int j = pos.y; j < pos.y + size.y; j++) {
			for (int i = pos.x; i < pos.x + size.z; i++) {
				colorSum += std::abs(average.r - arr[k][j][i].r);
				colorSum += std::abs(average.g - arr[k][j][i].g);
				colorSum += std::abs(average.b - arr[k][j][i].b);
			}
		}
	}

	// Calculates the average distance, and returns the result.
	// We divide by three, because we are averaging over 3 channels.
	return (colorSum / double(3 * size.x * size.y * size.z));
}

Octree::Octree(Pixel *** arr, Math::Vector3 corner_pos, Math::Vector3 tree_size, unsigned int threshold) {
	//std::cout << "_ " << __PRETTY_FUNCTION__ << std::endl;

	this->children = nullptr;
	this->pixel.r = 0;
	this->pixel.g = 0;
	this->pixel.b = 0;
	this->pos = corner_pos;
	this->size = tree_size;
	if (size.x == 1 && size.y == 1 && size.z == 1) {// or x*y*z==1
		this->pixel = arr[(int)size.z][(int)size.y][(int)size.x];//use Vector3i to avoid casting
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
