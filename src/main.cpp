/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rhoffsch <rhoffsch@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2018/09/18 22:45:30 by rhoffsch          #+#    #+#             */
/*   Updated: 2019/04/03 14:58:50 by rhoffsch         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

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

#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif

#define WINX 1600
#define WINY 900
#define WIN32_VS_FOLDER string("")

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

void	check_paddings() {
	//	cout << sizeof(BITMAPINFOHEADER) << " = " << sizeof(BMPINFOHEADER) << endl;
#ifdef _WIN322
	std::cout << sizeof(BITMAPFILEHEADER) << " = " << sizeof(BMPFILEHEADER) << endl;
	std::cout << "bfType\t" << offsetof(BMPINFOHEADERBITMAPFILEHEADER, bfType) << endl;
	std::cout << "bfSize\t" << offsetof(BITMAPFILEHEADER, bfSize) << endl;
	std::cout << "bfReserved1\t" << offsetof(BITMAPFILEHEADER, bfReserved1) << endl;
	std::cout << "bfReserved2\t" << offsetof(BITMAPFILEHEADER, bfReserved2) << endl;
	std::cout << "bfOffBits\t" << offsetof(BITMAPFILEHEADER, bfOffBits) << endl;
#endif//_WIN32
	std::cout << "unsigned short\t" << sizeof(unsigned short) << endl;
	std::cout << "unsigned long \t" << sizeof(unsigned long) << endl;
	std::cout << "long          \t" << sizeof(long) << endl;
	std::cout << "long long     \t" << sizeof(long long) << endl;
	std::cout << "int           \t" << sizeof(int) << endl;
	if (sizeof(BMPFILEHEADER) != 14 || sizeof(BMPINFOHEADER) != 40) {
		cerr << "Padding in structure, exiting..." << endl << endl;
		std::cout << "BMPFILEHEADER\t" << sizeof(BMPFILEHEADER) << endl;
		std::cout << "bfType     \t" << offsetof(BMPFILEHEADER, bfType) << endl;
		std::cout << "bfSize     \t" << offsetof(BMPFILEHEADER, bfSize) << endl;
		std::cout << "bfReserved1\t" << offsetof(BMPFILEHEADER, bfReserved1) << endl;
		std::cout << "bfReserved2\t" << offsetof(BMPFILEHEADER, bfReserved2) << endl;
		std::cout << "bfOffBits\t" << offsetof(BMPFILEHEADER, bfOffBits) << endl;
		std::cout << "-----" << endl;
		std::cout << "BMPINFOHEADER\t" << sizeof(BMPINFOHEADER) << endl;
		std::cout << "biSize     \t" << offsetof(BMPINFOHEADER, biSize) << endl;
		std::cout << "biWidth    \t" << offsetof(BMPINFOHEADER, biWidth) << endl;
		std::cout << "biHeight\t" << offsetof(BMPINFOHEADER, biHeight) << endl;
		std::cout << "biPlanes\t" << offsetof(BMPINFOHEADER, biPlanes) << endl;
		std::cout << "biBitCount\t" << offsetof(BMPINFOHEADER, biBitCount) << endl;
		std::cout << "biCompression\t" << offsetof(BMPINFOHEADER, biCompression) << endl;
		std::cout << "biSizeImage\t" << offsetof(BMPINFOHEADER, biSizeImage) << endl;
		std::cout << "biXPelsPerMeter\t" << offsetof(BMPINFOHEADER, biXPelsPerMeter) << endl;
		std::cout << "biYPelsPerMeter\t" << offsetof(BMPINFOHEADER, biYPelsPerMeter) << endl;
		std::cout << "biClrUsed\t" << offsetof(BMPINFOHEADER, biClrUsed) << endl;
		std::cout << "biClrImportant\t" << offsetof(BMPINFOHEADER, biClrImportant) << endl;
		exit(ERROR_PADDING);
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
			Object*	speAnchor = dynamic_cast<Object*>(this->_anchor);//specialisation part
			// turn this in Obj3d to get the BP, to get the size of the ovject,
			// to position the camera relatively to the obj's size.

			Cam*	speTarget = dynamic_cast<Cam*>(target);//specialisation part

			Math::Vector3	forward(0, -15, -35);
			if (this->copyRotation) {
				Math::Rotation	rot = speAnchor->local.getRot();
				forward.rotate(rot, 1);
				rot.mult(-1);
				speTarget->local.setRot(rot);
			}
			forward.mult(-1);// invert the forward to position the cam on the back, a bit up
			Math::Vector3	pos = speAnchor->local.getPos();
			pos.translate(forward);
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
	Object*			_anchor;
	Math::Vector3	_offset;

};

void blitToWindow(FrameBuffer* readFramebuffer, GLenum attachmentPoint, UIPanel* panel) {
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
	} else if (panel->getTexture()) {
		w = panel->getTexture()->getWidth();
		h = panel->getTexture()->getHeight();
	} else {
		std::cout << "FUCK " << __PRETTY_FUNCTION__ << std::endl;
		exit(2);
	}
	if (0) {
		std::cout << "copy " << w << "x" << h << "\tresized\t" << panel->_width << "x" << panel->_height \
			<< "\tat pos\t" << panel->_posX << ":" << panel->_posY << std::endl;
		// << " -> " << (panel->posX + panel->width) << "x" << (panel->posY + panel->height) << std::endl;
	}
	glBlitFramebuffer(0, 0, w, h, \
		panel->_posX, panel->_posY, panel->_posX2, panel->_posY2, GL_COLOR_BUFFER_BIT, GL_NEAREST);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
}

void	scene1() {
	Glfw	glfw(1600, 900);
	glfw.setTitle("This title is long, long enough to test how glfw manages oversized titles. At this point I dont really know what to write, so let's just bullshiting it ....................................................... is that enough? Well, it depends of the size of the current window. I dont really know how many characters i have to write for a width of 1920. Is it possible to higher the police ? It could save some characters. Ok, im bored, lets check if this title is long enough!");

	//Program for Obj3d (can render Obj3d) with vertex & fragmetns shaders
	Obj3dPG			obj3d_prog(OBJ3D_VS_FILE, OBJ3D_FS_FILE);

#ifndef BLUEPRINTS
	//Create Obj3dBP from .obj files
	Obj3dBP			the42BP("obj3d/42.obj", true);
	Obj3dBP			cubeBP("obj3d/cube.obj", true);
	Obj3dBP			teapotBP("obj3d/teapot2.obj", true);
	// Obj3dBP			rocketBP("obj3d/Rocket_Phoenix/AIM-54_Phoenix_OBJ/Aim-54_Phoenix.obj", true);
	Obj3dBP			rocketBP("obj3d/ARSENAL_VG33/Arsenal_VG33.obj", true);
	// Obj3dBP			lamboBP("obj3d/lambo/Lamborginhi_Aventador_OBJ/Lamborghini_Aventador.obj", true);
	Obj3dBP			lamboBP("obj3d/lambo/Lamborginhi_Aventador_OBJ/Lamborghini_Aventador_no_collider.obj", true);
	cout << "======" << endl;
#endif
#ifndef TEXTURES
		Texture*	texture1 = new Texture("images/lena.bmp");
		Texture*	texture2 = new Texture("images/skybox2.bmp");
		Texture*	texture3 = new Texture("images/skyboxfuck.bmp");
	//	Texture*	texture4 = new Texture("images/skybox4096.bmp");
		Texture*	texture5 = new Texture("images/skytest.bmp");
		// Texture*	texture6 = new Texture("obj3d/Rocket_Phoenix/AIM-54_Phoenix_OBJ/Phoenix.bmp");
		// Texture*	texture6 = new Texture("obj3d/ARSENAL_VG33/Arsenal_VG33.bmp");
	//	Texture*	texture7 = new Texture("obj3d/lambo/Lamborginhi_Aventador_OBJ/Lamborginhi_Aventador_diffuse.bmp");
		Texture		texture8 = *texture1;
#endif
#ifndef OBJ3D
	list<Obj3d*>	obj3dList;

	float s = 1.0f;//scale
	//Create Obj3d with the blueprint & by copy
	Obj3d			the42_1(cubeBP, obj3d_prog);//the42BP !
		the42_1.local.setPos(0, 0, 0);
		the42_1.setTexture(texture1);
		the42_1.displayTexture = true;
		the42_1.setPolygonMode(GL_FILL);
		the42_1.local.setScale(1, 1, 1);

	Obj3d			the42_2(the42_1);
		//the42_2.local.setPos(0, 3, -5);
		the42_2.local.setPos(-4, -2, -2);
		the42_2.setTexture(texture2);
		the42_2.displayTexture = false;
		// the42_2.setScale(3.0f, 0.75f, 0.3f);
		the42_2.setPolygonMode(GL_LINE);

	Obj3d			teapot1(teapotBP, obj3d_prog);
		teapot1.local.setPos(0, 0, 2);
		teapot1.local.getMatrix().setOrder(ROW_MAJOR);
		// teapot1.setRot(0, 90, 0);
		teapot1.setTexture(texture1);
		teapot1.displayTexture = false;
		teapot1.setPolygonMode(GL_LINE);
		// teapot1.setScale(1.5, 2, 0.75);

	Obj3d			cube1(cubeBP, obj3d_prog);
		cube1.local.setPos(0, -2, 3);
		cube1.setTexture(texture1);
		cube1.displayTexture = false;

	Object			empty1;
		empty1.local.setScale(1,1,1);

	Obj3d			rocket1(rocketBP, obj3d_prog);
		// rocket1.local.setPos(-10, -20, -2000);
		rocket1.local.setPos(0, -300, 0);
		rocket1.local.rotate(0, 180, 0);
		rocket1.setTexture(texture5);
		rocket1.displayTexture = true;
		// rocket1.setPolygonMode(GL_LINE);
		s = 10.0f;
		rocket1.local.setScale(s,s,s);
		rocket1.setParent(&empty1);


	// Properties::defaultSize = 13.0f;
	Obj3d			lambo1(lamboBP, obj3d_prog);
		lambo1.local.setPos(-20, 0, 0);
		// lambo1.local.setScale(1, 1, 1);
		lambo1.local.setPos(0, -5, 7);
		lambo1.setTexture(texture1);
		lambo1.displayTexture = true;
		lambo1.setPolygonMode(GL_LINE);
		s = 0.025f;
		// lambo1.setScale(s, s, s);

	Obj3d			lambo2(lamboBP, obj3d_prog);
		// lambo2.local.setPos(0, -1.9f, 0);
		lambo2.local.setPos(0, -6.0f, 0);
		// lambo2.local.setScale(1, 5, 1);
		lambo2.local.setRot(0, 180.0f, 0);
		lambo2.setTexture(texture1);
		lambo2.displayTexture = true;
		// lambo2.setPolygonMode(GL_LINE);
		s = 0.4f;
		// lambo2.local.setScale(s, s, s);
		// lambo2.setParent(&the42_1);
		lambo2.setParent(&rocket1);

	Obj3d			lambo3(lamboBP, obj3d_prog);
		lambo3.local.setPos(0, -4, 0);
		lambo3.local.setRot(0, 0.0f, 180);
		lambo3.setTexture(&texture8);
		lambo3.displayTexture = true;
		// lambo3.setPolygonMode(GL_LINE);
		s = 30.0f;
		// lambo3.local.setScale(s, s, s);
		// lambo3.setParent(&the42_1);
		lambo3.setParent(&lambo2);

		obj3dList.push_back(&the42_1);
		// obj3dList.push_back(&the42_2);
		obj3dList.push_back(&teapot1);
		// obj3dList.push_back(&cube1);
		obj3dList.push_back(&rocket1);
		obj3dList.push_back(&lambo1);
		obj3dList.push_back(&lambo2);
		obj3dList.push_back(&lambo3);

		if (true) {//spiral
			// Obj3d*	backObj = &lambo3;
			for (int i = 0; i < 20; i++) {
				Obj3d* lamboPlus = new Obj3d(lamboBP, obj3d_prog);
				lamboPlus->setParent(&lambo3);
				lamboPlus->displayTexture = (i % 2) ? true : false;
				lamboPlus->setTexture(&texture8);
				lamboPlus->setColor(i * 45 % 255, i * 10 % 255, i * 73 % 255);
				float maxScale = 3;
				float scale = (float)((i % (int)maxScale) - (maxScale / 2));
				lamboPlus->local.enlarge(scale, scale, scale);
				float	val = cosf(Math::toRadian(i * 10)) * 10;
				float	coef = 1.0f;
				lamboPlus->local.setPos(lambo3.local.getPos());
				lamboPlus->local.translate(float(i) / coef, val / coef, val / coef);
				lamboPlus->local.rotate(Math::Rotation(i * 5, i * 5, i * 5));

				obj3dList.push_back(lamboPlus);
				// backObj = lamboPlus;
			}
		}
#endif


	// Properties::defaultSize = PP_DEFAULT_SIZE;

	cout << &rocket1 << endl;
	// test_behaviors(rocket1);

	cout << "Object # : " << Object::getInstanceAmount() << endl;
	cout << "Obj3d # : " << Obj3d::getInstanceAmount() << endl;
	cout << endl;
	cout << "GL_MAX_CUBE_MAP_TEXTURE_SIZE " << GL_MAX_CUBE_MAP_TEXTURE_SIZE << endl;
	cout << "GL_MAX_TEXTURE_SIZE " << GL_MAX_TEXTURE_SIZE << endl;

	SkyboxPG	sky_pg(CUBEMAP_VS_FILE, CUBEMAP_FS_FILE);
	Skybox		skybox(*texture3, sky_pg);

#ifndef CAM
	Cam		cam(glfw);
	cam.local.setPos(0, 0, 10);
	cam.printProperties();
	cam.lockedMovement = false;
	cam.lockedOrientation = false;

	glfw.setMouseAngle(45);
	std::cout << "MouseAngle: " <<  glfw.getMouseAngle() << std::endl;
	//exit(0);

	if (false) {//cam anchor to rocket1, bugged with Z rot
		cam.local.setPos(0, 1.5f, 3.5f);
		cam.setParent(&rocket1);
		cam.lockedMovement = true;
		cam.lockedOrientation = true;
	}
#endif

	Fps	fps144(144);
	Fps	fps60(60);
	Fps	fps30(30);
	Fps* defaultFps = &fps144;

	//followObjectArgs	st = { defaultFps, &cam };


#ifndef BEHAVIORS
	cout << "behavior:" << endl;
	TransformBH		b1;
		b1.transform.rot.setUnit(ROT_DEG);
		b1.transform.rot.z = 10 * defaultFps->getTick();
		b1.modeRot = ADDITIVE;
		float ss = 1.0f + 0.1f * defaultFps->getTick();
		//b1.transform.scale = Math::Vector3(ss, ss, ss);
	//	b1.modeScale = MULTIPLICATIVE;
		cout << "___ adding rocket1: " << &rocket1 << endl;
		b1.addTarget(&rocket1);
		b1.addTarget(&lambo1);
		b1.removeTarget(&lambo1);
		b1.setTargetStatus(&rocket1, false);
		std::cout << "rocket1 status: " << b1.getTargetStatus(&rocket1) << std::endl;
		b1.setTargetStatus(&rocket1, true);
		std::cout << "rocket1 status: " << b1.getTargetStatus(&rocket1) << std::endl;

	TransformBH		b2;// = b1;//bug
		b2.transform.scale = Math::Vector3(0,0,0);
		b2.modeScale = ADDITIVE;
		// b2.transform.rot.z = 0.0f;
		b2.transform.rot.x = -45.0f * defaultFps->getTick();
		// b2.removeTarget(&rocket1);
		b2.addTarget(&empty1);
		//bug if i do:
		b2.addTarget(&empty1);
		b2.removeTarget(&rocket1);
	
	TransformBH		b3;// = b1;//bug
		b3.transform.scale = Math::Vector3(0,0,0);
		b3.modeScale = ADDITIVE;
		b3.transform.rot.y = 102.0f * defaultFps->getTick();
		b3.addTarget(&lambo2);

		cout << "b1: " << b1.getTargetList().size() << endl;
		cout << "b2: " << b2.getTargetList().size() << endl;

	TransformBH		b4;// = b1;//bug
		b4.transform.scale = Math::Vector3(0,0,0);
		b4.modeScale = ADDITIVE;
		b4.transform.rot.y = 720.0f * defaultFps->getTick();
		b4.addTarget(&teapot1);
	// exit(0);

	if (false) {// check behavior target, add remove

		cout << "behaviorsActive: " << (empty1.behaviorsActive ? "true" : "false") << endl;
		cout << "------------" << endl;
		cout << "lambo1:\t" << lambo1.behaviorList.size() << endl;
		cout << "b2:    \t" << b2.getTargetList().size() << endl;
		cout << "b2.addTarget(&lambo1);" << endl;
		b2.addTarget(&lambo1);
		cout << "lambo1:\t" << lambo1.behaviorList.size() << endl;
		cout << "b2:    \t" << b2.getTargetList().size() << endl;
		cout << "b2.removeTarget(&lambo1);" << endl;
		b2.removeTarget(&lambo1);
		cout << "lambo1:\t" << lambo1.behaviorList.size() << endl;
		cout << "b2:    \t" << b2.getTargetList().size() << endl;

		cout << "------------" << endl;
		cout << "lambo1:\t" << lambo1.behaviorList.size() << endl;
		cout << "b2:    \t" << b2.getTargetList().size() << endl;
		cout << "lambo1.addBehavior(&b2);" << endl;
		lambo1.addBehavior(&b2);
		cout << "lambo1:\t" << lambo1.behaviorList.size() << endl;
		cout << "b2:    \t" << b2.getTargetList().size() << endl;
		cout << "lambo1.removeBehavior(&b2);" << endl;
		lambo1.removeBehavior(&b2);
		cout << "lambo1:\t" << lambo1.behaviorList.size() << endl;
		cout << "b2:    \t" << b2.getTargetList().size() << endl;
	}
#endif
// exit(0);

#ifndef GAMEMANAGER
	GameManager	manager;
	manager.glfw = &glfw;
	manager.cam = &cam;

	glfw.activateDefaultCallbacks(&manager);

#endif

#ifndef RENDER
	cout << "Begin while loop" << endl;
	// cam.local.setScale(s,s,s);//bad, undefined behavior
	while (!glfwWindowShouldClose(glfw._window)) {
		if (defaultFps->wait_for_next_frame()) {
			//////////////////////////////////////////
			if (true) {
				if (false) {
					Math::Matrix4	matRocket = rocket1.getWorldMatrix();
					matRocket.printData();
					cout << "---------------" << endl;
				}
				if (false) {
					the42_1.getWorldMatrix().printData();
				}
				if (false) {
					cout << "---rocket1" << endl;
					rocket1.local.getScale().printData();
					// rocket1.getWorldMatrix().printData();
					cout << "---lambo2" << endl;
					lambo2.local.getScale().printData();
					// lambo2.getWorldMatrix().printData();
					cout << "---lambo3" << endl;
					lambo3.local.getScale().printData();
					// lambo3.getWorldMatrix().printData();
					cout << "---------------" << endl;
					lamboBP.getDimensions().printData();
					cout << "---------------" << endl;
				}
			}
			////////////////////////////////////////// motion/behaviors
			//this should be used in another func, life a special func managing all events/behavior at every frames
			if (true) {
				b1.run();
				b2.run();
				b3.run();
				b4.run();
				if (false) {
					Math::Vector3 p = teapot1.local.getPos();
					float r = 180 * defaultFps->getTick();
					p.rotateAround(cam.local.getPos(), Math::Rotation(r, 0, 0));
					teapot1.local.setPos(p);
				} else if (true) {
					float r = 180 * defaultFps->getTick();
					teapot1.local.rotateAround(cam.local.getPos(), Math::Rotation(r, 0, 0));
				}
				// Math::Matrix4	matEmpty = empty1.getWorldMatrix();
				// matEmpty.printData();
			}
			////////////////////////////////////////// motion end

			glfwPollEvents();
			glfw.updateMouse();//to do before cam's events
			cam.events(glfw, float(defaultFps->getTick()));
			// printFps();
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			renderObj3d(obj3dList, cam);
			renderSkybox(skybox, cam);
			glfwSwapBuffers(glfw._window);

			if (GLFW_PRESS == glfwGetKey(glfw._window, GLFW_KEY_ESCAPE))
				glfwSetWindowShouldClose(glfw._window, GLFW_TRUE);
		}
	}
	cout << "End while loop" << endl;
#endif

	cout << "deleting textures..." << endl;
	delete texture1;
	delete texture2;
	delete texture3;
//	delete texture4;
	delete texture5;
//	delete texture6;
	//delete texture7;
}

void scene2() {
	Glfw	glfw(1600, 900);
	glfw.setTitle("Tests camera anchor");

	//Program for Obj3d (can render Obj3d) with vertex & fragmetns shaders
	Obj3dPG			obj3d_prog(OBJ3D_VS_FILE, OBJ3D_FS_FILE);
	SkyboxPG		sky_pg(CUBEMAP_VS_FILE, CUBEMAP_FS_FILE);

	Obj3dBP			rocketBP("obj3d/ARSENAL_VG33/Arsenal_VG33.obj", true);

	Texture* texture3 = new Texture("images/skyboxfuck.bmp");
	Texture* texture5 = new Texture("images/skytest.bmp");

	Skybox		skybox(*texture3, sky_pg);


	list<Obj3d*>	obj3dList;

	Obj3d			rocket(rocketBP, obj3d_prog);
	rocket.local.setPos(0, 0, 0);
	rocket.setTexture(texture5);
	rocket.displayTexture = true;
	float s = 10.0f;
	rocket.local.setScale(s, s, s);

	Obj3d			rocket2(rocketBP, obj3d_prog);
	rocket2.local.setPos(5, 0, 0);
	rocket2.setTexture(texture5);
	rocket2.displayTexture = true;
	s = 5000.0f;
	rocket2.local.setScale(s, s, s);

	obj3dList.push_back(&rocket);
	obj3dList.push_back(&rocket2);


	Cam		cam(glfw);
	cam.local.setPos(0, 15, 35);
	cam.printProperties();
	cam.lockedMovement = false;
	cam.lockedOrientation = false;

	glfw.setMouseAngle(-1);
	std::cout << "MouseAngle: " << glfw.getMouseAngle() << std::endl;
	//exit(0);

	//cam.local.setPos(0, 1.5f, 3.5f);
	AnchorCameraBH	b1;
	b1.setAnchor(&rocket);
	b1.addTarget(&cam);
	b1.copyRotation = true;
	cam.lockedMovement = true;
	cam.lockedOrientation = true;


	Fps	fps144(144);
	Fps	fps60(60);
	Fps* defaultFps = &fps144;

	float	mvt = 30.0f * defaultFps->getTick();

	std::cout << "Begin while loop" << endl;
	int c = 0;
	while (!glfwWindowShouldClose(glfw._window)) {
		if (defaultFps->wait_for_next_frame()) {

			if (1) {
				b1.run();
			}

			//print data
			if (false) {
				c++;
				if (c == 60) {
					c = 0;
					system("cls");
					if (true) {
						std::cout << "--- ROCKET" << endl;
						//rocket.local.getScale().printData();
						rocket.local.getRot().printData();
						rocket.getWorldMatrix().printData();
					}
					if (true) {
						std::cout << "--- CAM" << endl;
						cam.local.getRot().printData();
						std::cout << "- local" << endl;
						cam.getLocalProperties().getMatrix().printData();
						std::cout << "- world" << endl;
						cam.getWorldMatrix().printData();
					}
				}
			}

			glfwPollEvents();
			glfw.updateMouse();//to do before cam's events
			cam.events(glfw, float(defaultFps->getTick()));
			if (true) {
				if (GLFW_PRESS == glfwGetKey(glfw._window, GLFW_KEY_LEFT))
					rocket.local.rotate(0,		mvt,	0);
				if (GLFW_PRESS == glfwGetKey(glfw._window, GLFW_KEY_RIGHT))
					rocket.local.rotate(0,		-mvt,	0);
				if (GLFW_PRESS == glfwGetKey(glfw._window, GLFW_KEY_DOWN))
					rocket.local.rotate(mvt,	0,		0);
				if (GLFW_PRESS == glfwGetKey(glfw._window, GLFW_KEY_UP))
					rocket.local.rotate(-mvt,	0,		0);
				if (GLFW_PRESS == glfwGetKey(glfw._window, GLFW_KEY_KP_1))
					rocket.local.rotate(0,		0,		mvt);
				if (GLFW_PRESS == glfwGetKey(glfw._window, GLFW_KEY_KP_2))
					rocket.local.rotate(0,		0,		-mvt);
				if (GLFW_PRESS == glfwGetKey(glfw._window, GLFW_KEY_KP_0))
					rocket.local.setRot(0,		0,		0);

				Math::Vector3	pos = rocket.local.getPos();
				if (GLFW_PRESS == glfwGetKey(glfw._window, GLFW_KEY_KP_8))
					pos.add(0, 0, -mvt);
				if (GLFW_PRESS == glfwGetKey(glfw._window, GLFW_KEY_KP_5))
					pos.add(0, 0, mvt);
				if (GLFW_PRESS == glfwGetKey(glfw._window, GLFW_KEY_KP_4))
					pos.add(-mvt, 0, 0);
				if (GLFW_PRESS == glfwGetKey(glfw._window, GLFW_KEY_KP_6))
					pos.add(mvt, 0, 0);
				if (GLFW_PRESS == glfwGetKey(glfw._window, GLFW_KEY_KP_7))
					pos.add(0, -mvt, 0);
				if (GLFW_PRESS == glfwGetKey(glfw._window, GLFW_KEY_KP_9))
					pos.add(0, mvt, 0);
				rocket.local.setPos(pos);


			//#define 	GLFW_KEY_RIGHT   262
			//#define 	GLFW_KEY_LEFT   263
			//#define 	GLFW_KEY_DOWN   264
			//#define 	GLFW_KEY_UP   265
			}

			// printFps();
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			renderObj3d(obj3dList, cam);
			renderSkybox(skybox, cam);
			glfwSwapBuffers(glfw._window);

			if (GLFW_PRESS == glfwGetKey(glfw._window, GLFW_KEY_ESCAPE))
				glfwSetWindowShouldClose(glfw._window, GLFW_TRUE);
		}
	}
	std::cout << "End while loop" << endl;

	std::cout << "deleting textures..." << endl;
	delete texture3;
	delete texture5;
}

#define BORDERS_ON	true
#define BORDERS_OFF	false
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
		for (int j = 0; j < node->height; j++) {
			for (int i = 0; i < node->width; i++) {
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
	std::cout << "pixel: " << sizeof(Pixel) << std::endl;

	QuadNode* root = new QuadNode(pix, 0, 0, w, h, THRESHOLD);
	std::cout << "root is leaf: " << (root->isLeaf() ? "true" : "false") << std::endl;

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
	//std::cout << __PRETTY_FUNCTION__ << std::endl;

	if (action == GLFW_PRESS) {
		//std::cout << "GLFW_PRESS" << std::endl;
		QuadTreeManager* manager = static_cast<QuadTreeManager*>(glfwGetWindowUserPointer(window));
		if (!manager) {
			std::cout << "static_cast failed" << std::endl;
		} else if (manager->glfw) {
			if (key == GLFW_KEY_EQUAL) {
				manager->threshold++;
				manager->glfw->setTitle(std::to_string(manager->threshold).c_str());
			}
			else if (key == GLFW_KEY_MINUS && manager->threshold > 0) {
				manager->threshold--;
				manager->glfw->setTitle(std::to_string(manager->threshold).c_str());
			}
			else if (key == GLFW_KEY_ENTER)
				manager->draw_borders = !manager->draw_borders;
		}
	}
}

void	scene_4Tree() {
	QuadTreeManager	manager;
	manager.glfw = new Glfw(WINX, WINY);
	manager.glfw->setTitle("Tests texture quadtree");
	manager.glfw->activateDefaultCallbacks(&manager);
	manager.glfw->func[GLFW_KEY_EQUAL] = keyCallback_quadTree;
	manager.glfw->func[GLFW_KEY_MINUS] = keyCallback_quadTree;
	manager.glfw->func[GLFW_KEY_ENTER] = keyCallback_quadTree;

	Texture* lena = new Texture(WIN32_VS_FOLDER + "images/lena.bmp");
	Texture* rgbtest = new Texture(WIN32_VS_FOLDER + "images/test_rgb.bmp");
	Texture* minirgb = new Texture(WIN32_VS_FOLDER + "images/minirgb.bmp");
	Texture* red = new Texture(WIN32_VS_FOLDER + "images/red.bmp");
	Texture* monkey = new Texture(WIN32_VS_FOLDER + "images/monkey.bmp");
	Texture* flower = new Texture(WIN32_VS_FOLDER + "images/flower.bmp");

	Texture* baseImage = flower;
	int w = baseImage->getWidth();
	int h = baseImage->getHeight();
	QuadNode* root = new QuadNode(baseImage->getData(), w, 0, 0, w, h, THRESHOLD);
	uint8_t* dataOctree = new uint8_t[w * h * 3];

	float size_coef = float(WINX) / float(baseImage->getWidth()) / 2.0f;
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

	std::cout << "Begin while loop" << endl;
	int	leafAmount = 0;
	while (!glfwWindowShouldClose(manager.glfw->_window)) {
		if (defaultFps->wait_for_next_frame()) {

			glfwPollEvents();
			//glfw.updateMouse();//to do before cam's events
			//cam.events(glfw, float(defaultFps->tick));

			fillData(dataOctree, root, &leafAmount, w, manager.draw_borders, manager.threshold, Math::Vector3(0,0,0));
			leafAmount = -1;
			//std::cout << "leafs: " << leafAmount << std::endl;
			//std::cout << "w * h * 3 = " << w << " * " << h << " * 3 = " << w * h * 3 << std::endl;
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

	std::cout << "End while loop" << endl;
	std::cout << "deleting textures..." << endl;
	delete lena;
	delete red;
	delete rgbtest;
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
	//tmp
	double			mouseX;
	double			mouseY;
	int				areaWidth;
	int				areaHeight;
	double			island;
	//vox
	Obj3d*			player;
	int	playerChunkX;
	int	playerChunkY;
	std::list<Obj3d*>	renderlist;
	Cam*			cam;
	int				range_chunk_display;
	int				range_chunk_memory;
	int				chunk_size;
	int				voxel_size;
	GLuint			polygon_mode;
	int				threshold;
};

void TestPerlin()
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
	//std::cout << playerPosX << " : " << playerPosY << std::endl;
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

			int index = (y * manager.areaWidth + x) * 3;
			uint8_t color = (uint8_t)(value * 255.0);
			if (color < 50) { // water
				data[index + 0] = 0;
				data[index + 1] = uint8_t(150.0 * std::clamp((double(color) / 50.0), 0.25, 1.0));
				data[index + 2] = uint8_t(255.0 * std::clamp((double(color) / 50.0), 0.25, 1.0));
			}
			else if (color < 75) { // sand
				data[index + 0] = 255.0 * ((double(color)) / 75.0);
				data[index + 1] = 200.0 * ((double(color)) / 75.0);
				data[index + 2] = 100.0 * ((double(color)) / 75.0);
			}
			else if (color > 200) { // snow
				data[index + 0] = color;
				data[index + 1] = color;
				data[index + 2] = color;
			}
			else if (color > 175) { // rocks
				data[index + 0] = 150.0 * value;
				data[index + 1] = 150.0 * value;
				data[index + 2] = 150.0 * value;
			}
			else {//grass
				data[index + 0] = 0;
				data[index + 1] = 200.0 * value;
				data[index + 2] = 100.0 * value;

			}
		}
	}
}

void	scene_procedural() {
	TestPerlin();
	ProceduralManager	manager;
	manager.glfw = new Glfw(WINX, WINY);
	manager.glfw->toggleCursor();
	manager.glfw->setTitle("Tests procedural");
	manager.glfw->activateDefaultCallbacks(&manager);

	const siv::PerlinNoise perlin(manager.seed);

	int screenSize = 250;
	manager.areaWidth = screenSize;
	manager.areaHeight = screenSize;
	manager.frequency = double(screenSize) / 75;
	uint8_t*	data = new uint8_t[manager.areaWidth * manager.areaHeight * 3];
	Texture* image = new Texture(data, manager.areaWidth, manager.areaHeight);

	int repeatX = WINX / manager.areaWidth;
	int repeatY = WINY / manager.areaHeight;
	std::cout << "repeatX: " << repeatX << std::endl;
	std::cout << "repeatZ: " << repeatY << std::endl;
	float size_coef = float(WINX) / float(image->getWidth()) / float(repeatX < repeatY ? repeatX : repeatY);
	size_coef = 1;
	UIImage	uiImage(image);
	uiImage.setPos(0, 0);
	uiImage.setSize(uiImage.getTexture()->getWidth() * size_coef, uiImage.getTexture()->getHeight() * size_coef);


	Fps	fps144(144);
	Fps	fps60(60);
	Fps* defaultFps = &fps144;

	int thread_amount = manager.core_amount - 1;
	std::thread* threads_list = new std::thread[thread_amount];

	std::cout << "Begin while loop" << endl;
	while (!glfwWindowShouldClose(manager.glfw->_window)) {
		if (defaultFps->wait_for_next_frame()) {

			glfwPollEvents();
			//glfw.updateMouse();//to do before cam's events
			//cam.events(glfw, float(defaultFps->tick));

			// printFps();

			glfwGetCursorPos(manager.glfw->_window, &manager.mouseX, &manager.mouseY);
			int playerPosX = manager.mouseX - (WINX / 2);//center of screen is 0:0
			int playerPosY = WINY - manager.mouseY - (WINY / 2);//center of screen is 0:0   //invert glfw Y to match opengl image
			Math::Vector3	vec(playerPosX, playerPosY, 0);
			double dist = (double(vec.magnitude()) / double(WINY*2));
			std::cout << playerPosX << ":" << playerPosY << "  \t" << dist << std::endl;

			for (size_t i = 0; i < thread_amount; i++) {//compute data with threads
				int start = ((manager.areaHeight * (i + 0)) / thread_amount);
				int end = ((manager.areaHeight * (i + 1)) / thread_amount);
				//std::cout << start << "\t->\t" << end << "\t" << end - start << std::endl;
				threads_list[i] = std::thread(th_buildData, std::ref(data), std::ref(manager), start, end);
			}
			for (size_t i = 0; i < thread_amount; i++) {
				threads_list[i].join();
			}

			image->updateData(data, manager.areaWidth, manager.areaHeight);
			uiImage.setPos(manager.mouseX - (manager.areaWidth / 2), WINY - manager.mouseY - (manager.areaHeight / 2));
			uiImage.setSize(uiImage.getTexture()->getWidth() * size_coef, uiImage.getTexture()->getHeight() * size_coef);

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
	delete[] threads_list;

	std::cout << "End while loop" << endl;
	std::cout << "deleting textures..." << endl;
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
	int threshold = 3;

	int startMid = (manager.range_chunk_memory / 2) - (20 / 2);
	int endMid = startMid + 20;
	int start = (manager.range_chunk_memory / 2) - (manager.range_chunk_display / 2);
	int end = start + manager.range_chunk_display;
	std::cout << start << " -> " << end << "\t/ " << manager.range_chunk_memory << std::endl;
	for (size_t j = 0; j < manager.range_chunk_memory; j++) {
		for (size_t i = 0; i < manager.range_chunk_memory; i++) {
			//std::cout << j << ":" << i << std::endl;
			if (i >= start && i < end && j >= start && j < end)//around player
				threshold = manager.threshold;
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
					chunkMemory4Tree[j][i] = new QuadNode(chunkMemory[j][i], w, 0, 0, w, h, THRESHOLD);
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
					chunkMemory4Tree[j][i] = new QuadNode(chunkMemory[j][i], w, 0, 0, w, h, THRESHOLD);
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
					chunkMemory4Tree[j][i] = new QuadNode(chunkMemory[j][i], w, 0, 0, w, h, THRESHOLD);
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
					chunkMemory4Tree[j][i] = new QuadNode(chunkMemory[j][i], w, 0, 0, w, h, THRESHOLD);
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

void	scene_vox() {
#ifndef INIT_GLFW
	Obj3dBP::defaultSize = 1;
	TestPerlin();
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

	Obj3dPG		obj3d_prog(OBJ3D_VS_FILE, OBJ3D_FS_FILE);
	SkyboxPG	sky_pg(CUBEMAP_VS_FILE, CUBEMAP_FS_FILE);

	Obj3dBP		cubebp("obj3d/cube.obj", true, false);

	Texture*	tex_skybox = new Texture("images/skybox4.bmp");
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
	for (size_t j = 0; j < manager.range_chunk_memory; j++) {
		chunkMemory[j] = new uint8_t*[manager.range_chunk_memory];
		chunkMemory4Tree[j] = new QuadNode*[manager.range_chunk_memory];
		for (size_t i = 0; i < manager.range_chunk_memory; i++) {
			//std::cout << i << ":" << j << std::endl;
			chunkMemory[j][i] = nullptr;
			chunkMemory4Tree[j][i] = nullptr;
			int x, y, w, h;
			x = startX + manager.chunk_size * i;
			y = startY + manager.chunk_size * j;
			w = manager.chunk_size;
			h = manager.chunk_size;
			chunkMemory[j][i] = generatePerlinNoise(manager, x, y, w, h);//make a class Rect ?
			chunkMemory4Tree[j][i] = new QuadNode(chunkMemory[j][i], w, 0, 0, w, h, THRESHOLD);
		}
	}
	int startDisplay = (manager.range_chunk_memory - manager.range_chunk_display) / 2;
	int endDisplay = startDisplay + manager.range_chunk_display;
	int memoryTotalRange = manager.chunk_size * manager.range_chunk_memory;
	uint8_t* dataChunkMemory = new uint8_t[3 * memoryTotalRange * memoryTotalRange];
	for (size_t j = 0; j < manager.range_chunk_memory; j++) {
		for (size_t i = 0; i < manager.range_chunk_memory; i++) {
				int leafamount = 0;
				Math::Vector3 color(0, 0, 0);
				if ((i >= startDisplay && i < endDisplay) && (j >= startDisplay && j < endDisplay))
					color = Math::Vector3(255, 0, 0);
				fillData(dataChunkMemory + 3 * (j * memoryTotalRange*manager.chunk_size + i*manager.chunk_size),
					chunkMemory4Tree[j][i], &leafamount, memoryTotalRange, false, THRESHOLD, color);
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
			//renderSkybox(skybox, cam);
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
				for (size_t j = 0; j < manager.range_chunk_memory; j++) {
					for (size_t i = 0; i < manager.range_chunk_memory; i++) {
						int leafamount = 0;
						Math::Vector3 color(0, 0, 0);
						if ((i >= startDisplay && i < endDisplay) && (j >= startDisplay && j < endDisplay))
							color = Math::Vector3(255, 0, 0);
						fillData(dataChunkMemory + 3 * (j * memoryTotalRange * manager.chunk_size + i * manager.chunk_size),
							chunkMemory4Tree[j][i], &leafamount, memoryTotalRange, false, THRESHOLD, color);
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
				for (size_t j = 0; j < manager.range_chunk_memory; j++) {
					for (size_t i = 0; i < manager.range_chunk_memory; i++) {
						int leafamount = 0;
						Math::Vector3 color(0, 0, 0);
						if ((i >= startDisplay && i < endDisplay) && (j >= startDisplay && j < endDisplay))
							color = Math::Vector3(255, 0, 0);
						fillData(dataChunkMemory + 3 * (j * memoryTotalRange * manager.chunk_size + i * manager.chunk_size),
							chunkMemory4Tree[j][i], &leafamount, memoryTotalRange, false, THRESHOLD, color);
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

class OctreeManager : public QuadTreeManager
{
public:
	OctreeManager() : QuadTreeManager() {
		this->seed = 777;
		std::srand(this->seed);
		this->frequency = 8;
		this->frequency = std::clamp(this->frequency, 0.1, 64.0);
		this->octaves = 12;
		this->octaves = std::clamp(this->octaves, 1, 16);

		this->player = nullptr;
		this->chunk_size = 32;
		this->chunk_display = 5;
		this->polygon_mode = GL_POINT;
		this->polygon_mode = GL_LINE;
		this->polygon_mode = GL_FILL;
		this->threshold = 0;
	}
	~OctreeManager() {}

	unsigned int		seed;
	siv::PerlinNoise*	perlin;
	double				frequency;
	int					octaves;
	std::list<Obj3d*>	renderlist;
	GLuint				polygon_mode;
	Obj3d*				player;
	unsigned int		chunk_size;
	int					chunk_display;
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

float	getDirectionBetween(Properties& local_referenciel, Math::Vector3 target_pos) {
	Math::Vector3	ref_pos(local_referenciel.getPos());
	Math::Rotation	ref_rot(local_referenciel.getRot());
	Math::Vector3	diff(target_pos);
	diff.sub(ref_pos);

	Math::Vector3	right = VEC3_RIGHT;
	Math::Vector3	up = VEC3_UP;
	right.ZYXrotate(ref_rot, -ROT_WAY);
	up.ZYXrotate(ref_rot, -ROT_WAY);
	Math::Vector3	forward = Math::Vector3::cross(up, right);

	return Math::Vector3::dot(forward, diff);
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

	siv::PerlinNoise perlin(m.seed);
	m.perlin = &perlin;

	Obj3dBP::defaultSize = 1;
	Texture*	tex_skybox = new Texture("images/skybox4.bmp");
	Obj3dPG		obj3d_prog(OBJ3D_VS_FILE, OBJ3D_FS_FILE);
	SkyboxPG	sky_pg(CUBEMAP_VS_FILE, CUBEMAP_FS_FILE);
	Skybox		skybox(*tex_skybox, sky_pg);
	Obj3dBP		cubebp("obj3d/cube.obj", true, false);

	Cam		cam(*(m.glfw));
	cam.speed = 40;
	cam.local.setPos(0, 100, 0);
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

#define M_PERLIN_GENERATION		1
#define M_OCTREE_OPTIMISATION	1
#define M_DISPLAY_BLACK			1

	cam.local.setPos(50, 150, 50);
	//chunk generator
	PerlinSettings	ps(perlin);
	Math::Vector3	playerPos = cam.local.getPos();
	ChunkGenerator	generator(playerPos, ps, m.chunk_size);

	//build all obj
	float	scale_coef = 0.99;
	float	scale_coe2 = 1.0;
	//scale_coef = 1;


	list<Obj3d*>	octreeDisplay;
	for (unsigned int k = 0; k < generator.size.z; k++) {
		for (unsigned int j = 0; j < generator.size.y; j++) {
			for (unsigned int i = 0; i < generator.size.x; i++) {
				Chunk* chunkPtr = generator.grid[k][j][i];
				chunkPtr->root->browse(2, [&m, &cubebp, &obj3d_prog, scale_coef, scale_coe2, &octreeDisplay, chunkPtr](Octree* node) {
					if (M_DISPLAY_BLACK || (node->pixel.r != 0 && node->pixel.g != 0 && node->pixel.b != 0)) {
						Math::Vector3	worldPos(chunkPtr->pos);
						worldPos.add(node->pos);

						if (node->pixel.r == 75) {
							Obj3d* cube = new Obj3d(cubebp, obj3d_prog);
							cube->setColor(node->pixel.r, node->pixel.g, node->pixel.b);
							cube->local.setPos(worldPos);
							cube->local.setScale(node->size.x * scale_coef, node->size.y * scale_coef, node->size.z * scale_coef);
							cube->setPolygonMode(m.polygon_mode);
							cube->displayTexture = false;
							m.renderlist.push_back(cube);
							if (M_DISPLAY_BLACK) {
								Obj3d* cube2 = new Obj3d(cubebp, obj3d_prog);
								cube2->setColor(0, 0, 0);
								cube2->local.setPos(worldPos);
								cube2->local.setScale(node->size.x * scale_coe2, node->size.y * scale_coe2, node->size.z * scale_coe2);
								cube2->setPolygonMode(GL_LINE);
								cube2->displayTexture = false;
								octreeDisplay.push_back(cube2);
							}
						}
					}
				});
			}
		}
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

			if (generator.updateGrid(cam.local.getPos())) {
				for (auto i : m.renderlist)
					delete i;
				m.renderlist.clear();
				for (auto i : octreeDisplay)
					delete i;
				octreeDisplay.clear();

				for (unsigned int k = 0; k < generator.size.z; k++) {
					for (unsigned int j = 0; j < generator.size.y; j++) {
						for (unsigned int i = 0; i < generator.size.x; i++) {
							Chunk* chunkPtr = generator.grid[k][j][i];
							if (!chunkPtr->root) {
								std::cout << "fuck there is no root\n";
								exit(1);
							}
							chunkPtr->root->browse(2, [&m, &cubebp, &obj3d_prog, scale_coef, scale_coe2, &octreeDisplay, chunkPtr, &cam](Octree* node) {
								if (M_DISPLAY_BLACK || (node->pixel.r != 0 && node->pixel.g != 0 && node->pixel.b != 0)) {
									Math::Vector3	worldPos(chunkPtr->pos);
									worldPos.add(node->pos);
									Math::Vector3	center(worldPos);
									center.add(node->size.x / 2, node->size.y / 2, node->size.z / 2);

									if (node->pixel.r < 150 && getDirectionBetween(cam.local, center) > 0) {
										Obj3d* cube = new Obj3d(cubebp, obj3d_prog);
										cube->setColor(node->pixel.r, node->pixel.g, node->pixel.b);
										cube->local.setPos(worldPos);
										cube->local.setScale(node->size.x * scale_coef, node->size.y * scale_coef, node->size.z * scale_coef);
										cube->setPolygonMode(m.polygon_mode);
										cube->displayTexture = false;
										m.renderlist.push_back(cube);
										if (M_DISPLAY_BLACK) {
											Obj3d* cube2 = new Obj3d(cubebp, obj3d_prog);
											cube2->setColor(0, 0, 0);
											cube2->local.setPos(worldPos);
											cube2->local.setScale(node->size.x * scale_coe2, node->size.y * scale_coe2, node->size.z * scale_coe2);
											cube2->setPolygonMode(GL_LINE);
											cube2->displayTexture = false;
											octreeDisplay.push_back(cube2);
										}
									}
								}
							});
						}
					}
				}
				std::cout << "Rendered Objects: " << m.renderlist.size() << std::endl;
			}

			// printFps();
			GLuint	mode = m.polygon_mode;
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			renderObj3d(m.renderlist, cam);
			renderObj3d(octreeDisplay, cam);

			//renderSkybox(skybox, cam);
			glfwSwapBuffers(m.glfw->_window);


			if (GLFW_PRESS == glfwGetKey(m.glfw->_window, GLFW_KEY_ESCAPE))
				glfwSetWindowShouldClose(m.glfw->_window, GLFW_TRUE);
		}
	}

	std::cout << "End while loop" << endl;
	std::cout << "deleting textures..." << endl;
}

int		main(void) {
	check_paddings();
	// test_behaviors();
	//test_mult_mat4(); exit(0);
	std::cout << "____START____ : " << Misc::getCurrentDirectory() << std::endl;
	//	test_obj_loader();

	//scene1();
	//scene2();
	//scene_4Tree();
	//scene_procedural();
	//scene_vox();
	scene_octree();
	// while(1);

	return (EXIT_SUCCESS);
}
