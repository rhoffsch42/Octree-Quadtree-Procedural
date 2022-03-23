#include "job.hpp"

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
	std::cout << "job executed : new hmap : " << this->hmap << " " << this->index << "\n";
	#endif
	return this->done;
}

void	JobBuildHeighMap::deliver(ChunkGenerator& generator) const {
	Math::Vector3	ind = generator.worldToGrid(this->index);
	ind.y = 0;//same hmap for all Y // not used...
	int x = ind.x;
	int z = ind.z;
	if (this->index != this->hmap->getIndex() || this->index.y != 0) {
		std::cerr << "wrong index for job " << this->index << " and hmap " << this->hmap << this->hmap->getIndex() << "\n";
		Misc::breakExit(-14);
	}
	if (x < 0 || z < 0 || x >= generator.gridSize.x || z >= generator.gridSize.z) {
		std::cerr << "out of memory hmap " << this->hmap << " index " << ind << "\n";
		generator.trash_mutex.lock();
		generator.trashHeightMaps.push_back(this->hmap);
		generator.trash_mutex.unlock();
		generator.map_jobsHmap[this->index] = false;
		return;
	}
	if (generator.heightMaps[z][x]) {
		HeightMap* h = generator.heightMaps[z][x];
		std::cerr << "Heightmap " << this->hmap << " overriding " << h << " " << ind << "\n";
		std::cerr << "This shouldn't happen, exiting...\n";
		//Misc::breakExit(-14);
	}

	generator.heightMaps[z][x] = this->hmap;
	generator.map_jobsHmap[this->index] = false;
	//std::cout << this->hmap << " hmap " << this->hmap->getIndex() << " plugged in " << ind << "\n";
}

JobBuildChunk::JobBuildChunk(Math::Vector3 index, Math::Vector3 chunk_size, HeightMap* hmap_ptr)
	: JobBuildGenerator(index, chunk_size), hmap(hmap_ptr) {}

bool	JobBuildChunk::execute(PerlinSettings& perlinSettings) {
	if (!this->hmap) {// should not be possible
		std::cerr << "JobBuildChunk error: HeightMap nullptr, for index: " << this->index << "\n";
		//Misc::breakExit(-14);
		this->done = false;//should already be false, better be sure
		return this->done;
	}
	this->chunk = new Chunk(this->index, this->chunkSize, perlinSettings, this->hmap);
	this->hmap->unDispose();
	//this->chunk->glth_buildMesh();
	this->done = true;
#ifdef CHUNK_GEN_DEBUG
	std::cout << "job executed : new chunk : " << this->chunk << " " << this->index << "\n";
#endif
	return this->done;
}

void	JobBuildChunk::deliver(ChunkGenerator& generator) const {
	Math::Vector3	ind = generator.worldToGrid(this->index);
	int x = ind.x;
	int y = ind.y;
	int z = ind.z;

	if (this->index != this->chunk->index) {
		std::cerr << "wrong index for job " << this->index << " and chunk " << this->chunk << this->chunk->index << "\n";
		Misc::breakExit(-14);
	}
	if (x < 0 || y < 0 || z < 0 || x >= generator.gridSize.x || y >= generator.gridSize.y || z >= generator.gridSize.z) {
		std::cerr << "out of memory chunk " << this->chunk << this->index << " on grid: " << ind << "\n";
		generator.trash_mutex.lock();
		generator.trashChunks.push_back(this->chunk);
		generator.trash_mutex.unlock();
		generator.map_jobsChunk[this->index] = false;
		return;
	}
	if (generator.grid[z][y][x]) {
		Chunk* c = generator.grid[z][y][x];
		std::cerr << "Chunk " << this->chunk << this->index << " overriding " << c << c->index << " on grid: " << ind << "\n";
		std::cerr << "This shouldn't happen, exiting...\n";
		//Misc::breakExit(-14);
	}
	generator.grid[z][y][x] = this->chunk;
	generator.map_jobsChunk[this->index] = false;
	//std::cout << this->chunk << " chunk " << this->chunk->index << " plugged in " << ind << "\n";
	generator.chunksChanged = true;
}
