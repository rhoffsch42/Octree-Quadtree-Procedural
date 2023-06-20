#pragma once

#include "simplegl.h"
#include "program.hpp"
#include "object.hpp"
#include "obj3dPG.hpp"
#include "obj3d_ipg.hpp"
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
#include "text_pg.hpp"


#include "quadtree.hpp"
#include "octree.hpp"
#include "chunk.hpp"
#include "chunkgenerator.hpp"
#include "perlin.hpp"
#include "quadtree.hpp"

#include <cmath>
#include <string>
#include <cstdio>
#include <list>
#include <algorithm>
#include <thread>
#include <cassert>
#include <mutex>
#include <functional>

#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif

#define WINX 1920
#define WINY 1080
#define SIMPLEGL_FOLDER		std::string("SimpleGL/")
#define WIN32_VS_FOLDER		std::string("")

#define TREES_DEBUG