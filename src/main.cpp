#if 1
#include <iostream>
#include <chrono>
#include "trees.h"
class SharedObj
{
public:
	static int ID;
	SharedObj() {
		this->_id = SharedObj::ID;
		SharedObj::ID++;
		std::cout << "Constructor " << this->_id << "\n";
	}
	~SharedObj() {
		std::cout << "Destructor " << this->_id << "\n";
	}
	int				_id;
private:
};

#define SSH_SIZE 5
void th_1(std::shared_ptr<SharedObj> p) {
	using namespace std::chrono_literals;
	{
		static std::mutex io_mutex; // the static is shared between threads, allowing the local declaration of the mutex to control IO
		std::lock_guard<std::mutex> lk(io_mutex);
		std::cout << "thread : " << p.use_count() << std::endl;
	}
	std::this_thread::sleep_for(1s);
}
void th_2(std::shared_ptr<SharedObj>* o) {
	using namespace std::chrono_literals;
	{
		static std::mutex io_mutex; // the static is shared between threads, allowing the local declaration of the mutex to control IO
		std::lock_guard<std::mutex> lk(io_mutex);
		std::cout << "thread created\n";
	}
	std::this_thread::sleep_for(1s);
	for (size_t i = 0; i < 5; i++) {
		o[i] = nullptr;
	}

}
int SharedObj::ID = 1;
void	test_shared_ptr() {
	{
		using namespace std::chrono_literals;
		if (0) {
			std::cout << ".\n";
			std::shared_ptr<SharedObj> ptr(new SharedObj()); // Create a new pointer to manage an object
			std::cout << ".\n";
			ptr.reset(new SharedObj());                // Reset to manage a different object
			std::cout << ".\n";
			ptr = std::make_shared<SharedObj>();       // Use `make_shared` rather than `new`.
			std::cout << ".\n";
			// important: std::make_shared<>() allocates a new object, and will use relevant constructor depending on params
			return;
		}
		if (1) {
			SharedObj* o1 = new SharedObj();
			SharedObj* o2 = new SharedObj();
			std::shared_ptr<SharedObj>	p1_1(o1);//care it sends a reference to a constructor

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

			std::shared_ptr<SharedObj>	p2_1(o2);
			std::weak_ptr<SharedObj> wp2 = p2_1;
			std::cout << "wp2 : " << wp2.use_count() << std::endl;

			std::cout << "reassigning p2_1 with o1 ptr\n";
			p2_1 = wp1.lock();// It creates and returns a shared_ptr that can be empty if use_count() = 0. weak_ptr::lock() is equivalent to open_schrodinger_cat_box().
			std::cout << "wp1 : " << wp1.use_count() << std::endl;
			std::cout << "wp2 : " << wp2.use_count() << std::endl;

			t1.join(); t2.join(); t3.join();
			std::cout << "end\n";
		}
		else if (1) {
			std::shared_ptr<SharedObj> grid[SSH_SIZE];
			std::shared_ptr<SharedObj> grabber[SSH_SIZE];
			for (size_t i = 0; i < 5; i++) {
				grid[i] = std::make_shared<SharedObj>();
				grabber[i] = grid[i];
			}
			std::thread t1{ th_2, grid };
			t1.join();
			std::cout << "thread joined\n";
		}
	}
	std::exit(0);
}
#endif // test shared ptr

#if 1
#include "trees.h"
#include <typeinfo>
#include <thread>
#include <chrono>
using namespace std::chrono_literals;

