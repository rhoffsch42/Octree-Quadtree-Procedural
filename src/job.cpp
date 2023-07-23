#include "trees.h"
#include "job.hpp"

#ifdef TREES_DEBUG
 #define TREES_JOB_DEBUG
#endif
#ifdef TREES_JOB_DEBUG 
 #define D(x) std::cout << "[Job] " << x
 #define D_(x) x
 #define D_SPACER "-- job.cpp -------------------------------------------------\n"
 #define D_SPACER_END "----------------------------------------------------------------\n"
#else
 #define D(x)
 #define D_(x)
 #define D_SPACER ""
 #define D_SPACER_END ""
#endif


//Job
Job::Job() {}

//JobBuildGenerator
JobBuildGenerator::JobBuildGenerator(ChunkGenerator* generator, ChunkGrid* grid, Math::Vector3 worldIndex, Math::Vector3 chunkSize)
	: _generator(generator), _grid(grid), _worldIndex(worldIndex), _chunkSize(chunkSize) {}
bool			JobBuildGenerator::prepare(PerlinSettings* settings) {
	this->_settings = settings;
	return (this->_settings != nullptr);
}
Math::Vector3	JobBuildGenerator::getChunkSize() const { return this->_chunkSize; }
Math::Vector3	JobBuildGenerator::getWorldIndex() const { return this->_worldIndex; }

//JobBuildHeightMap
JobBuildHeightMap::JobBuildHeightMap(ChunkGenerator* generator, ChunkGrid* grid, Math::Vector3 worldIndex, Math::Vector3 chunk_size)
	: JobBuildGenerator(generator, grid, worldIndex, chunk_size) {}
bool	JobBuildHeightMap::execute() {
	this->_hmap = new HeightMap(*this->_settings, this->_worldIndex, this->_chunkSize);
	//this->_hmap->glth_buildPanel();
	this->done = true;
	#ifdef CHUNK_GEN_DEBUG
	//D("job executed : new hmap : " << this->_hmap << " " << this->_worldIndex << "\n");
	#endif
	return this->done;
}
bool	JobBuildHeightMap::deliver() const {
	//D(__PRETTY_FUNCTION__ << "\n");
	Math::Vector3	gridIndex = this->_grid->worldToGrid(this->_worldIndex);
	gridIndex.y = 0;//same hmap for all Y
	if (this->_worldIndex != this->_hmap->getIndex() || this->_worldIndex.y != 0) {// important: index.y must be 0, it is used in a map<Math::Vector3, ...>
		D("wrong index for job " << this->_worldIndex << " and hmap " << this->_hmap << this->_hmap->getIndex() << "\n");
		Misc::breakExit(-14);
		return false;
	}
	Math::Vector3 gridSize = this->_grid->getSize();

	if (gridIndex.x < 0 || gridIndex.z < 0 || gridIndex.x >= gridSize.x || gridIndex.z >= gridSize.z) {
		D("out of memory hmap " << this->_hmap << " index " << gridIndex << "\n");
		this->_generator->map_jobsHmap[this->_worldIndex] = false;
		return true;// this is normal, the hmap is only outdated for the current grid
	}

	this->_grid->replaceHeightMap(this->_hmap, gridIndex);
	this->_generator->map_jobsHmap[this->_worldIndex] = false;
	#ifdef CHUNK_GEN_DEBUG
	D(this->_hmap << " hmap " << this->_hmap->getIndex() << " plugged in " << gridIndex << "\n");
	#endif
	return true;
}

