#include "scenes.hpp"
#include <iostream>

#ifdef SCENES_DEBUG
#define SCENES_MAIN_DEBUG
#define SCENES_MAIN_INFO_DEBUG
#endif
#ifdef SCENES_MAIN_DEBUG 
#define D(x) std::cout << "[Main] " << x
#define D_(x) x
#define D_SPACER "-- main.cpp -------------------------------------------------\n"
#define D_SPACER_END "----------------------------------------------------------------\n"
#else 
#define D(x)
#define D_(x)
#define D_SPACER ""
#define D_SPACER_END ""
#endif

void	pdebug(bool reset = false) {
	const char* s = "0123456789`~!@#$%^&*()_+-=[]{}\\|;:'\",./<>?";
	static unsigned int i = 0;
	std::cout << s[i];
	i++;
	if (reset || i == 42)
		i = 0;
}

void	blitToWindow(FrameBuffer* readFramebuffer, GLenum attachmentPoint, UIPanel* panel) {
	GLuint fbo;
	if (readFramebuffer) {
		fbo = readFramebuffer->fbo;
	}
	else {
		fbo = panel->getFbo();
	}
	// glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);

	//glViewport(0, 0, manager->glfw->getWidth(), manager->glfw->getHeight());//size of the window/image or panel width ?
	glReadBuffer(attachmentPoint);
	GLenum drawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, drawBuffers);

	int w;
	int h;
	if (readFramebuffer) {
		w = readFramebuffer->getWidth();
		h = readFramebuffer->getHeight();
	}
	else if (panel->getTexture()) {
		w = panel->getTexture()->getWidth();
		h = panel->getTexture()->getHeight();
	}
	else {
		D("FUCK " << __PRETTY_FUNCTION__ << "\n");
		Misc::breakExit(2);
	}
	if (0) {
		D("copy " << w << "x" << h << "\tresized\t" << panel->_width << "x" << panel->_height << "\tat pos\t" << panel->_posX << ":" << panel->_posY << "\n");
		// << " -> " << (panel->posX + panel->width) << "x" << (panel->posY + panel->height) << "\n";
	}
	glBlitFramebuffer(0, 0, w, h, \
		panel->_posX, panel->_posY, panel->_posX2, panel->_posY2, GL_COLOR_BUFFER_BIT, GL_NEAREST);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
}

void	check_paddings() {
	//	D(sizeof(BITMAPINFOHEADER) << " = " << sizeof(BMPINFOHEADER) << "\n")
#ifdef _WIN322
	D(sizeof(BITMAPFILEHEADER) << " = " << sizeof(BMPFILEHEADER) << "\n");
	D("bfType\t" << offsetof(BMPINFOHEADERBITMAPFILEHEADER, bfType) << "\n");
	D("bfSize\t" << offsetof(BITMAPFILEHEADER, bfSize) << "\n");
	D("bfReserved1\t" << offsetof(BITMAPFILEHEADER, bfReserved1) << "\n");
	D("bfReserved2\t" << offsetof(BITMAPFILEHEADER, bfReserved2) << "\n");
	D("bfOffBits\t" << offsetof(BITMAPFILEHEADER, bfOffBits) << "\n");
#endif//_WIN32
	D("unsigned short\t" << sizeof(unsigned short) << "\n");
	D("unsigned long \t" << sizeof(unsigned long) << "\n");
	D("long          \t" << sizeof(long) << "\n");
	D("long long     \t" << sizeof(long long) << "\n");
	D("int           \t" << sizeof(int) << "\n");
	if ((sizeof(BMPFILEHEADER) != 14) || (sizeof(BMPINFOHEADER) != 40)) {
		D("Padding in structure, exiting...\n" << "\n");
		D("BMPFILEHEADER\t" << sizeof(BMPFILEHEADER) << "\n");
		D("bfType     \t" << offsetof(BMPFILEHEADER, bfType) << "\n");
		D("bfSize     \t" << offsetof(BMPFILEHEADER, bfSize) << "\n");
		D("bfReserved1\t" << offsetof(BMPFILEHEADER, bfReserved1) << "\n");
		D("bfReserved2\t" << offsetof(BMPFILEHEADER, bfReserved2) << "\n");
		D("bfOffBits\t" << offsetof(BMPFILEHEADER, bfOffBits) << "\n");
		D("-----\n");
		D("BMPINFOHEADER\t" << sizeof(BMPINFOHEADER) << "\n");
		D("biSize     \t" << offsetof(BMPINFOHEADER, biSize) << "\n");
		D("biWidth    \t" << offsetof(BMPINFOHEADER, biWidth) << "\n");
		D("biHeight\t" << offsetof(BMPINFOHEADER, biHeight) << "\n");
		D("biPlanes\t" << offsetof(BMPINFOHEADER, biPlanes) << "\n");
		D("biBitCount\t" << offsetof(BMPINFOHEADER, biBitCount) << "\n");
		D("biCompression\t" << offsetof(BMPINFOHEADER, biCompression) << "\n");
		D("biSizeImage\t" << offsetof(BMPINFOHEADER, biSizeImage) << "\n");
		D("biXPelsPerMeter\t" << offsetof(BMPINFOHEADER, biXPelsPerMeter) << "\n");
		D("biYPelsPerMeter\t" << offsetof(BMPINFOHEADER, biYPelsPerMeter) << "\n");
		D("biClrUsed\t" << offsetof(BMPINFOHEADER, biClrUsed) << "\n");
		D("biClrImportant\t" << offsetof(BMPINFOHEADER, biClrImportant) << "\n");
		Misc::breakExit(ERROR_PADDING);
	}
}