#ifdef TREES_DEBUG
 #define TREES_MAIN_DEBUG
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
		this->ps->flattering = PERLIN_DEFAULT_FLATTERING;
		this->ps->island = PERLIN_DEFAULT_ISLAND;

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
			D("Not enough threads: " << this->cpuThreadAmount << "\n");
			Misc::breakExit(-44);
		}
	}
	~OctreeManager() {}

	unsigned int		seed;
	siv::PerlinNoise*	perlin;
	PerlinSettings*		ps;

	std::vector<Object*>	renderVec;
	std::vector<Object*>	renderVecOctree;
	std::vector<Object*>	renderVecGrid;
	std::vector<Object*>	renderVecVoxels[6];//6faces
	std::vector<Object*>	renderVecChunk;
	std::vector<Object*>	renderVecSkybox;

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
	m.glfw = new Glfw(1600, 900);
	glfwSetWindowPos(m.glfw->_window, 100, 50);

	//Blueprint global settings
	Obj3dBP::config.modelSize = 1;
	Obj3dBP::config.rescale = true;
	Obj3dBP::config.center = false;
	Obj3dBP::config.dataMode = BP_INDICES;
	if (input[0] % 2 == 1)
		Obj3dBP::config.dataMode = BP_LINEAR;
	Obj3dBP		cubebp(SIMPLEGL_FOLDER + "obj3d/cube.obj");
	Obj3dBP		lambobp0(SIMPLEGL_FOLDER + "obj3d/lambo/Lamborginhi_Aventador_OBJ/Lamborghini_Aventador_no_collider_lod_0.obj");
	Obj3dBP		lambobp1(SIMPLEGL_FOLDER + "obj3d/lambo/Lamborginhi_Aventador_OBJ/Lamborghini_Aventador_no_collider_lod_1.obj");
	Obj3dBP		lambobp2(SIMPLEGL_FOLDER + "obj3d/lambo/Lamborginhi_Aventador_OBJ/Lamborghini_Aventador_no_collider_lod_2.obj");
	Obj3dBP		lambobp3(SIMPLEGL_FOLDER + "obj3d/lambo/Lamborginhi_Aventador_OBJ/Lamborghini_Aventador_no_collider_lod_3.obj");

	lambobp0.lodManager.addLod(&lambobp1, 8);
	lambobp0.lodManager.addLod(&lambobp2, 20);
	lambobp0.lodManager.addLod(&lambobp3, 60);
	std::cout << lambobp0.lodManager.toString() << "\n";
	//std::exit(0);

	Texture*	lenatex = new Texture(SIMPLEGL_FOLDER + "images/lena.bmp");
	Texture*	lambotex = new Texture(SIMPLEGL_FOLDER + "obj3d/lambo/Lamborginhi_Aventador_OBJ/Lamborginhi_Aventador_diffuse.bmp");
	Texture*	tex_skybox = new Texture(SIMPLEGL_FOLDER + "images/skybox4.bmp");

	Obj3dPG		rendererObj3d(SIMPLEGL_FOLDER + OBJ3D_VS_FILE, SIMPLEGL_FOLDER + OBJ3D_FS_FILE);
	Obj3dIPG	rendererObj3dInstanced(SIMPLEGL_FOLDER + OBJ3D_INSTANCED_VS_FILE, SIMPLEGL_FOLDER + OBJ3D_FS_FILE);
	SkyboxPG	rendererSkybox(SIMPLEGL_FOLDER + CUBEMAP_VS_FILE, SIMPLEGL_FOLDER + CUBEMAP_FS_FILE);

	Skybox		skybox(*tex_skybox, rendererSkybox);
	m.renderVecSkybox.push_back(&skybox);

	Cam		cam(m.glfw->getWidth(), m.glfw->getHeight());
	cam.local.setPos(-5, -5, -5);
	cam.speed = 25;
	cam.printProperties();
	cam.lockedMovement = false;
	cam.lockedOrientation = false;
	//m.glfw->setMouseAngle(-1);//?
	m.cam = &cam;

	Obj3dPG* renderer = &rendererObj3d;
	if (input[0] >= '3') {
		renderer = &rendererObj3dInstanced;
		D("Instanced rendering enabled\n");
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
	//lenatex->updateData(tex_data, w, h);

	Math::Vector3	lamboGrid(50, 1, 50);
	if (1) {//lambos
		for (size_t k = 0; k < lamboGrid.z; k++) {
			for (size_t j = 0; j < lamboGrid.y; j++) {
				for (size_t i = 0; i < lamboGrid.x; i++) {
					Obj3d* lambo = new Obj3d(lambobp0, *renderer);
					lambo->local.setPos(i*3, j*1.5, k*7);
					lambo->local.enlarge(5, 5, 5);
					lambo->setColor(222, 0, 222);
					lambo->setTexture(lambotex);
					lambo->displayTexture = true;
					lambo->setPolygonMode(GL_FILL);
					m.renderVec.push_back(lambo);
				}
			}
		}
	}
	if (0) {//cubes
		for (size_t i = 0; i < 10; i++) {
			for (size_t j = 0; j < 10; j++) {
				Obj3d* cube = new Obj3d(cubebp, *renderer);
				cube->local.setPos(-10 + float(i) * -1.1, j * 1.1, 0);
				cube->local.setScale(1, 1, 1);
				cube->setColor(2.5 * i, 0, 0);
				cube->displayTexture = true;
				cube->setTexture(lenatex);
				cube->setPolygonMode(GL_FILL);
				m.renderVec.push_back(cube);
			}
		}
	}


	Obj3d* frontobj = static_cast<Obj3d*>(m.renderVec.front());
	Obj3dBP* frontbp = frontobj->getBlueprint();
	#ifndef SHADER_INIT
	for (auto o : m.renderVec)
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

	Fps	fps(1000);
	std::cout << "Fps tick: " << fps.getTick() << std::endl;
	D("renderVec: " << m.renderVec.size() << "\n");
	D("Begin while loop\n");
	while (!glfwWindowShouldClose(m.glfw->_window)) {
		if (fps.wait_for_next_frame()) {
			m.glfw->setTitle(std::to_string(fps.getFps()) + " fps "
				+ std::to_string((m.renderVec.front()->local.getPos() - cam.local.getPos()).len())
			);

			glfwPollEvents();
			m.glfw->updateMouse();//to do before cam's events
			m.cam->events(*m.glfw, float(fps.getTick()));

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			#if 1
			// PG_FORCE_DRAW | PG_FRUSTUM_CULLING
			//renderer->renderAllObjects(m.renderVec, cam);												// fps: 2:53/35		WITHOUT LOD
			renderer->renderAllObjects(m.renderVec, cam, PG_FRUSTUM_CULLING);							// fps: 2:550/35	WITHOUT LOD
			#else //optimized mass renderer (non instanced) non moving objects, same BP
			{
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

				for (Object* object : m.renderVec) {
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
			#endif

			rendererSkybox.renderAllObjects(m.renderVecSkybox, cam, PG_FORCE_DRAW);//this will unbind obj3d pg vao and texture
			glfwSwapBuffers(m.glfw->_window);

			if (GLFW_PRESS == glfwGetKey(m.glfw->_window, GLFW_KEY_ESCAPE))
				glfwSetWindowShouldClose(m.glfw->_window, GLFW_TRUE);
		}
	}

	D("End while loop\n");
}

#define M_PERLIN_GENERATION		1
#define M_OCTREE_OPTIMISATION	1
#define M_DRAW_MINIMAP			0
#define M_MERGE_CHUNKS			0

#define M_DRAW_DEBUG			1
#define M_DRAW_GRID_BOX			1
#define M_DRAW_GRID_CHUNK		0
#define M_DISPLAY_BLACK			0

void	printSettings(OctreeManager& m) {

	INFO("cpu threads amount: " << m.cpuThreadAmount << "\n");

	//INFO(D_VALUE_NAME(PG_FORCE_LINKBUFFERS));

	INFO(D_VALUE_NAME(M_PERLIN_GENERATION));
	INFO(D_VALUE_NAME(M_OCTREE_OPTIMISATION));
	INFO(D_VALUE_NAME(M_DRAW_MINIMAP));
	INFO(D_VALUE_NAME(M_MERGE_CHUNKS));

	INFO(D_VALUE_NAME(M_DRAW_DEBUG));
	INFO(D_VALUE_NAME(M_DRAW_GRID_BOX));
	INFO(D_VALUE_NAME(M_DRAW_GRID_CHUNK));
	INFO(D_VALUE_NAME(M_DISPLAY_BLACK));

	INFO(D_VALUE_NAME(LODS_AMOUNT));
	INFO(D_VALUE_NAME(LOD_MIN_VERTEX_ARRAY_SIZE));
	INFO(D_VALUE_NAME(OCTREE_THRESHOLD));
	INFO(D_VALUE_NAME(CHUNK_DEFAULT_SIZE));

	INFO(D_VALUE_NAME(HMAP_BUILD_TEXTUREDATA_IN_CTOR));
	INFO(D_VALUE_NAME(PERLIN_NORMALIZER));
	INFO(D_VALUE_NAME(PERLIN_DEFAULT_OCTAVES));
	INFO(D_VALUE_NAME(PERLIN_DEFAULT_FREQUENCY));
	INFO(D_VALUE_NAME(PERLIN_DEFAULT_FLATTERING));
	INFO(D_VALUE_NAME(PERLIN_DEFAULT_HEIGHTCOEF));
	INFO(D_VALUE_NAME(PERLIN_DEFAULT_ISLAND));
}

static void		keyCallback_ocTree(GLFWwindow* window, int key, int scancode, int action, int mods) {
	(void)window; (void)key; (void)scancode; (void)action; (void)mods;
	//D(__PRETTY_FUNCTION__ << "\n")

	float	move = 150;
	OctreeManager* manager = static_cast<OctreeManager*>(glfwGetWindowUserPointer(window));
	if (!manager) {
		D("static_cast failed\n");
		return;
	}
	if (action == GLFW_PRESS) {
		D("GLFW_PRESS:" << key << "\n");
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
				for (auto o : manager->renderVecChunk) {
					((Obj3d*)(o))->setPolygonMode(manager->polygon_mode);
				}
			} else if (key == GLFW_KEY_X) {
				manager->cam->local.translate(move, 0, 0);
			} else if (key == GLFW_KEY_Y) {
				manager->cam->local.translate(0, move, 0);
			} else if (key == GLFW_KEY_Z) {
				manager->cam->local.translate(0, 0, move);
			} else if (key == GLFW_KEY_LEFT_SHIFT) {
				manager->shiftPressed = true;
				manager->cam->speed = manager->playerSpeed * 5;
			}
		}
	}
	else if (action == GLFW_RELEASE) {
		D("GLFW_RELEASE:" << key << "\n");
		if (manager->glfw) {
			if (key == GLFW_KEY_LEFT_SHIFT) {
				D("GLFW_KEY_LEFT_SHIFT\n");
				manager->shiftPressed = false;
				manager->cam->speed = manager->playerSpeed;
			}
		}
	}
}

