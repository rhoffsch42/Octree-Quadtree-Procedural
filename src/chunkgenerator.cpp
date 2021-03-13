#include "chunkgenerator.hpp"
#include "compiler_settings.h"
#include <algorithm>

 std::mutex	ChunkGenerator::chunks_mutex;


ChunkGenerator::ChunkGenerator(Math::Vector3 player_pos, const PerlinSettings& perlin_settings, Math::Vector3 chunk_size, Math::Vector3 grid_size, Math::Vector3 grid_size_displayed)
	: chunkSize(chunk_size),
	gridSize(grid_size),//better be odd for equal horizon on every cardinal points
	gridDisplaySize(grid_size_displayed),//better be odd for equal horizon on every cardinal points
	settings(perlin_settings)
{
	std::cout << "_ " << __PRETTY_FUNCTION__ << std::endl;
	// current tile index
	this->updatePlayerPos(player_pos);
	this->gridPos = this->currentChunk;
	this->gridDisplayIndex = Math::Vector3(
		int(this->gridSize.x / 2),
		int(this->gridSize.y / 2),
		int(this->gridSize.z / 2));
	//clamp grid display
	this->gridDisplaySize.x = std::clamp(this->gridDisplaySize.x, 1.0f, this->gridSize.x);
	this->gridDisplaySize.y = std::clamp(this->gridDisplaySize.y, 1.0f, this->gridSize.y);
	this->gridDisplaySize.z = std::clamp(this->gridDisplaySize.z, 1.0f, this->gridSize.z);

	//init grid
	Math::Vector3 v(this->gridSize);
	v.div(2);
	Math::Vector3	smallestChunk(this->currentChunk);
	//smallestChunk.sub(v);
	smallestChunk.sub((int)v.x, (int)v.y, (int)v.z);//a Vector3i would be better

	this->heightMaps = new HeightMap**[this->gridSize.z];
	for (unsigned int k = 0; k < this->gridSize.z; k++) {
		this->heightMaps[k] = new HeightMap*[this->gridSize.x];
		for (unsigned int i = 0; i < this->gridSize.x; i++) {
			this->heightMaps[k][i] = new HeightMap(this->settings,
				(smallestChunk.x + i) * this->chunkSize.x, (smallestChunk.z + k) * this->chunkSize.z,
				this->chunkSize.x, this->chunkSize.z);
		}
	}

	this->grid = new Chunk * **[this->gridSize.z];
	for (unsigned int k = 0; k < this->gridSize.z; k++) {
		this->grid[k] = new Chunk * *[this->gridSize.y];
		for (unsigned int j = 0; j < this->gridSize.y; j++) {
			this->grid[k][j] = new Chunk * [this->gridSize.x];
			for (unsigned int i = 0; i < this->gridSize.x; i++) {
				this->settings.map = this->heightMaps[k][i]->map;
				Math::Vector3	tileNumber(smallestChunk.x + i, smallestChunk.y + j, smallestChunk.z + k);
				this->grid[k][j][i] = new Chunk(tileNumber, this->chunkSize, this->settings);
				this->settings.map = nullptr;
			}
		}
	}
	this->_chunksReadyForMeshes = true;
	this->playerChangedChunk = true;
}

ChunkGenerator::~ChunkGenerator() {
	for (unsigned int k = 0; k < this->gridSize.z; k++) {
		for (unsigned int i = 0; i < this->gridSize.z; i++) {
			delete this->heightMaps[k][i];
		}
		delete[] this->heightMaps[k];
	}
	delete[] this->heightMaps;

	for (unsigned int k = 0; k < this->gridSize.z; k++) {
		for (unsigned int j = 0; j < this->gridSize.y; j++) {
			for (unsigned int i = 0; i < this->gridSize.x; i++) {
				delete this->grid[k][j][i];
			}
			delete[] this->grid[k][j];
		}
		delete[] this->grid[k];
	}
	delete[] this->grid;
}

static void		initValues(float diff, float& inc, float& start, float& end, float& endShift, float intersection, float size) {
	if (diff >= 0) {
		inc = 1;
		start = 0;//inclusive, we start here
		end = size;//exclusive cause we will use `!=` for the comparison
	} else {
		inc = -1;
		start = size - 1;//inclusive, we start here
		end = 0 - 1;//exclusive cause we will use `!=` for the comparison
	}
	endShift = start + inc * intersection;//exclusive
}