//JobBuildChunk
//tmp
void	JobBuildChunk::_buildVertexArray() {
	Math::Vector3	gridIndex = this->_grid->worldToGrid(this->getWorldIndex());
	//build the vertex array (should be done directly in JobBuildChunk::execute())
	//(should be updated when the chunk distance crosses a step distance...)
	Math::Vector3 playerGridIndex = this->_grid->worldToGrid(this->_grid->getPlayerChunkWorldIndex());

	/*
		Then we would take the desired level if possible: max(desired, available)
		pb: how to know what is the max lod currently generated?
			-> we need to modify the chunk job: generate data at a desired lod, if it's not already done.
			-> need: bool Chunk::LODsGenerated[LODS_AMOUNT]; or uint8_t minLODsGenerated; to not conflict with empty chunk (full AIR)
	*/

	//lod depending on distance from the player
	double maxChunkDist = 30;
	double step = maxChunkDist / LODS_AMOUNT;
	double chunkDistFromPlayer = (gridIndex - playerGridIndex).len();
	double threshold = (chunkDistFromPlayer / maxChunkDist) * 100;
	int lod = 0;
	if (chunkDistFromPlayer > 1) {
		chunkDistFromPlayer -= 3;//most precise lod for at least 3 chunk dist
		lod = std::clamp(int(chunkDistFromPlayer / step), 0, LODS_AMOUNT - 1);
	}

	threshold = 0;//override
	//lod = 0;//override
	this->_chunk->buildVertexArray(Math::Vector3(0, 0, 0), lod, threshold);
	//this->chunk->clearOctreeData();
}

JobBuildChunk::JobBuildChunk(ChunkGenerator* generator, ChunkGrid* grid, Math::Vector3 worldIndex, Math::Vector3 chunk_size, HeightMap* hmap)
	: JobBuildGenerator(generator, grid, worldIndex, chunk_size), _hmap(hmap) {}
bool	JobBuildChunk::execute() {
	if (!this->_hmap) {// should not be possible
		D("JobBuildChunk error: HeightMap nullptr, for index: " << this->_worldIndex << "\n");
		//Misc::breakExit(-14);
		this->done = false;//should already be false, better be sure
		return this->done;
	}
	this->_chunk = new Chunk(this->_worldIndex, this->_chunkSize, *this->_settings, this->_hmap);
	this->_hmap->unDispose();
	this->_buildVertexArray();

	//this->_chunk->glth_buildAllMeshes();
	this->done = true;
	#ifdef CHUNK_GEN_DEBUG
	//D("job executed : new chunk : " << this->_chunk << " " << this->_worldIndex << "\n");
	#endif
	return this->done;
}
bool	JobBuildChunk::deliver() const {
	//D(__PRETTY_FUNCTION__ << "\n");
	Math::Vector3	gridSize = this->_grid->getSize();
	Math::Vector3	gridIndex = this->_grid->worldToGrid(this->_worldIndex);
	int x = gridIndex.x;
	int y = gridIndex.y;
	int z = gridIndex.z;

	if (this->_worldIndex != this->_chunk->index) {
		D("wrong index for job " << this->_worldIndex << " and chunk " << this->_chunk << this->_chunk->index << "\n");
			Misc::breakExit(-14);
			return false;
	}
	if (x < 0 || y < 0 || z < 0 || x >= gridSize.x || y >= gridSize.y || z >= gridSize.z) {
		D("out of memory chunk " << this->_chunk << this->_worldIndex << " on grid: " << gridIndex << "\n");
		this->_generator->map_jobsChunk[this->_worldIndex] = false;
		return true;// this is normal, the chunk is only outdated for the current grid
	}

	this->_grid->replaceChunk(this->_chunk, gridIndex);
	this->_generator->map_jobsChunk[this->_worldIndex] = false;
	#ifdef CHUNK_GEN_DEBUG
	D(this->_chunk << " chunk " << this->_chunk->index << " plugged in " << gridIndex << "\n");
	#endif
	return true;
}

//JobBuildChunkVertexArray
JobBuildChunkVertexArray::JobBuildChunkVertexArray(ChunkShPtr* chunk, uint8_t LODsFlag, std::vector<uint8_t> octreeThresholds)
	: _chunk(*chunk), _LODsFlag(LODsFlag), _octreeThresholds(octreeThresholds)
{
	this->_octreeThresholds.resize(LODS_AMOUNT, 0);
}
bool	JobBuildChunkVertexArray::execute() {
	Chunk* c = nullptr;
	for (uint8_t lod = 0; lod < LODS_AMOUNT; lod++) {
		c = this->_chunk.get();
		if (!c) {
			D("Error: empty ChunkShPtr\n");
			Misc::breakExit(534);
		} else {
			//c->glth_clearMeshesData();//should be already empty at this point
			c->buildVertexArray(Math::Vector3(), lod, this->_octreeThresholds[lod]);
		}
	}
	return true;
}
