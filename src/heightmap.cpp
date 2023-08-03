#include "heightmap.hpp"

#ifdef TREES_DEBUG
#define TREES_HEIGHTMAP_DEBUG
#endif
#ifdef TREES_HEIGHTMAP_DEBUG 
#define D(x) std::cout << "[HeightMap] " << x
#define D_(x) x
#define D_SPACER "-- heightmap.cpp -------------------------------------------------\n" 
#define D_SPACER_END "----------------------------------------------------------------\n"
#else 
#define D(x)
#define D_(x)
#define D_SPACER ""
#define D_SPACER_END ""
#endif

int HeightMap::count = 0;
std::mutex	HeightMap::m;
static void	_countAdd(int n) {
	HeightMap::m.lock();
	HeightMap::count += n;
	HeightMap::m.unlock();
}
HeightMap::HeightMap(PerlinSettings& perlin_settings, Math::Vector3 chunk_index, Math::Vector3 chunk_size)
	: IDisposable(), _index(chunk_index)
{
	_countAdd(1);
	this->_posX = this->_index.x * chunk_size.x;
	this->_posZ = this->_index.z * chunk_size.z;
	this->_sizeX = chunk_size.x;
	this->_sizeZ = chunk_size.z;

	this->buildMap(perlin_settings);
}

HeightMap::HeightMap(PerlinSettings& perlin_settings, int posx, int posz, int sizex, int sizez)
	: IDisposable(), _posX(posx), _posZ(posz), _sizeX(sizex), _sizeZ(sizez)
{
	_countAdd(1);
	this->buildMap(perlin_settings);
}

void	HeightMap::buildMap(PerlinSettings& perlin_settings) {
	this->map = perlin_settings.genHeightMap(this->_posX, this->_posZ, this->_sizeX, this->_sizeZ);
	#if HMAP_BUILD_TEXTUREDATA_IN_CTOR
	this->textureData = PerlinSettings::HeightmapToTextureData(map, this->_sizeX, this->_sizeZ);
	#endif
}

HeightMap::~HeightMap() {
	_countAdd(-1);
	//std::cout << __PRETTY_FUNCTION__ << this << "\n";

	std::thread::id thread_id = std::this_thread::get_id();
	if (Glfw::thread_id != thread_id) {
		//D("Error: Heightmap " << this << " dtor called in the wrong thread. " << "Glfw::thread_id: " << Glfw::thread_id << ", != " << thread_id << "\n");
	}

	if (this->texture) {
		delete this->texture;
		if (Glfw::thread_id != thread_id) {
			D("Error: Heightmap texture " << this << " dtor called in the wrong thread. " << "Glfw::thread_id: " << Glfw::thread_id << ", != " << thread_id << "\n");
		}
		D_(else {
			D("Good: Heightmap texture " << this << " dtor called in the right thread. " << "Glfw::thread_id: " << Glfw::thread_id << ", != " << thread_id << "\n");
		});
	}
	if (this->panel) {
		delete this->panel;
		if (Glfw::thread_id != thread_id) {
			D("Error: Heightmap panel " << this << " dtor called in the wrong thread. " << "Glfw::thread_id: " << Glfw::thread_id << ", != " << thread_id << "\n");
		}
		D_(else {
			D("Good: Heightmap panel " << this << " dtor called in the right thread. " << "Glfw::thread_id: " << Glfw::thread_id << ", != " << thread_id << "\n");
		});
	}

	//should be deleted only here, it is used by Chunk::
	for (unsigned int z = 0; z < this->_sizeZ; z++) {
		delete[] this->map[z];
	}
	delete[] this->map;
	this->map = nullptr;

	// if the UIPanel wasn't built, there is this remaining data.
	delete[] this->textureData;
	this->textureData = nullptr;
}

//opengl thread
void	HeightMap::glth_buildPanel() {
	//Glfw::glThreadSafety();
	if (this->panel) {
		D("Error: Panel already built: " << this->panel << "\n");
		Misc::breakExit(575);
	}
	if (!this->textureData) {
		this->textureData = PerlinSettings::HeightmapToTextureData(map, this->_sizeX, this->_sizeZ);
	}
	this->texture = new Texture(this->textureData, this->_sizeX, this->_sizeZ);
	this->panel = new UIImage(this->texture);
	this->panel->setPos(0, 0);
	this->panel->setSize(this->_sizeX, this->_sizeZ);

	delete[] this->textureData;
	this->textureData = nullptr;
}

Math::Vector3	HeightMap::getIndex() const { return this->_index; }
