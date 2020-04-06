#include "simplegl.h"

#include "program.hpp"
#include "object.hpp"
#include "obj3dPG.hpp"
#include "obj3dBP.hpp"
#include "obj3d.hpp"
#include "misc.hpp"
#include "cam.hpp"
#include "texture.hpp"
#include "skyboxPG.hpp"
#include "skybox.hpp"
#include "glfw.hpp"
#include "transformBH.hpp"
#include "fps.hpp"
#include "gamemanager.hpp"
#include "framebuffer.hpp"
#include "uipanel.hpp"
#include "quadtree.hpp"
#include "octree.hpp"
#include "chunk.hpp"
#include "chunkgenerator.hpp"

#include "perlin.hpp"

#include <thread>
#include <cmath>
#include <string>
#include <cstdio>
#include <vector>
#include <list>
#include <algorithm>
#include <cassert>

class OctreeManager : public QuadTreeManager
{
public:
	OctreeManager() : QuadTreeManager() {
		std::srand(777);
		this->player = nullptr;
		this->chunk_size = 16;
		this->polygon_mode = GL_POINT;
		this->polygon_mode = GL_LINE;
		this->polygon_mode = GL_FILL;
		this->threshold = 0;
	}
	~OctreeManager() {}

	std::list<Obj3d*>	renderlist;
	GLuint				polygon_mode;
	Obj3d*				player;
	unsigned int		chunk_size;
	unsigned int		threshold;
};

static void		keyCallback_ocTree(GLFWwindow* window, int key, int scancode, int action, int mods) {
	(void)window; (void)key; (void)scancode; (void)action; (void)mods;
	//std::cout << __PRETTY_FUNCTION__ << std::endl;

	if (action == GLFW_PRESS) {
		//std::cout << "GLFW_PRESS" << std::endl;
		OctreeManager* manager = static_cast<OctreeManager*>(glfwGetWindowUserPointer(window));
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
				for (std::list<Obj3d*>::iterator it = manager->renderlist.begin(); it != manager->renderlist.end(); ++it) {
					(*it)->setPolygonMode(manager->polygon_mode);
				}
			}
		}
	}
}