static int	calcExceed(int posA, int sizeA, int posB, int sizeB) {
	/*
		A========.========A
		    B----.----B

		sizeB <= sizeA
	*/
	if (sizeB > sizeA) {
		std::cout << "Error: Grid Displayed is larger than the total Grid in memory" << std::endl;
		exit(4);
	}
	int sign = posA <= posB ? 1 : -1;							// set up the sign to mirror the diff if it is negative to handle only positive part
	int diffpos = sign * (posB - posA);							// mirrored if negative
	int diffside = std::max((sizeB - sizeA) / 2 + diffpos, 0);	// std::max(diff,0) : if the diff is negative, nothing exceeds the bounds
	return sign * diffside;										// mirrored back if sign was negative
	/*
		.========A
		  .----B		//B does not exceed A
		      .----C    //C exceeds A
	*/
}

//return true if player step in another chunk
bool	ChunkGenerator::updateGrid_old(Math::Vector3 player_pos) {
	Math::Vector3	old_tile = this->currentChunk;
	this->updatePlayerPos(player_pos);
	Math::Vector3	diff(this->currentChunk);
	diff.sub(old_tile);

	if (diff.magnitude()) {//gridDisplayed
		std::cout << "diff: ";
		diff.printData();

		//calculate if we need to move the memory grid and load new chunks
		Math::Vector3	diffGrid;
		diffGrid.x = calcExceed(this->gridPos.x, this->gridSize.x, this->currentChunk.x, this->gridDisplaySize.x);
		diffGrid.y = calcExceed(this->gridPos.y, this->gridSize.y, this->currentChunk.y, this->gridDisplaySize.y);
		diffGrid.z = calcExceed(this->gridPos.z, this->gridSize.z, this->currentChunk.z, this->gridDisplaySize.z);
		this->gridPos.add(diffGrid);
		std::cout << "\t--diff        \t"; diff.printData();
		std::cout << "\t--diffGrid    \t"; diffGrid.printData();
		std::cout << "\t--gridPos     \t"; this->gridPos.printData();
		std::cout << "\t--currentChunk\t"; this->currentChunk.printData();

		this->gridDisplayIndex.add(diff);
		this->gridDisplayIndex.sub(diffGrid);
		if (diffGrid.magnitude()) {//grid (memory)

			Math::Vector3	halfGrid(int(this->gridSize.x / 2), int(this->gridSize.y / 2), int(this->gridSize.z / 2));//a Vector3i would be better
			//Math::Vector3	smallestChunk(this->currentChunk); smallestChunk.sub(halfGrid);
			Math::Vector3	smallestChunk(this->gridPos); smallestChunk.sub(halfGrid);

			Math::Vector3	absdiff(std::abs(diffGrid.x), std::abs(diffGrid.y), std::abs(diffGrid.z));
			Math::Vector3	intersection(this->gridSize);
			intersection.sub(absdiff);

			std::cout << "size of intersection cube dimension:\n";	intersection.printData();

			//now we define the incrementers, start points and end points to shift the intersection cube and delete unused data
			Math::Vector3	inc, start, end, endShift;
			initValues(diffGrid.x, inc.x, start.x, end.x, endShift.x, intersection.x, this->gridSize.x);
			initValues(diffGrid.y, inc.y, start.y, end.y, endShift.y, intersection.y, this->gridSize.y);
			initValues(diffGrid.z, inc.z, start.z, end.z, endShift.z, intersection.z, this->gridSize.z);

			//todo: generate heightmaps only once, save them, shift them the same way // done?

			Math::Vector3	progress(0, 0, 0);
			this->chunks_mutex.lock();
			for (int k = start.z; k != end.z; k += inc.z) {
				progress.y = 0;
				for (int j = start.y; j != end.y; j += inc.y) {
					progress.x = 0;
					for (int i = start.x; i != end.x; i += inc.x) {
						if (progress.z < absdiff.z || progress.y < absdiff.y || progress.x < absdiff.x) {//if we override a chunk with an existing one
							if (CHUNK_GEN_DEBUG) { std::cout << "delete " << i << ":" << j << ":" << k << std::endl; }
							delete this->grid[k][j][i];
							if (progress.y == 0 && (absdiff.x != 0 || absdiff.z != 0)) {//do it only once, for all the Y, only if we go up/down
								delete this->heightMaps[k][i];
								if (CHUNK_GEN_DEBUG) { std::cout << "deleting hmap " << i << ":" << k << std::endl; }
							}
						}
						if (progress.z < intersection.z && progress.y < intersection.y && progress.x < intersection.x) {
							if (CHUNK_GEN_DEBUG) { std::cout << i << ":" << j << ":" << k << " becomes " << i + diffGrid.x << ":" << j + diffGrid.y << ":" << k + diffGrid.z << std::endl; }
							this->grid[k][j][i] = this->grid[k + int(diffGrid.z)][j + int(diffGrid.y)][i + int(diffGrid.x)];
							this->grid[k + int(diffGrid.z)][j + int(diffGrid.y)][i + int(diffGrid.x)] = nullptr;//useless but cleaner, it will become another chunk on next pass (generated or shifted)
							if (progress.y == 0) {//do it only once, for all the Y
								this->heightMaps[k][i] = this->heightMaps[k + int(diffGrid.z)][i + int(diffGrid.x)];
								//this->heightMaps[k + int(diff.z)][i + int(diff.x)] = nullptr;//dont do that, if diff.z and .x are 0, it leeks the previously created heightmap
							}
						} else {
							if (CHUNK_GEN_DEBUG) { std::cout << "gen new chunk: " << i << ":" << j << ":" << k << std::endl; }
							if (progress.y == 0) {//do it only once, for all the Y
								this->heightMaps[k][i] = new HeightMap(this->settings,
									(smallestChunk.x + i) * this->chunkSize.x, (smallestChunk.z + k) * this->chunkSize.z,
									this->chunkSize.x, this->chunkSize.z);
							}
							this->settings.map = this->heightMaps[k][i]->map;
							this->grid[k][j][i] = new Chunk(
								Math::Vector3(smallestChunk.x + i, smallestChunk.y + j, smallestChunk.z + k),//what happen when we send that to a ref?
								this->chunkSize, this->settings);
							this->settings.map = nullptr;
						}
						progress.x++;
					}
					progress.y++;
					if (CHUNK_GEN_DEBUG) { std::cout << "_ X row\n"; }
				}
				progress.z++;
				if (CHUNK_GEN_DEBUG) { std::cout << "_ Y row\n"; }
			}
			if (CHUNK_GEN_DEBUG) { std::cout << "_ Z row\n"; }

			std::cout << "\n\n";
			this->_chunksReadyForMeshes = true;
			this->chunks_mutex.unlock();
		}//end grid memory
		this->playerChangedChunk = true;
		return true;
	}//end gridDisplayed
	return false;
}

