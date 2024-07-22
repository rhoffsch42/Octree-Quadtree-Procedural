
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
#define TREES_MAIN_DEBUG
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
	std::vector<ChunkShPtr>	renderVecChunkShPtr;
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
	lambobp0.lodManager.addLod(&lambobp2, 30);
	lambobp0.lodManager.addLod(&lambobp3, 80);
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

#define TRIM_FLOOR				1

void	printSettings(OctreeManager& m) {

	INFO("cpu threads amount: " << m.cpuThreadAmount << LF);
	INFO("Glfw::thread_id: " << Glfw::thread_id << LF);

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
	INFO(D_VALUE_NAME(GRID_GARBAGE_DELETION_STEP));
	INFO(D_VALUE_NAME(GRID_GARBAGE_DELETION_RECOMMENDED));

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
				manager->cam->speed = manager->playerSpeed * 8;
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

Obj3dBP* createMergedBP_offsetVerticesWithPos(std::vector<Object*> objects) {
	std::vector<SimpleVertex> vertices;
	std::vector<unsigned int> indices;
	for (auto obj : objects) {
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

	// recreating full mesh without updating it in the ChunkGrid:: ?? todo: it should crash when entering a new chunk, check that and fix if needed

	D("Building new fullMesh...\n");
	Obj3dBP* fullMeshBP = new Obj3dBP(vertices, indices, BP_DONT_NORMALIZE);
	D("BP ready\n");
	return fullMeshBP;
}


static void	debugGridGeometry(int len, int start, int end, std::string prefix = "") {
	std::cout << prefix << " : ";
	for (int i = 0; i < len; i++) {
		if (i == 0) { std::cout << "["; }
		else if (i == len - 1) { std::cout << "]"; }
		else if (i == start) { std::cout << "<"; }
		else if (i == end - 1) { std::cout << ">"; }
		else { std::cout << "."; }
	}std::cout << LF;
}

/*
	todo: Swap system to have instant grabbing (still need to load the chunks/hmaps in the GPU)
		- grabbing could be done by the helper thread, it builds data handlers 
		- when data is ready, the gl thread ask for new data: it's only a swap of some pointers.
		- when data is transfered, helper clears old data handlers
		- gl thread loads hmap/chunk in the GPU
*/
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
	//D("0408 preclear\n");
	manager.renderVecChunkShPtr.clear();
	//D("0408 postclear\n");

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
#if TRIM_FLOOR // only above ground and ground itself
	Math::Vector3 minRendered = grid.worldToGrid(Math::Vector3(1, 1, 1)); // x and z ignored
	startRendered.y = std::max(startRendered.y, minRendered.y);
	endRendered.y = std::max(endRendered.y, startRendered.y + 5);
	startRendered.y = std::max(startRendered.y, geometry.renderedGridIndex.y);
	endRendered.y = std::min(endRendered.y, geometry.renderedGridIndex.y + geometry.renderedGridSize.y);
#endif
	//debugGridGeometry(grid.getSize().x, startRendered.x, endRendered.x, "x");
	debugGridGeometry(grid.getSize().y, startRendered.y, endRendered.y, "y");
	//debugGridGeometry(grid.getSize().z, startRendered.z, endRendered.z, "z");
	D(" > Grid index rendered : " << startRendered << " -> " << endRendered << "\n");
	D(" > World index rendered : " << grid.gridToWorld(startRendered) << " -> " << grid.gridToWorld(endRendered) << "\n");

	if (1) {// actual grabbing + Obj3d creation
		INFO(grid.getGridChecks());
		double start = glfwGetTime();
		grid.glth_loadAllChunksToGPU();
		start = glfwGetTime() - start;
		INFO("data loaded in GPU in " << start * 1000 << " ms\n");
		grid.pushRenderedChunks(&manager.renderVecChunkShPtr);
		int emptyptrCount = 0;
		for (auto& sptr : manager.renderVecChunkShPtr) {
			if (!sptr.get()) {
				emptyptrCount++;
				//INFO("Error: shared_ptr is empty ?!\n"); // meaning not yet generated ?
				//Misc::breakExit(5422);
			}
			else {
				//std::cout << "<";
				if (sptr.get()->mesh) {//nullptr means empty chunk (or not yet generated, but this shouldn't ne the case as glth_loadAllChunksToGPU() was called just above)
					//std::cout << "o";
					Math::Vector3 chunkGridIndex = grid.worldToGrid(sptr.get()->index);
					#if TRIM_FLOOR
					if (chunkGridIndex.y >= startRendered.y && chunkGridIndex.y < endRendered.y)
					#endif
						manager.renderVecChunk.push_back(sptr.get()->mesh);
				}
			}
		}
		std::cout << std::endl;
		INFO("grid pushed " << manager.renderVecChunkShPtr.size() << " ChunkShPtr\n");
		INFO("grid pushed " << emptyptrCount << " empty ChunkShPtr\n");

		//for (auto x = 0; x < sizeArray; x++) {
		//	Obj3d* o = dynamic_cast<Obj3d*>(manager.renderVecChunk[x]);
		//	o->setPolygonMode(GL_LINE);
		//}

		//merge BPs
		if (M_MERGE_CHUNKS) {//merge BPs for a single draw call with the renderVecChunk
			INFO("Merging all chunks...\n");
			// recreating full mesh without updating it in the ChunkGrid:: ?? todo: it should crash when entering a new chunk, check that and fix if needed
			D("Building fullMesh...\n");
			Obj3dBP* fullMeshBP = createMergedBP_offsetVerticesWithPos(manager.renderVecChunk);
			Obj3d* fullMesh = new Obj3d(*fullMeshBP, obj3d_prog);
			D("Done, " << manager.renderVecChunk.size() << " chunks merged.\n");
			manager.renderVecChunk.clear();
			manager.renderVecChunk.push_back(fullMesh);
			//todo: fixleak: this will leak in the next grab when clearing renderVecChunk
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
						Octree<Voxel>* node = chunk->root->getNode(chunk->root->pos, siz);
						if (node) {
							if (node->size.len() != siz.len())
								node->element._value = 254;
							else
								node->element._value = 253;
						}
					}

				}
			}
		}
	}
	#endif // M_DRAW_DEBUG

	//INFO("Counting total_polygons from renderVecChunk\n");
	for (Object* o : manager.renderVecChunk) {
		Obj3d* obj = dynamic_cast<Obj3d*>(o);
		if (!obj) { D("grabObjects() : dynamic cast failed on object: " << o << "\n"); Misc::breakExit(456); }
		//total_polygons += obj->getBlueprint()->getPolygonAmount();
		total_polygons += ((Obj3dBP*)obj->getBlueprint()->lodManager.getCurrentLodBlueprint())->getPolygonAmount();
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

	Math::Vector3	gridSize = grid.getSize();
	int hmap_max = int(gridSize.x) * int(gridSize.z);
	int chunks_max = hmap_max * int(gridSize.y);
	HeightMap::m.lock();
	if (hmap_max < HeightMap::count)
		INFO("Error: HeightMap::count should not exceed " << hmap_max << ". ");
	INFO("HeightMap::count\t" << HeightMap::count << "\n");
	HeightMap::m.unlock();
	Chunk::m.lock();
	if (chunks_max < Chunk::count)
		INFO("Error: Chunk::count should not exceed " << chunks_max << ". ");
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
			else if (key == GLFW_KEY_T) {
				m->grid->glth_testGarbage(20);
				std::exit(0);
			}
		}
	}
}

