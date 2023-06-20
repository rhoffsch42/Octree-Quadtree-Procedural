#include "job.hpp"
#include "trees.h"

#ifdef TREES_DEBUG
 #define TREES_JOB_DEBUG
#endif
#ifdef TREES_JOB_DEBUG 
 #define D(x) std::cout << "[Job] " << x ;
 #define D_(x) x ;
 #define D_SPACER "-- job.cpp -------------------------------------------------\n"
 #define D_SPACER_END "----------------------------------------------------------------\n"
#else
 #define D(x)
 #define D_(x)
 #define D_SPACER ""
 #define D_SPACER_END ""
#endif

Job::Job() : assigned(false), done(false) {}

JobBuildGenerator::JobBuildGenerator(Math::Vector3 ind, Math::Vector3 chunk_size)
	: index(ind), chunkSize(chunk_size) {}

JobBuildHeighMap::JobBuildHeighMap(Math::Vector3 index, Math::Vector3 chunk_size)
	: JobBuildGenerator(index, chunk_size) {}

bool	JobBuildHeighMap::execute(PerlinSettings& perlinSettings) {
	this->hmap = new HeightMap(perlinSettings, this->index, this->chunkSize);
	//this->hmap->glth_buildPanel();
	this->done = true;
	#ifdef CHUNK_GEN_DEBUG
	D("job executed : new hmap : " << this->hmap << " " << this->index << "\n")
	#endif
	return this->done;
}

void	JobBuildHeighMap::deliver(ChunkGenerator& generator) const {
	Math::Vector3	ind = generator.worldToGrid(this->index);
	ind.y = 0;//same hmap for all Y // not used...
	int x = ind.x;
	int z = ind.z;
	if (this->index != this->hmap->getIndex() || this->index.y != 0) {
		D("wrong index for job " << this->index << " and hmap " << this->hmap << this->hmap->getIndex() << "\n")
		Misc::breakExit(-14);
	}
	if (x < 0 || z < 0 || x >= generator.gridSize.x || z >= generator.gridSize.z) {
		D("out of memory hmap " << this->hmap << " index " << ind << "\n")
		generator.trash_mutex.lock();
		generator.trashHeightMaps.push_back(this->hmap);
		generator.trash_mutex.unlock();
		generator.map_jobsHmap[this->index] = false;
		return;
	}
	if (generator.heightMaps[z][x]) {
		HeightMap* h = generator.heightMaps[z][x];
		D("Heightmap " << this->hmap << " overriding " << h << " " << ind << "\n")
		D("This shouldn't happen, exiting...\n")
		//Misc::breakExit(-14);
	}

	generator.heightMaps[z][x] = this->hmap;
	generator.map_jobsHmap[this->index] = false;
	//D(this->hmap << " hmap " << this->hmap->getIndex() << " plugged in " << ind << "\n")
}

JobBuildChunk::JobBuildChunk(Math::Vector3 index, Math::Vector3 chunk_size, HeightMap* hmap_ptr)
	: JobBuildGenerator(index, chunk_size), hmap(hmap_ptr) {}

bool	JobBuildChunk::execute(PerlinSettings& perlinSettings) {
	if (!this->hmap) {// should not be possible
		D("JobBuildChunk error: HeightMap nullptr, for index: " << this->index << "\n")
		//Misc::breakExit(-14);
		this->done = false;//should already be false, better be sure
		return this->done;
	}
	this->chunk = new Chunk(this->index, this->chunkSize, perlinSettings, this->hmap);
	this->hmap->unDispose();
	//this->chunk->glth_buildMesh();
	this->done = true;
#ifdef CHUNK_GEN_DEBUG
	D("job executed : new chunk : " << this->chunk << " " << this->index << "\n")
#endif
	return this->done;
}

void	JobBuildChunk::deliver(ChunkGenerator& generator) const {
	Math::Vector3	ind = generator.worldToGrid(this->index);
	int x = ind.x;
	int y = ind.y;
	int z = ind.z;

	if (this->index != this->chunk->index) {
		D("wrong index for job " << this->index << " and chunk " << this->chunk << this->chunk->index << "\n")
		Misc::breakExit(-14);
	}
	if (x < 0 || y < 0 || z < 0 || x >= generator.gridSize.x || y >= generator.gridSize.y || z >= generator.gridSize.z) {
		D("out of memory chunk " << this->chunk << this->index << " on grid: " << ind << "\n")
		generator.trash_mutex.lock();
		generator.trashChunks.push_back(this->chunk);
		generator.trash_mutex.unlock();
		generator.map_jobsChunk[this->index] = false;
		return;
	}
	if (generator.grid[z][y][x]) {
		Chunk* c = generator.grid[z][y][x];
		D("Chunk " << this->chunk << this->index << " overriding " << c << c->index << " on grid: " << ind << "\n")
		D("This shouldn't happen, exiting...\n")
		//Misc::breakExit(-14);
	}
	//build the vertex array (should be done directly in JobBuildChunk::execute())
	//(should be updated when the chunk distance crosses a step distance...)
	Math::Vector3 playerGridIndex = generator.worldToGrid(generator.currentChunkWorldIndex);

	/*
		The part below should be done at the selection of the chunk, when grabbing(). Tesslevels should be pregenerated, or at best at a certain level.
		this should result in big grid of octrees with the highest nodes beeing always generated with the max tess lvl. 32 lvls ?
		Then we would take the desired level if possible: max(desired, available)
		pb: how to know what is the max tesslvl currently generated?
		we need to modify the chunk job: generate data at a desired tessLvl, if it's not already done. need: bool Chunk::tessLvlGenerated[TESSELATION_LVLS]; or uint8_t minTessLvlGenerated;
		the final goal is having a single octree covering (at max) coo[min double : max double] and generating world at different tess lvl depending on player dist
		sectoring might be needed to win some time, depending on the procedural generation and the type of world
		caching ?
	*/
	//tesselation depending on distance from the player
	double maxDist = 60;// chunks
	double step = maxDist / TESSELATION_LVLS;
	double playerDist = (ind - playerGridIndex).len();
	double threshold = (playerDist / maxDist) * 100;
	int tessLevel = 0;
	if (playerDist > 1) {
		playerDist -= 3;//?
		tessLevel = std::clamp(int(playerDist / step), 0, TESSELATION_LVLS-3);
	}

	threshold = 0;//override
	//tessLevel = 0;//override
	this->chunk->buildVertexArraysFromOctree(this->chunk->root, Math::Vector3(0, 0, 0), tessLevel, &threshold);
	//this->chunk->clearOctreeData();

	generator.grid[z][y][x] = this->chunk;
	generator.map_jobsChunk[this->index] = false;
	//std::cout << this->chunk << " chunk " << this->chunk->index << " plugged in " << ind << "\n";
	generator.chunksChanged = true;
}
