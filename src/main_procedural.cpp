#include "trees.h"

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
		this->zoom_coef = 1;
		this->areaWidth = 200;
		this->areaHeight = 200;
		this->island = 0;
		this->island = std::clamp(this->island, -2.0, 2.0);
	}
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
	double			zoom_coef;
	int				areaWidth;
	int				areaHeight;
	double			island;
};

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	(void)window;(void)xoffset;
	// std::cout << xoffset << " | " << yoffset << std::endl;

	ProceduralManager* manager = static_cast<ProceduralManager*>(glfwGetWindowUserPointer(window));
	manager->zoom_coef += yoffset;
	manager->zoom_coef = std::clamp(manager->zoom_coef, 1.0, 10.0);
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

void TestPerlin() {
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

void	th_buildData(uint8_t* data, ProceduralManager & manager, int yStart, int yEnd) {
	const siv::PerlinNoise perlin(manager.seed);
	double playerPosX, playerPosY;
	playerPosX = manager.mouseX - (WINX / 2);//center of screen is 0:0
	playerPosY = WINY - manager.mouseY - (WINY / 2);//center of screen is 0:0   //invert glfw Y to match opengl image
	playerPosX += manager.posOffsetX;
	playerPosY += manager.posOffsetY;
	int screenCornerX, screenCornerY;
	screenCornerX = playerPosX - (manager.areaWidth / 2);
	screenCornerY = playerPosY - (manager.areaHeight / 2);


	for (int y = yStart; y < yEnd; ++y) {
		for (int x = 0; x < manager.areaWidth; ++x) {
			double value;
			double posX = screenCornerX + x;//pos of the generated pixel/elevation/data
			double posY = screenCornerY + y;
			double nx = double(posX) / double(manager.areaWidth);//normalised 0..1
			double ny = double(posY) / double(manager.areaHeight);

			value = perlin.accumulatedOctaveNoise2D_0_1(nx * manager.frequency,
				ny * manager.frequency,
				manager.octaves);
			value = std::pow(value, manager.flattering);
			Math::Vector3	vec(posX, posY, 0);
			double dist = (double(vec.magnitude()) / double(WINY / 2));//normalized 0..1
			value = std::clamp(value + manager.island * (0.5 - dist), 0.0, 1.0);

			int				index = (y * manager.areaWidth + x) * 3;
			uint8_t 		elevation = (uint8_t)(value * 255.0);
			Math::Vector3	color = genColor(elevation);
			data[index + 0] = color.x;
			data[index + 1] = color.y;
			data[index + 2] = color.z;
		}
	}
}

void	scene_procedural() {
	ProceduralManager	manager;
	manager.glfw = new Glfw(WINX, WINY);
	manager.glfw->toggleCursor();
	manager.glfw->setTitle("Tests procedural");
	manager.glfw->activateDefaultCallbacks(&manager);
	glfwSetScrollCallback(manager.glfw->_window, scroll_callback);
	const siv::PerlinNoise perlin(manager.seed);

	manager.frequency = double(manager.areaWidth) / 75;
	uint8_t*	data = new uint8_t[manager.areaWidth * manager.areaHeight * 3];
	Texture*	image = new Texture(data, manager.areaWidth, manager.areaHeight);

	UIImage	uiImage(image);
	uiImage.setPos(0, 0);
	uiImage.setSize(uiImage.getTexture()->getWidth() * manager.zoom_coef, uiImage.getTexture()->getHeight() * manager.zoom_coef);

	Fps	fps144(144);
	Fps	fps60(60);
	Fps	fps30(30);
	Fps* defaultFps = &fps30;

	unsigned int spare_thread_amount = manager.core_amount - 1;
	std::thread* threads_list = new std::thread[spare_thread_amount];

	std::cout << "Begin while loop" << endl;
	while (!glfwWindowShouldClose(manager.glfw->_window)) {
		if (defaultFps->wait_for_next_frame()) {

			glfwPollEvents();
			defaultFps->printFps();

			glfwGetCursorPos(manager.glfw->_window, &manager.mouseX, &manager.mouseY);
			int playerPosX = manager.mouseX - (WINX / 2);//center of screen is 0:0
			int playerPosY = WINY - manager.mouseY - (WINY / 2);//center of screen is 0:0   //invert glfw Y to match opengl image
			Math::Vector3	vec(playerPosX, playerPosY, 0);
			double dist = (double(vec.magnitude()) / double(WINY*2));

			int start, end;
			for (unsigned int i = 0; i < manager.core_amount; i++) {
				start = ((manager.areaHeight * (i + 0)) / manager.core_amount);
				end = ((manager.areaHeight * (i + 1)) / manager.core_amount);
				if (i < spare_thread_amount)//compute data with other threads
					threads_list[i] = std::thread(th_buildData, std::ref(data), std::ref(manager), start, end);
				else//current thread
					th_buildData(data, manager, start, end);
			}
			for (unsigned int i = 0; i < spare_thread_amount; i++) {
				threads_list[i].join();
			}

			image->updateData(data, manager.areaWidth, manager.areaHeight);
			uiImage.setPos(manager.mouseX - (manager.areaWidth * manager.zoom_coef / 2), WINY - manager.mouseY - (manager.areaHeight * manager.zoom_coef / 2));
			uiImage.setSize(uiImage.getTexture()->getWidth() * manager.zoom_coef, uiImage.getTexture()->getHeight() * manager.zoom_coef);

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			blitToWindow(nullptr, GL_COLOR_ATTACHMENT0, &uiImage);
			glfwSwapBuffers(manager.glfw->_window);

			int mvtSpeed = 5;
			if (GLFW_PRESS == glfwGetKey(manager.glfw->_window, GLFW_KEY_UP)) {
				manager.posOffsetY += mvtSpeed;
			}
			if (GLFW_PRESS == glfwGetKey(manager.glfw->_window, GLFW_KEY_DOWN)) {
				manager.posOffsetY -= mvtSpeed;
			}

			if (GLFW_PRESS == glfwGetKey(manager.glfw->_window, GLFW_KEY_RIGHT)) {
				manager.posOffsetX += mvtSpeed;
			}
			if (GLFW_PRESS == glfwGetKey(manager.glfw->_window, GLFW_KEY_LEFT)) {
				manager.posOffsetX -= mvtSpeed;
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
		}
	}

	std::cout << "End while loop" << endl;
	std::cout << "deleting textures..." << endl;
	delete[] threads_list;
	delete[] data;
	delete image;
	delete manager.glfw;
}

int		main(void) {
	TestPerlin();
	scene_procedural();
	return 0;
}