//tmp debug
void   gridChecks(Chunk**** grid, HeightMap*** heightMaps, Math::Vector3 gridSize) {
	int		nonulls = 0;
	int		hmnonulls = 0;
	int nulls = 0;
	int hmnulls = 0;
	for (int k = 0; k < gridSize.z; k++) {
		for (int j = 0; j < gridSize.y; j++) {
			for (int i = 0; i < gridSize.x; i++) {
				if (grid[k][j][i])
					nonulls++;
				else
					nulls++;
				if (j == 0) {
					if (heightMaps[k][i])
						hmnonulls++;
					else {
						hmnulls++;
					}
				}
			}
		}
	}
	std::cout << " > grid checks" << std::endl;
	std::cout << " > nonull: " << nonulls << std::endl;
	std::cout << " > nulls: " << nulls << std::endl;
	std::cout << " >> total: " << nonulls + nulls << std::endl;
	std::cout << " > hmap nonull: " << hmnonulls << std::endl;
	std::cout << " > hmnulls: " << hmnulls << std::endl;
	std::cout << " >> total: " << hmnonulls + hmnulls << std::endl;

}

//return true if player step in another chunk
bool	ChunkGenerator::updateGrid(Math::Vector3 player_pos) {
	Math::Vector3	old_tile = this->currentChunk;
	this->updatePlayerPos(player_pos);
	Math::Vector3	diff(this->currentChunk);
	diff.sub(old_tile);

	if (diff.magnitude() == 0)
		return false;
	//gridDisplayed
	this->chunks_mutex.lock();
	this->playerChangedChunk = true;
	std::cout << "diff: ";
	diff.printData();

	//calculate if we need to move the memory grid and load new chunks
	Math::Vector3	diffGrid;
	diffGrid.x = calcExceed(this->gridPos.x, this->gridSize.x, this->currentChunk.x, this->gridDisplaySize.x);
	diffGrid.y = calcExceed(this->gridPos.y, this->gridSize.y, this->currentChunk.y, this->gridDisplaySize.y);
	diffGrid.z = calcExceed(this->gridPos.z, this->gridSize.z, this->currentChunk.z, this->gridDisplaySize.z);
	this->gridPos.add(diffGrid);
	std::cout << "\t--diff        \t"; diff.printData();
	std::cout << "\t--diffGrid    \t"; diffGrid.printData();
	std::cout << "\t--gridPos     \t"; this->gridPos.printData();
	std::cout << "\t--currentChunk\t"; this->currentChunk.printData();

	this->gridDisplayIndex.add(diff);
	this->gridDisplayIndex.sub(diffGrid); //should be inside the next block? yes, test it after the refacto

	std::cout << "\t--gridDisplayIndex        \t"; this->gridDisplayIndex.printData();

	if (diffGrid.magnitude() == 0) {
		this->chunks_mutex.unlock();
		return true;
	}
	//grid (memory)
	std::vector<int>		indexZ;
	std::vector<int>		indexY;
	std::vector<int>		indexX;
	std::vector<Chunk*>		chunks;
	std::vector<HeightMap*>	hmaps;
	std::vector<Chunk*>		chunksToDelete;
	std::vector<HeightMap*>	hmapsToDelete;

	//store the chunks and heightmaps with their new index 3D
	for (int z = 0; z < this->gridSize.z; z++) {
		for (int y = 0; y < this->gridSize.y; y++) {
			for (int x = 0; x < this->gridSize.x; x++) {
				indexZ.push_back(z - diffGrid.z);//each tile goes in the opposite way of the player movement
				indexY.push_back(y - diffGrid.y);
				indexX.push_back(x - diffGrid.x);
				chunks.push_back(this->grid[z][y][x]);
				this->grid[z][y][x] = nullptr;
				if (y == 0) {// there is only one hmap for an entire column of chunks
					hmaps.push_back(this->heightMaps[z][x]);
					this->heightMaps[z][x] = nullptr;
				} else {
					hmaps.push_back(nullptr);	// to match the chunks index for the next hmap
				}
			}
		}
	}

	std::cout << "chunks referenced: " << chunks.size() << std::endl;
	std::cout << "hmaps referenced (with nulls): " << hmaps.size() << std::endl;

	//reassign the chunks in the grid if they are inside it
	std::cout << "reassigning chunks...";
	for (size_t n = 0; n < chunks.size(); n++) {
		if (indexZ[n] < this->gridSize.z && indexY[n] < this->gridSize.y && indexX[n] < this->gridSize.x \
			&& indexZ[n] >= 0 && indexY[n] >= 0 && indexX[n] >= 0) {
			this->grid[indexZ[n]][indexY[n]][indexX[n]] = chunks[n];
		} else {//is outside of the memory grid
			chunksToDelete.push_back(chunks[n]);
		}
	}
	std::cout << " OK" << std::endl;

	//reassign the heightmaps
	std::cout << "reassigning heightmaps...";
	for (size_t n = 0; n < hmaps.size(); n++) {
		if (hmaps[n]) {
			if (indexZ[n] < this->gridSize.z && indexX[n] < this->gridSize.x \
				&& indexZ[n] >= 0 && indexX[n] >= 0) {
				this->heightMaps[indexZ[n]][indexX[n]] = hmaps[n];
			} else {//is outside of the memory grid
				hmapsToDelete.push_back(hmaps[n]);
			}
		}
	}
	std::cout << " OK" << std::endl;

	//debug checks before rebuild
	std::cout << "checks...\n";
	gridChecks(this->grid, this->heightMaps, this->gridSize);
	std::cout << " OK" << std::endl;

	//rebuild the empty chunks and heightmaps
	int chunksRebuilt = 0;
	int hmapsRebuilt = 0;
	Math::Vector3	halfGrid(int(this->gridSize.x / 2), int(this->gridSize.y / 2), int(this->gridSize.z / 2));//a Vector3i would be better
	Math::Vector3	smallestChunk(this->gridPos); smallestChunk.sub(halfGrid);
	std::cout << "rebuilding new heightmaps...";
	for (int z = 0; z < this->gridSize.z; z++) {
		for (int x = 0; x < this->gridSize.x; x++) {
			if (!this->heightMaps[z][x]) {
				this->heightMaps[z][x] = new HeightMap(this->settings,
					(smallestChunk.x + x) * this->chunkSize.x, (smallestChunk.z + z) * this->chunkSize.z,
					this->chunkSize.x, this->chunkSize.z);
				hmapsRebuilt++;
			}
		}
	}
	std::cout << "rebuilding new chunks...";
	for (int z = 0; z < this->gridSize.z; z++) {
		for (int y = 0; y < this->gridSize.y; y++) {
			for (int x = 0; x < this->gridSize.x; x++) {
				if (!this->grid[z][y][x]) {
					this->settings.map = this->heightMaps[z][x]->map;
					this->grid[z][y][x] = new Chunk(Math::Vector3(smallestChunk.x + x, smallestChunk.y + y, smallestChunk.z + z),
													this->chunkSize, this->settings);
					chunksRebuilt++;
					this->settings.map = nullptr;
				}
			}
		}
	}
	std::cout << "chunksRebuilt : " << chunksRebuilt << std::endl;
	std::cout << "hmapsRebuilt : " << hmapsRebuilt << std::endl;
	std::cout << " OK" << std::endl;

	//debug checks after rebuild
	std::cout << "checks...\n";
	gridChecks(this->grid, this->heightMaps, this->gridSize);
	std::cout << " OK" << std::endl;
	this->_chunksReadyForMeshes = true;

#if 1
	//deleting old chunks and heighmaps
	//there is still a risk for them to be used by the renderer, we should make a list with these now useless data to be deleted at the end of each X frames
	std::cout << "deleting chunks... " << chunksToDelete.size() << std::endl;
	for (size_t n = 0; n < chunksToDelete.size(); n++) {
		std::cout << "deleting chunk : " << chunksToDelete[n] << " ( mesh Object* : " << chunksToDelete[n]->mesh << " )" << std::endl;
		delete chunksToDelete[n];
	}
	std::cout << "deleting hmaps... " << hmapsToDelete.size() << std::endl;
	for (size_t n = 0; n < hmapsToDelete.size(); n++) {
		std::cout << "deleting hmaps : " << hmapsToDelete[n] << std::endl;
		delete hmapsToDelete[n];
	}
#endif
	this->chunks_mutex.unlock();
	return true;
}

