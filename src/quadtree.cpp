#include "quadtree.hpp"

Pixel   getAverage_old(Pixel** arr, int x, int y, int width, int height) {
	//std::cout << "__getA.. " << width << "x" << height << " at " << x << ":" << y << std::endl;
	unsigned int r = 0;
	unsigned int g = 0;
	unsigned int b = 0;
	//Adds the color values for each channel.
	unsigned int counter = 0;
	for (int j = y; j < y + height; j++) {
		for (int i = x; i < x + width; i++) {
			//std::cout << arr[j][i].r << "_" << arr[j][i].g << "_" << arr[j][i].b << std::endl;
			r += (unsigned int)(arr[j][i].r);
			g += (unsigned int)(arr[j][i].g);
			b += (unsigned int)(arr[j][i].b);
			counter++;
		}
	}
	//number of pixels evaluated
	unsigned int area = width * height;
	if (area != counter) {
		std::cout << "fuck calcul" << std::endl;
		exit(1);
	}
	r = r / area;
	g = g / area;
	b = b / area;
	//std::cout << "average:\t" << r << "\t" << g << "\t" << b << std::endl;
	// Returns the color that represent the average.
	return Pixel(r,g,b);
}

Pixel   getAverage(uint8_t* data, int baseWidth, int x, int y, int width, int height) {
	//std::cout << "__getA.. " << width << "x" << height << " at " << x << ":" << y << std::endl;
	unsigned int r = 0;
	unsigned int g = 0;
	unsigned int b = 0;
	//Adds the color values for each channel.
	unsigned int counter = 0;
	for (int j = y; j < y + height; j++) {
		for (int i = x; i < x + width; i++) {
			//std::cout << arr[j][i].r << "_" << arr[j][i].g << "_" << arr[j][i].b << std::endl;
			r += (unsigned int)(data[(j * baseWidth + i) * 3 + 0]);
			g += (unsigned int)(data[(j * baseWidth + i) * 3 + 1]);
			b += (unsigned int)(data[(j * baseWidth + i) * 3 + 2]);
			counter++;
		}
	}
	//number of pixels evaluated
	unsigned int area = width * height;
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

double	measureDetail_old(Pixel** arr, int x, int y, int width, int height, Pixel average) {
	double	colorSum = 0;

	// Calculates the distance between every pixel in the region
	// and the average color. The Manhattan distance is used, and
	// all the distances are added.
	for (int j = y; j < y + height; j++) {
		for (int i = x; i < x + width; i++) {
			colorSum += std::abs(average.r - arr[j][i].r);
			colorSum += std::abs(average.g - arr[j][i].g);
			colorSum += std::abs(average.b - arr[j][i].b);
		}
	}

	// Calculates the average distance, and returns the result.
	// We divide by three, because we are averaging over 3 channels.
	return (colorSum / double(3 * width * height));
}
double	measureDetail(uint8_t* data, int baseWidth, int x, int y, int width, int height, Pixel average) {
	double	colorSum = 0;

	// Calculates the distance between every pixel in the region
	// and the average color. The Manhattan distance is used, and
	// all the distances are added.
	for (int j = y; j < y + height; j++) {
		for (int i = x; i < x + width; i++) {
			colorSum += double(std::abs(average.r - data[(j * baseWidth + i) * 3 + 0]));
			colorSum += double(std::abs(average.g - data[(j * baseWidth + i) * 3 + 1]));
			colorSum += double(std::abs(average.b - data[(j * baseWidth + i) * 3 + 2]));
		}
	}

	// Calculates the average distance, and returns the result.
	// We divide by three, because we are averaging over 3 channels.
	return (colorSum / double(3 * width * height));
}

//data[y][x];
//data[y * width + x];

QuadNode::QuadNode(uint8_t* data, int baseWidth, int x, int y, int w, int h, unsigned int threshold) {
	//std::cout << "__ QuadNode::QuadNode(data, ...)" << std::endl;

	this->children = nullptr;
	this->pixel.r = 0;
	this->pixel.g = 0;
	this->pixel.b = 0;
	this->x = x;
	this->y = y;
	this->width = w;
	this->height = h;
	if (this->width == 1 && this->height == 1) {// or w*h==1
		this->pixel.r = data[(y * baseWidth + x) * 3 + 0];
		this->pixel.g = data[(y * baseWidth + x) * 3 + 1];
		this->pixel.b = data[(y * baseWidth + x) * 3 + 2];
		this->detail = measureDetail(data, baseWidth, x, y, w, h, this->pixel);//should be 0
		if (this->detail != 0) {
			std::cout << "FUCK: 1x1 area, detail: " << this->detail << std::endl;
			exit(10);
		}
		if (w * h >= DEBUG_LEAF_AREA && DEBUG_LEAF) {
			std::cout << "new leaf: " << w << "x" << h << " at " << x << ":" << y << "\t";
			std::cout << (int)this->pixel.r << "  \t" << (int)this->pixel.g << "  \t" << (int)this->pixel.b;
			std::cout << "\t" << this->detail << std::endl;
		}
		return;
	}
	this->pixel = getAverage(data, baseWidth, x, y, w, h);
	this->detail = measureDetail(data, baseWidth, x, y, w, h, this->pixel);
	if (this->detail <= double(threshold)) {//not that much details in the area
		//this->pixel = getAverage(arr, x, y, w, h);
		if (w * h >= DEBUG_LEAF_AREA && DEBUG_LEAF) {
			std::cout << "new leaf: " << w << "x" << h << " at " << x << ":" << y << "\t";
			std::cout << (int)this->pixel.r << "  \t" << (int)this->pixel.g << "  \t" << (int)this->pixel.b;
			std::cout << "\t" << this->detail << std::endl;
		}
		//std::cout << this->pixel.r << " " << this->pixel.g << " " << this->pixel.b << std::endl;
	}
	else {

		//enough detail to split
		this->children = new QuadNode * [4];
		//this->children = (QuadNode**)malloc(sizeof(QuadNode*) * 4);
		if (!this->children) {
			std::cout << "malloc failed\n"; exit(5);
		}
		this->children[0] = nullptr;
		this->children[1] = nullptr;
		this->children[2] = nullptr;
		this->children[3] = nullptr;

		/*
			Le probleme vient des maps impaires (width et/ou height)
				+ ratio initial 1000x100 => 100x10 => 10x1 => 5x1
				+ division qui donnent des carre impaire (2,6,10,14,18,...  ie: 2x 1,3,5,7,9,11,13,...)
			separation des nodes child, certaines deviennent donc inexistentes
				-> 1122
				-> 112
		*/

		/*
		_________
		| 0 | 1 |
		|---|---|
		| 2 | 3 |
		`````````
		*/
		int	width13 = this->width / 2;//is rounded, so smaller when width is odd
		int	height23 = this->height / 2;//is rounded, so smaller when height is odd
		int width02 = this->width - width13;
		int height01 = this->height - height23;

		this->children[0] = new QuadNode(data, baseWidth, x, y, width02, height01, threshold);
		if (width13)
			this->children[1] = new QuadNode(data, baseWidth, x + width02, y, width13, height01, threshold);
		if (height23)
			this->children[2] = new QuadNode(data, baseWidth, x, y + height01, width02, height23, threshold);
		if (width13 && height23)
			this->children[3] = new QuadNode(data, baseWidth, x + width02, y + height01, width13, height23, threshold);
	}
}

QuadNode::QuadNode(Pixel** arr, int x, int y, int w, int h, unsigned int threshold) {
	//std::cout << "__ QuadNode::QuadNode(...)" << std::endl;

	this->children = nullptr;
	this->pixel.r = 0;
	this->pixel.g = 0;
	this->pixel.b = 0;
	this->x = x;
	this->y = y;
	this->width = w;
	this->height = h;
	if (this->width == 1 && this->height == 1) {// or w*h==1
		this->pixel = arr[y][x];
		this->detail = measureDetail_old(arr, x, y, w, h, this->pixel);//should be 0
		if (this->detail != 0) {
			std::cout << "1x1 area, detail: " << this->detail << std::endl;
			exit(10);
		}
		if (w * h >= DEBUG_LEAF_AREA && DEBUG_LEAF) {
			std::cout << "new leaf: " << w << "x" << h << " at " << x << ":" << y << "\t";
			std::cout << (int)this->pixel.r << "  \t" << (int)this->pixel.g << "  \t" << (int)this->pixel.b;
			std::cout << "\t" << this->detail << std::endl;
		}
		return;
	}
	this->pixel = getAverage_old(arr, x, y, w, h);
	this->detail = measureDetail_old(arr, x, y, w, h, this->pixel);
	if (this->detail <= double(threshold)) {//not that much details in the area
		//this->pixel = getAverage(arr, x, y, w, h);
		if (w * h >= DEBUG_LEAF_AREA && DEBUG_LEAF) {
			std::cout << "new leaf: " << w << "x" << h << " at " << x << ":" << y << "\t";
			std::cout << (int)this->pixel.r << "  \t" << (int)this->pixel.g << "  \t" << (int)this->pixel.b;
			std::cout << "\t" << this->detail << std::endl;
		}
		//std::cout << this->pixel.r << " " << this->pixel.g << " " << this->pixel.b << std::endl;
	} else {

		//enough detail to split
		this->children = new QuadNode * [4];
		//this->children = (QuadNode**)malloc(sizeof(QuadNode*) * 4);
		if (!this->children) {
			std::cout << "malloc failed\n"; exit(5);
		}
		this->children[0] = nullptr;
		this->children[1] = nullptr;
		this->children[2] = nullptr;
		this->children[3] = nullptr;

		/*
			Le probleme vient des maps impaires (width et/ou height)
				+ ratio initial 1000x100 => 100x10 => 10x1 => 5x1
				+ division qui donnent des carre impaire (2,6,10,14,18,...  ie: 2x 1,3,5,7,9,11,13,...)
			separation des nodes child, certaines deviennent donc inexistentes
				-> 1122
				-> 112
		*/

		/*
		_________
		| 0 | 1 |
		|---|---|
		| 2 | 3 |
		`````````
		*/
		int	width13 = this->width / 2;//is rounded, so smaller when width is odd
		int	height23 = this->height / 2;//is rounded, so smaller when height is odd
		int width02 = this->width - width13;
		int height01 = this->height - height23;

		this->children[0] = new QuadNode(arr, x, y, width02, height01, threshold);
		if (width13)
			this->children[1] = new QuadNode(arr, x + width02, y, width13, height01, threshold);
		if (height23)
			this->children[2] = new QuadNode(arr, x, y + height01, width02, height23, threshold);
		if (width13 && height23)
			this->children[3] = new QuadNode(arr, x + width02, y + height01, width13, height23, threshold);
	}
}

QuadNode::~QuadNode() {
	if (this->children) {
		for (size_t i = 0; i < 4; i++) {
			if (this->children[i])
				delete this->children[i];
		}
		delete[] this->children;
	}
}

bool		QuadNode::isLeaf() const {
	return (this->children == nullptr);
}
