#include "chunkgenerator.hpp"

ChunkGenerator::ChunkGenerator(Math::Vector3 player_pos, PerlinSettings perlin_settings, unsigned int chunk_size)
	: settings(perlin_settings)
{
	std::cout << "_ " << __PRETTY_FUNCTION__ << std::endl;
	// current tile index
	this->chunkSize = chunk_size;
	this->updatePlayerPos(player_pos);

	//init grid
	this->size = Math::Vector3(3, 3, 3);//better be odd for equal horizon on every cardinal points
	Math::Vector3 v(this->size);
	v.div(2);
	Math::Vector3	smallestTile(this->currentTile);
	smallestTile.sub(v);

	this->grid = new Chunk***[this->size.z];
	for (unsigned int k = 0; k < this->size.z; k++) {
		this->grid[k] = new Chunk**[this->size.y];
		for (unsigned int j = 0; j < this->size.y; j++) {
			this->grid[k][j] = new Chunk * [this->size.x];
			for (unsigned int i = 0; i < this->size.x; i++) {
				int x = (smallestTile.x + i) * this->chunkSize;
				int z = (smallestTile.z + k) * this->chunkSize;
				this->settings.genHeightMap(x, z, this->chunkSize, this->chunkSize);//this can be optimized to do it only once per Y (and even keep the generated heightmap in memory for later use)
				this->grid[k][j][i] = new Chunk(
						Math::Vector3(smallestTile.x + i, smallestTile.y + j, smallestTile.z + k),//what happen when we send that to a ref?
						this->chunkSize, this->settings);
			}
		}
	}
}

ChunkGenerator::~ChunkGenerator() {
	for (unsigned int k = 0; k < this->size.z; k++) {
		for (unsigned int j = 0; j < this->size.y; j++) {
			for (unsigned int i = 0; i < this->size.x; i++) {
				delete this->grid[k][j][i];
			}
			delete[] this->grid[k][j];
		}
		delete[] this->grid[k];
	}
	delete[] this->grid;
}

void		ChunkGenerator::initValues(float diff, float& inc, float& start, float& end, float& endShift, float intersection, float size) {
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

bool	ChunkGenerator::updateGrid(Math::Vector3 player_pos) {
	Math::Vector3	old_pos = this->playerPos;
	Math::Vector3	old_tile = this->currentTile;
	this->updatePlayerPos(player_pos);
	Math::Vector3	diff(this->currentTile);
	diff.sub(old_tile);

	if (diff.magnitude()) {
		std::cout << "diff: ";
		diff.printData();

		Math::Vector3	halfGrid(int(this->size.x / 2), int(this->size.y / 2), int(this->size.z / 2));
		Math::Vector3	smallestTile(this->currentTile); smallestTile.sub(halfGrid);
		Math::Vector3	smallestTile_old(old_tile); smallestTile_old.sub(halfGrid);

		//if grid size is even, as we are already on the first tile of the second half, we add `halfGrid - 1` to get the highest tile
		if (int(this->size.z) % 2 == 0)	halfGrid.z--;
		if (int(this->size.y) % 2 == 0)	halfGrid.y--;
		if (int(this->size.x) % 2 == 0)	halfGrid.x--;
		Math::Vector3	highestTile(this->currentTile); highestTile.add(halfGrid);
		Math::Vector3	highestTile_old(old_tile); highestTile_old.add(halfGrid);

		Math::Vector3	intersectionSmallestTile(
			std::max(smallestTile.x, smallestTile_old.x),
			std::max(smallestTile.y, smallestTile_old.y),
			std::max(smallestTile.z, smallestTile_old.z));
		Math::Vector3	intersectionHighestTile(
			std::min(highestTile.x, highestTile_old.x),
			std::min(highestTile.y, highestTile_old.y),
			std::min(highestTile.z, highestTile_old.z));
		Math::Vector3	intersection(intersectionHighestTile);
		intersection.sub(intersectionSmallestTile);//the diff 
		intersection.add(1, 1, 1);//so the dimension is the diff + 1 tile

		std::cout << "size of intersection cube dimension:\n";	intersection.printData();

		//now we define the incrementers, start points and end points to shift the intersection cube and delete unused data
		Math::Vector3	inc, start, end, endShift;
		this->initValues(diff.x, inc.x, start.x, end.x, endShift.x, intersection.x, this->size.x);
		this->initValues(diff.y, inc.y, start.y, end.y, endShift.y, intersection.y, this->size.y);
		this->initValues(diff.z, inc.z, start.z, end.z, endShift.z, intersection.z, this->size.z);

		//todo: generate heightmaps only once, save them, shift them the same way

		Math::Vector3	progress(0, 0, 0);
		Math::Vector3	absdiff(std::abs(diff.x), std::abs(diff.y), std::abs(diff.z));
		for (int k = start.z; k != end.z; k += inc.z) {
			progress.y = 0;
			for (int j = start.y; j != end.y; j += inc.y) {
				progress.x = 0;
				for (int i = start.x; i != end.x; i += inc.x) {
					if (progress.z < intersection.z && progress.y < intersection.y && progress.x < intersection.x) {
						if (progress.z < absdiff.z || progress.y < absdiff.y || progress.x < absdiff.x) {//if we override a chunk with an existing one
							std::cout << "delete " << i << ":" << j << ":" << k << std::endl;
							delete this->grid[k][j][i];
						}
						std::cout << i << ":" << j << ":" << k << " becomes " << i + diff.x << ":" << j + diff.y << ":" << k + diff.z << std::endl;
						this->grid[k][j][i] = this->grid[k + int(diff.z)][j + int(diff.y)][i + int(diff.x)];
						this->grid[k + int(diff.z)][j + int(diff.y)][i + int(diff.x)] = nullptr;//useless but cleaner, it will become another chunk on next pass (generated or shifted)
					} else {
						std::cout << "gen new chunk: " << i << ":" << j << ":" << k << std::endl;
						int x = (smallestTile.x + i) * this->chunkSize;
						int z = (smallestTile.z + k) * this->chunkSize;
						this->settings.genHeightMap(x, z, this->chunkSize, this->chunkSize);//this can be optimized to do it only once per Y (and even keep the generated heightmap in memory for later use)
						this->grid[k][j][i] = new Chunk(
							Math::Vector3(smallestTile.x + i, smallestTile.y + j, smallestTile.z + k),//what happen when we send that to a ref?
							this->chunkSize, this->settings);
					}
					progress.x++;
				}
				progress.y++;
				std::cout << "_ X row\n";
			}
			progress.z++;
			std::cout << "_ Y row\n";
		}
		std::cout << "_ Z row\n";

		std::cout << "\n\n";
		return true;
	}
	return false;
}

void	ChunkGenerator::updatePlayerPos(Math::Vector3 player_pos) {
	this->playerPos = player_pos;
	this->currentTile.x = int(this->playerPos.x / this->chunkSize);
	this->currentTile.y = int(this->playerPos.y / this->chunkSize);
	this->currentTile.z = int(this->playerPos.z / this->chunkSize);
	if (this->playerPos.x < 0) // cauze x=-32..+32 ----> x / 32 = 0; 
		this->currentTile.x--;
	if (this->playerPos.y < 0) // same
		this->currentTile.y--;
	if (this->playerPos.z < 0) // same
		this->currentTile.z--;
}