class AnchorCameraBH : public Behavior
{
	/*
		La rotation fonctionne bien sur la cam (ca a l'air),
		mais le probleme vient de l'ordre de rotation sur l'anchor.
		? Ne pas utiliser ce simple system de rotation
		? rotater par rapport au system local de l'avion
		? se baser sur une matrice cam-point-at
			https://mikro.naprvyraz.sk/docs/Coding/Atari/Maggie/3DCAM.TXT
	*/
public:
	AnchorCameraBH() : Behavior() {
		this->copyRotation = true;
	}
	void	behaveOnTarget(BehaviorManaged* target) {
		if (this->_anchor) {
			Object* speAnchor = dynamic_cast<Object*>(this->_anchor);//specialisation part
			// turn this in Obj3d to get the BP, to get the size of the ovject,
			// to position the camera relatively to the obj's size.

			Cam* speTarget = dynamic_cast<Cam*>(target);//specialisation part

			Math::Vector3	forward(0, -15, -35);
			if (this->copyRotation) {
				Math::Rotation	rot = speAnchor->local.getRot();
				forward.rotate(rot, 1);
				rot.mult(-1);
				speTarget->local.setRot(rot);
			}
			forward.mult(-1);// invert the forward to position the cam on the back, a bit up
			Math::Vector3	pos = speAnchor->local.getPos();
			pos += forward;
			speTarget->local.setPos(pos);
		}
	}
	bool	isCompatible(BehaviorManaged* target) const {
		//dynamic_cast check for Cam
		(void)target;
		return (true);
	}

	void			setAnchor(Object* anchor) {
		this->_anchor = anchor;
	}

	bool			copyRotation;
private:
	Object* _anchor;
	Math::Vector3	_offset;

};


void	fillData(uint8_t* dst, QuadNode* node, int* leafAmount, int baseWidth, bool draw_borders, int threshold, Math::Vector3 color) {
	if (!node)
		return;
	//if (node->isLeaf()) {
	if (node->detail <= threshold) {
		//(*leafAmount)++;
		//D("leaf: " << node->width << "x" << node->height << " at " << node->x << ":" << node->y << "\n")
		if (node->width == 0 || node->height == 0) {
			D("error with tree data\n"); Misc::breakExit(2);
		}
		if (node->width * node->height >= DEBUG_LEAF_AREA && DEBUG_LEAF && *leafAmount == 0 && DEBUG_FILL_TOO) {
			D("Fill new leaf: " << node->width << "x" << node->height << " at " << node->x << ":" << node->y << "\t" \
				<< (int)node->pixel.r << "  \t" << (int)node->pixel.g << "  \t" << (int)node->pixel.b << "\n");
		}
		for (unsigned int j = 0; j < node->height; j++) {
			for (unsigned int i = 0; i < node->width; i++) {
				unsigned int posx = node->x + i;
				unsigned int posy = node->y + j;
				unsigned int index = (posy * baseWidth + posx) * 3;
				//D(((posy * baseWidth + posx) * 3 + 0) << "\n")
				//D(((posy * baseWidth + posx) * 3 + 1) << "\n")
				//D(((posy * baseWidth + posx) * 3 + 2) << "\n")
				if (draw_borders && (i == 0 || j == 0)) {
					dst[index + 0] = 0;
					dst[index + 1] = 0;
					dst[index + 2] = 0;
				}
				else {
					if (1 && (posx == 0 || posy == 0)) {
						dst[index + 0] = color.x;
						dst[index + 1] = color.y;
						dst[index + 2] = color.z;
					}
					else {
						dst[index + 0] = node->pixel.r;
						dst[index + 1] = node->pixel.g;
						dst[index + 2] = node->pixel.b;
					}
				}
			}
		}
	}
	else if (node->children) {
		fillData(dst, node->children[0], leafAmount, baseWidth, draw_borders, threshold, color);
		fillData(dst, node->children[1], leafAmount, baseWidth, draw_borders, threshold, color);
		fillData(dst, node->children[2], leafAmount, baseWidth, draw_borders, threshold, color);
		fillData(dst, node->children[3], leafAmount, baseWidth, draw_borders, threshold, color);
	}
}