unsigned int	grabObjects(ChunkGenerator& generator, ChunkGrid& grid, OctreeManager& manager, Obj3dBP& cubebp, Obj3dPG& obj3d_prog, Texture* tex) {
	D(__PRETTY_FUNCTION__ << "\n");
	INFO(grid.getGridChecks() << "\n");

	#if M_DRAW_MINIMAP
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

	//todo use smart pointers (not for renderVecVoxels)
	for (auto i : manager.renderVec)
		delete i;
	for (auto i : manager.renderVecOctree)
		delete i;
	for (auto i : manager.renderVecGrid)
		delete i;
	//dont delete obj in renderVecVoxels as they're all present and already deleted in renderVec

	manager.renderVec.clear();
	manager.renderVecOctree.clear();
	manager.renderVecGrid.clear();
	for (size_t i = 0; i < 6; i++)
		manager.renderVecVoxels[i].clear();
	manager.renderVecChunk.clear();

	unsigned int	hiddenBlocks = 0;
	unsigned int	total_polygons = 0;
	unsigned int	cubgrid = 0;

	float	scale_coef = 0.99f;
	float	scale_coef2 = 0.95f;
	//scale_coef = 1;
	std::stringstream polygon_debug;
	polygon_debug << "chunks polygons: ";

	GridGeometry	geometry = grid.getGeometry();
	// rendered box
	Math::Vector3	startRendered = geometry.renderedGridIndex;// grid.getRenderedGridIndex();
	Math::Vector3	endRendered = startRendered + geometry.renderedGridSize; //grid.getRenderedSize();
#if 0 // all the grid
	startRendered = Math::Vector3();
	endRendered = geometry.gridSize; //Math::Vector3(generator.gridSize);
#endif
	D(" > rendered " << startRendered << " -> " << endRendered << "\n");

	if (1) {// actual grabbing + Obj3d creation
		grid.glth_loadAllChunksToGPU();//tmp_leak_check
		grid.pushRenderedChunks(&manager.renderVecChunk);//tmp_leak_check

		//for (auto x = 0; x < sizeArray; x++) {
		//	Obj3d* o = dynamic_cast<Obj3d*>(manager.renderVecChunk[x]);
		//	o->setPolygonMode(GL_LINE);
		//}

		//merge BPs
		if (M_MERGE_CHUNKS) {//merge BPs for a single draw call with the renderVecChunk
			INFO("Merging all chunks...\n");
			std::vector<SimpleVertex> vertices;
			std::vector<unsigned int> indices;
			for (auto obj : manager.renderVecChunk) {
				Obj3d* o = dynamic_cast<Obj3d*>(obj);
				if (!o) {
					D("dynamic cast failed on object: " << obj << "\n");
					Misc::breakExit(456);
				}
				Math::Vector3 pos = o->local.getPos();
				Obj3dBP* bp = o->getBlueprint();
				std::vector<SimpleVertex> verts = bp->getVertices();
				//offset the vertices with the obj3d pos
				std::for_each(verts.begin(), verts.end(), [pos](SimpleVertex& vertex) { vertex.position += pos; });
				vertices.insert(vertices.end(), verts.begin(), verts.end());
			}
			D_(std::cout << "\n");


			#ifndef RECREATE_FULLMESH
			// recreating full mesh without updating it in the ChunkGrid:: ?? todo: it should crash when entering a new chunk, check that and fix if needed
			Obj3dBP* fullMeshBP = grid.getFullMeshBP();
			Obj3d* fullMesh = grid.getFullMesh();
			D("Deleting old fullMesh...\n");
			if (fullMeshBP)
				delete fullMeshBP;
			if (fullMesh)
				delete fullMesh;
			D("Building new fullMesh...\n");
			fullMeshBP = new Obj3dBP(vertices, indices, BP_DONT_NORMALIZE);
			D("BP ready\n");
			fullMesh = new Obj3d(*fullMeshBP, obj3d_prog);
			D("Obj3d ready.\n");
			D("Done, " << manager.renderVecChunk.size() << " chunks merged.\n");
			manager.renderVecChunk.clear();
			manager.renderVecChunk.push_back(fullMesh);
			#endif
		}

	}

	#if M_DRAW_DEBUG
	#if M_DRAW_GRID_BOX
	{
		Math::Vector3 pos = geometry.gridWorldIndex; // grid.getWorldIndex();
		Math::Vector3 scale = geometry.gridSize;
		pos.scale(geometry.chunkSize);
		scale.scale(geometry.chunkSize);
		Obj3d* cubeGrid = new Obj3d(cubebp, obj3d_prog);
		cubeGrid->setColor(255, 0, 0);
		cubeGrid->local.setPos(pos);
		cubeGrid->local.setScale(scale);
		cubeGrid->setPolygonMode(GL_LINE);
		cubeGrid->displayTexture = false;
		manager.renderVecGrid.push_back(cubeGrid);
		cubgrid++;
	}
	#endif // M_DRAW_GRID_BOX
	ChunkShPtr*** chunkArray = grid.getGrid();
	for (unsigned int k = startRendered.z; k < endRendered.z; k++) {
		for (unsigned int j = startRendered.y; j < endRendered.y; j++) {
			for (unsigned int i = startRendered.x; i < endRendered.x; i++) {
				if (chunkArray[k][j][i]) {//at this point, the chunk might not be generated yet
					Chunk* chunk = chunkArray[k][j][i].get();

					#if M_DRAW_GRID_CHUNK
					{
						Obj3d* cubeGrid = new Obj3d(cubebp, obj3d_prog);
						cubeGrid->setColor(255, 0, 0);
						cubeGrid->local.setPos(chunk->pos);
						cubeGrid->local.setScale(chunk->size * 1); // scale_coef
						cubeGrid->setPolygonMode(GL_LINE);
						cubeGrid->displayTexture = false;
						manager.renderVecGrid.push_back(cubeGrid);
						cubgrid++;
					}
					#endif // M_DRAW_GRID_CHUNK

					if (0) {//coloring origin of each octree/chunk
						Math::Vector3	siz(1, 1, 1);
						#ifdef OCTREE_OLD
						Octree_old* root = chunk->root->getRoot(chunk->root->pos, siz);
						if (root) {
							if (root->size.len() != siz.len())
								root->pixel = Pixel(254, 0, 0);
							else
								root->pixel = Pixel(0, 255, 0);
						}
						#else // OCTREE_OLD
						Octree<Voxel>* node = chunk->root->getNode(chunk->root->pos, siz);
						if (node) {
							if (node->size.len() != siz.len())
								node->element._value = 254;
							else
								node->element._value = 253;
						}
						#endif // OCTREE_OLD
					}

					#if 0// oldcode, browsing to build 1 obj3d per cube (not chunk)
					chunk->root->browse([&manager, &cubebp, &obj3d_prog, scale_coef, scale_coef2, chunk, tex, &hiddenBlocks](Octree_old* node) {
						if (!node->isLeaf())
						return;
					if (M_DISPLAY_BLACK || (node->pixel.r != 0 && node->pixel.g != 0 && node->pixel.b != 0)) {// pixel 0?
						Math::Vector3	worldPos = chunk->pos + node->pos;
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
								manager.renderVec.push_back(cube);
							}
							else {
								// push only the faces next to an EMPTY_VOXEL in an all-in buffer
								manager.renderVec.push_back(cube);
								if ((node->neighbors & NEIGHBOR_FRONT) != NEIGHBOR_FRONT) { manager.renderVecVoxels[CUBE_FRONT_FACE].push_back(cube); }
								if ((node->neighbors & NEIGHBOR_RIGHT) != NEIGHBOR_RIGHT) { manager.renderVecVoxels[CUBE_RIGHT_FACE].push_back(cube); }
								if ((node->neighbors & NEIGHBOR_LEFT) != NEIGHBOR_LEFT) { manager.renderVecVoxels[CUBE_LEFT_FACE].push_back(cube); }
								if ((node->neighbors & NEIGHBOR_BOTTOM) != NEIGHBOR_BOTTOM) { manager.renderVecVoxels[CUBE_BOTTOM_FACE].push_back(cube); }
								if ((node->neighbors & NEIGHBOR_TOP) != NEIGHBOR_TOP) { manager.renderVecVoxels[CUBE_TOP_FACE].push_back(cube); }
								if ((node->neighbors & NEIGHBOR_BACK) != NEIGHBOR_BACK) { manager.renderVecVoxels[CUBE_BACK_FACE].push_back(cube); }
							}
							if (M_DISPLAY_BLACK) {
								Obj3d* cube2 = new Obj3d(cubebp, obj3d_prog);
								cube2->setColor(0, 0, 0);
								cube2->local.setPos(worldPos);
								cube2->local.setScale(node->size.x * scale_coef2, node->size.y * scale_coef2, node->size.z * scale_coef2);
								cube2->setPolygonMode(GL_LINE);
								cube2->displayTexture = false;
								manager.renderVecOctree.push_back(cube2);
							}
						}
					}
						});
					#endif
				}
			}
		}
	}
	#endif // M_DRAW_DEBUG

	//INFO("Counting total_polygons from renderVecChunk\n");
	for (Object* o : manager.renderVecChunk) {
		Obj3d* obj = dynamic_cast<Obj3d*>(o);
		if (!obj) { D("grabObjects() : dynamic cast failed on object: " << o << "\n"); Misc::breakExit(456); }
		total_polygons += obj->getBlueprint()->getPolygonAmount();
	}

	if (grid.playerChangedChunk)
		grid.playerChangedChunk = false;
	if (grid.chunksChanged)
		grid.chunksChanged = false;

	//INFO("polygon debug:\n" << polygon_debug.str() << "\n");
	//INFO("total polygons:\t" << total_polygons << "\n");
	//INFO("LOD:\t" << LOD << "\n");
	//INFO("hiddenBlocks:\t" << hiddenBlocks << "\n");
	//for (auto i : manager.renderVecVoxels)
	//	INFO("renderVecVoxels[]: " << i.size() << "\n");
	//INFO("renderVecOctree: " << manager.renderVecOctree.size() << "\n");
	//INFO("renderVecChunk: " << manager.renderVecChunk.size() << "\n");
	//INFO("renderVecGrid: " << manager.renderVecGrid.size() << "\n");
	//INFO("cubes grid : " << cubgrid << "\n");
	//D(D_SPACER_END);

	HeightMap::m.lock();
	INFO("HeightMap::count\t" << HeightMap::count << "\n");
	HeightMap::m.unlock();
	Chunk::m.lock();
	INFO("Chunk::count    \t" << Chunk::count << "\n");
	Chunk::m.unlock();
	return total_polygons;
}