void	scene_octree() {
#ifndef INIT_GLFW
	OctreeManager	m;
	m.glfw = new Glfw(WINX, WINY);
	glDisable(GL_CULL_FACE);
	m.glfw->setTitle("Tests octree");
	m.glfw->activateDefaultCallbacks(&m);
	m.glfw->func[GLFW_KEY_EQUAL] = keyCallback_ocTree;
	m.glfw->func[GLFW_KEY_MINUS] = keyCallback_ocTree;
	m.glfw->func[GLFW_KEY_ENTER] = keyCallback_ocTree;

	Obj3dBP::defaultSize = 1;
	Texture*	tex_skybox = new Texture("images/skybox4.bmp");
	Obj3dPG		obj3d_prog(OBJ3D_VS_FILE, OBJ3D_FS_FILE);
	SkyboxPG	sky_pg(CUBEMAP_VS_FILE, CUBEMAP_FS_FILE);
	Skybox		skybox(*tex_skybox, sky_pg);
	Obj3dBP		cubebp("obj3d/cube.obj", true, false);

	Cam		cam(*(m.glfw));
	cam.speed = 4;
	cam.local.setPos(0, 0, 0);
	cam.printProperties();
	cam.lockedMovement = false;
	cam.lockedOrientation = false;
	//m.glfw->setMouseAngle(-1);//?
	std::cout << "MouseAngle: " << m.glfw->getMouseAngle() << std::endl;
	m.cam = &cam;
#endif
	//QuadNode* root = new QuadNode(baseImage->getData(), w, 0, 0, w, h, THRESHOLD);
	//uint8_t* dataOctree = new uint8_t[w * h * 3];

#ifndef BASE_OBJ3D
	Obj3d		cubeo(cubebp, obj3d_prog);
	cubeo.local.setPos(0, 0, 0);
	cubeo.local.setScale(1, 1, 1);
	cubeo.setColor(255, 0, 0);
	cubeo.displayTexture = false;
	cubeo.setPolygonMode(GL_LINE);
	//m.renderlist.push_back(&cubeo);

	//Obj3d		player1(cubebp, obj3d_prog);
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
#ifndef OCTREE
	Pixel	black(0, 0, 0);
	Pixel	red(255, 0, 0);
	Pixel	green(0, 255, 0);
	Pixel	blue(0, 0, 255);
	Pixel	white(255, 255, 255);
	Pixel	purple(255, 0, 255);
	Pixel	cyan(0, 255, 255);
	Pixel	yellow(255, 255, 0);
	Pixel*	colors[8] = { &black, &red, &green, &blue, &white, &purple, &cyan, &yellow };

	unsigned int half = m.chunk_size / 2;
	Pixel*** raw_3Ddata = new Pixel**[m.chunk_size];
	for (unsigned int k = 0; k < m.chunk_size; k++) {
		raw_3Ddata[k] = new Pixel*[m.chunk_size];
		for (unsigned int j = 0; j < m.chunk_size; j++) {
			raw_3Ddata[k][j] = new Pixel[m.chunk_size];
			for (unsigned int i = 0; i < m.chunk_size; i++) {
				unsigned int kk = (k < half) ? 0 : 4;
				unsigned int jj = (j < half) ? 0 : 2;
				unsigned int ii = (i < half) ? 0 : 1;
				raw_3Ddata[k][j][i] = *colors[kk + jj + ii];
			}
		}
	}

	Math::Vector3	pos(0, 0, 0);
	Math::Vector3	size(m.chunk_size, m.chunk_size, m.chunk_size);
	Octree* root = new Octree(raw_3Ddata, pos, size, 0);

	//build all obj
	if (0) {
		for (unsigned int k = 0; k < m.chunk_size; k++) {
			for (unsigned int j = 0; j < m.chunk_size; j++) {
				for (unsigned int i = 0; i < m.chunk_size; i++) {
					Obj3d* cube = new Obj3d(cubebp, obj3d_prog);
					cube->setColor(raw_3Ddata[k][j][i].r, raw_3Ddata[k][j][i].g, raw_3Ddata[k][j][i].b);
					cube->local.setPos(i, j, k);
					cube->local.setScale(1, 1, 1);
					cube->setPolygonMode(GL_LINE);
					cube->displayTexture = false;
					m.renderlist.push_back(cube);
				}
			}
		}
	}
	else {
		root->browse(m.threshold, [&m, &cubebp, &obj3d_prog](Octree* node) {
			Obj3d* cube = new Obj3d(cubebp, obj3d_prog);
			cube->setColor(node->pixel.r, node->pixel.g, node->pixel.b);
			cube->local.setPos(node->pos);
			cube->local.setScale(node->size.x, node->size.y, node->size.z);
			cube->setPolygonMode(GL_LINE);
			cube->displayTexture = false;
			m.renderlist.push_back(cube);
		});
	}

#endif
	Fps	fps144(144);
	Fps	fps60(60);
	Fps* defaultFps = &fps60;

	std::cout << "Begin while loop" << endl;
	while (!glfwWindowShouldClose(m.glfw->_window)) {
		if (defaultFps->wait_for_next_frame()) {

			glfwPollEvents();
			m.glfw->updateMouse();//to do before cam's events
			m.cam->events(*m.glfw, float(defaultFps->getTick()));

			// printFps();
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			renderObj3d(m.renderlist, cam);
			//renderSkybox(skybox, cam);
			glfwSwapBuffers(m.glfw->_window);

			if (GLFW_PRESS == glfwGetKey(m.glfw->_window, GLFW_KEY_ESCAPE))
				glfwSetWindowShouldClose(m.glfw->_window, GLFW_TRUE);
		}
	}

	std::cout << "End while loop" << endl;
	std::cout << "deleting textures..." << endl;
}