#define THRESHOLD 0
QuadNode* textureToQuadTree(Texture* tex) {
	uint8_t* data = tex->getData();
	unsigned int	w = tex->getWidth();
	unsigned int	h = tex->getHeight();
	Pixel** pix = new Pixel * [h];
	for (size_t j = 0; j < h; j++) {
		pix[j] = new Pixel[w];
		for (size_t i = 0; i < w; i++) {
			pix[j][i].r = data[(j * w + i) * 3 + 0];
			pix[j][i].g = data[(j * w + i) * 3 + 1];
			pix[j][i].b = data[(j * w + i) * 3 + 2];
		}
	}
	D("pixel: " << sizeof(Pixel) << "\n");

	QuadNode* root = new QuadNode(pix, 0, 0, w, h, THRESHOLD);
	D("root is leaf: " << (root->isLeaf() ? "true" : "false") << "\n");

	return root;
}


#define BORDERS_ON	true
#define BORDERS_OFF	false
class QuadTreeManager : public GameManager {
public:
	QuadTreeManager() : GameManager() {
		this->threshold = 0;
		this->draw_borders = BORDERS_OFF;
	}
	virtual ~QuadTreeManager() {}
	unsigned int	threshold;
	bool			draw_borders;
};

static void		keyCallback_quadTree(GLFWwindow* window, int key, int scancode, int action, int mods) {
	(void)window; (void)key; (void)scancode; (void)action; (void)mods;
	//D(__PRETTY_FUNCTION__ << "\n");

	if (action == GLFW_PRESS) {
		//D("GLFW_PRESS\n");
		QuadTreeManager* manager = static_cast<QuadTreeManager*>(glfwGetWindowUserPointer(window));
		if (!manager) {
			D("static_cast failed\n");
		}
		else if (manager->glfw) {
			if (key == GLFW_KEY_EQUAL || key == GLFW_KEY_KP_ADD) {
				manager->threshold++;
				manager->glfw->setTitle(std::to_string(manager->threshold).c_str());
			}
			else if ((key == GLFW_KEY_MINUS || key == GLFW_KEY_KP_SUBTRACT) && manager->threshold > 0) {
				manager->threshold--;
				manager->glfw->setTitle((std::string("Threshold: ") + std::to_string(manager->threshold)).c_str());
			}
			else if (key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER)
				manager->draw_borders = !manager->draw_borders;
		}
		//std::cout << key << "\n";
	}
}