/*
*   V1:
*	current grid generation loop:
*		- [helper0] checks for player chunk change and shift grid if needed
*		- [helper0] builds jobs for missing hmaps and chunks
*		- [threads] execute jobs:
			- build chunks by generating the octree with perlinsettings
			- build the vertexArrays depending on LOD and octree_threshold, plug vertex in the corresponding vertex_array[lod]
*		- [helper0] delivers jobs:
						- plugs hmap/chunk in the grid, put old hmap/chunk in garbage if needed
*		- [gl_main] build new meshes:
						- for all LODs if vertex_array[lod] is not empty
						- grabs all rendered chunks, selecting available meshes in all vertex_array[]
* 
*	todo:
*		- check why there are some chunks in the garbade after the first loop, although the player didnt move.
*		- check every ctor by copy, they can access private members, useless to use accessors
*		- inconsistencies? with the use of ref or ptr on some pipeline
*		- generate the vertexArray[lod] and its BP only when needed (when close enough from the cam/player)
*	done:
*		- memory leaks in chunks generation : LODs were not deleted
*	bugs :
*		- when renderedGrid.size = grid.size, race between the renderer and grid.updater
*
*	[Checklist] all gl calls have to be done on the gl context (here main thread)
*
*  -------------------------------------------------------------------------------------------
*	V2:
*   the grid is the entire world, ie the root of the octree
*   we render everything in the grid
*	chunks treeLODs are generated on the fly, depending on the distance from the player
*
*	* treeLOD != obj3d LOD
*	- start with a quadtree, then octree:
*		- start by generating the world with a crappy treeLOD. 1 single leaf or 8 leaves ?
*		- then, depending on the distance from the player, generate the next treeLODs, 1 per 1
*
*
*
* 
*/
void	scene_octree() {
	#ifndef INIT_GLFW
	float	win_height = 1080;
	float	win_width = 1920;
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
	//m.glfw->func[GLFW_KEY_T] = keyCallback_debugGrid; // test : currently garbage deletion in the grid

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
	Obj3dPG* renderer = &rendererObj3d;// TODO: need a voxel renderer
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
	//m.playerSpeed *= 10;

	#define USE_THREADS
	#ifdef USE_THREADS
	INFO("USE_THREADS : true\n");
	#else
	INFO("USE_THREADS : false\n");
	#endif

	#ifndef GENERATOR
	int grid_size = GRID_SIZE;
	if (0) {
		std::cout << "Enter grid size (min 7, max 35):\n";
		std::cin >> grid_size;
		grid_size = (grid_size < 7) ? 7 : grid_size;
		grid_size = (grid_size > 35) ? 35 : grid_size;
	}
	int	g = grid_size;
	int	r = grid_size - 4;// g * 2 / 3;
	//m.gridSize = Math::Vector3(g, std::max(5,g/4), g);
	//m.renderedGridSize = Math::Vector3(r, std::max(3,r/4), r);
	m.gridSize = Math::Vector3(g, g/4, g);
	m.renderedGridSize = Math::Vector3(r, r/4, r);

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
				INFO("\n>>>>>>>> " << frames << " FRAMES <<<<<<<<\n");
				INFO("cam: " << cam.local.getPos() << "\n");
				INFO("Grid HeightMaps max\t" << (m.gridSize.z * m.gridSize.x) << "\n");
				INFO("HeightMap::count   \t" << HeightMap::count << "\n");
				INFO("Grid Chunks max    \t" << (m.gridSize.z * m.gridSize.y * m.gridSize.x) << "\n");
				INFO("Chunk::count       \t" << Chunk::count << "\n");
				INFO("Garbage            \t" << grid.getGarbageSize() << "\n");
			}
			std::string decimals = std::to_string(polygons / 1'000'000.0);
			m.glfw->setTitle(
				std::to_string(fps.getFps()) + " fps | "
				+ std::to_string(int(polygons/1'000'000)) + "m"
				+ ( decimals.c_str()+decimals.find('.')+1 )
				+ " polys | threshold "
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

			// garbage
			size_t garbage = grid.getGarbageSize();
			if (garbage >= GRID_GARBAGE_DELETION_RECOMMENDED) {
				INFO("Too much garbage (" << garbage << "), trying to delete...\n");
				grid.glth_try_emptyGarbage();
			}
		}
		else {
			grid.glth_try_emptyGarbage();
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
		Math::Vector3		index(55, 1, 8); // 13554 vertices LOD_0

		if (0) {
			// find big chunk
			Math::Vector3 highest = index;
			size_t highestV = 0;
			size_t i = 0;
			FOR(z, 0, 100) {
				FOR(y, -10, 10) {
					FOR(x, 0, 100) {
						index = Math::Vector3(x, y, z);
						hmap = new HeightMap(ps, index, chunk_size);
						chunk = new Chunk(index, chunk_size, ps, hmap);
						FOR(lod, 0, 1) {
							size_t v = chunk->buildVertexArray(Math::Vector3(), lod, 0);
							if (v > highestV) {
								highestV = v;
								highest = index;
							}
						}

						delete hmap;
						delete chunk;
						{
							int j = i + 1;
							//std::cout << j << LF;
							if (j % 100 == 0)
								std::cout << ".";
							if (j % 5000 == 0)
								std::cout << " " << j << "\n";
						}
						i++;
					}
				}
			}
			std::cout << highest << "\t" << highestV << LF;
			std::exit(0);
		}

		FOR(i, 0, 5000) {
			hmap = new HeightMap(ps, index, chunk_size);
			chunk = new Chunk(index, chunk_size, ps, hmap);
			FOR(lod, 0, LODS_AMOUNT) {
				chunk->buildVertexArray(Math::Vector3(), lod, 0);
			}
			chunk->glth_buildAllMeshes();
			//D(chunk->meshBP->lodManager.toString() << "\n");
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


void	printClassSizes() {
	std::cout << "sizeof(Octree) = " << sizeof(Octree<Voxel>) << "\n";
	std::cout << "sizeof(Voxel) = " << sizeof(Voxel) << "\n";
	std::cout << "sizeof(Chunk) = " << sizeof(Chunk) << "\n";
	std::cout << "sizeof(HeightMap) = " << sizeof(HeightMap) << "\n";
	std::cout << "sizeof(SimpleVector2) = " << sizeof(SimpleVector2) << "\n";
	std::cout << "sizeof(SimpleVertex) = " << sizeof(SimpleVertex) << "\n";
	std::cout << "sizeof(std::vector) = " << sizeof(std::vector<SimpleVertex>) << "\n";
	std::cout << "sizeof(std::vector) = " << sizeof(std::vector<unsigned int>) << "\n";
}	

#if 1 // main
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <cmath>

//thread safe cout : https://stackoverflow.com/questions/14718124/how-to-easily-make-stdcout-thread-safe
//multithread monitor example : https://stackoverflow.com/questions/51668477/c-lock-a-mutex-as-if-from-another-thread
int		main(int ac, char **av) {
	//printClassSizes(); exit(0);

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
	//scene_checkMemory();
	//scene_benchmarks();
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
