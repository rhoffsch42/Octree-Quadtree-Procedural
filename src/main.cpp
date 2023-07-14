#include <compiler_settings.h>

#if 1
#include <iostream>
#include <chrono>
#include "trees.h"
class SharedObj
{
public:
	SharedObj() {
		std::cout << "Constructor\n";
		this->_array = nullptr;
		this->_size = 0;
	}
	~SharedObj() {
		std::cout << "Destructor\n";
		this->deleteData();
	}
	void	buildArray(int n, unsigned int amount = 1) {
		this->deleteData();
		this->_array = new int[amount];
		this->_size = amount;
		for (unsigned int i = 0; i < this->_size; i++) {
			this->_array[i] = n;
		}
	}
	void	printData() const {
		if (this->_array) {
			for (unsigned int i = 0; i < this->_size; i++) {
				std::cout << this->_array[i] << " ";
			} std::cout << std::endl;
		}
	}
	void	deleteData() {
		if (this->_array) {
			delete this->_array;
			this->_array = nullptr;
		}
	}
private:
	int*			_array;
	unsigned int	_size;
};

void th_1(std::shared_ptr<SharedObj> p) {
	using namespace std::chrono_literals;
	{
		static std::mutex io_mutex; // the static is shared between threads, allowing the local declaration of the mutex to control IO
		std::lock_guard<std::mutex> lk(io_mutex);
		std::cout << "thread : " << p.use_count() << std::endl;
	}
	std::this_thread::sleep_for(1s);
}
void	test_shared_ptr() {
	{
		using namespace std::chrono_literals;
		SharedObj* o1 = new SharedObj();
		SharedObj* o2 = new SharedObj();
		std::shared_ptr<SharedObj>	p1_1 = std::make_shared<SharedObj>(*o1);
		std::cout << p1_1.use_count() << std::endl;
		std::shared_ptr<SharedObj>	p1_2 = p1_1;
		std::cout << p1_1.use_count() << std::endl;
		std::weak_ptr<SharedObj> wp1 = p1_1;
		std::cout << "wp1 : " << wp1.use_count() << std::endl;

		std::thread t1{ th_1, p1_1 }, t2{ th_1, p1_1 }, t3{ th_1, p1_1 };
		std::cout << "shared with 3 threds\n";
		std::cout << "wp1 : " << wp1.use_count() << std::endl;
		std::this_thread::sleep_for(1s);
		std::cout << "threads should be off now\n";
		p1_1.reset();
		std::cout << "1 resets\n";
		std::cout << "wp1 : " << wp1.use_count() << std::endl;

		std::shared_ptr<SharedObj>	p2_1 = std::make_shared<SharedObj>(*o2);
		std::weak_ptr<SharedObj> wp2 = p2_1;
		std::cout << "wp2 : " << wp2.use_count() << std::endl;

		std::cout << "reassigning p2_1 with o1 ptr\n";
		p2_1 = wp1.lock();// It creates and returns a shared_ptr that can be empty if use_count() = 0. weak_ptr::lock() is equivalent to open_schrodinger_cat_box().
		std::cout << "wp1 : " << wp1.use_count() << std::endl;
		std::cout << "wp2 : " << wp2.use_count() << std::endl;

		t1.join(); t2.join(); t3.join();
		std::cout << "end\n";
	}
	std::exit(0);
}
#endif // test shared ptr

#if 1
#include "simplegl.h"
#include "trees.h"
#include <typeinfo>
#include <thread>
#include <chrono>
using namespace std::chrono_literals;

#ifdef TREES_DEBUG
 //#define TREES_MAIN_DEBUG
 #define TREES_MAIN_INFO_DEBUG
#endif
#ifdef TREES_MAIN_DEBUG 
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
#ifdef TREES_MAIN_INFO_DEBUG
 #define INFO(x) std::cout << "[INFO] " << x
 #define INFO_(x) x
#else 
 #define INFO(x)
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
		D("FUCK " << __PRETTY_FUNCTION__ << "\n")
		Misc::breakExit(2);
	}
	if (0) {
		D("copy " << w << "x" << h << "\tresized\t" << panel->_width << "x" << panel->_height << "\tat pos\t" << panel->_posX << ":" << panel->_posY << "\n")
		// << " -> " << (panel->posX + panel->width) << "x" << (panel->posY + panel->height) << "\n";
	}
	glBlitFramebuffer(0, 0, w, h, \
		panel->_posX, panel->_posY, panel->_posX2, panel->_posY2, GL_COLOR_BUFFER_BIT, GL_NEAREST);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
}