bool	ChunkGenerator::buildMeshesAndMapTiles() {
	if (this->_chunksReadyForMeshes) {
		for (unsigned int k = 0; k < this->gridSize.z; k++) {
				for (unsigned int i = 0; i < this->gridSize.x; i++) {
					this->heightMaps[k][i]->glth_buildPanel();
					for (unsigned int j = 0; j < this->gridSize.y; j++) {
						this->grid[k][j][i]->buildMesh();
				}
			}
		}
		this->_chunksReadyForMeshes = false;
		std::cout << "ChunkGenerator meshes built\n";
		return true;
	}
	return false;
}

void	ChunkGenerator::updatePlayerPos(Math::Vector3 player_pos) {
	std::lock_guard<std::mutex> guard(this->mutex1);
	this->playerPos = player_pos;
	this->currentChunk.x = int(this->playerPos.x / this->chunkSize.x);
	this->currentChunk.y = int(this->playerPos.y / this->chunkSize.y);
	this->currentChunk.z = int(this->playerPos.z / this->chunkSize.z);
	if (this->playerPos.x < 0) // cauze if x=-32..+32 ----> x / 32 = 0; 
		this->currentChunk.x--;
	if (this->playerPos.y < 0) // same
		this->currentChunk.y--;
	if (this->playerPos.z < 0) // same
		this->currentChunk.z--;
}

void	ChunkGenerator::printData() {
	std::cout << "currentChunk:"; this->currentChunk.printData();
	std::cout << std::endl;
	for (int k = 0; k < this->gridSize.z; k++) {
		for (int j = 0; j < this->gridSize.y; j++) {
			for (int i = 0; i < this->gridSize.x; i++) {
				std::cout << "[" << k << "]";
				std::cout << "[" << j << "]";
				std::cout << "[" << i << "]\t";
				this->grid[k][j][i]->printData();
				std::cout << std::endl;
			}
		}
	}
}
