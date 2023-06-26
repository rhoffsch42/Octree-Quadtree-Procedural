#pragma once

#include "heightmap.hpp" // also for PerlinSettings
#include "chunkgenerator.hpp"
#include "chunkgrid.hpp"
class ChunkGenerator;
class ChunkGrid;

// nouveau check du helper qui voit que certains chunk sont pas crees, les jobs sont crees mais pas finis, donc il cree des doublons de job... qui sont repluggés apres...
// empecher le helper0 de creer des jobs doublons
class Job
{
public:
	Job();
	//virtual bool	execute() = 0;
	//args could differ, make a struct with args and a dyn_cast in execute()?
	//can lambda expressions [] work?

	bool	assigned;
	bool	done;
};

class JobBuildGenerator : public Job
{
public:
	virtual bool	execute(PerlinSettings& perlinSettings) = 0;
	virtual void	deliver(ChunkGenerator& generator, ChunkGrid& grid) const = 0;
	Math::Vector3	index;//world
	Math::Vector3	chunkSize;
protected:
	JobBuildGenerator(Math::Vector3 ind, Math::Vector3 chunk_size);
};

class JobBuildHeighMap : public JobBuildGenerator
{
public:
	JobBuildHeighMap(Math::Vector3 index, Math::Vector3 chunk_size);
	virtual bool	execute(PerlinSettings& perlinSettings);
	virtual	void	deliver(ChunkGenerator& generator, ChunkGrid& grid) const;
	HeightMap* hmap = nullptr;
};

class JobBuildChunk : public JobBuildGenerator
{
public:
	JobBuildChunk(Math::Vector3 index, Math::Vector3 chunk_size, HeightMap* hmap_ptr);
	virtual bool	execute(PerlinSettings& perlinSettings);
	virtual void	deliver(ChunkGenerator& generator, ChunkGrid& grid) const;

	HeightMap* hmap = nullptr;
	Chunk* chunk = nullptr;
};