unsigned int 	rebuildWithThreshold(ChunkGenerator& generator, ChunkGrid& grid, OctreeManager& manager, Obj3dBP& cubebp, Obj3dPG& obj3d_prog, Texture* tex) {
	int lod = 0;
	/*
		0 = no LOD, taking smallest voxels (size = 1)
		5 = log2(32), max level, ie the size of a chunk
	*/
	Math::Vector3	gridSize = grid.getSize();
	ChunkShPtr*** chunkArray = grid.getGrid();
	for (auto k = 0; k < gridSize.z; k++) {
		for (auto j = 0; j < gridSize.y; j++) {
			for (auto i = 0; i < gridSize.x; i++) {
				Octree<Voxel>* root = chunkArray[k][j][i]->root;
				chunkArray[k][j][i]->buildVertexArray(Math::Vector3(0, 0, 0), lod, manager.threshold);
				chunkArray[k][j][i]->glth_buildAllMeshes();
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
		D("static_cast failed\n");
		return;
	}
	if (action == GLFW_PRESS) {
		D("GLFW_PRESS:" << key << "\n");
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
*	current grid generation loop:
*		- [helper0] checks for player chunk change and shift grid if needed
*		- [helper0] builds jobs for missing hmaps and chunks
*		- [threads] execute jobs: build chunks by generating the octree with perlinsettings
*		- [helper0] delivers jobs: (shared_ptr<Chunk> are lost here, in ChunkGrid::replaceChunk())
						- build vertex array depending on LOD and octree_threshold, plug vertex in the corresponding vertex_array[lod]
						- plugs hmap/chunk in the grid
*		- [gl_main] build new meshes:
						- for all LODs if vertex_array[lod] is not empty
						- grabs all rendered chunks, selecting available meshes in all vertex_array[]
* 
*	todo:
*		- check memory leaks in chunks generation
*		- check every ctor by copy, they can access private members, useless to use accessors
*		- inconsistencys with the use of ref or ptr on some pipeline
*		- in ChunkGrid::updateGrid() : _deleteChunksAndHeightmaps(&chunksToDelete, &hmapsToDelete);
			! it has gl stuff in it, why it is currently done outside of the gl thread ?
*				-> this must be sent to some trashes to let the glth do it
*		- generate the vertexArray[lod] and its BP only when needed (when close enough from the cam/player)
*	done:
*	bugs :
*		- quand le renderedGrid.size = grid.size, race entre le renderer et le grid.updater
*
*	[Checklist] all gl calls have to be done on the gl context (here main thread)
*/
void	scene_octree() {
	#ifndef INIT_GLFW
	float	win_height = 900;
	float	win_width = 1600;
	OctreeManager	m;
	printSettings(m);
	std::this_thread::sleep_for(1s);

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
	m.renderVecSkybox.push_back(&skybox);
	Obj3dPG* renderer = &rendererObj3d;
	//renderer = &rendererObj3dInstanced;
	Chunk::renderer = &rendererObj3d;
#endif //MAIN_PG

	//Blueprint global settings
	Obj3dBP::config.modelSize = 1;
	Obj3dBP::config.dataMode = BP_LINEAR;
	Obj3dBP::config.rescale = true;
	Obj3dBP::config.center = false;
	Obj3dBP		cubebp(SIMPLEGL_FOLDER + "obj3d/cube.obj");
	Chunk::cubeBlueprint = &cubebp;
	Obj3dBP::config.dataMode = BP_INDICES;

	INFO(cubebp.lodManager.toString());
	//std::exit(0);

	Cam		cam(m.glfw->getWidth(), m.glfw->getHeight());
	cam.speed = m.playerSpeed;
	cam.printProperties();
	cam.lockedMovement = false;
	cam.lockedOrientation = false;
	//m.glfw->setMouseAngle(-1);//?
	D("MouseAngle: " << m.glfw->getMouseAngle() << "\n");
	m.cam = &cam;
	#endif //INIT_GLFW

	#ifndef BASE_OBJ3D
	Obj3d		cubeo(cubebp, *renderer);
	cubeo.local.setPos(0, 0, 0);
	cubeo.local.setScale(1, 1, 1);
	cubeo.setColor(255, 0, 0);
	cubeo.displayTexture = false;
	cubeo.setPolygonMode(GL_LINE);
	//m.renderVec.push_back(&cubeo);

	#endif

	cam.local.setPos(28, 50, 65);//buried close to the surface
	cam.local.setPos(280, 50, 65);//
	cam.local.setPos(320, 100, 65);//
	//cam.local.setPos(280, -100, 65);//crash
	//cam.local.setPos(-13, 76, 107);//crash
	Math::Vector3	playerPos = cam.local.getPos();

	#define USE_THREADS
	#ifdef USE_THREADS
	INFO("USE_THREADS : true\n");
	#else
	INFO("USE_THREADS : false\n");
	#endif

	#ifndef GENERATOR
	int grid_size = 25;
	if (0) {
		std::cout << "Enter grid size (min 7, max 35):\n";
		std::cin >> grid_size;
		grid_size = (grid_size < 7) ? 7 : grid_size;
		grid_size = (grid_size > 35) ? 35 : grid_size;
	}
	int	g = grid_size;
	int	r = grid_size - 4;// g * 2 / 3;
	m.gridSize = Math::Vector3(g, std::max(5,g/4), g);
	m.renderedGridSize = Math::Vector3(r, std::max(3,r/4), r);
	//m.gridSize = Math::Vector3(5, 5, 5);
	//m.renderedGridSize = Math::Vector3(5, 5, 5);
	INFO("Grid size : " << m.gridSize.toString() << "\n");
	INFO("Rebdered grid size : " << m.renderedGridSize.toString() << "\n");
	INFO("Total hmaps : " << m.gridSize.x * m.gridSize.z << "\n");
	INFO("Total chunks : " << m.gridSize.x * m.gridSize.y * m.gridSize.z << "\n");
	ChunkGrid		grid(m.chunk_size, m.gridSize, m.renderedGridSize);
	ChunkGenerator	generator(*m.ps);
	m.generator = &generator;
	m.grid = &grid;

	std::unique_lock<std::mutex> chunks_lock(grid.chunks_mutex, std::defer_lock);
	#ifdef USE_THREADS
	generator.initAllBuilders(m.cpuThreadAmount - 1, &cam, &grid);
	#endif

	#endif // GENERATOR

	Fps	fps(135);
	INFO("Maximum fps : " << fps.getMaxFps() << "\n");
	INFO("Maximum tick : " << fps.getMaxTick() << "\n");
	INFO("Tick : " << fps.getTick() << "\n");

	//#define MINIMAP // need to build a framebuffer with the entire map, update it each time the player changes chunk

	#ifdef USE_THREADS

	glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
	glfwFocusWindow(m.glfw->_window);
	std::this_thread::sleep_for(1s);

	unsigned int polygons = 0;
	unsigned int frames = 0;
	D("cam: " << cam.local.getPos().toString() << "\n");
	INFO("Begin while loop, renderer: " << typeid(renderer).name() << "\n");
	//std::cout.setstate(std::ios_base::failbit);



	while (!glfwWindowShouldClose(m.glfw->_window)) {
		if (fps.wait_for_next_frame()) {
			//std::this_thread::sleep_for(1s);
			frames++;
			if (frames % 1000 == 0) {
				D(">>>>>>>>>>>>>>>>>>>> " << frames << " FRAMES <<<<<<<<<<<<<<<<<<<<\n");
				D("cam: " << cam.local.getPos() << "\n");
			}
			std::string decimals = std::to_string(polygons / 1'000'000.0);
			m.glfw->setTitle(
				std::to_string(fps.getFps()) + " fps | "
				+ std::to_string(int(polygons/1'000'000)) + "m"
				+ ( decimals.c_str()+decimals.find('.')+1 )
				+ " polys (LOD_0) | threshold "
				+ std::to_string((int)m.threshold)
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
					INFO("grabbed " << m.renderVecChunk.size() << " in " << start*1000 << " ms\n");
					chunks_lock.unlock();
					// the generator can do what he wants with the grid, the renderer has what he needs for the current frame
				}
				generator.job_mutex.unlock();
				glFinish();// why here ?
			}

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			//D("renderVec\n")
			if (0) { // opti faces, not converted to mesh
				float speed = cam.speed;//save
				for (size_t i = 0; i < 6; i++) {
					cam.speed = i;//use this as a flag for the renderer, tmp
					renderer->renderAllObjects(m.renderVecVoxels[i], cam, PG_FORCE_DRAW);
				}
				cam.speed = speed;//restore
			}
			else if (0) {//whole cube
				renderer->renderAllObjects(m.renderVec, cam, PG_FORCE_DRAW);
			}
			else if (1) { // converted to mesh
				grid.chunks_mutex.lock();
				Chunk::renderer->renderAllObjects(m.renderVecChunk, cam, PG_FORCE_DRAW);//PG_FRUSTUM_CULLING PG_FORCE_DRAW
				//Chunk::renderer->renderAllObjectsMultiDraw(m.renderVecChunk, cam, PG_FORCE_DRAW);
				grid.chunks_mutex.unlock();
			}
			//D("octreeRender\n")
			//renderer->renderAllObjects(m.renderVecOctree, cam, PG_FORCE_DRAW);
#if (M_DRAW_GRID_BOX || M_DRAW_GRID_CHUNK)
			glDisable(GL_CULL_FACE);
			renderer->renderAllObjects(m.renderVecGrid, cam, PG_FORCE_DRAW);
			glEnable(GL_CULL_FACE);
#endif
			//rendererSkybox.renderAllObjects(m.renderVecSkybox, cam, PG_FORCE_DRAW);

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

			if (m.thresholdUpdated) {
				polygons = rebuildWithThreshold(generator, grid, m, cubebp, *renderer, tex_lena);//should be a job ?
				m.thresholdUpdated = false;
			}

			if (GLFW_PRESS == glfwGetKey(m.glfw->_window, GLFW_KEY_ESCAPE))
				glfwSetWindowShouldClose(m.glfw->_window, GLFW_TRUE);
		}
	}

	std::cout << "'ESCAPE' exiting...\n";
	std::cout.clear();
	//glfwMakeContextCurrent(nullptr);
	#else  // not USE_THREADS
	std::thread helper0(std::bind(&ChunkGenerator::th_updater, &generator, &cam, &grid));

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
			if (0 && grid.updateGrid(m.cam->local.getPos())) {
				grabObjects(generator, grid, m, cubebp, *renderer, tex_lena);
			}
			else if (1) {//with helper thread
				if (!generator.jobsToDo.empty()) {
					INFO("===================================== STARTING BUILDING =====================================\n");
					generator.executeAllJobs(generator.settings, std::string("[main thread]\t"));
					INFO("===================================== DONE BUILDING =====================================\n");
				}
				else if ((grid.playerChangedChunk || grid.chunksChanged) && chunks_lock.try_lock()) {
					//D("[renderer] lock chunks_mutex\n")
					double start = glfwGetTime();
					INFO(&generator << " : grabbing meshes...\n");
					grabObjects(generator, grid, m, cubebp, *renderer, tex_lena);
					start = glfwGetTime() - start;
					INFO("grabbed " << m.renderVecChunk.size() << " in " << start * 1000 << " ms\n");
					if (grid.playerChangedChunk)
						grid.playerChangedChunk = false;
					if (grid.chunksChanged)
						grid.chunksChanged = false;
					chunks_lock.unlock();
				}
			}

			//GLuint	mode = m.polygon_mode;
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			//D("renderVec\n")
			if (0) { // opti faces, not converted to mesh
				float speed = cam.speed;//save
				for (size_t i = 0; i < 6; i++) {
					cam.speed = i;//use this as a flag for the renderer, tmp
					renderer->renderAllObjects(m.renderVecVoxels[i], cam, PG_FORCE_DRAW);
				}
				cam.speed = speed;//restore
			}
			else if (0) {//whole cube
				renderer->renderAllObjects(m.renderVec, cam, PG_FORCE_DRAW);
			}
			else { // converted to mesh
				//D("Rendering " << m.renderVecChunk.size() << " Chunks\n");
				Chunk::renderer->renderAllObjects(m.renderVecChunk, cam, PG_FRUSTUM_CULLING);//PG_FRUSTUM_CULLING PG_FORCE_DRAW
			}
			//D("octree render\n")
			//renderer->renderAllObjects(m.renderVecOctree, cam, PG_FORCE_DRAW);
			//D("rendered grid\n")
			#if true
			glDisable(GL_CULL_FACE);
			renderer->renderAllObjects(m.renderVecGrid, cam, PG_FORCE_DRAW);
			glEnable(GL_CULL_FACE);
			#endif
			//rendererSkybox.renderAllObjects(m.renderVecSkybox, cam, PG_FORCE_DRAW);

			glfwSwapBuffers(m.glfw->_window);
			//generator.try_deleteUnusedData();

			if (GLFW_PRESS == glfwGetKey(m.glfw->_window, GLFW_KEY_ESCAPE))
				glfwSetWindowShouldClose(m.glfw->_window, GLFW_TRUE);
		}
	}
	D("End while loop\n");
	generator.terminateThreads = true;
	helper0.join();
	D("joined helper0\n");
	D("exiting...\n");
	#endif // USE_THREADS
}

void	maxUniforms() {
	D("GL_MAX_VERTEX_UNIFORM_COMPONENTS\t" << GL_MAX_VERTEX_UNIFORM_COMPONENTS << "\n");
	D("GL_MAX_GEOMETRY_UNIFORM_COMPONENTS\t" << GL_MAX_GEOMETRY_UNIFORM_COMPONENTS << "\n");
	D("GL_MAX_FRAGMENT_UNIFORM_COMPONENTS\t" << GL_MAX_FRAGMENT_UNIFORM_COMPONENTS << "\n");
	D("GL_MAX_TESS_CONTROL_UNIFORM_COMPONENTS\t" << GL_MAX_TESS_CONTROL_UNIFORM_COMPONENTS << "\n");
	D("GL_MAX_TESS_EVALUATION_UNIFORM_COMPONENTS\t" << GL_MAX_TESS_EVALUATION_UNIFORM_COMPONENTS << "\n");
	D("GL_MAX_COMPUTE_UNIFORM_COMPONENTS\t" << GL_MAX_COMPUTE_UNIFORM_COMPONENTS << "\n");
	D("GL_MAX_UNIFORM_LOCATIONS\t" << GL_MAX_UNIFORM_LOCATIONS << "\n");
	Misc::breakExit(0);
}

void	scene_checkMemory() {
	char input[100];
	Glfw*		glfw = new Glfw(WINX, WINY);
	Texture*	tex_bigass = new Texture(SIMPLEGL_FOLDER + "images/skybox4096.bmp");
	Obj3dPG		renderer(SIMPLEGL_FOLDER + OBJ3D_VS_FILE, SIMPLEGL_FOLDER + OBJ3D_FS_FILE);
	//Blueprint global settings
	Obj3dBP::config.modelSize = 1;
	Obj3dBP::config.dataMode = BP_LINEAR;
	Obj3dBP::config.rescale = true;
	Obj3dBP::config.center = false;
	Obj3dBP		cubebp(SIMPLEGL_FOLDER + "obj3d/cube.obj");
	Chunk::cubeBlueprint = &cubebp;
	Obj3dBP::config.dataMode = BP_INDICES;
	Chunk::renderer = &renderer;

	if (0/*texture*/) {
		D(">> Texture loaded\n");
		D("Texture unload load loop... ENTER\n");
		std::cin >> input;

		for (size_t i = 0; i < 100; i++) {
			//unload
			tex_bigass->unloadFromGPU();
			glfwSwapBuffers(glfw->_window);
			D(">> Texture unloaded\n");

			//load
			tex_bigass->loadToGPU();
			glfwSwapBuffers(glfw->_window);
			D(">> Texture loaded\n");
		}
	}
	if (0/*skybox*/) {
		SkyboxPG	rendererSkybox(SIMPLEGL_FOLDER + CUBEMAP_VS_FILE, SIMPLEGL_FOLDER + CUBEMAP_FS_FILE);

		for (size_t i = 0; i < 100; i++) {
			Skybox* sky = new Skybox(*tex_bigass, rendererSkybox);
			delete sky;
		}
	}
	if (1/*Chunks & heightmap*/) {
		siv::PerlinNoise	perlin(777);
		PerlinSettings		ps(perlin);
		HeightMap*			hmap = nullptr;
		Chunk*				chunk = nullptr;
		Math::Vector3		chunk_size(32, 32, 32);
		Math::Vector3		index(1, 2, 3);

		FOR(i, 0, 10000) {
			hmap = new HeightMap(ps, index, chunk_size);
			chunk = new Chunk(index, chunk_size, ps, hmap);
			FOR(lod, 0, LODS_AMOUNT) { chunk->buildVertexArray(Math::Vector3(), lod, 0); }
			//chunk->glth_buildAllMeshes();
			delete hmap;
			delete chunk;
			{
				int j = i + 1;
				if (j % 100 == 0)
					std::cout << ".";
				if (j % 5000 == 0)
					std::cout << " " << j << "\n";
			}



		}
	}

	std::cout << "end...\n";
	std::exit(0);
}

#define SSSIZE 2500
void	benchmark_octree() {
	Glfw glfw;
	//Blueprint global settings
	Obj3dBP::config.modelSize = 1;
	Obj3dBP::config.dataMode = BP_LINEAR;
	Obj3dBP::config.rescale = true;
	Obj3dBP::config.center = false;
	Obj3dBP		cubebp(SIMPLEGL_FOLDER + "obj3d/cube.obj");
	Chunk::cubeBlueprint = &cubebp;
	Obj3dBP::config.dataMode = BP_INDICES;

	OctreeManager	m;
	Math::Vector3	index(7, 2, 0);
	Math::Vector3	size(32, 32, 32);
	HeightMap* hmap = new HeightMap(*m.ps, index, size);
	Chunk* test = new Chunk(index, size, *m.ps, hmap);
	//test->glth_buildAllMeshes();
	if (test->meshBP) {
		D("polys: " << test->meshBP->getPolygonAmount() << "\n");
		test->meshBP->freeData(BP_FREE_ALL);
		delete test->meshBP;
		test->meshBP = nullptr;
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
	D("\n\n" << double(start) << std::endl);
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

	#define _DEBUG
	_CrtMemState sOld;
	_CrtMemState sNew;
	_CrtMemState sDiff;
	_CrtMemCheckpoint(&sOld); //take a snapchot

	//maxUniforms();
	//check_paddings();
	// test_behaviors();
	//test_mult_mat4(); Misc::breakExit(0);
	//	test_obj_loader();

	D("____START____ :\t" << Misc::getCurrentDirectory() << "\n");
	//test_memory_opengl_obj3dbp();
	//test_memory_opengl();
	//test_shared_ptr(); return 0;
	//testtype(true);	testtype(false); Misc::breakExit(0);
	//benchmark_octree();
	//scene_4Tree();
	//scene_procedural();
	//scene_benchmarks();
	//scene_checkMemory();
	scene_octree();
	//scene_test_thread();
	// while(1);

	D("____END____ : \n");
	_CrtMemCheckpoint(&sNew); //take a snapchot 
	if (1 || _CrtMemDifference(&sDiff, &sOld, &sNew)) // if there is a difference
	{
		OutputDebugStringA("-----------_CrtMemDumpStatistics ---------");
		_CrtMemDumpStatistics(&sDiff);
		OutputDebugStringA("-----------_CrtMemDumpAllObjectsSince ---------");
		_CrtMemDumpAllObjectsSince(&sOld);
		OutputDebugStringA("-----------_CrtDumpMemoryLeaks ---------");
		_CrtDumpMemoryLeaks();
	} else {
		D("no diff for memory check\n");
	}
	return (EXIT_SUCCESS);
}
#endif //main

#endif //main all