void	scene_4Tree() {
	std::cout << "Keys :\n"
		"[ENTER]\ttoggle grid\n"
		"    [+]\tincrease threshold\n"
		"    [-]\tdecrease threshold\n";

	unsigned int winX = 1600;
	unsigned int winY = 900;
	QuadTreeManager	manager;
	manager.glfw = new Glfw(winX, winY);
	glfwSetWindowPos(manager.glfw->_window, 100, 50);
	manager.glfw->setTitle("Tests texture quadtree");
	manager.glfw->activateDefaultCallbacks(&manager);
	manager.glfw->func[GLFW_KEY_EQUAL] = keyCallback_quadTree;
	manager.glfw->func[GLFW_KEY_MINUS] = keyCallback_quadTree;
	manager.glfw->func[GLFW_KEY_ENTER] = keyCallback_quadTree;
	manager.glfw->func[GLFW_KEY_KP_ADD] = keyCallback_quadTree;
	manager.glfw->func[GLFW_KEY_KP_SUBTRACT] = keyCallback_quadTree;
	manager.glfw->func[GLFW_KEY_KP_ENTER] = keyCallback_quadTree;

	Texture* lena = new Texture(SIMPLEGL_FOLDER + "images/lena.bmp");
	Texture* flower = new Texture(SIMPLEGL_FOLDER + "images/flower.bmp");

	Texture* baseImage = flower;
	int w = baseImage->getWidth();
	int h = baseImage->getHeight();
	QuadNode* root = new QuadNode(baseImage->getData(), w, 0, 0, w, h, THRESHOLD);
	uint8_t* dataOctree = new uint8_t[w * h * 3];

	float size_coef = float(winX) / float(baseImage->getWidth()) / 2.0f;
	UIImage	uiBaseImage(baseImage);
	uiBaseImage.setPos(0, 0);
	uiBaseImage.setSize(uiBaseImage.getTexture()->getWidth() * size_coef, uiBaseImage.getTexture()->getHeight() * size_coef);

	Texture* image4Tree = new Texture(dataOctree, w, h);

	UIImage	ui4Tree(image4Tree);
	ui4Tree.setPos(baseImage->getWidth() * size_coef, 0);
	ui4Tree.setSize(ui4Tree.getTexture()->getWidth() * size_coef, ui4Tree.getTexture()->getHeight() * size_coef);

	Fps	fps144(144);
	Fps	fps60(60);
	Fps* defaultFps = &fps144;

	D("Begin while loop\n");
	int	leafAmount = 0;
	while (!glfwWindowShouldClose(manager.glfw->_window)) {
		if (defaultFps->wait_for_next_frame()) {

			glfwPollEvents();
			//glfw.updateMouse();//to do before cam's events
			//cam.events(glfw, float(defaultFps->tick));

			fillData(dataOctree, root, &leafAmount, w, manager.draw_borders, manager.threshold, Math::Vector3(0, 0, 0));
			leafAmount = -1;
			//D("leafs: " << leafAmount << "\n")
			//D("w * h * 3 = " << w << " * " << h << " * 3 = " << w * h * 3 << "\n")
			image4Tree->updateData(dataOctree, root->width, root->height);

			// printFps();
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			blitToWindow(nullptr, GL_COLOR_ATTACHMENT0, &uiBaseImage);
			blitToWindow(nullptr, GL_COLOR_ATTACHMENT0, &ui4Tree);
			glfwSwapBuffers(manager.glfw->_window);

			if (GLFW_PRESS == glfwGetKey(manager.glfw->_window, GLFW_KEY_ESCAPE))
				glfwSetWindowShouldClose(manager.glfw->_window, GLFW_TRUE);
		}
	}

	D("End while loop\n");
	D("deleting textures...\n");
}

class ProceduralManager : public GameManager {
public:
	ProceduralManager() : GameManager() {
		this->cam = nullptr;
		this->perlin = nullptr;
		this->core_amount = std::thread::hardware_concurrency();
		D(" number of cores: " << this->core_amount << "\n");
		this->seed = 888;
		std::srand(this->seed);
		this->frequency = 4;
		this->frequency = std::clamp(this->frequency, 0.1, 64.0);
		this->octaves = 12;
		this->octaves = std::clamp(this->octaves, 1, 16);
		this->flattering = 1;
		this->flattering = std::clamp(this->flattering, 0.01, 10.0);
		this->posOffsetX = 0;
		this->posOffsetY = 0;
		this->needRebuild = true;
		this->terminateThreads = false;
		this->threadsDoneWork = 0;
		//tmp
		this->mouseX = 0;
		this->mouseY = 0;
		this->island = 0;
		this->island = std::clamp(this->island, -2.0, 2.0);
		//vox
		this->player = nullptr;
		this->playerChunkX = 0;
		this->playerChunkY = 0;
		this->range_chunk_display = 5;
		this->range_chunk_memory = 41;
		this->chunk_size = 32;
		this->voxel_size = 1;
		this->polygon_mode = GL_POINT;
		this->polygon_mode = GL_LINE;
		this->polygon_mode = GL_FILL;
		this->threshold = 5;
	}
	/*
		sans opti:
		30*30*256 = 775 680 octets / chunk
		775 680 * 9 = 6 981 120 octets (block de jeu) affichés
		6 981 120 * 6 * 2 = 83 773 440 polygones
		775 680 * 49 = 38 008 320 octets en memoire
	*/

