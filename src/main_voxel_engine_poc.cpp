#include "trees.h"

void	renderObj3d(list<Obj3d*>	obj3dList, Cam& cam) {//make a renderObj3d for only 1 obj
	// cout << "render all Obj3d" << endl;
	//assuming all Obj3d have the same program
	if (obj3dList.empty())
		return;
	Obj3d*		obj = *(obj3dList.begin());
	Obj3dPG&	pg = obj->getProgram();
	glUseProgram(pg._program);//used once for all obj3d
	Math::Matrix4	proMatrix(cam.getProjectionMatrix());
	Math::Matrix4	viewMatrix = cam.getViewMatrix();
	proMatrix.mult(viewMatrix);// do it in shader ? NO cauz shader will do it for every vertix
	for (Obj3d* object : obj3dList)
		object->render(proMatrix);
	for (Obj3d* object : obj3dList) {
		object->local._matrixChanged = false;
		object->_worldMatrixChanged = false;
	}
}

void	renderSkybox(Skybox& skybox, Cam& cam) {
	// cout << "render Skybox" << endl;
	SkyboxPG&	pg = skybox.getProgram();
	glUseProgram(pg._program);//used once
	
	Math::Matrix4	proMatrix(cam.getProjectionMatrix());
	Math::Matrix4&	viewMatrix = cam.getViewMatrix();
	proMatrix.mult(viewMatrix);

	skybox.render(proMatrix);
}

void blitToWindow(FrameBuffer* readFramebuffer, GLenum attachmentPoint, UIPanel* panel) {
	GLuint fbo;
	int w;
	int h;
	if (readFramebuffer) {
		fbo = readFramebuffer->fbo;
		w = readFramebuffer->getWidth();
		h = readFramebuffer->getHeight();
	} else if (panel->getTexture()) {
		fbo = panel->getFbo();
		w = panel->getTexture()->getWidth();
		h = panel->getTexture()->getHeight();
	} else {
		std::cout << __PRETTY_FUNCTION__ << "failed" << std::endl;
		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		return ;
	}
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
	glReadBuffer(attachmentPoint);
	GLenum drawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, drawBuffers);

	glBlitFramebuffer(0, 0, w, h, \
		panel->_posX, panel->_posY, panel->_posX2, panel->_posY2, GL_COLOR_BUFFER_BIT, GL_NEAREST);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
}

