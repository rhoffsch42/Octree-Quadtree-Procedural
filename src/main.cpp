#include "trees.h"

class QuadTreeManager : public GameManager {
public:
	QuadTreeManager() : GameManager() {
		this->threshold = 0;
		this->draw_borders = false;
	}
	virtual ~QuadTreeManager() {}
	unsigned int	threshold;
	bool			draw_borders;
};

void blitToWindow(FrameBuffer* readFramebuffer, GLenum attachmentPoint, UIPanel* panel) {
	GLuint fbo;
	if (readFramebuffer) {
		fbo = readFramebuffer->fbo;
	}
	else {
		fbo = panel->getFbo();
	}
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);

	glReadBuffer(attachmentPoint);
	GLenum drawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, drawBuffers);

	int w;
	int h;
	if (readFramebuffer) {
		w = readFramebuffer->getWidth();
		h = readFramebuffer->getHeight();
	} else if (panel->getTexture()) {
		w = panel->getTexture()->getWidth();
		h = panel->getTexture()->getHeight();
	} else {
		std::cout << __PRETTY_FUNCTION__ << "failed" << std::endl;
		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		return ;
	}
	glBlitFramebuffer(0, 0, w, h, \
		panel->_posX, panel->_posY, panel->_posX2, panel->_posY2, GL_COLOR_BUFFER_BIT, GL_NEAREST);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
}

void		keyCallback_quadTree(GLFWwindow* window, int key, int scancode, int action, int mods) {
	(void)window; (void)key; (void)scancode; (void)action; (void)mods;
	//std::cout << __PRETTY_FUNCTION__ << std::endl;

	if (action == GLFW_PRESS) {
		//std::cout << "GLFW_PRESS" << std::endl;
		QuadTreeManager* manager = static_cast<QuadTreeManager*>(glfwGetWindowUserPointer(window));
		if (!manager) {
			std::cout << "static_cast failed" << std::endl;
		} else if (manager->glfw) {
			if (key == GLFW_KEY_EQUAL) {
				manager->threshold++;
				manager->glfw->setTitle(std::string("threshold: ") + std::to_string(manager->threshold).c_str());
			}
			else if (key == GLFW_KEY_MINUS && manager->threshold > 0) {
				manager->threshold--;
				manager->glfw->setTitle(std::string("threshold: ") + std::to_string(manager->threshold).c_str());
			}
			else if (key == GLFW_KEY_ENTER)
				manager->draw_borders = !manager->draw_borders;
		}
	}
}

void	scene_4Tree() {
	QuadTreeManager	manager;
	manager.glfw = new Glfw(WINX, WINY);
	manager.glfw->setTitle(std::string("threshold: ") + std::to_string(manager.threshold).c_str());
	manager.glfw->activateDefaultCallbacks(&manager);
	manager.glfw->func[GLFW_KEY_EQUAL] = keyCallback_quadTree;
	manager.glfw->func[GLFW_KEY_MINUS] = keyCallback_quadTree;
	manager.glfw->func[GLFW_KEY_ENTER] = keyCallback_quadTree;

	Texture* lena = new Texture(SIMPLEGL_FOLDER + "images/lena.bmp");
	Texture* flower = new Texture(SIMPLEGL_FOLDER + "images/flower.bmp");

	Texture* baseImage = flower;
	int width = baseImage->getWidth();
	int height = baseImage->getHeight();
	QuadNode* root = new QuadNode(baseImage->getData(), width, 0, 0, width, height, 0);
	uint8_t* data4Tree = new uint8_t[width * height * 3];

	float size_coef = float(WINX) / float(width) / 2.0f;
	if (size_coef * float(height) > float(WINY))
		size_coef = float(WINY) / float(height) / 2.0f;; 
	UIImage	uiBaseImage(baseImage);
	uiBaseImage.setPos(0, 0);
	uiBaseImage.setSize(width * size_coef, height * size_coef);

	Texture* image4Tree = new Texture(data4Tree, width, height);
	UIImage	ui4Tree(image4Tree);
	ui4Tree.setPos(width * size_coef, 0);
	ui4Tree.setSize(width * size_coef, height * size_coef);

	Fps	fps144(144);
	Fps	fps60(60);
	Fps* defaultFps = &fps60;
	Math::Vector3	color = Math::Vector3(0,0,0);

	std::cout << "Begin while loop" << endl;
	while (!glfwWindowShouldClose(manager.glfw->_window)) {
		if (defaultFps->wait_for_next_frame()) {

			glfwPollEvents();

			bool draw = manager.draw_borders;
			root->browse(manager.threshold, [data4Tree, width, draw, color](QuadNode* node){
				for (unsigned int j = 0; j < node->height; j++) {
					for (unsigned int i = 0; i < node->width; i++) {
						unsigned int posx = node->x + i;
						unsigned int posy = node->y + j;
						unsigned int index = (posy * width + posx) * 3;

						if (draw && (i == 0 || j == 0)) {
							data4Tree[index + 0] = color.x;
							data4Tree[index + 1] = color.y;
							data4Tree[index + 2] = color.z;
						} else {
							data4Tree[index + 0] = node->pixel.r;
							data4Tree[index + 1] = node->pixel.g;
							data4Tree[index + 2] = node->pixel.b;
						}
					}
				}
			});
			image4Tree->updateData(data4Tree, root->width, root->height);

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			blitToWindow(nullptr, GL_COLOR_ATTACHMENT0, &uiBaseImage);
			blitToWindow(nullptr, GL_COLOR_ATTACHMENT0, &ui4Tree);
			glfwSwapBuffers(manager.glfw->_window);

			if (GLFW_PRESS == glfwGetKey(manager.glfw->_window, GLFW_KEY_ESCAPE))
				glfwSetWindowShouldClose(manager.glfw->_window, GLFW_TRUE);
		}
	}

	std::cout << "End while loop" << endl;
	std::cout << "deleting textures..." << endl;
	delete lena;
	delete flower;
}

int		main(void) {
	scene_4Tree();
	return 0;
}