	virtual ~ProceduralManager() {}
	siv::PerlinNoise* perlin;
	unsigned int	core_amount;
	unsigned int	seed;
	double			frequency;
	int				octaves;
	double			flattering;
	int				posOffsetX;
	int				posOffsetY;
	//threads
	bool			needRebuild;
	bool			terminateThreads;
	int				threadsDoneWork;
	std::condition_variable	cv;
	std::mutex		job_mutex;
	//tmp
	double			mouseX;
	double			mouseY;
	double			island;
	//vox
	Obj3d* player;
	int	playerChunkX;
	int	playerChunkY;
	std::list<Object*>	renderVec;
	Cam* cam;
	int				range_chunk_display;
	int				range_chunk_memory;
	int				chunk_size;
	int				voxel_size;
	GLuint			polygon_mode;
	int				threshold;
};

void checksPerlin()
{
	siv::PerlinNoise perlinA(std::random_device{});
	siv::PerlinNoise perlinB;

	std::array<std::uint8_t, 256> state;
	perlinA.serialize(state);
	perlinB.deserialize(state);

	assert(perlinA.accumulatedOctaveNoise3D(0.1, 0.2, 0.3, 4)
		== perlinB.accumulatedOctaveNoise3D(0.1, 0.2, 0.3, 4));

	perlinA.reseed(1234);
	perlinB.reseed(1234);

	assert(perlinA.accumulatedOctaveNoise3D(0.1, 0.2, 0.3, 4)
		== perlinB.accumulatedOctaveNoise3D(0.1, 0.2, 0.3, 4));

	perlinA.reseed(std::mt19937{ 1234 });
	perlinB.reseed(std::mt19937{ 1234 });

	assert(perlinA.accumulatedOctaveNoise3D(0.1, 0.2, 0.3, 4)
		== perlinB.accumulatedOctaveNoise3D(0.1, 0.2, 0.3, 4));
}

void	th_buildData(uint8_t* data, ProceduralManager& manager, int yStart, int yEnd, int winX, int winY) {
	std::thread::id					threadID = std::this_thread::get_id();
	std::unique_lock<std::mutex>	job_lock(manager.job_mutex);
	const siv::PerlinNoise			perlin(manager.seed);
	int								screenCornerX, screenCornerY;
	unsigned int					data_size = (yEnd - yStart) * winX * 3;
	unsigned int					data_offset = yStart * winX * 3;
	uint8_t* newdata = new uint8_t[data_size];

	std::cout << "[" << threadID << "] started\n";
	while (!manager.terminateThreads) {
		job_lock.unlock();
		screenCornerX = -(winX / 2) + manager.posOffsetX;
		screenCornerY = -(winY / 2) + manager.posOffsetY; //invert glfw Y to match opengl image

		//D("yStart " << yStart << "\n")
		//D("yEnd " << yEnd << "\n")
		for (int y = yStart; y < yEnd; ++y) {
			job_lock.lock();
			//std::cout << y << " ";
			job_lock.unlock();
			for (int x = 0; x < winX; ++x) {

				double value;
				double posX = screenCornerX + x;//pos of the generated pixel/elevation/data
				double posY = screenCornerY + y;
				double nx = double(posX) / double(winX);//normalised 0..1
				double ny = double(posY) / double(winY);

				value = perlin.accumulatedOctaveNoise2D_0_1(nx * manager.frequency,
					ny * manager.frequency,
					manager.octaves);
				value = std::pow(value, manager.flattering);
				Math::Vector3	vec(posX, posY, 0);
				double dist = (double(vec.len()) / double(winY / 2));//normalized 0..1
				value = std::clamp(value + manager.island * (0.5 - dist), 0.0, 1.0);
				int index = ((y - yStart) * winX + x) * 3;

				uint8_t color = (uint8_t)(value * 255.0);
				if (color < 50) { // water
					newdata[index + 0] = 0;
					newdata[index + 1] = uint8_t(150.0 * std::clamp((double(color) / 50.0), 0.25, 1.0));
					newdata[index + 2] = uint8_t(255.0 * std::clamp((double(color) / 50.0), 0.25, 1.0));
				}
				else if (color < 75) { // sand
					newdata[index + 0] = 255.0 * ((double(color)) / 75.0);
					newdata[index + 1] = 200.0 * ((double(color)) / 75.0);
					newdata[index + 2] = 100.0 * ((double(color)) / 75.0);
				}
				else if (color > 200) { // snow
					newdata[index + 0] = color;
					newdata[index + 1] = color;
					newdata[index + 2] = color;
				}
				else if (color > 175) { // rocks
					newdata[index + 0] = 150.0 * value;
					newdata[index + 1] = 150.0 * value;
					newdata[index + 2] = 150.0 * value;
				}
				else {//grass
					newdata[index + 0] = 0;
					newdata[index + 1] = 200.0 * value;
					newdata[index + 2] = 100.0 * value;
				}

			}
		}

		job_lock.lock();
		// copy new data to data
		//std::cout << "[" << threadID << "] copy pixels " << (data_offset / 3) << " -> " << (data_offset + data_size ) / 3 << "\n";
		memcpy(data + data_offset, newdata, data_size);
		manager.threadsDoneWork++;
		//std::cout << "[" << threadID << "] done work lines " << yStart << " -> " << yEnd << "\n";
		manager.cv.wait(job_lock, [&manager] { return ((manager.needRebuild && manager.threadsDoneWork == 0) || manager.terminateThreads); });//wait until condition
		//std::cout << "[" << threadID << "] awakened\n";
	}
}