class ProceduralManager : public GameManager {
public:
	ProceduralManager() : GameManager() {
		this->perlin = nullptr;
		this->core_amount = std::thread::hardware_concurrency();
		std::cout << " number of cores: " << this->core_amount << endl;
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
		//tmp
		this->mouseX = 0;
		this->mouseY = 0;
		this->areaWidth = 0;
		this->areaHeight = 0;
		this->island = 0;
		this->island = std::clamp(this->island, -2.0, 2.0);
		//vox
		this->player = nullptr;
		this->playerChunkX = 0;
		this->playerChunkY = 0;
		this->range_chunk_display = 3;
		this->range_chunk_memory = 15;
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
		775 680 * 9 = 6 981 120 octets (block de jeu) affiches
		6 981 120 * 6 * 2 = 83 773 440 polygones
		775 680 * 49 = 38 008 320 octets en memoire
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
	//tmp
	double			mouseX;
	double			mouseY;
	int				areaWidth;
	int				areaHeight;
	double			island;
	//vox
	std::list<Obj3d*>	renderlist;
	Object*			player;
	int				playerChunkX;
	int				playerChunkY;
	Cam*			cam;
	int				range_chunk_display;
	int				range_chunk_memory;
	int				chunk_size;
	int				voxel_size;
	GLuint			polygon_mode;
	int				threshold;
};

Math::Vector3	genColor(uint8_t elevation) {
	double value = double(elevation) / 255.0;
	Math::Vector3	color;

	if (elevation < 50) { // water
		color.x = 0;
		color.y = uint8_t(150.0 * std::clamp((double(elevation) / 50.0), 0.25, 1.0));
		color.z = uint8_t(255.0 * std::clamp((double(elevation) / 50.0), 0.25, 1.0));
	}
	else if (elevation < 75) { // sand
		color.x = 255.0 * ((double(elevation)) / 75.0);
		color.y = 200.0 * ((double(elevation)) / 75.0);
		color.z = 100.0 * ((double(elevation)) / 75.0);
	}
	else if (elevation > 200) { // snow
		color.x = elevation;
		color.y = elevation;
		color.z = elevation;
	}
	else if (elevation > 175) { // rocks
		color.x = 150.0 * value;
		color.y = 150.0 * value;
		color.z = 150.0 * value;
	}
	else {//grass
		color.x = 0;
		color.y = 200.0 * value;
		color.z = 100.0 * value;

	}
	return color;
}

void		buildChunk(ProceduralManager& manager, QuadNode* node, int chunkI, int chunkJ, int threshold) {//can be used for every tree node
	if (!node)
		return;
	//if (node->isLeaf()) {
	if (node->detail <= threshold) {
		//std::cout << "leaf: " << node->width << "x" << node->height << " at " << node->x << ":" << node->y << std::endl;
		if (node->width == 0 || node->height == 0) {
			std::cout << "error with tree data\n"; exit(2);
		}
		if (node->width * node->height >= DEBUG_LEAF_AREA && DEBUG_LEAF && DEBUG_BUILD_TOO) {
			std::cout << "Display new leaf: " << node->width << "x" << node->height << " at " << node->x << ":" << node->y << "\t";
			std::cout << (int)node->pixel.r << "  \t" << (int)node->pixel.g << "  \t" << (int)node->pixel.b << std::endl;
		}
	#ifndef RENDER_WORLD // make a special render func for that (or change buildChunk)
		Obj3d* cube = manager.renderlist.front();
		int polmode = cube->getPolygonMode();
		uint8_t	elevation = node->pixel.r;
		Math::Vector3	color = genColor(elevation);
		if (0 && (node->x == 0 || node->y == 0))
			cube->setColor(0,0,0);
		else
			cube->setColor(color.x, color.y, color.z);

		int worldPosX = manager.playerChunkX * manager.chunk_size + node->x + chunkI * manager.chunk_size - (manager.range_chunk_memory / 2 * manager.chunk_size);
		int worldPosY = manager.playerChunkY * manager.chunk_size + node->y + chunkJ * manager.chunk_size - (manager.range_chunk_memory / 2 * manager.chunk_size);

		cube->local.setPos(worldPosX, 0, worldPosY);
		cube->local.setScale(node->width, node->pixel.r / 3, node->height);// height is opengl z
		//cube->local.setScale(node->width, 1, node->height);// height is opengl z

		cube->setPolygonMode(manager.polygon_mode);
		renderObj3d(manager.renderlist, *manager.cam);
		cube->setPolygonMode(polmode);
	#endif
	}
	else if (node->children) {
		buildChunk(manager, node->children[0], chunkI, chunkJ, threshold);
		buildChunk(manager, node->children[1], chunkI, chunkJ, threshold);
		buildChunk(manager, node->children[2], chunkI, chunkJ, threshold);
		buildChunk(manager, node->children[3], chunkI, chunkJ, threshold);
	}
}

void		buildWorld(ProceduralManager & manager, QuadNode*** memory4Tree) {
	//std::cout << "building world...\n";

	int longTreshold = 12;
	int midTreshold = 8;
	int closeThrehold = 2;
	int threshold = 3;

	int startMid = (manager.range_chunk_memory / 2) - (20 / 2);
	int endMid = startMid + 20;
	int startClose = (manager.range_chunk_memory / 2) - (manager.range_chunk_display / 2);
	int endClose = startClose + manager.range_chunk_display;
	int	playerI = (manager.range_chunk_memory / 2);
	int	playerJ = (manager.range_chunk_memory / 2);

	// std::cout << startClose << " -> " << endClose << "\t/ " << manager.range_chunk_memory << std::endl;
	for (int j = 0; j < manager.range_chunk_memory; j++) {
		for (int i = 0; i < manager.range_chunk_memory; i++) {
			//std::cout << j << ":" << i << std::endl;
			if (i == playerI && j == playerJ)//player chunk
				threshold = manager.threshold;
			else if (i >= startClose && i < endClose && j >= startClose && j < endClose)//around player
				threshold = closeThrehold;
			else if (i >= startMid && i < endMid && j >= startMid && j < endMid)//around player
				threshold = midTreshold;
			buildChunk(manager, memory4Tree[j][i], i, j, threshold);
			threshold = longTreshold;
		}
	}
}

uint8_t*	generatePerlinNoise(ProceduralManager& manager, int posX, int posY, int width, int height) {
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
			double dist = (double(vec.magnitude()) / double(WINY / 2));//normalized 0..1
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

void	updateChunksX(ProceduralManager& manager, QuadNode*** chunkMemory4Tree, uint8_t*** chunkMemory, int change) {
	if (abs(change) > 1) {
		std::cout << "player went too fast on X, or teleported" << std::endl;
		exit(1);
	}

	bool displayDebug = false;

	manager.playerChunkX += change;
	int startX = manager.playerChunkX * manager.chunk_size - (manager.chunk_size * manager.range_chunk_memory / 2);
	int startY = manager.playerChunkY * manager.chunk_size - (manager.chunk_size * manager.range_chunk_memory / 2);

	if (change > 0) {//if we went on right, we shift everything on left, and rebuild last column
		for (int i = 0; i < manager.range_chunk_memory; i++) {
			for (int j = 0; j < manager.range_chunk_memory; j++) {
				if (i == 0) {//delete first column
					delete[] chunkMemory[j][i];
					delete chunkMemory4Tree[j][i];
					if (displayDebug)
						std::cout << "delete first column on line " << j << " column " << i << std::endl;
				}
				if (i == manager.range_chunk_memory - 1) {//last column, rebuild
					if (displayDebug)
						std::cout << "rebuild last column on line " << j << " column " << i << std::endl;
					int x, y, w, h;
					x = startX + manager.chunk_size * i;
					y = startY + manager.chunk_size * j;
					w = manager.chunk_size;
					h = manager.chunk_size;
					chunkMemory[j][i] = generatePerlinNoise(manager, x, y, w, h);
					chunkMemory4Tree[j][i] = new QuadNode(chunkMemory[j][i], w, 0, 0, w, h, 0);
				}
				else {//shift on left
					if (displayDebug)
						std::cout << "shift on left on line " << j << " column " << i << std::endl;
					chunkMemory[j][i] = chunkMemory[j][i + 1];
					chunkMemory4Tree[j][i] = chunkMemory4Tree[j][i + 1];
				}
			}
			if (displayDebug)
				std::cout << "-------------\n";
		}
	}
	else if (change < 0) {//if we went on left, ie shift everything on right, rebuild first column
		for (int i = manager.range_chunk_memory - 1; i >= 0; i--) {
			for (int j = 0; j < manager.range_chunk_memory; j++) {
				if (i == manager.range_chunk_memory - 1) {//delete last column
					delete[] chunkMemory[j][i];
					delete chunkMemory4Tree[j][i];
					if (displayDebug)
						std::cout << "delete last column on line " << j << " column " << i << std::endl;
				}
				if (i == 0) {//first column, rebuild
					if (displayDebug)
						std::cout << "rebuild first column on line " << j << " column " << i << std::endl;
					int x, y, w, h;
					x = startX + manager.chunk_size * i;
					y = startY + manager.chunk_size * j;
					w = manager.chunk_size;
					h = manager.chunk_size;
					chunkMemory[j][i] = generatePerlinNoise(manager, x, y, w, h);
					chunkMemory4Tree[j][i] = new QuadNode(chunkMemory[j][i], w, 0, 0, w, h, 0);
				}
				else {//shift on right
					if (displayDebug)
						std::cout << "shift on right on line " << j << " column " << i << std::endl;
					chunkMemory[j][i] = chunkMemory[j][i - 1];
					chunkMemory4Tree[j][i] = chunkMemory4Tree[j][i - 1];
				}
			}
			if (displayDebug)
				std::cout << "-------------\n";
		}
	}
	std::cout << "updated Chunks X\n";
}

void	updateChunksY(ProceduralManager& manager, QuadNode*** chunkMemory4Tree, uint8_t*** chunkMemory, int change) {
	if (abs(change) > 1) {
		std::cout << "player went too fast on Y, or teleported" << std::endl;
		exit(1);
	}

	bool displayDebug = false;

	manager.playerChunkY += change;
	int startX = manager.playerChunkX * manager.chunk_size - (manager.chunk_size * manager.range_chunk_memory / 2);
	int startY = manager.playerChunkY * manager.chunk_size - (manager.chunk_size * manager.range_chunk_memory / 2);

	if (change > 0) {//if we went on bottom, we shift everything on top, and rebuild last column
		for (int j = 0; j < manager.range_chunk_memory; j++) {
			for (int i = 0; i < manager.range_chunk_memory; i++) {
				if (j == 0) {//delete first line
					delete[] chunkMemory[j][i];
					delete chunkMemory4Tree[j][i];
					if (displayDebug)
						std::cout << "delete first line on column " << i << " line " << j << std::endl;
				}
				if (j == manager.range_chunk_memory - 1) {//last line, rebuild
					if (displayDebug)
						std::cout << "rebuild last line on column " << i << " line " << j << std::endl;
					int x, y, w, h;
					x = startX + manager.chunk_size * i;
					y = startY + manager.chunk_size * j;
					w = manager.chunk_size;
					h = manager.chunk_size;
					chunkMemory[j][i] = generatePerlinNoise(manager, x, y, w, h);
					chunkMemory4Tree[j][i] = new QuadNode(chunkMemory[j][i], w, 0, 0, w, h, 0);
				}
				else {//shift on top
					if (displayDebug)
						std::cout << "shift on top on column " << i << " line " << j << std::endl;
					chunkMemory[j][i] = chunkMemory[j + 1][i];
					chunkMemory4Tree[j][i] = chunkMemory4Tree[j + 1][i];
				}
			}
			if (displayDebug)
				std::cout << "-------------\n";
		}
	}
	else if (change < 0) {//if we went on left, ie shift everything on right, rebuild first column
		for (int j = manager.range_chunk_memory - 1; j >= 0; j--) {
			for (int i = 0; i < manager.range_chunk_memory; i++) {
				if (j == manager.range_chunk_memory - 1) {//delete last column
					delete[] chunkMemory[j][i];
					delete chunkMemory4Tree[j][i];
					if (displayDebug)
						std::cout << "delete first line on column " << i << " line " << j << std::endl;
				}
				if (j == 0) {//first column, rebuild
					if (displayDebug)
						std::cout << "rebuild last line on column " << i << " line " << j << std::endl;
					int x, y, w, h;
					x = startX + manager.chunk_size * i;
					y = startY + manager.chunk_size * j;
					w = manager.chunk_size;
					h = manager.chunk_size;
					chunkMemory[j][i] = generatePerlinNoise(manager, x, y, w, h);
					chunkMemory4Tree[j][i] = new QuadNode(chunkMemory[j][i], w, 0, 0, w, h, 0);
				}
				else {//shift on bottom
					if (displayDebug)
						std::cout << "shift on top on column " << i << " line " << j << std::endl;
					chunkMemory[j][i] = chunkMemory[j - 1][i];
					chunkMemory4Tree[j][i] = chunkMemory4Tree[j - 1][i];
				}
			}
			if (displayDebug)
				std::cout << "-------------\n";
		}
	}
	std::cout << "updated Chunks Y\n";
}


static void		keyCallback_vox(GLFWwindow* window, int key, int scancode, int action, int mods) {
	(void)window; (void)key; (void)scancode; (void)action; (void)mods;
	//std::cout << __PRETTY_FUNCTION__ << std::endl;

	if (action == GLFW_PRESS) {
		//std::cout << "GLFW_PRESS" << std::endl;
		ProceduralManager* manager = static_cast<ProceduralManager*>(glfwGetWindowUserPointer(window));
		if (!manager) {
			std::cout << "static_cast failed" << std::endl;
		}
		else if (manager->glfw) {
			if (key == GLFW_KEY_EQUAL) {
				manager->threshold++;
				manager->glfw->setTitle(std::to_string(manager->threshold).c_str());
			}
			else if (key == GLFW_KEY_MINUS && manager->threshold > 0) {
				manager->threshold--;
				manager->glfw->setTitle(std::to_string(manager->threshold).c_str());
			}
			else if (key == GLFW_KEY_ENTER) {
				manager->polygon_mode++;
				manager->polygon_mode = GL_POINT + (manager->polygon_mode % 3);
			}
		}
	}
}

void	fillData(uint8_t* dst, QuadNode* node, int* leafAmount, int baseWidth, bool draw_borders, int threshold, Math::Vector3 color) {
	if (!node)
		return;
	//if (node->isLeaf()) {
	if (node->detail <= threshold) {
		//(*leafAmount)++;
		//std::cout << "leaf: " << node->width << "x" << node->height << " at " << node->x << ":" << node->y << std::endl;
		if (node->width == 0 || node->height == 0) {
			std::cout << "error with tree data\n"; exit(2);
		}
		if (node->width * node->height >= DEBUG_LEAF_AREA && DEBUG_LEAF && *leafAmount == 0 && DEBUG_FILL_TOO) {
			std::cout << "Fill new leaf: " << node->width << "x" << node->height << " at " << node->x << ":" << node->y << "\t";
			std::cout << (int)node->pixel.r << "  \t" << (int)node->pixel.g << "  \t" << (int)node->pixel.b << std::endl;
		}
		for (unsigned int j = 0; j < node->height; j++) {
			for (unsigned int i = 0; i < node->width; i++) {
				unsigned int posx = node->x + i;
				unsigned int posy = node->y + j;
				unsigned int index = (posy * baseWidth + posx) * 3;
				//std::cout << ((posy * baseWidth + posx) * 3 + 0) << std::endl;
				//std::cout << ((posy * baseWidth + posx) * 3 + 1) << std::endl;
				//std::cout << ((posy * baseWidth + posx) * 3 + 2) << std::endl;
				if (draw_borders && (i == 0 || j == 0)) {
					dst[index + 0] = 0;
					dst[index + 1] = 0;
					dst[index + 2] = 0;
				} else {
					if (1 && (posx == 0 || posy == 0)) {
						dst[index + 0] = color.x;
						dst[index + 1] = color.y;
						dst[index + 2] = color.z;
					} else {
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

void	scene_vox() {
#ifndef INIT_GLFW
	Obj3dBP::defaultSize = 1;
	ProceduralManager	manager;
	siv::PerlinNoise perlin(manager.seed);
	manager.perlin = &perlin;
	manager.glfw = new Glfw(WINX, WINY);
	glDisable(GL_CULL_FACE);
	manager.glfw->setTitle("Tests vox");
	manager.glfw->activateDefaultCallbacks(&manager);
	manager.glfw->func[GLFW_KEY_EQUAL] = keyCallback_vox;
	manager.glfw->func[GLFW_KEY_MINUS] = keyCallback_vox;
	manager.glfw->func[GLFW_KEY_ENTER] = keyCallback_vox;

	Obj3dPG		obj3d_prog(SIMPLEGL_FOLDER + OBJ3D_VS_FILE, SIMPLEGL_FOLDER + OBJ3D_FS_FILE);
	SkyboxPG	sky_pg(SIMPLEGL_FOLDER + CUBEMAP_VS_FILE, SIMPLEGL_FOLDER + CUBEMAP_FS_FILE);

	Obj3dBP		cubebp(SIMPLEGL_FOLDER + "obj3d/cube.obj", true, false);

	Texture*	tex_skybox = new Texture(SIMPLEGL_FOLDER + "images/skybox4096.bmp");
	Skybox		skybox(*tex_skybox, sky_pg);

	Cam		cam(*(manager.glfw));
	cam.speed = 4*15;
	cam.local.setPos(0, 30, 0);
	cam.printProperties();
	cam.lockedMovement = false;
	cam.lockedOrientation = false;
	//manager.glfw->setMouseAngle(-1);//?
	std::cout << "MouseAngle: " << manager.glfw->getMouseAngle() << std::endl;
	manager.cam = &cam;

	Fps	fps144(144);
	Fps	fps60(60);
	Fps* defaultFps = &fps60;

#endif// INIT_GLFW

#ifndef BASE_OBJ3D
	Obj3d		cubeo(cubebp, obj3d_prog);
	cubeo.local.setPos(0, 0, 0);
	cubeo.local.setScale(1, 1, 1);
	cubeo.setColor(255, 0, 0);
	cubeo.displayTexture = false;
	cubeo.setPolygonMode(GL_LINE);
	manager.renderlist.push_back(&cubeo);

	Obj3d		player1(cubebp, obj3d_prog);
	player1.local.setPos(0, 35, 0);
	player1.local.setScale(1, 2, 1);
	player1.local.enlarge(5, 5, 5);
	player1.setColor(255, 0, 0);
	player1.displayTexture = false;
	player1.setPolygonMode(GL_FILL);
	manager.player = &player1;
	manager.player = &cam;

	std::list<Obj3d*>	playerList;
	playerList.push_back(&player1);
#endif

#ifndef CHUNKS
	// build the heightmaps depending of tile size (for now 7x7 tiles, tile is 30x30 cubes)
	std::cout << "manager.chunk_size " << manager.chunk_size << std::endl;
	std::cout << "manager.range_chunk_memory " << manager.range_chunk_memory << std::endl;
	std::cout << "manager.range_chunk_display " << manager.range_chunk_display << std::endl;

	uint8_t*** chunkMemory = new uint8_t**[manager.range_chunk_memory];
	QuadNode*** chunkMemory4Tree = new QuadNode**[manager.range_chunk_memory];
	Math::Vector3	playerPos = player1.local.getPos();
	int	playerChunkX = (int)playerPos.x / manager.chunk_size;
	int	playerChunkY = (int)playerPos.z / manager.chunk_size;//opengl y is height, so we use opengl z here
	int startX = playerChunkX * manager.chunk_size - (manager.chunk_size * manager.range_chunk_memory / 2);
	int startY = playerChunkY * manager.chunk_size - (manager.chunk_size * manager.range_chunk_memory / 2);
	for (int j = 0; j < manager.range_chunk_memory; j++) {
		chunkMemory[j] = new uint8_t*[manager.range_chunk_memory];
		chunkMemory4Tree[j] = new QuadNode*[manager.range_chunk_memory];
		for (int i = 0; i < manager.range_chunk_memory; i++) {
			//std::cout << i << ":" << j << std::endl;
			chunkMemory[j][i] = nullptr;
			chunkMemory4Tree[j][i] = nullptr;
			int x, y, w, h;
			x = startX + manager.chunk_size * i;
			y = startY + manager.chunk_size * j;
			w = manager.chunk_size;
			h = manager.chunk_size;
			chunkMemory[j][i] = generatePerlinNoise(manager, x, y, w, h);//make a class Rect ?
			chunkMemory4Tree[j][i] = new QuadNode(chunkMemory[j][i], w, 0, 0, w, h, 0);
		}
	}
	int startDisplay = (manager.range_chunk_memory - manager.range_chunk_display) / 2;
	int endDisplay = startDisplay + manager.range_chunk_display;
	int memoryTotalRange = manager.chunk_size * manager.range_chunk_memory;
	uint8_t* dataChunkMemory = new uint8_t[3 * memoryTotalRange * memoryTotalRange];
	for (int j = 0; j < manager.range_chunk_memory; j++) {
		for (int i = 0; i < manager.range_chunk_memory; i++) {
				int leafamount = 0;
				Math::Vector3 color(0, 0, 0);
				if ((i >= startDisplay && i < endDisplay) && (j >= startDisplay && j < endDisplay))
					color = Math::Vector3(255, 0, 0);
				fillData(dataChunkMemory + 3 * (j * memoryTotalRange*manager.chunk_size + i*manager.chunk_size),
					chunkMemory4Tree[j][i], &leafamount, memoryTotalRange, false, 0, color);
		}
	}

	Texture*	grid = new Texture(dataChunkMemory, memoryTotalRange, memoryTotalRange);
	UIImage		gridPanel(grid);
	gridPanel.setPos((WINX / 2) + manager.playerChunkX * manager.chunk_size - (manager.range_chunk_memory / 2 * manager.chunk_size),
					(WINY / 2) + manager.playerChunkY * manager.chunk_size - (manager.range_chunk_memory / 2 * manager.chunk_size));//z cauz opengl
	gridPanel.setSize(memoryTotalRange, memoryTotalRange);
#endif // CHUNKS



	//int thread_amount = manager.core_amount - 1;
	//std::thread* threads_list = new std::thread[thread_amount];

	std::cout << "Begin while loop" << endl;
	while (!glfwWindowShouldClose(manager.glfw->_window)) {
		if (defaultFps->wait_for_next_frame()) {

			glfwPollEvents();
			manager.glfw->updateMouse();//to do before cam's events
			cam.events(*(manager.glfw), float(defaultFps->getTick()));

			//cubeb.local.rotate(50 * defaultFps->getTick(), 50 * defaultFps->getTick(), 0);

			//defaultFps->printFps();

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			buildWorld(manager, chunkMemory4Tree);
			renderObj3d(playerList, cam);
			renderSkybox(skybox, cam);
			//blitToWindow(nullptr, GL_COLOR_ATTACHMENT0, &gridPanel);
			glfwSwapBuffers(manager.glfw->_window);


#ifndef KEY_EVENTS
			int mvtSpeed = 5;
			if (GLFW_PRESS == glfwGetKey(manager.glfw->_window, GLFW_KEY_UP)) {
				manager.posOffsetY += mvtSpeed;
				player1.local.translate(VEC3_FORWARD);
			}
			if (GLFW_PRESS == glfwGetKey(manager.glfw->_window, GLFW_KEY_DOWN)) {
				manager.posOffsetY -= mvtSpeed;
				player1.local.translate(VEC3_BACKWARD);
			}

			if (GLFW_PRESS == glfwGetKey(manager.glfw->_window, GLFW_KEY_RIGHT)) {
				manager.posOffsetX += mvtSpeed;
				player1.local.translate(VEC3_RIGHT);
			}
			if (GLFW_PRESS == glfwGetKey(manager.glfw->_window, GLFW_KEY_LEFT)) {
				manager.posOffsetX -= mvtSpeed;
				player1.local.translate(VEC3_LEFT);
			}

			if (GLFW_PRESS == glfwGetKey(manager.glfw->_window, GLFW_KEY_SPACE)) {
				player1.local.translate(VEC3_UP);
			}
			if (GLFW_PRESS == glfwGetKey(manager.glfw->_window, GLFW_KEY_C)) {
				player1.local.translate(VEC3_DOWN);
			}


			if (GLFW_PRESS == glfwGetKey(manager.glfw->_window, GLFW_KEY_KP_7)) {
				manager.frequency += 0.1;
				manager.frequency = std::clamp(manager.frequency, 0.1, 64.0);
			}
			if (GLFW_PRESS == glfwGetKey(manager.glfw->_window, GLFW_KEY_KP_4)) {
				manager.frequency -= 0.1;
				manager.frequency = std::clamp(manager.frequency, 0.1, 64.0);
			}

			if (GLFW_PRESS == glfwGetKey(manager.glfw->_window, GLFW_KEY_KP_8)) {
				manager.flattering += 0.1;
				manager.flattering = std::clamp(manager.flattering, 0.01, 10.0);
			}
			if (GLFW_PRESS == glfwGetKey(manager.glfw->_window, GLFW_KEY_KP_5)) {
				manager.flattering -= 0.1;
				manager.flattering = std::clamp(manager.flattering, 0.01, 10.0);
			}

			if (GLFW_PRESS == glfwGetKey(manager.glfw->_window, GLFW_KEY_KP_9)) {
				manager.island += 0.05;
				manager.island = std::clamp(manager.island, 0.01, 2.0);
			}
			if (GLFW_PRESS == glfwGetKey(manager.glfw->_window, GLFW_KEY_KP_6)) {
				manager.island -= 0.05;
				manager.island = std::clamp(manager.island, -2.0, 2.0);
			}

			if (GLFW_PRESS == glfwGetKey(manager.glfw->_window, GLFW_KEY_ESCAPE))
				glfwSetWindowShouldClose(manager.glfw->_window, GLFW_TRUE);
#endif
			Math::Vector3	playerpos = manager.player->local.getPos();
			int currentchunkX = int(playerpos.x / manager.chunk_size);
			int currentchunkY = int(playerpos.z / manager.chunk_size);//opengl y is height, so we use opengl z here
			if (playerpos.x < 0) // cauz x=-29..+29 ----> x / 30 = 0; 
				currentchunkX--;
			if (playerpos.z < 0) // same
				currentchunkY--;
			if (currentchunkX != manager.playerChunkX) {
				updateChunksX(manager, chunkMemory4Tree, chunkMemory, currentchunkX - manager.playerChunkX);
				std::cout << "===<<< Player changed chunk X: " << manager.playerChunkX << ":" << manager.playerChunkY << std::endl;
				std::cout << "filling data to texture... ";
				for (int j = 0; j < manager.range_chunk_memory; j++) {
					for (int i = 0; i < manager.range_chunk_memory; i++) {
						int leafamount = 0;
						Math::Vector3 color(0, 0, 0);
						if ((i >= startDisplay && i < endDisplay) && (j >= startDisplay && j < endDisplay))
							color = Math::Vector3(255, 0, 0);
						fillData(dataChunkMemory + 3 * (j * memoryTotalRange * manager.chunk_size + i * manager.chunk_size),
							chunkMemory4Tree[j][i], &leafamount, memoryTotalRange, false, 0, color);
					}
				}
				std::cout << "Done" << std::endl;
				gridPanel.getTexture()->updateData(dataChunkMemory, memoryTotalRange, memoryTotalRange);
				playerPos = player1.local.getPos();
				gridPanel.setPos((WINX / 2) + manager.playerChunkX * manager.chunk_size - (manager.range_chunk_memory / 2 * manager.chunk_size),
								(WINY / 2) + manager.playerChunkY * manager.chunk_size - (manager.range_chunk_memory / 2 * manager.chunk_size));//z cauz opengl
				gridPanel.setSize(memoryTotalRange, memoryTotalRange);
			}
			if (currentchunkY != manager.playerChunkY) {
				updateChunksY(manager, chunkMemory4Tree, chunkMemory, currentchunkY - manager.playerChunkY);
				std::cout << "===<<< Player changed chunk Y: " << manager.playerChunkX << ":" << manager.playerChunkY << std::endl;
				std::cout << "filling data to texture... ";
				for (int j = 0; j < manager.range_chunk_memory; j++) {
					for (int i = 0; i < manager.range_chunk_memory; i++) {
						int leafamount = 0;
						Math::Vector3 color(0, 0, 0);
						if ((i >= startDisplay && i < endDisplay) && (j >= startDisplay && j < endDisplay))
							color = Math::Vector3(255, 0, 0);
						fillData(dataChunkMemory + 3 * (j * memoryTotalRange * manager.chunk_size + i * manager.chunk_size),
							chunkMemory4Tree[j][i], &leafamount, memoryTotalRange, false, 0, color);
					}
				}
				std::cout << "Done" << std::endl;
				gridPanel.getTexture()->updateData(dataChunkMemory, memoryTotalRange, memoryTotalRange);
				playerPos = player1.local.getPos();
				gridPanel.setPos((WINX / 2) + manager.playerChunkX * manager.chunk_size - (manager.range_chunk_memory / 2 * manager.chunk_size),
								(WINY / 2) + manager.playerChunkY * manager.chunk_size - (manager.range_chunk_memory / 2 * manager.chunk_size));//z cauz opengl
				gridPanel.setSize(memoryTotalRange, memoryTotalRange);
			}

		}
	}
	//delete[] threads_list;

	std::cout << "End while loop" << endl;
	std::cout << "deleting textures..." << endl;
	delete tex_skybox;
}

int		main(void) {
	scene_vox();
	return (EXIT_SUCCESS);
}
