#pragma once

#include "heightmap.hpp" // also for PerlinSettings
#include "chunkgenerator.hpp"
#include "chunkgrid.hpp"
//class PerlinSettings;
class ChunkGenerator;
class ChunkGrid;

class Job
{
public:
	virtual bool	execute() = 0;
	virtual bool	deliver() const = 0;

	bool	assigned = false;
	bool	done = false;
protected:
	Job();
};

class JobBuildGenerator : public Job
{
public:
	JobBuildGenerator() = delete;
	bool			prepare(PerlinSettings* settings);
	virtual bool	execute() = 0;
	virtual bool	deliver() const = 0;
	Math::Vector3	getChunkSize() const;
	Math::Vector3	getWorldIndex() const;
protected:
	JobBuildGenerator(ChunkGenerator* generator, ChunkGrid* grid, Math::Vector3 worldIndex, Math::Vector3 chunkSize);
	ChunkGenerator* _generator;
	PerlinSettings* _settings = nullptr; // Provided by the thread executing the job, using prepare() before execute().
	ChunkGrid*		_grid;
	Math::Vector3	_chunkSize;
	Math::Vector3	_worldIndex;
};

#ifndef GDGDGDDF
class JobBuildHeightMap : public JobBuildGenerator
{
public:
	JobBuildHeightMap(ChunkGenerator* generator, ChunkGrid* grid, Math::Vector3 worldIndex, Math::Vector3 chunk_size);
	virtual bool	execute();
	virtual	bool	deliver() const;
private:
	HeightMap*	_hmap = nullptr;
};

class JobBuildChunk : public JobBuildGenerator
{
public:
	JobBuildChunk(ChunkGenerator* generator, ChunkGrid* grid, Math::Vector3 worldIndex, Math::Vector3 chunk_size, HeightMap* hmap);
	virtual bool	execute();
	virtual bool	deliver() const;

private:
	HeightMap*	_hmap = nullptr;
	Chunk*		_chunk = nullptr;
	void			_buildVertexArray();
};


#define LOD_0	0b00000001
#define LOD_1	0b00000010
#define LOD_2	0b00000100
#define LOD_3	0b00001000
#define LOD_4	0b00010000
#define LOD_5	0b00100000
#define LOD_ALL	0b11111111

/*
	Currently build all LODs
*/
class JobBuildChunkVertexArray : public Job
{
public:
	/*
		octreeThresholds should have LODS_AMOUNT values (1 for each LOD). Extras will be ignored, missing ones will be defaulted to 0 (full octree details).
		ie: octreeThresholds.resize(LODS_AMOUNT, 0)
	*/
	JobBuildChunkVertexArray(ChunkShPtr* chunk, uint8_t LODsFlag, std::vector<uint8_t> octreeThresholds);
	bool	execute();
	bool	deliver();
private:
	ChunkShPtr				_chunk;
	uint8_t					_LODsFlag;
	std::vector<uint8_t>	_octreeThresholds;
	std::vector<uint8_t>	_vertexArray[LODS_AMOUNT];
};
#endif