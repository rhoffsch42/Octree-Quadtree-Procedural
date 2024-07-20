#pragma once

#include "simplegl_includes.h"

#include "quadtree.hpp"
#include "octree.hpp"
#include "chunk.hpp"
#include "chunkgenerator.hpp"
#include "perlin.hpp"
#include "quadtree.hpp"
#include "utils.hpp"

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

//#define TREES_DEBUG