void	check_paddings() {
	//	D(sizeof(BITMAPINFOHEADER) << " = " << sizeof(BMPINFOHEADER) << "\n")
#ifdef _WIN322
	D(sizeof(BITMAPFILEHEADER) << " = " << sizeof(BMPFILEHEADER) << "\n")
	D("bfType\t" << offsetof(BMPINFOHEADERBITMAPFILEHEADER, bfType) << "\n")
	D("bfSize\t" << offsetof(BITMAPFILEHEADER, bfSize) << "\n")
	D("bfReserved1\t" << offsetof(BITMAPFILEHEADER, bfReserved1) << "\n")
	D("bfReserved2\t" << offsetof(BITMAPFILEHEADER, bfReserved2) << "\n")
	D("bfOffBits\t" << offsetof(BITMAPFILEHEADER, bfOffBits) << "\n")
#endif//_WIN32
	D("unsigned short\t" << sizeof(unsigned short) << "\n")
	D("unsigned long \t" << sizeof(unsigned long) << "\n")
	D("long          \t" << sizeof(long) << "\n")
	D("long long     \t" << sizeof(long long) << "\n")
	D("int           \t" << sizeof(int) << "\n")
	if ((sizeof(BMPFILEHEADER) != 14) || (sizeof(BMPINFOHEADER) != 40)) {
		D("Padding in structure, exiting...\n" << "\n")
		D("BMPFILEHEADER\t" << sizeof(BMPFILEHEADER) << "\n")
		D("bfType     \t" << offsetof(BMPFILEHEADER, bfType) << "\n")
		D("bfSize     \t" << offsetof(BMPFILEHEADER, bfSize) << "\n")
		D("bfReserved1\t" << offsetof(BMPFILEHEADER, bfReserved1) << "\n")
		D("bfReserved2\t" << offsetof(BMPFILEHEADER, bfReserved2) << "\n")
		D("bfOffBits\t" << offsetof(BMPFILEHEADER, bfOffBits) << "\n")
		D("-----\n")
		D("BMPINFOHEADER\t" << sizeof(BMPINFOHEADER) << "\n")
		D("biSize     \t" << offsetof(BMPINFOHEADER, biSize) << "\n")
		D("biWidth    \t" << offsetof(BMPINFOHEADER, biWidth) << "\n")
		D("biHeight\t" << offsetof(BMPINFOHEADER, biHeight) << "\n")
		D("biPlanes\t" << offsetof(BMPINFOHEADER, biPlanes) << "\n")
		D("biBitCount\t" << offsetof(BMPINFOHEADER, biBitCount) << "\n")
		D("biCompression\t" << offsetof(BMPINFOHEADER, biCompression) << "\n")
		D("biSizeImage\t" << offsetof(BMPINFOHEADER, biSizeImage) << "\n")
		D("biXPelsPerMeter\t" << offsetof(BMPINFOHEADER, biXPelsPerMeter) << "\n")
		D("biYPelsPerMeter\t" << offsetof(BMPINFOHEADER, biYPelsPerMeter) << "\n")
		D("biClrUsed\t" << offsetof(BMPINFOHEADER, biClrUsed) << "\n")
		D("biClrImportant\t" << offsetof(BMPINFOHEADER, biClrImportant) << "\n")
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

#define BORDERS_ON	true
#define BORDERS_OFF	false
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
				<< (int)node->pixel.r << "  \t" << (int)node->pixel.g << "  \t" << (int)node->pixel.b << "\n")
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
	//D(__PRETTY_FUNCTION__ << "\n")

	if (action == GLFW_PRESS) {
		//D("GLFW_PRESS\n")
		QuadTreeManager* manager = static_cast<QuadTreeManager*>(glfwGetWindowUserPointer(window));
		if (!manager) {
			D("static_cast failed\n")
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

	D("Begin while loop\n")
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

	D("End while loop\n")
	D("deleting textures...\n")
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
	std::list<Object*>	renderlist;
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
	uint8_t*						newdata = new uint8_t[data_size];

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
				int index = ((y-yStart) * winX + x) * 3;

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

#define STORAGE_LIST		0
#define STORAGE_ARRAY		1
#define MAX_STORAGE_MODE	2
class OctreeManager : public QuadTreeManager
{
public:
	OctreeManager() : QuadTreeManager() {
		this->seed = 777;
		std::srand(this->seed);
		this->perlin = new siv::PerlinNoise(this->seed);
		this->ps = new PerlinSettings(*this->perlin);

		this->ps->frequency = 6;
		this->ps->octaves = 6;//def 6
		this->ps->flattering = 0.6;//1 for no impact
		this->ps->island = 0;// 0 for no impact
		this->ps->heightCoef = 0.5;

		//default
		this->ps->flattering = PERLIN_DEF_FLATTERING;
		this->ps->island = PERLIN_DEF_ISLAND;

		this->ps->frequency = std::clamp(this->ps->frequency, 0.1, 64.0);
		this->ps->octaves = std::clamp((int)this->ps->octaves, 1, 16);
		this->ps->flattering = std::clamp(this->ps->flattering, 0.1, 5.0);
		this->ps->island = std::clamp(this->ps->island, 0.1, 5.0);

		this->minimapPanels = nullptr;
		this->playerMinimap = nullptr;
		this->minimapCoef = 0.8f;
		this->player = nullptr;
		this->playerSpeed = 20;//unit/s
		this->shiftPressed = false;
		int s = 32;
		this->chunk_size = Math::Vector3(s, s, s);
		int	g = 5;
		int	d = 3;// g * 2 / 3;
		this->gridSize = Math::Vector3(g, g, g);
		this->renderedGridSize = Math::Vector3(d, d, d);

		this->minimapCenterX = this->chunk_size.x * this->gridSize.x * 0.5f * this->minimapCoef;
		this->minimapCenterZ = this->chunk_size.z * this->gridSize.z * 0.5f * this->minimapCoef;

		this->polygon_mode = GL_POINT;
		this->polygon_mode = GL_LINE;
		this->polygon_mode = GL_FILL;
		this->threshold = 0;
		this->thresholdUpdated = false;

		this->cpuThreadAmount = std::thread::hardware_concurrency();
		if (this->cpuThreadAmount < 3) {
			D("Not enough threads: " << this->cpuThreadAmount << "\n")
			Misc::breakExit(-44);
		}
	}
	~OctreeManager() {}

	unsigned int		seed;
	siv::PerlinNoise*	perlin;
	PerlinSettings*		ps;

	std::list<Object*>	renderlist;
	std::list<Object*>	renderlistOctree;
	std::list<Object*>	renderlistGrid;
	std::list<Object*>	renderlistVoxels[6];//6faces
	std::list<Object*>	renderlistChunk;
	Object**			renderArrayChunk = nullptr;
	unsigned int		renderArrayChunk_maxsize;
	std::list<Object*>	renderlistSkybox;

	UIPanel***			minimapPanels;
	UIImage*			playerMinimap;
	float				minimapCoef;
	int					minimapCenterX;
	int					minimapCenterZ;

	GLuint				polygon_mode;
	Obj3d*				player;
	float				playerSpeed;
	bool				shiftPressed;
	Math::Vector3		chunk_size;
	Math::Vector3		gridSize;
	Math::Vector3		renderedGridSize;
	ChunkGenerator*		generator;
	ChunkGrid*			grid;

	double				threshold;
	bool				thresholdUpdated;
	uint8_t				storageMode = STORAGE_LIST; // 0 = STORAGE_LIST 1 = STORAGE_ARRAY

	unsigned int		cpuThreadAmount;
};

void	scene_benchmarks() {
	char input[100];
	std::cout << "Choose:\n";
	std::cout << "\t1 - glDrawArrays\n";
	std::cout << "\t2 - glDrawElements\n";
	std::cout << "\t3 - glDrawArraysInstanced\n";
	std::cout << "\t4 - glDrawElementsInstanced\n";
	input[0] = '1';	input[1] = 0;
	std::cin >> input;
	OctreeManager	m;
	m.glfw = new Glfw(WINX, WINY);

	//Blueprint global settings
	Obj3dBP::defaultSize = 1;
	Obj3dBP::rescale = true;
	Obj3dBP::center = false;
	Obj3dBP::defaultDataMode = BP_INDICES;
	if (input[0] % 2 == 1)
		Obj3dBP::defaultDataMode = BP_LINEAR;
	Obj3dBP		cubebp(SIMPLEGL_FOLDER + "obj3d/cube.obj");
	Obj3dBP		lambobp(SIMPLEGL_FOLDER + "obj3d/lambo/Lamborginhi_Aventador_OBJ/Lamborghini_Aventador_no_collider.obj");

	Texture* lenatex = new Texture(SIMPLEGL_FOLDER + "images/lena.bmp");
	Texture* lambotex = new Texture(SIMPLEGL_FOLDER + "obj3d/lambo/Lamborginhi_Aventador_OBJ/Lamborginhi_Aventador_diffuse.bmp");
	Texture* tex_skybox = new Texture(SIMPLEGL_FOLDER + "images/skybox4.bmp");

	Obj3dPG		rendererObj3d(SIMPLEGL_FOLDER + OBJ3D_VS_FILE, SIMPLEGL_FOLDER + OBJ3D_FS_FILE);
	Obj3dIPG	rendererObj3dInstanced(SIMPLEGL_FOLDER + OBJ3D_INSTANCED_VS_FILE, SIMPLEGL_FOLDER + OBJ3D_FS_FILE);
	SkyboxPG	rendererSkybox(SIMPLEGL_FOLDER + CUBEMAP_VS_FILE, SIMPLEGL_FOLDER + CUBEMAP_FS_FILE);

	Skybox		skybox(*tex_skybox, rendererSkybox);
	m.renderlistSkybox.push_back(&skybox);

	Cam		cam(m.glfw->getWidth(), m.glfw->getHeight());
	cam.local.setPos(-5, -5, -5);
	cam.speed = 5;
	cam.printProperties();
	cam.lockedMovement = false;
	cam.lockedOrientation = false;
	//m.glfw->setMouseAngle(-1);//?
	m.cam = &cam;

	Obj3dPG* renderer = &rendererObj3d;
	if (input[0] >= '3') {
		renderer = &rendererObj3dInstanced;
		D("Instanced rendering enabled\n")
	}

	//texture edit
	uint8_t* tex_data = lenatex->getData();
	unsigned int w = lenatex->getWidth();
	unsigned int h = lenatex->getHeight();
	int	maxs = w * h * 3;
	for (int i = 0; i < maxs; i++) {
		if (i % 3 == 0)
			tex_data[i] = 255;
	}
	lenatex->updateData(tex_data, w, h);


	if (1) {//cubes
		for (size_t i = 0; i < 10; i++) {
			for (size_t j = 0; j < 10; j++) {
				Obj3d* cube = new Obj3d(cubebp, *renderer);
				cube->local.setPos(-10 + float(i) * -1.1, j * 1.1, 0);
				cube->local.setScale(1, 1, 1);
				cube->setColor(2.5 * i, 0, 0);
				cube->displayTexture = true;
				cube->setTexture(lenatex);
				cube->setPolygonMode(GL_FILL);
				m.renderlist.push_back(cube);
			}
		}
	}
	if (1) {//lambos
		for (size_t k = 0; k < 1; k++) {
			for (size_t j = 0; j < 50; j++) {
				for (size_t i = 0; i < 50; i++) {
					Obj3d* lambo = new Obj3d(lambobp, *renderer);
					lambo->local.setPos(i * 3, j * 1.5, k);
					lambo->local.enlarge(5, 5, 5);
					lambo->setColor(222, 0, 222);
					lambo->setTexture(lambotex);
					lambo->displayTexture = true;
					lambo->setPolygonMode(GL_FILL);
					m.renderlist.push_back(lambo);
				}
			}
		}
	}

	Fps	fps(135);
	Fps* defaultFps = &fps;

	Obj3d* frontobj = static_cast<Obj3d*>(m.renderlist.front());
	Obj3dBP* frontbp = frontobj->getBlueprint();
#ifndef SHADER_INIT
	for (auto o : m.renderlist)
		o->update();
	glUseProgram(renderer->_program);
	glUniform1i(renderer->_dismod, 0);// 1 = display plain_color, 0 = vertex_color
	glUniform3f(renderer->_plain_color, 200, 0, 200);

	glUniform1f(renderer->_tex_coef, 1.0f);
	glBindVertexArray(frontbp->getVao());
	glBindTexture(GL_TEXTURE_2D, lambotex->getId());
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	int	vertices_amount = frontbp->getPolygonAmount() * 3;
#endif
#define BUFFER_OFFSET(i) ((char *)NULL + (i))
	glfwSwapInterval(0);//0 = disable vsynx
	glDisable(GL_CULL_FACE);
	D("renderlist: " << m.renderlist.size() << "\n")
	D("Begin while loop\n")
	while (!glfwWindowShouldClose(m.glfw->_window)) {
		if (defaultFps->wait_for_next_frame()) {
			m.glfw->setTitle(std::to_string(defaultFps->getFps()) + " fps");

			glfwPollEvents();
			m.glfw->updateMouse();//to do before cam's events
			m.cam->events(*m.glfw, float(defaultFps->getTick()));

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			if (input[0] >= '3') {//instanced
				renderer->renderObjects(m.renderlist, cam, PG_FORCE_DRAW);
			}
			else if (1) {
				renderer->renderObjects(m.renderlist, cam, PG_FORCE_DRAW);
			}
			else {//optimized mass renderer (non instanced) non moving objects, same BP
				Math::Matrix4	VPmatrix(cam.getProjectionMatrix());
				Math::Matrix4	Vmatrix = cam.getViewMatrix();
				VPmatrix.mult(Vmatrix);

				glUseProgram(renderer->_program);
				if (frontobj->displayTexture && frontobj->getTexture() != nullptr) {
					glUniform1f(renderer->_tex_coef, 1.0f);
					glActiveTexture(GL_TEXTURE0);//required for some drivers
					glBindTexture(GL_TEXTURE_2D, frontobj->getTexture()->getId());
				}
				else { glUniform1f(renderer->_tex_coef, 0.0f); }
				glBindVertexArray(frontbp->getVao());

				for (Object* object : m.renderlist) {
					Math::Matrix4	MVPmatrix(VPmatrix);
					MVPmatrix.mult(object->getWorldMatrix());
					MVPmatrix.setOrder(COLUMN_MAJOR);

					glUniformMatrix4fv(renderer->_mat4_mvp, 1, GL_FALSE, MVPmatrix.getData());
					//if (Obj3dBP::defaultDataMode == BP_LINEAR)
					if (frontbp->getDataMode() == BP_LINEAR)
						glDrawArrays(GL_TRIANGLES, 0, vertices_amount);
					else {// should be BP_INDICES
						glDrawElements(GL_TRIANGLES, vertices_amount, GL_UNSIGNED_INT, 0);
					}

				}
				glBindVertexArray(0);
				glBindTexture(GL_TEXTURE_2D, 0);
			}

			rendererSkybox.renderObjects(m.renderlistSkybox, cam, PG_FORCE_DRAW);//this will unbind obj3d pg vao and texture
			glfwSwapBuffers(m.glfw->_window);

			if (GLFW_PRESS == glfwGetKey(m.glfw->_window, GLFW_KEY_ESCAPE))
				glfwSetWindowShouldClose(m.glfw->_window, GLFW_TRUE);
		}
	}

	D("End while loop\n")
}

#define M_THREADS_BUILDERS		10
#define M_PERLIN_GENERATION		1
#define M_OCTREE_OPTIMISATION	1
#define M_DISPLAY_BLACK			1
#define M_DRAW_BOX_GRID			1
#define M_DRAW_GRID_CHUNK		0
#define M_DRAW_MINIMAP			0
#define M_MERGE_CHUNKS			0

void	printSettings(OctreeManager& m) {
	INFO("cpu threads amount: " << m.cpuThreadAmount << "\n");
	#if M_MERGE_CHUNKS
	INFO("M_MERGE_CHUNKS : true\n");
	#else
	INFO("M_MERGE_CHUNKS : false\n");
	#endif
	#if M_DRAW_MINIMAP
	INFO("M_DRAW_MINIMAP : true\n");
	#else
	INFO("M_DRAW_MINIMAP : false\n");
	#endif
	#if M_DRAW_GRID_CHUNK
	INFO("M_DRAW_GRID_CHUNK : true\n");
	#else
	INFO("M_DRAW_GRID_CHUNK : false\n");
	#endif
}

static void		keyCallback_ocTree(GLFWwindow* window, int key, int scancode, int action, int mods) {
	(void)window; (void)key; (void)scancode; (void)action; (void)mods;
	//D(__PRETTY_FUNCTION__ << "\n")

	float	move = 150;
	OctreeManager* manager = static_cast<OctreeManager*>(glfwGetWindowUserPointer(window));
	if (!manager) {
		D("static_cast failed\n")
		return;
	}
	if (action == GLFW_PRESS) {
		D("GLFW_PRESS:" << key << "\n")
		if (manager->glfw) {
			if (key == GLFW_KEY_EQUAL) {
				double inc = std::clamp(manager->threshold * 0.05, 1.0, 5.0);
				manager->threshold = std::clamp(manager->threshold + inc, 1.0, 100.0);
				manager->thresholdUpdated = true;
			} else if (key == GLFW_KEY_MINUS && manager->threshold > 0) {
				double inc = std::clamp(manager->threshold * (1.0 / 0.05), 1.0, 5.0);
				manager->threshold = std::clamp(manager->threshold - inc, 1.0, 100.0);
				manager->thresholdUpdated = true;
			} else if (key == GLFW_KEY_ENTER) {
				manager->polygon_mode++;
				manager->polygon_mode = GL_POINT + (manager->polygon_mode % 3);
				for (std::list<Object*>::iterator it = manager->renderlistChunk.begin(); it != manager->renderlistChunk.end(); ++it) {
					((Obj3d*)(*it))->setPolygonMode(manager->polygon_mode);
				}
			} else if (key == GLFW_KEY_X) {
				manager->cam->local.translate(move, 0, 0);
			} else if (key == GLFW_KEY_Y) {
				manager->cam->local.translate(0, move, 0);
			} else if (key == GLFW_KEY_Z) {
				manager->cam->local.translate(0, 0, move);
			} else if (key == GLFW_KEY_LEFT_SHIFT) {
				manager->shiftPressed = true;
				manager->cam->speed = manager->playerSpeed * 3;
			} else if (key == GLFW_KEY_T) {
				manager->storageMode = (manager->storageMode + 1) % MAX_STORAGE_MODE;
				INFO("rendering with " << (manager->storageMode == STORAGE_LIST ? "list\n" : "array\n"));
			}
		}
	}
	else if (action == GLFW_RELEASE) {
		D("GLFW_RELEASE:" << key << "\n")
		if (manager->glfw) {
			if (key == GLFW_KEY_LEFT_SHIFT) {
				D("GLFW_KEY_LEFT_SHIFT\n")
				manager->shiftPressed = false;
				manager->cam->speed = manager->playerSpeed;
			}
		}
	}
}

unsigned int	grabObjects(ChunkGenerator& generator, ChunkGrid& grid, OctreeManager& manager, Obj3dBP& cubebp, Obj3dPG& obj3d_prog, Texture* tex) {
	D(__PRETTY_FUNCTION__ << "\n")
	INFO(grid.getGridChecks() << "\n");

#if M_DRAW_MINIMAP == 1
		//assemble minimap (currently inverted on the Y axis)
		float	zmax = generator.gridSize.z * generator.chunkSize.z;//for gl convertion
	for (unsigned int k = 0; k < generator.gridSize.z; k++) {
		for (unsigned int i = 0; i < generator.gridSize.x; i++) {
			if (generator.heightMaps[k][i] && generator.heightMaps[k][i]->panel) {//at this point, the hmap might not be generated yet, nor the panel
				float posx = i * generator.chunkSize.x + 0;
				float posz = k * generator.chunkSize.z + 0;
				float posx2 = posx + generator.chunkSize.x;
				float posz2 = posz + generator.chunkSize.z;
				generator.heightMaps[k][i]->panel->setPos(posx * manager.minimapCoef, (zmax - posz) * manager.minimapCoef);//top left corner
				generator.heightMaps[k][i]->panel->setPos2(posx2 * manager.minimapCoef, (zmax - posz2) * manager.minimapCoef);//bot right corner
			}
		}
	}
	//blitToWindow(nullptr, GL_COLOR_ATTACHMENT0, &uiBaseImage);
#endif

	//todo use smart pointers (not for renderlistVoxels)
	for (auto i : manager.renderlist)
		delete i;
	for (auto i : manager.renderlistOctree)
		delete i;
	for (auto i : manager.renderlistGrid)
		delete i;
	//dont delete obj in renderlistVoxels as they're all present and already deleted in renderlist

	manager.renderlist.clear();
	manager.renderlistOctree.clear();
	manager.renderlistGrid.clear();
	for (size_t i = 0; i < 6; i++)
		manager.renderlistVoxels[i].clear();
	manager.renderlistChunk.clear();

	unsigned int	hiddenBlocks = 0;
	unsigned int	total_polygons = 0;
	unsigned int	cubgrid = 0;

	float	scale_coef = 0.99f;
	float	scale_coe2 = 0.95f;
	//scale_coef = 1;
	std::stringstream polygon_debug;
	polygon_debug << "chunks polygons: ";

	// rendered box
	Math::Vector3	startRendered = grid.getRenderedGridIndex();
	Math::Vector3	endRendered = startRendered + grid.getRenderedSize();
#if 0 // all the grid
	startRendered = Math::Vector3();
	endRendered = Math::Vector3(generator.gridSize);
#endif
	D(" > rendered " << startRendered << " -> " << endRendered << "\n")

	if (M_DRAW_BOX_GRID) {
		Math::Vector3 pos = grid.getWorldIndex();
		pos.scale(grid.getChunkSize());
		Math::Vector3 scale = grid.getSize();
		scale.scale(grid.getChunkSize());
		Obj3d* cubeGrid = new Obj3d(cubebp, obj3d_prog);
		cubeGrid->setColor(255, 0, 0);
		cubeGrid->local.setPos(pos);
		cubeGrid->local.setScale(scale);
		cubeGrid->setPolygonMode(GL_LINE);
		cubeGrid->displayTexture = false;
		manager.renderlistGrid.push_back(cubeGrid);
		cubgrid++;
	}
	unsigned int sizeArray = 0;
	/*
		0 = no tesselation, taking smallest voxels (size = 1)
		5 = log2(32), max level, ie the size of a chunk
	*/
	unsigned int tesselation_lvl = -1;
	if (1) {// actual grabbing
		grid.glth_loadChunksToGPU();
		for (unsigned int tessLvl = 0; tessLvl < TESSELATION_LVLS; tessLvl++) {
			//INFO("Tesselation lvl[" << tessLvl << "] Pushing Chunks...\n");
			grid.pushRenderedChunks(&manager.renderlistChunk, tessLvl);
			sizeArray = grid.pushRenderedChunks(manager.renderArrayChunk, tessLvl, sizeArray);
		}
		if (manager.renderlistChunk.size() != sizeArray) {
			D("Warning: difference between list and array size for rendered chunks : " << manager.renderlistChunk.size() << " != " << sizeArray << "\n")
				//std::exit(1);
		}

		//for (auto x = 0; x < sizeArray; x++) {
		//	Obj3d* o = dynamic_cast<Obj3d*>(manager.renderArrayChunk[x]);
		//	o->setPolygonMode(GL_LINE);
		//}

		//merge BPs
		if (M_MERGE_CHUNKS) {//merge BPs for a single draw call with the renderArrayChunk
			if (sizeArray) {
				INFO("Merging all chunks...\n");
					std::vector<SimpleVertex> vertices;
				std::vector<unsigned int> indices;
				for (unsigned int x = 0; x < sizeArray; x++) {
					Obj3d* o = dynamic_cast<Obj3d*>(manager.renderArrayChunk[x]);
					if (!o) {
						D("sizeArray " << sizeArray << " | renderArrayChunk_maxsize " << manager.renderArrayChunk_maxsize << " | x " << x << "\n")
							D("dynamic cast failed on object: " << manager.renderArrayChunk[x] << "\n")
							Misc::breakExit(456);
					}
					Math::Vector3 pos = o->local.getPos();
					Obj3dBP* bp = o->getBlueprint();
					std::vector<SimpleVertex> verts = bp->getVertices();
					//offset the vertices with the obj3d pos
					std::for_each(verts.begin(), verts.end(), [pos](SimpleVertex& vertex) { vertex.position += pos; });
					vertices.insert(vertices.end(), verts.begin(), verts.end());
					if (x % 50 == 0) { D_(std::cout << x << " ") }
				}
				D_(std::cout << "\n")

					Obj3dBP* fullMeshBP = grid.getFullMeshBP();
				Obj3d* fullMesh = grid.getFullMesh();
				D(std::cout << "Deleting old fullMesh...\n")
					if (fullMeshBP)
						delete fullMeshBP;
				if (fullMesh)
					delete fullMesh;
				D(std::cout << "Building new fullMesh...\n")
					fullMeshBP = new Obj3dBP(vertices, indices, BP_DONT_NORMALIZE);
				D(std::cout << "BP ready\n")
					fullMesh = new Obj3d(*fullMeshBP, obj3d_prog);
				D(std::cout << "Obj3d ready.\n")
					manager.renderlistChunk.clear();
				manager.renderlistChunk.push_back(fullMesh);
				D(std::cout << "Done, " << sizeArray << " chunks merged.\n")
			}
		}

	}
	if (1) {//Grid and other visual debug
		ChunkPtr*** chunkArray = grid.getGrid();
		for (unsigned int k = startRendered.z; k < endRendered.z; k++) {
			for (unsigned int j = startRendered.y; j < endRendered.y; j++) {
				for (unsigned int i = startRendered.x; i < endRendered.x; i++) {
					Chunk* chunkPtr = chunkArray[k][j][i];
					if (chunkPtr) {//at this point, the chunk might not be generated yet
						//if (chunkPtr->mesh[0])
							//chunkPtr->mesh[0]->setPolygonMode(manager.polygon_mode);

						if (M_DRAW_GRID_CHUNK) {
							Obj3d* cubeGrid = new Obj3d(cubebp, obj3d_prog);
							cubeGrid->setColor(255, 0, 0);
							cubeGrid->local.setPos(chunkPtr->pos);
							//cubeGrid->local.setScale(chunkPtr->size * scale_coe2);
							cubeGrid->setPolygonMode(GL_LINE);
							cubeGrid->displayTexture = false;
							manager.renderlistGrid.push_back(cubeGrid);
							cubgrid++;
						}
						if (0) {//coloring origin of each octree/chunk
							Math::Vector3	siz(1, 1, 1);
#ifdef OCTREE_OLD
							Octree_old* root = chunkPtr->root->getRoot(chunkPtr->root->pos, siz);
							if (root) {
								if (root->size.len() != siz.len())
									root->pixel = Pixel(254, 0, 0);
								else
									root->pixel = Pixel(0, 255, 0);
							}
#else
							Octree<Voxel>* node = chunkPtr->root->getNode(chunkPtr->root->pos, siz);
							if (node) {
								if (node->size.len() != siz.len())
									node->element._value = 254;
								else
									node->element._value = 253;
							}
#endif
						}
#if 0// oldcode, browsing to build 1 obj3d per cube (not chunk)
						chunkPtr->root->browse([&manager, &cubebp, &obj3d_prog, scale_coef, scale_coe2, chunkPtr, tex, &hiddenBlocks](Octree_old* node) {
							if (!node->isLeaf())
							return;
						if (M_DISPLAY_BLACK || (node->pixel.r != 0 && node->pixel.g != 0 && node->pixel.b != 0)) {// pixel 0?
							Math::Vector3	worldPos = chunkPtr->pos + node->pos;
							Math::Vector3	center = worldPos + (node->size / 2);
							if ((node->pixel.r < VOXEL_EMPTY.r \
								|| node->pixel.g < VOXEL_EMPTY.g \
								|| node->pixel.b < VOXEL_EMPTY.b) \
								&& node->neighbors < NEIGHBOR_ALL)// should be < NEIGHBOR_ALL or (node->n & NEIGHBOR_ALL) != 0
							{
								Obj3d* cube = new Obj3d(cubebp, obj3d_prog);
								cube->setColor(node->pixel.r, node->pixel.g, node->pixel.b);
								cube->local.setPos(worldPos);
								cube->local.setScale(node->size.x * scale_coef, node->size.y * scale_coef, node->size.z * scale_coef);
								cube->setPolygonMode(manager.polygon_mode);
								cube->displayTexture = true;
								cube->displayTexture = false;
								cube->setTexture(tex);

								//if (node->neighbors == 1)// faire une texture pour chaque numero
								//	cube->setColor(0, 127, 127);

								//we can see if there is false positive on fully obstructed voxel, some are partially obstructed
								if (node->neighbors == NEIGHBOR_ALL) {//should not be drawn
									cube->setColor(100, 200, 100);
									cube->setPolygonMode(manager.polygon_mode);
									cube->setPolygonMode(GL_FILL);
									hiddenBlocks++;
									manager.renderlist.push_back(cube);
								}
								else {
									// push only the faces next to an EMPTY_VOXEL in an all-in buffer
									manager.renderlist.push_back(cube);
									if ((node->neighbors & NEIGHBOR_FRONT) != NEIGHBOR_FRONT) { manager.renderlistVoxels[CUBE_FRONT_FACE].push_back(cube); }
									if ((node->neighbors & NEIGHBOR_RIGHT) != NEIGHBOR_RIGHT) { manager.renderlistVoxels[CUBE_RIGHT_FACE].push_back(cube); }
									if ((node->neighbors & NEIGHBOR_LEFT) != NEIGHBOR_LEFT) { manager.renderlistVoxels[CUBE_LEFT_FACE].push_back(cube); }
									if ((node->neighbors & NEIGHBOR_BOTTOM) != NEIGHBOR_BOTTOM) { manager.renderlistVoxels[CUBE_BOTTOM_FACE].push_back(cube); }
									if ((node->neighbors & NEIGHBOR_TOP) != NEIGHBOR_TOP) { manager.renderlistVoxels[CUBE_TOP_FACE].push_back(cube); }
									if ((node->neighbors & NEIGHBOR_BACK) != NEIGHBOR_BACK) { manager.renderlistVoxels[CUBE_BACK_FACE].push_back(cube); }
								}
								if (M_DISPLAY_BLACK) {
									Obj3d* cube2 = new Obj3d(cubebp, obj3d_prog);
									cube2->setColor(0, 0, 0);
									cube2->local.setPos(worldPos);
									cube2->local.setScale(node->size.x * scale_coe2, node->size.y * scale_coe2, node->size.z * scale_coe2);
									cube2->setPolygonMode(GL_LINE);
									cube2->displayTexture = false;
									manager.renderlistOctree.push_back(cube2);
								}
							}
						}
							});
#endif
					}
				}
			}
		}
	}

	if (manager.storageMode == STORAGE_LIST) {
		//INFO("Counting total_polygons from renderlistChunk\n");
		for (Object* o : manager.renderlistChunk) {
			Obj3d* obj = dynamic_cast<Obj3d*>(o);
			if (!obj) { D("grabObjects() : dynamic cast failed on object: " << o << "\n"); Misc::breakExit(456); }
			total_polygons += obj->getBlueprint()->getPolygonAmount();
		}
	}
	else {
		//INFO("Counting total_polygons from renderArrayChunk\n");
		for (unsigned int x = 0; x < sizeArray; x++) {
			Obj3d* obj = dynamic_cast<Obj3d*>(manager.renderArrayChunk[x]);
			if (!obj) { D("grabObjects() : dynamic cast failed on object: " << manager.renderArrayChunk[x] << "\n"); Misc::breakExit(456); }
			total_polygons += obj->getBlueprint()->getPolygonAmount();
		}
	}

	if (grid.playerChangedChunk)
		grid.playerChangedChunk = false;
	if (grid.chunksChanged)
		grid.chunksChanged = false;

	//INFO("polygon debug:\n" << polygon_debug.str() << "\n");
	//INFO("total polygons:\t" << total_polygons << "\n");
	//INFO("tesselation level:\t" << tesselation_lvl << "\n");
	//INFO("hiddenBlocks:\t" << hiddenBlocks << "\n");
	//for (auto i : manager.renderlistVoxels)
	//	INFO("renderlistVoxels[]: " << i.size() << "\n");
	//INFO("renderlistOctree: " << manager.renderlistOctree.size() << "\n");
	//INFO("renderlistChunk: " << manager.renderlistChunk.size() << "\n");
	//INFO("renderArrayChunk: " << sizeArray << "\n");
	//INFO("renderlistGrid: " << manager.renderlistGrid.size() << "\n");
	//INFO("cubes grid : " << cubgrid << "\n");
	//D(D_SPACER_END)

	return total_polygons;
}

unsigned int 	rebuildWithThreshold(ChunkGenerator& generator, ChunkGrid& grid, OctreeManager& manager, Obj3dBP& cubebp, Obj3dPG& obj3d_prog, Texture* tex) {
	int tessLevel = 0;
	/*
		0 = no tesselation, taking smallest voxels (size = 1)
		5 = log2(32), max level, ie the size of a chunk
	*/
	Math::Vector3	gridSize = grid.getSize();
	ChunkPtr*** chunkArray = grid.getGrid();
	for (auto k = 0; k < gridSize.z; k++) {
		for (auto j = 0; j < gridSize.y; j++) {
			for (auto i = 0; i < gridSize.x; i++) {
				Octree<Voxel>* root = chunkArray[k][j][i]->root;
				chunkArray[k][j][i]->buildVertexArraysFromOctree(root, Math::Vector3(0, 0, 0), tessLevel, &manager.threshold);
				chunkArray[k][j][i]->glth_buildMesh();
			}
		}
	}
	// return total_polygons
	return grabObjects(generator, grid, manager, cubebp, obj3d_prog, tex);
}

static void		keyCallback_debugGrid(GLFWwindow* window, int key, int scancode, int action, int mods) {
	(void)window; (void)key; (void)scancode; (void)action; (void)mods;
	//D(__PRETTY_FUNCTION__ << "\n")

	OctreeManager* m = static_cast<OctreeManager*>(glfwGetWindowUserPointer(window));
	if (!m) {
		D("static_cast failed\n")
			return;
	}
	if (action == GLFW_PRESS) {
		D("GLFW_PRESS:" << key << "\n")
			if (m->glfw) {
				if (key == GLFW_KEY_B) {
					std::cout << "B pressed : debug\n";
					std::cout << m->grid->getGridChecks() << "\n";
					std::cout << "jobs remaining : " << m->generator->jobsToDo.size() << "\n";
					std::cout << m->grid->toString() << std::endl;
				}
			}
	}
}

/*
*	todo:
*		- in ChunkGrid::updateGrid() : _deleteUnusedDataLocal(&chunksToDelete, &hmapsToDelete);
*			this must be sent to the trashes in the generator, and let thread do it
				-> it has gl stuff in it, why it is currently done outside of the gl thread ?
*	bugs :
*		- quand le renderedGrid.size = grid.size, race entre le renderer et le grid.updater
*		- la tesselation des chunks n'est pas mise à jour
*
*/
void	scene_octree() {
	#ifndef INIT_GLFW
	float	win_height = 900;
	float	win_width = 1600;
	OctreeManager	m;
	printSettings(m);

	std::this_thread::sleep_for(1s);
	//m.glfw = new Glfw(WINX, WINY);
	m.glfw = new Glfw(win_width, win_height);
	glfwSetWindowPos(m.glfw->_window, 100, 50);
	glfwSetWindowUserPointer(m.glfw->_window, static_cast<void*>(&m));
	glfwSwapInterval(0);//0 = disable vsynx
	m.glfw->setTitle("Tests octree");
	m.glfw->activateDefaultCallbacks(&m);
	m.glfw->func[GLFW_KEY_EQUAL] = keyCallback_ocTree; // manager->threshold +
	m.glfw->func[GLFW_KEY_MINUS] = keyCallback_ocTree; // manager->threshold -
	m.glfw->func[GLFW_KEY_ENTER] = keyCallback_ocTree; // switch polygon mode
	m.glfw->func[GLFW_KEY_X] = keyCallback_ocTree; // jump cam (+150)
	m.glfw->func[GLFW_KEY_Y] = keyCallback_ocTree; // jump cam (+150)
	m.glfw->func[GLFW_KEY_Z] = keyCallback_ocTree; // jump cam (+150)
	m.glfw->func[GLFW_KEY_LEFT_SHIFT] = keyCallback_ocTree; // run
	m.glfw->func[GLFW_KEY_T] = keyCallback_ocTree; // switch objects storage mode (array or list)
	m.glfw->func[GLFW_KEY_B] = keyCallback_debugGrid; // switch objects storage mode (array or list)

	Texture* tex_skybox = new Texture(SIMPLEGL_FOLDER + "images/skybox4.bmp");
	Texture* tex_lena = new Texture(SIMPLEGL_FOLDER + "images/lena.bmp");
	Texture* tex_player = new Texture(SIMPLEGL_FOLDER + "images/player_icon.bmp");

	m.playerMinimap = new UIImage(tex_player);

#ifndef TEXT_PG
	TextPG::fonts_folder = Misc::getCurrentDirectory() + SIMPLEGL_FOLDER + "fonts/";
	TextPG			rendererText_arial(SIMPLEGL_FOLDER + "shaders/text.vs.glsl", SIMPLEGL_FOLDER + "shaders/text.fs.glsl");
	if (rendererText_arial.init_freetype("arial.ttf", win_width, win_height) == -1) {
		std::cerr << "TextPG::init_freetype failed\n";
		Misc::breakExit(-1);
	}
	Text			text1;
	text1.text = "This is sample text";
	text1.color = Math::Vector3(0.5, 0.8f, 0.2f);
	text1.local.setPos(200, 200, 1.0f);
	text1.local.setScale(1, 1, 1);
	Text			text2;
	text2.text = "(C) LearnOpenGL.com";
	text2.color = Math::Vector3(0.3f, 0.7f, 0.9f);
	text2.local.setPos(540.0f, 570.0f, 0.5f);
	text2.local.setScale(0.5, 1, 1);
#endif //TEXT_PG

#ifndef MAIN_PG
	Obj3dPG			rendererObj3d(SIMPLEGL_FOLDER + OBJ3D_VS_FILE, SIMPLEGL_FOLDER + OBJ3D_FS_FILE);
	Obj3dIPG		rendererObj3dInstanced(SIMPLEGL_FOLDER + OBJ3D_INSTANCED_VS_FILE, SIMPLEGL_FOLDER + OBJ3D_FS_FILE);
	SkyboxPG		rendererSkybox(SIMPLEGL_FOLDER + CUBEMAP_VS_FILE, SIMPLEGL_FOLDER + CUBEMAP_FS_FILE);
	Skybox			skybox(*tex_skybox, rendererSkybox);
	m.renderlistSkybox.push_back(&skybox);
	Obj3dPG* renderer = &rendererObj3d;
	//renderer = &rendererObj3dInstanced;
	Chunk::renderer = &rendererObj3d;
#endif //MAIN_PG

	//Blueprint global settings
	Obj3dBP::defaultSize = 1;
	Obj3dBP::defaultDataMode = BP_LINEAR;
	Obj3dBP::rescale = true;
	Obj3dBP::center = false;
	Obj3dBP		cubebp(SIMPLEGL_FOLDER + "obj3d/cube.obj");
	Chunk::cubeBlueprint = &cubebp;
	Obj3dBP::defaultDataMode = BP_INDICES;

	Cam		cam(m.glfw->getWidth(), m.glfw->getHeight());
	cam.speed = m.playerSpeed;
	cam.printProperties();
	cam.lockedMovement = false;
	cam.lockedOrientation = false;
	//m.glfw->setMouseAngle(-1);//?
	D("MouseAngle: " << m.glfw->getMouseAngle() << "\n")
		m.cam = &cam;
	#endif //INIT_GLFW

	#ifndef BASE_OBJ3D
	Obj3d		cubeo(cubebp, *renderer);
	cubeo.local.setPos(0, 0, 0);
	cubeo.local.setScale(1, 1, 1);
	cubeo.setColor(255, 0, 0);
	cubeo.displayTexture = false;
	cubeo.setPolygonMode(GL_LINE);
	//m.renderlist.push_back(&cubeo);

	//Obj3d		player1(cubebp, *rendererObj3d.program);
	//player1.local.setPos(0, 35, 0);
	//player1.local.setScale(1, 2, 1);
	//player1.local.enlarge(5, 5, 5);
	//player1.setColor(255, 0, 0);
	//player1.displayTexture = false;
	//player1.setPolygonMode(GL_FILL);
	//m.player = &player1;

	//std::list<Obj3d*>	playerList;
	//playerList.push_back(&player1);
	#endif

	cam.local.setPos(28, 50, 65);//buried close to the surface
	cam.local.setPos(280, 50, 65);//
	cam.local.setPos(320, 100, 65);//
	//cam.local.setPos(280, -100, 65);//crash
	//cam.local.setPos(-13, 76, 107);//crash
	Math::Vector3	playerPos = cam.local.getPos();

	#ifndef GENERATOR
	int grid_size = 35;
	if (0) {
		std::cout << "Enter grid size (min 7, max 35):\n";
		std::cin >> grid_size;
		grid_size = (grid_size < 7) ? 7 : grid_size;
		grid_size = (grid_size > 35) ? 35 : grid_size;
	}
	int	g = grid_size;
	int	d = grid_size - 4;// g * 2 / 3;
	m.gridSize = Math::Vector3(g, g / 4, g);
	m.renderedGridSize = Math::Vector3(d, d / 4, d);
	//m.gridSize = Math::Vector3(5, 5, 5);
	//m.renderedGridSize = Math::Vector3(5, 5, 5);
	INFO("Grid size : " << m.gridSize.toString() << "\n");
	INFO("Rebdered grid size : " << m.renderedGridSize.toString() << "\n");
	INFO("Total hmaps : " << m.gridSize.x * m.gridSize.z << "\n");
	INFO("Total chunks : " << m.gridSize.x * m.gridSize.y * m.gridSize.z << "\n");
	ChunkGenerator	generator(*m.ps);
	ChunkGrid		grid(m.chunk_size, m.gridSize, m.renderedGridSize);
	m.generator = &generator;
	m.grid = &grid;

	#ifndef INIT_RENDER_ARRAY
	unsigned int x = grid.getRenderedSize().x;
	unsigned int y = grid.getRenderedSize().y;
	unsigned int z = grid.getRenderedSize().z;
	unsigned int len = x * y;
	if (x != 0 && len / x != y) {
		D("grid size too big, causing overflow\n")
			Misc::breakExit(99);
	}
	len = x * y * z;
	if ((z != 0 && len / z != x * y) || len == 4294967295) {
		D("grid size too big, causing overflow\n")
			Misc::breakExit(99);
	}
	len++;
	INFO("m.renderArrayChunk size for rendered chunks : " << len << "\n");
	m.renderArrayChunk = new Object * [len];
	m.renderArrayChunk[0] = nullptr;
	m.renderArrayChunk_maxsize = len;
	#endif // INIT_RENDER_ARRAY
	#endif // GENERATOR
	std::unique_lock<std::mutex> chunks_lock(grid.chunks_mutex, std::defer_lock);

	Fps	fps(135);
	INFO("Maximum fps : " << fps.getMaxFps() << "\n");
	//#define MINIMAP // need to build a framebuffer with the entire map, update it each time the player changes chunk

	#define USE_THREADS
	#ifdef USE_THREADS
	INFO("USE_THREADS : true\n");
	#else
	INFO("USE_THREADS : false\n");
	#endif
	#ifdef USE_THREADS

	generator.initAllBuilders(m.cpuThreadAmount - 1, &cam, &grid);
	glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
	glfwFocusWindow(m.glfw->_window);
	std::this_thread::sleep_for(1s);

	unsigned int polygons = 0;
	unsigned int frames = 0;
	D("cam: " << cam.local.getPos().toString() << "\n")
	INFO("Begin while loop, renderer: " << typeid(renderer).name() << "\n");
	//std::cout.setstate(std::ios_base::failbit);

	while (!glfwWindowShouldClose(m.glfw->_window)) {
		if (fps.wait_for_next_frame()) {
			//std::this_thread::sleep_for(1s);
			frames++;
			if (frames % 1000 == 0) {
				D(">>>>>>>>>>>>>>>>>>>> " << frames << " FRAMES <<<<<<<<<<<<<<<<<<<<\n")
				D("cam: " << cam.local.getPos() << "\n")
			}
			std::string decimals = std::to_string(polygons / 1'000'000.0);
			m.glfw->setTitle(
				std::to_string(fps.getFps()) + " fps | "
				+ std::to_string(int(polygons/1'000'000)) + "m"
				+ ( decimals.c_str()+decimals.find('.')+1 )
				+ " polys | threshold "
				+ std::to_string(m.threshold)
			);

			glfwPollEvents();
			m.glfw->updateMouse();//to do before cam's events
			{
				std::lock_guard<std::mutex> guard(generator.mutex_cam);
				m.cam->events(*m.glfw, float(fps.getTick()));
			}
			if (generator.job_mutex.try_lock()) {
				if ((grid.playerChangedChunk || grid.chunksChanged) && chunks_lock.try_lock()) {
					//D("[renderer] lock chunks_mutex\n")
					double start = glfwGetTime();
					INFO("grabbing meshes...\n");
					polygons = grabObjects(generator, grid, m, cubebp, *renderer, tex_lena);
					start = glfwGetTime() - start;
					INFO("grabbed " << m.renderlistChunk.size() << " in " << start << " seconds\n");
					chunks_lock.unlock();
					// the generator can do what he wants with the grid, the renderer has what he needs for the current frame
				}
				generator.job_mutex.unlock();
				glFinish();// why here ?
			}

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			//D("renderlist\n")
			if (0) { // opti faces, not converted to mesh
				float speed = cam.speed;//save
				for (size_t i = 0; i < 6; i++) {
					cam.speed = i;//use this as a flag for the renderer, tmp
					renderer->renderObjects(m.renderlistVoxels[i], cam, PG_FORCE_DRAW);
				}
				cam.speed = speed;//restore
			}
			else if (0) {//whole cube
				renderer->renderObjects(m.renderlist, cam, PG_FORCE_DRAW);
			}
			else if (1) { // converted to mesh
				grid.chunks_mutex.lock();
				if (m.storageMode == STORAGE_LIST) {
					Chunk::renderer->renderObjects(m.renderlistChunk, cam, PG_FORCE_DRAW);//PG_FRUSTUM_CULLING
					//Chunk::renderer->renderObjectsMultiDraw(m.renderlistChunk, cam, PG_FORCE_DRAW);//PG_FRUSTUM_CULLING
				} else
					Chunk::renderer->renderObjects(m.renderArrayChunk, cam, PG_FORCE_DRAW);//PG_FRUSTUM_CULLING
				grid.chunks_mutex.unlock();
			}
			//D("octreeRender\n")
			//renderer->renderObjects(m.renderlistOctree, cam, PG_FORCE_DRAW);
#if (M_DRAW_BOX_GRID || M_DRAW_GRID_CHUNK)
			glDisable(GL_CULL_FACE);
			renderer->renderObjects(m.renderlistGrid, cam, PG_FORCE_DRAW);
			glEnable(GL_CULL_FACE);
#endif
			//rendererSkybox.renderObjects(m.renderlistSkybox, cam, PG_FORCE_DRAW);

			//rendererText_arial.render(text1, Math::Matrix4());
			//rendererText_arial.render(text2, Math::Matrix4());

#ifdef MINIMAP
			if (chunks_lock.try_lock() && generator.grid[0][0][0]) {
				D("[Main] Minimap lock\n")
				//thread player management?
				// coo in 3d world
				playerPos = cam.local.getPos();
				float	zmax = generator.gridSize.z * generator.chunkSize.z;//for gl convertion
				float	px_topleftChunk = generator.grid[0][0][0]->pos.x;//need to setup a mutex or it will crash
				float	pz_topleftChunk = generator.grid[0][0][0]->pos.z;

				// coo on 2d screen
				float	px = (playerPos.x - px_topleftChunk) * m.minimapCoef;
				float	pz = (zmax - (playerPos.z - pz_topleftChunk)) * m.minimapCoef;
				m.playerMinimap->setPos(px, pz);
				m.playerMinimap->setPos2(px + m.playerMinimap->getTexture()->getWidth() * m.minimapCoef,
					pz + m.playerMinimap->getTexture()->getHeight() * m.minimapCoef);
				//build minimap
				//too many blit, build a single image for all the minimap! for all the UI?
				for (unsigned int z = 0; z < generator.gridSize.z; z++) {
					for (unsigned int x = 0; x < generator.gridSize.x; x++) {
						if (generator.heightMaps[z][x])
							blitToWindow(nullptr, GL_COLOR_ATTACHMENT0, generator.heightMaps[z][x]->panel);
					}
				}
				blitToWindow(nullptr, GL_COLOR_ATTACHMENT0, m.playerMinimap);
				D("[Main] Minimap unlock\n")
				chunks_lock.unlock();
			}
#endif
			glfwSwapBuffers(m.glfw->_window);
			generator.try_deleteUnusedData();

			if (m.thresholdUpdated) {
				polygons = rebuildWithThreshold(generator, grid, m, cubebp, *renderer, tex_lena);//should be a job ?
				m.thresholdUpdated = false;
			}

			if (GLFW_PRESS == glfwGetKey(m.glfw->_window, GLFW_KEY_ESCAPE))
				glfwSetWindowShouldClose(m.glfw->_window, GLFW_TRUE);
		}
	}
	std::cout.clear();
	glfwMakeContextCurrent(nullptr);
	#else  // not USE_THREADS
	std::thread helper0(std::bind(&ChunkGenerator::th_updater, &generator, &cam));

	INFO("Begin while loop\n");
	while (!glfwWindowShouldClose(m.glfw->_window)) {
		if (fps.wait_for_next_frame()) {
			//D(">>>>>>>>>>>>>>>>>>>> NEW FRAME <<<<<<<<<<<<<<<<<<<<\n")
			m.glfw->setTitle(std::to_string(fps.getFps()) + " fps");
			glfwPollEvents();
			m.glfw->updateMouse();//to do before cam's events
			{
				std::lock_guard<std::mutex> guard(generator.mutex_cam);
				m.cam->events(*m.glfw, float(fps.getTick()));
			}
			//no helper thread
			if (0 && generator.updateGrid(m.cam->local.getPos())) {
				generator.glth_buildMeshesAndMapTiles();
				grabObjectFromGenerator(generator, m, cubebp, *renderer, tex_lena);
			}
			else if (1) {//with helper thread
				if (!generator.jobsToDo.empty()) {
					INFO("===================================== STARTING BUILDING =====================================\n");
					generator.build(generator.settings, std::string("[main thread]\t"));
					INFO("===================================== DONE BUILDING =====================================\n");
				}
				else if ((generator.playerChangedChunk || generator.chunksChanged) && chunks_lock.try_lock()) {
					//D("[renderer] lock chunks_mutex\n")
					double start = glfwGetTime();
					D(&generator << " : grabbing meshes...\n")
					grabObjectFromGenerator(generator, m, cubebp, *renderer, tex_lena);
					start = glfwGetTime() - start;
					D("grabbed in " << start << " seconds\n")
					if (generator.playerChangedChunk)
						generator.playerChangedChunk = false;
					if (generator.chunksChanged)
						generator.chunksChanged = false;
					chunks_lock.unlock();
				}
			}

			//GLuint	mode = m.polygon_mode;
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			//D("renderlist\n")
			if (0) { // opti faces, not converted to mesh
				float speed = cam.speed;//save
				for (size_t i = 0; i < 6; i++) {
					cam.speed = i;//use this as a flag for the renderer, tmp
					renderer->renderObjects(m.renderlistVoxels[i], cam, PG_FORCE_DRAW);
				}
				cam.speed = speed;//restore
			}
			else if (0) {//whole cube
				renderer->renderObjects(m.renderlist, cam, PG_FORCE_DRAW);
			}
			else { // converted to mesh
				Chunk::renderer->renderObjects(m.renderlistChunk, cam, PG_FORCE_DRAW);//PG_FRUSTUM_CULLING
			}
			//D("octree render\n")
			//renderer->renderObjects(m.renderlistOctree, cam, PG_FORCE_DRAW);
			//D("rendered grid\n")
			#if true
			glDisable(GL_CULL_FACE);
			renderer->renderObjects(m.renderlistGrid, cam, PG_FORCE_DRAW);
			glEnable(GL_CULL_FACE);
			#endif
			//rendererSkybox.renderObjects(m.renderlistSkybox, cam, PG_FORCE_DRAW);

			glfwSwapBuffers(m.glfw->_window);
			//generator.try_deleteUnusedData();

			if (GLFW_PRESS == glfwGetKey(m.glfw->_window, GLFW_KEY_ESCAPE))
				glfwSetWindowShouldClose(m.glfw->_window, GLFW_TRUE);
		}
	}
	D("End while loop\n")
	generator.terminateThreads = true;
	generator.cv.notify_all();
	glfwMakeContextCurrent(nullptr);
	helper0.join();
	D("[Main] joined helper0\n")
	D("[Main] exiting...\n")
	D("[Main] exiting...\n")

	#endif // USE_THREADS
}

void	maxUniforms() {
	D("GL_MAX_VERTEX_UNIFORM_COMPONENTS\t" << GL_MAX_VERTEX_UNIFORM_COMPONENTS << "\n")
	D("GL_MAX_GEOMETRY_UNIFORM_COMPONENTS\t" << GL_MAX_GEOMETRY_UNIFORM_COMPONENTS << "\n")
	D("GL_MAX_FRAGMENT_UNIFORM_COMPONENTS\t" << GL_MAX_FRAGMENT_UNIFORM_COMPONENTS << "\n")
	D("GL_MAX_TESS_CONTROL_UNIFORM_COMPONENTS\t" << GL_MAX_TESS_CONTROL_UNIFORM_COMPONENTS << "\n")
	D("GL_MAX_TESS_EVALUATION_UNIFORM_COMPONENTS\t" << GL_MAX_TESS_EVALUATION_UNIFORM_COMPONENTS << "\n")
	D("GL_MAX_COMPUTE_UNIFORM_COMPONENTS\t" << GL_MAX_COMPUTE_UNIFORM_COMPONENTS << "\n")
	D("GL_MAX_UNIFORM_LOCATIONS\t" << GL_MAX_UNIFORM_LOCATIONS << "\n")
	Misc::breakExit(0);
}

void	scene_checkMemory() {
	char input[100];
	Glfw* glfw = new Glfw(WINX, WINY);
	std::string	pathPrefix("SimpleGL/");

	D("Texture build... ENTER\n")
	std::cin >> input;
	Texture* tex_bigass = new Texture(pathPrefix + "images/skybox4096.bmp");
	D(">> Texture loaded\n")
	//Textrues
#if 0
	D("Texture unload load loop... ENTER\n")
	std::cin >> input;

	for (size_t i = 0; i < 100; i++) {
		//unload
		tex_bigass->unloadFromGPU();
		glfwSwapBuffers(glfw->_window);
		D(">> Texture unloaded\n")

		//load
		tex_bigass->loadToGPU();
		glfwSwapBuffers(glfw->_window);
		D(">> Texture loaded\n")
	}
#endif
	//Skybox
#if 1
	SkyboxPG	rendererSkybox(pathPrefix + CUBEMAP_VS_FILE, pathPrefix + CUBEMAP_FS_FILE);

	for (size_t i = 0; i < 100; i++) {
		Skybox* sky = new Skybox(*tex_bigass, rendererSkybox);
		delete sky;
	}
#endif
	//framebuffer
#if 1
#endif
//exit
	std::cout << "end... ENTER\n";
	std::cin >> input;
	Misc::breakExit(0);

}

#define SSSIZE 2500
void	benchmark_octree() {
	Glfw glfw;
	//Blueprint global settings
	Obj3dBP::defaultSize = 1;
	Obj3dBP::defaultDataMode = BP_LINEAR;
	Obj3dBP::rescale = true;
	Obj3dBP::center = false;
	Obj3dBP		cubebp(SIMPLEGL_FOLDER + "obj3d/cube.obj");
	Chunk::cubeBlueprint = &cubebp;
	Obj3dBP::defaultDataMode = BP_INDICES;

	OctreeManager	m;
	Math::Vector3	index(7, 2, 0);
	Math::Vector3	size(32, 32, 32);
	HeightMap* hmap = new HeightMap(*m.ps, index, size);
	Chunk* test = new Chunk(index, size, *m.ps, hmap);
	//test->glth_buildMesh();
	if (test->meshBP[0]) {
		D("polys: " << test->meshBP[0]->getPolygonAmount() << "\n")
		test->meshBP[0]->freeData(BP_FREE_ALL);
		delete test->meshBP[0];
		test->meshBP[0] = nullptr;
	}
	//Misc::breakExit(0);
	double start = glfwGetTime();
	Chunk* c;
	for (size_t i = 0; i < SSSIZE; i++) {
		c = new Chunk(index, size, *m.ps, hmap);
		delete c;
		//if (i % 50 == 0)
		//	D_(std::cout << i);
		//else
		//	D_(std::cout << ".");
	}
	start = glfwGetTime() - start;
	D("\n\n" << double(start) << std::endl)
	Misc::breakExit(0);
}

#if 1 // main
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <cmath>

//thread safe cout : https://stackoverflow.com/questions/14718124/how-to-easily-make-stdcout-thread-safe
//multithread monitor example : https://stackoverflow.com/questions/51668477/c-lock-a-mutex-as-if-from-another-thread
int		main(int ac, char **av) {

	//Misc::breakExit(5);
	//playertest();
	//_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	//_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);

	_CrtMemState sOld;
	_CrtMemState sNew;
	_CrtMemState sDiff;
	_CrtMemCheckpoint(&sOld); //take a snapchot

	//maxUniforms();
	//check_paddings();
	// test_behaviors();
	//test_mult_mat4(); Misc::breakExit(0);
	//	test_obj_loader();

	D("____START____ :" << Misc::getCurrentDirectory() << "\n")
	//test_memory_opengl_obj3dbp();
	//test_memory_opengl();
	//test_shared_ptr();
	//testtype(true);	testtype(false); Misc::breakExit(0);
	//benchmark_octree();
	//scene_4Tree();
	//scene_procedural();
	//scene_benchmarks();
	//scene_checkMemory();
	//scene_octree();
	//scene_test_thread();
	// while(1);

	D("____END____ : \n")
	_CrtMemCheckpoint(&sNew); //take a snapchot 
	if (_CrtMemDifference(&sDiff, &sOld, &sNew)) // if there is a difference
	{
		OutputDebugStringA("-----------_CrtMemDumpStatistics ---------");
		_CrtMemDumpStatistics(&sDiff);
		OutputDebugStringA("-----------_CrtMemDumpAllObjectsSince ---------");
		_CrtMemDumpAllObjectsSince(&sOld);
		OutputDebugStringA("-----------_CrtDumpMemoryLeaks ---------");
		_CrtDumpMemoryLeaks();
	} else {
		D("no diff for memory check\n")
	}
	return (EXIT_SUCCESS);
}
#endif //main

#endif //main all