void	printPerlinSettings(ProceduralManager& manager) {
	std::cout << "pos offset\t" << manager.posOffsetX << ":" << manager.posOffsetY << "\n"
		<< "frequency \t" << manager.frequency << "\n"
		<< "flattering\t" << manager.flattering << "\n"
		<< "island    \t" << manager.island << "\n---------------\n";
}

void	scene_procedural() {
	std::cout << "Keys :\n"
		"      [KP_0]\tprint settings\n"
		"[KP_7][KP_4]\tfrequency +-0.1 [0.1 : 64.0]\n"
		"[KP_8][KP_5]\tflattering +-0.1 [0.01 : 10.0]\n"
		"[KP_9][KP_6]\tisland +-0.01 [-2 : 2]\n"
		"    [Arrows]\tpos offset\n"
		"    [ESCAPE]\texit\n";

#ifndef INITS
	unsigned int winX = 400;
	unsigned int winY = 400;
	checksPerlin();
	ProceduralManager	manager;
	manager.glfw = new Glfw(winX, winY);
	glfwSetWindowPos(manager.glfw->_window, 100, 50);
	manager.glfw->toggleCursor();
	manager.glfw->setTitle("Tests procedural");
	manager.glfw->activateDefaultCallbacks(&manager);

	const siv::PerlinNoise perlin(manager.seed);
	manager.frequency = double(winY) / 75.0;
	uint8_t* data = new uint8_t[winX * winY * 3];
	Texture* image = new Texture(data, winX, winY);

	UIImage	uiImage(image);
	uiImage.setPos(0, 0);
	uiImage.setSize(uiImage.getTexture()->getWidth(), uiImage.getTexture()->getHeight());

	Fps	fps144(144);
	Fps	fps60(60);
	Fps* defaultFps = &fps144;

	unsigned int thread_amount = manager.core_amount - 1;
	std::thread* threads_list = new std::thread[thread_amount];
	for (unsigned int i = 0; i < thread_amount; i++) {
		int start = ((winY * (i + 0)) / thread_amount);
		int end = ((winY * (i + 1)) / thread_amount);
		//D(start << "\t->\t" << end << "\t" << end - start << "\n")
		threads_list[i] = std::thread(th_buildData, std::ref(data), std::ref(manager), start, end, winX, winY);
	}
#endif INITS

	D("Begin while loop\n");
	while (!glfwWindowShouldClose(manager.glfw->_window)) {
		if (defaultFps->wait_for_next_frame()) {

			glfwPollEvents();
			//glfw.updateMouse();//to do before cam's events
			//cam.events(glfw, float(defaultFps->tick));

			// printFps();

			manager.job_mutex.lock();
			if (manager.threadsDoneWork == thread_amount) {
				image->updateData(data, winX, winY);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				blitToWindow(nullptr, GL_COLOR_ATTACHMENT0, &uiImage);
				glfwSwapBuffers(manager.glfw->_window);
				manager.threadsDoneWork = 0;
				manager.needRebuild = false;
				printPerlinSettings(manager);
			}
			manager.job_mutex.unlock();

#ifndef KEY_EVENTSSS
			int mvtSpeed = 5;
			if (!manager.needRebuild) {
				if (GLFW_PRESS == glfwGetKey(manager.glfw->_window, GLFW_KEY_UP)) {
					manager.posOffsetY += mvtSpeed;
					manager.needRebuild = true;
				}
				if (GLFW_PRESS == glfwGetKey(manager.glfw->_window, GLFW_KEY_DOWN)) {
					manager.posOffsetY -= mvtSpeed;
					manager.needRebuild = true;
				}

				if (GLFW_PRESS == glfwGetKey(manager.glfw->_window, GLFW_KEY_RIGHT)) {
					manager.posOffsetX += mvtSpeed;
					manager.needRebuild = true;
				}
				if (GLFW_PRESS == glfwGetKey(manager.glfw->_window, GLFW_KEY_LEFT)) {
					manager.posOffsetX -= mvtSpeed;
					manager.needRebuild = true;
				}

				if (GLFW_PRESS == glfwGetKey(manager.glfw->_window, GLFW_KEY_KP_7)) {
					manager.frequency += 0.1;
					manager.frequency = std::clamp(manager.frequency, 0.1, 64.0);
					manager.needRebuild = true;
				}
				if (GLFW_PRESS == glfwGetKey(manager.glfw->_window, GLFW_KEY_KP_4)) {
					manager.frequency -= 0.1;
					manager.frequency = std::clamp(manager.frequency, 0.1, 64.0);
					manager.needRebuild = true;
				}

				if (GLFW_PRESS == glfwGetKey(manager.glfw->_window, GLFW_KEY_KP_8)) {
					manager.flattering += 0.1;
					manager.flattering = std::clamp(manager.flattering, 0.01, 10.0);
					manager.needRebuild = true;
				}
				if (GLFW_PRESS == glfwGetKey(manager.glfw->_window, GLFW_KEY_KP_5)) {
					manager.flattering -= 0.1;
					manager.flattering = std::clamp(manager.flattering, 0.01, 10.0);
					manager.needRebuild = true;
				}

				if (GLFW_PRESS == glfwGetKey(manager.glfw->_window, GLFW_KEY_KP_9)) {
					manager.island += 0.05;
					manager.island = std::clamp(manager.island, -2.0, 2.0);
					manager.needRebuild = true;
				}
				if (GLFW_PRESS == glfwGetKey(manager.glfw->_window, GLFW_KEY_KP_6)) {
					manager.island -= 0.05;
					manager.island = std::clamp(manager.island, -2.0, 2.0);
					manager.needRebuild = true;
				}
				if (GLFW_PRESS == glfwGetKey(manager.glfw->_window, GLFW_KEY_KP_0)) {
					printPerlinSettings(manager);
				}
				if (manager.needRebuild == true) {
					manager.cv.notify_all();
				}
			}
			if (GLFW_PRESS == glfwGetKey(manager.glfw->_window, GLFW_KEY_ESCAPE)) {
				glfwSetWindowShouldClose(manager.glfw->_window, GLFW_TRUE);
				manager.terminateThreads = true;
				manager.cv.notify_all();
			}
#endif
		}
	}

	manager.cv.notify_all();
	for (unsigned int i = 0; i < thread_amount; i++) {
		threads_list[i].join();
	}
	delete[] threads_list;

	D("End while loop\n");
	D("deleting textures...\n");
}

uint8_t* generatePerlinNoise(ProceduralManager& manager, int posX, int posY, int width, int height) {
	uint8_t* data = new uint8_t[width * height * 3];
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			double value;
			double nx = double(posX + x) / double(500);//normalised 0..1
			double ny = double(posY + y) / double(500);

			value = manager.perlin->accumulatedOctaveNoise2D_0_1(nx * manager.frequency,
				ny * manager.frequency,
				manager.octaves);

			value = std::pow(value, manager.flattering);
			Math::Vector3	vec(x, y, 0);
			double dist = (double(vec.len()) / double(WINY / 2));//normalized 0..1
			value = std::clamp(value + manager.island * (0.5 - dist), 0.0, 1.0);

			uint8_t color = (uint8_t)(value * 255.0);
			int index = (y * width + x) * 3;
			data[index + 0] = color;
			data[index + 1] = color;
			data[index + 2] = color;
		}
	}
	return data;
}
