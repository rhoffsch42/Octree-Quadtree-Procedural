#pragma once

#include <cstdlib>
#include <iostream>
#include <stdlib.h>

#define DEBUG_LEAF		false
#define DEBUG_LEAF_AREA	1
#define DEBUG_FILL_TOO	false
#define DEBUG_BUILD_TOO	true

class Pixel
{
public:
	Pixel() {
		r = 0;
		g = 0;
		b = 0;
	}
	Pixel(uint8_t red, uint8_t green, uint8_t blue) {
		r = red;
		g = green;
		b = blue;
	}
	uint8_t	r;
	uint8_t	g;
	uint8_t	b;
};

/*
_________
| A | B |
|---|---|
| C | D |
`````````
	result: https://docs.google.com/spreadsheets/d/17MNseuZ7234wSgKaGH4x13Qxni6oD9iySb00nfcebaU/edit?usp=sharing
*/


class QuadNode
{
public:
	QuadNode(Pixel** arr, int x, int y, int width, int height, unsigned int threshold);
	QuadNode(uint8_t* data, int baseWidth, int x, int y, int w, int h, unsigned int threshold);//For Texture::data
	~QuadNode();
	bool	isLeaf() const;

	/*
		QuadNode::browse is declared in the hpp, because of linkage problems
		https://stackoverflow.com/questions/36880799/how-i-can-pass-lambda-expression-to-c-template-as-parameter
		https://stackoverflow.com/questions/3575901/can-lambda-functions-be-templated
	*/
	template<class UnaryPredicate>
	void	browse(int threshold, UnaryPredicate p) {
		if (this->detail <= threshold || this->isLeaf()) {
			if (this->width == 0 || this->height == 0) {
				std::cout << "error with tree data\n";
				exit(1);
			}
			p(this);// lamda-expr
			/*
				func ptr works too, but args cant be transfered like in lambda-expr
				create a lambda expression using the func and args instead
			*/
		} else if (this->children) {
			for (size_t i = 0; i < 4; i++) {
				if (this->children[i])
					this->children[i]->browse(threshold, p);
			}
		}
	}

	template <typename T>
	int functionQ(T foo, int a) {
		return foo(a);
	}


	QuadNode **		children;
	Pixel			pixel;
	double			detail;
	unsigned int	x;
	unsigned int	y;
	unsigned int	width;
	unsigned int	height;
private:
};