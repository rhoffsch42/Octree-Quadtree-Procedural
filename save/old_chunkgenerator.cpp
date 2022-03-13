static int	calcExceed(int posA, int sizeA, int posB, int sizeB);
void	ChunkGenerator::test_calcExceed() {
	/*
			A========.========A		0 19
			   B------.------B		1 15
				B------.------B		2 15
				 B------.------B	3 15
			A========.========A		0 19
			 B------.------B		-1 15
			B------.------B			-2 15
		   B------.------B			-3 15

			A=========.========A	0 20
			   B-------.------B		1 16
				B-------.------B	2 16
				 B-------.------B	3 16
			A=========.========A	0 20
			 B-------.------B		-1 16
			B-------.------B		-2 16
		   B-------.------B			-3 16

			A========.========A		0 19
			  B-------.------B		1 16
			   B-------.------B		2 16
				B-------.------B	3 16
			A========.========A		0 19
			 B-------.------B		0 16
			B-------.------B		-1 16
		   B-------.------B			-2 16

			A=========.========A	0 20
				B------.------B		1 15
				 B------.------B	2 15
				  B------.------B	3 15
			A=========.========A	0 20
			 B------.------B		-2 15
			B------.------B			-3 15
		   B------.------B			-4 15
	*/
	std::cout << calcExceed(0, 19, 1, 15) << std::endl;
	std::cout << calcExceed(0, 19, 2, 15) << std::endl;
	std::cout << calcExceed(0, 19, 3, 15) << std::endl;
	std::cout << calcExceed(0, 19, -1, 15) << std::endl;
	std::cout << calcExceed(0, 19, -2, 15) << std::endl;
	std::cout << calcExceed(0, 19, -3, 15) << std::endl;
	std::cout << "\n";
	std::cout << calcExceed(0, 20, 1, 16) << std::endl;
	std::cout << calcExceed(0, 20, 2, 16) << std::endl;
	std::cout << calcExceed(0, 20, 3, 16) << std::endl;
	std::cout << calcExceed(0, 20, -1, 16) << std::endl;
	std::cout << calcExceed(0, 20, -2, 16) << std::endl;
	std::cout << calcExceed(0, 20, -3, 16) << std::endl;
	std::cout << "\n";
	std::cout << calcExceed(0, 19, 1, 16) << std::endl;
	std::cout << calcExceed(0, 19, 2, 16) << std::endl;
	std::cout << calcExceed(0, 19, 3, 16) << std::endl;
	std::cout << calcExceed(0, 19, 0, 16) << std::endl;
	std::cout << calcExceed(0, 19, -1, 16) << std::endl;
	std::cout << calcExceed(0, 19, -2, 16) << std::endl;
	std::cout << "\n";
	std::cout << calcExceed(0, 20, 1, 15) << std::endl;
	std::cout << calcExceed(0, 20, 2, 15) << std::endl;
	std::cout << calcExceed(0, 20, 3, 15) << std::endl;
	std::cout << calcExceed(0, 20, -2, 15) << std::endl;
	std::cout << calcExceed(0, 20, -3, 15) << std::endl;
	std::cout << calcExceed(0, 20, -4, 15) << std::endl;
}

static int	calcExceed(int posA, int sizeA, int posB, int sizeB) {
	/*
		A========.========A		0 19
			B----.----B			0 11
			B-------.------B	3 16
		A=========.========A	1 20

		sizeB <= sizeA
	*/
	if (sizeB > sizeA) {
		std::cout << "Error: Grid Displayed is larger than the total Grid in memory, or sizes are odd\n";
		exit(4);
	}
	int sign = posA <= posB ? 1 : -1;					// set up the sign to mirror the diff if it is negative to handle only positive part
	int diffpos = sign * (posB - posA);					// mirrored if negative
	int diffsize = sizeB - sizeA;
	if (diffsize % 2 != 0)
		if ((sizeA % 2 == 0 && posA > posB) || (sizeB % 2 == 0 && posB > posA))
			diffsize += -1;								// compensate loss when halving int
	int diffside = std::max(diffsize / 2 + diffpos, 0);	// std::max(diff,0) : if the diff is negative, nothing exceeds the bounds
	return sign * diffside;								// mirrored back if sign was negative
	/*
		.========A
		  .----B		//B does not exceed A
			  .----C    //C exceeds A
	*/
}
static int	exceed1d(int posA, int sizeA, int posB, int sizeB);
void	ChunkGenerator::exceedTest() {
	std::cout << exceed1d(0, 11, 0, 7) << "\n";
	std::cout << exceed1d(0, 11, 4, 7) << "\n";
	std::cout << exceed1d(0, 11, 5, 7) << "\n";
	std::cout << exceed1d(0, 11, -1, 7) << "\n";
}
static int	exceed1d(int posA, int sizeA, int posB, int sizeB) {
	/*
		.=========A		0 11
		.-----B			0 7
		    .-----B		4 7
		     .-----B	5 7
	   .-----B			-1 7
		sizeB <= sizeA
	*/
	if (sizeB > sizeA) {
		std::cout << "Error: Grid Displayed is larger than the total Grid in memory, or sizes are odd\n";
		exit(4);
	}
	int relB = posB - posA;
	if (relB >= 0)
		relB = std::max(0, relB + sizeB - sizeA);
	return relB;
}
static Math::Vector3	exceed(Math::Vector3 posA, Math::Vector3 sizeA, Math::Vector3 posB, Math::Vector3 sizeB) {
	return Math::Vector3(
		exceed1d(posA.x, sizeA.x, posB.x, sizeB.x),
		exceed1d(posA.y, sizeA.y, posB.y, sizeB.y),
		exceed1d(posA.z, sizeA.z, posB.z, sizeB.z));
}

static void		initValues(float diff, float& inc, float& start, float& end, float& endShift, float intersection, float size) {
	if (diff >= 0) {
		inc = 1;
		start = 0;//inclusive, we start here
		end = size;//exclusive cause we will use `!=` for the comparison
	}
	else {
		inc = -1;
		start = size - 1;//inclusive, we start here
		end = 0 - 1;//exclusive cause we will use `!=` for the comparison
	}
	endShift = start + inc * intersection;//exclusive
}

//return true if player step in another chunk
bool	ChunkGenerator::updateGrid_old(Math::Vector3 player_pos) {
	std::unique_lock<std::mutex> chunks_lock(this->chunks_mutex);
	//more direct but more complicated than the new updateGrid()
	Math::Vector3	old_chunk = this->currentChunkWorldIndex;
	this->_updatePlayerPos(player_pos);
	Math::Vector3	diff = this->currentChunkWorldIndex - old_chunk;

	if (diff.len()) {//gridDisplayed
		std::cout << "diff: " << diff << "\n";

		//calculate if we need to move the memory grid and load new chunks
		Math::Vector3	diffGrid;
		diffGrid.x = calcExceed(this->gridIndex.x, this->gridSize.x, this->currentChunkWorldIndex.x, this->gridDisplaySize.x);
		diffGrid.y = calcExceed(this->gridIndex.y, this->gridSize.y, this->currentChunkWorldIndex.y, this->gridDisplaySize.y);
		diffGrid.z = calcExceed(this->gridIndex.z, this->gridSize.z, this->currentChunkWorldIndex.z, this->gridDisplaySize.z);
		this->gridIndex += diffGrid;
		std::cout << "\t--diff             \t" << diff << "\n";
		std::cout << "\t--diffGrid         \t" << diffGrid << "\n";
		std::cout << "\t--gridIndex        \t" << this->gridIndex << "\n";
		std::cout << "\t--currentChunkWorldIndex\t" << this->currentChunkWorldIndex << "\n";

		this->gridDisplayIndex += diff;
		this->gridDisplayIndex -= diffGrid;
		if (diffGrid.len()) {//grid (memory)

			Math::Vector3	halfGrid(int(this->gridSize.x / 2), int(this->gridSize.y / 2), int(this->gridSize.z / 2));//a Vector3i would be better
			//Math::Vector3	smallestChunk = this->currentChunkWorldIndex - halfGrid;
			Math::Vector3	smallestChunk = this->gridIndex - halfGrid;

			Math::Vector3	absdiff(std::abs(diffGrid.x), std::abs(diffGrid.y), std::abs(diffGrid.z));
			Math::Vector3	intersection = this->gridSize - absdiff;

			std::cout << "size of intersection cube dimension:\n" << intersection << "\n";

			//now we define the incrementers, start points and end points to shift the intersection cube and delete unused data
			Math::Vector3	inc, start, end, endShift;
			initValues(diffGrid.x, inc.x, start.x, end.x, endShift.x, intersection.x, this->gridSize.x);
			initValues(diffGrid.y, inc.y, start.y, end.y, endShift.y, intersection.y, this->gridSize.y);
			initValues(diffGrid.z, inc.z, start.z, end.z, endShift.z, intersection.z, this->gridSize.z);

			Math::Vector3	progress(0, 0, 0);
			for (int k = start.z; k != end.z; k += inc.z) {
				progress.y = 0;
				for (int j = start.y; j != end.y; j += inc.y) {
					progress.x = 0;
					for (int i = start.x; i != end.x; i += inc.x) {
						if (progress.z < absdiff.z || progress.y < absdiff.y || progress.x < absdiff.x) {//if we override a chunk with an existing one
							if (CHUNK_GEN_DEBUG) { std::cout << "delete " << i << ":" << j << ":" << k << "\n"; }
							delete this->grid[k][j][i];
							if (progress.y == 0 && (absdiff.x != 0 || absdiff.z != 0)) {//do it only once, for all the Y, only if we go up/down
								delete this->heightMaps[k][i];
								if (CHUNK_GEN_DEBUG) { std::cout << "deleting hmap " << i << ":" << k << "\n"; }
							}
						}
						if (progress.z < intersection.z && progress.y < intersection.y && progress.x < intersection.x) {
							if (CHUNK_GEN_DEBUG) { std::cout << i << ":" << j << ":" << k << " becomes " << i + diffGrid.x << ":" << j + diffGrid.y << ":" << k + diffGrid.z << "\n"; }
							this->grid[k][j][i] = this->grid[k + int(diffGrid.z)][j + int(diffGrid.y)][i + int(diffGrid.x)];
							this->grid[k + int(diffGrid.z)][j + int(diffGrid.y)][i + int(diffGrid.x)] = nullptr;//useless but cleaner, it will become another chunk on next pass (generated or shifted)
							if (progress.y == 0) {//do it only once, for all the Y
								this->heightMaps[k][i] = this->heightMaps[k + int(diffGrid.z)][i + int(diffGrid.x)];
								//this->heightMaps[k + int(diff.z)][i + int(diff.x)] = nullptr;//dont do that, if diff.z and .x are 0, it leeks the previously created heightmap
							}
						}
						else {
							if (CHUNK_GEN_DEBUG) { std::cout << "gen new chunk: " << i << ":" << j << ":" << k << "\n"; }
							if (progress.y == 0) {//do it only once, for all the Y
								this->heightMaps[k][i] = new HeightMap(this->settings,
									(smallestChunk.x + i) * this->chunkSize.x, (smallestChunk.z + k) * this->chunkSize.z,
									this->chunkSize.x, this->chunkSize.z);
							}
							this->grid[k][j][i] = new Chunk(
								Math::Vector3(smallestChunk.x + i, smallestChunk.y + j, smallestChunk.z + k),//what happen when we send that to a ref?
								this->chunkSize, this->settings, this->heightMaps[k][i]);
						}
						progress.x++;
					}
					progress.y++;
				}
				progress.z++;
			}

			this->_chunksReadyForMeshes = true;
			chunks_lock.unlock();
		}//end grid memory
		this->playerChangedChunk = true;
		return true;
	}//end gridDisplayed
	return false;
}

void	ChunkGenerator::th_builders_old(GLFWwindow* context) {
	std::cout << __PRETTY_FUNCTION__ << "\n";
	glfwMakeContextCurrent(context);
	Glfw::initDefaultState();
	std::thread::id threadID = std::this_thread::get_id();
	std::stringstream ss;
	ss << threadID;
	std::string	threadIDstr = "[" + ss.str() + "] ";
	glfwSetWindowTitle(context, threadIDstr.c_str());
	Chunk* fakeChunk = reinterpret_cast<Chunk*>(2);
	HeightMap* fakeHeightmap = reinterpret_cast<HeightMap*>(3);
	std::unique_lock<std::mutex> chunks_lock(this->chunks_mutex);

	//build job variables
	PerlinSettings	perlinSettings(this->settings);//if they change later, we have to update them, cpy them when finding a job?
	Math::Vector3	halfGrid = Math::Vector3(int(this->gridSize.x / 2), int(this->gridSize.y / 2), int(this->gridSize.z / 2)); // same
	Math::Vector3	chunk_size(this->chunkSize);//same
	Chunk* newChunk = nullptr;
	HeightMap* newHmap = nullptr;

	while (!this->terminateBuilders.try_lock()) {
		std::cout << "[" << threadID << "] waiting \n";
		glfwSetWindowTitle(context, (threadIDstr + "waiting").c_str());
		this->cv.wait(chunks_lock, [this] { return !this->_chunksReadyForMeshes; });//unlock and wait
		if (this->terminateBuilders.try_lock()) {
			break;
		}
		for (size_t z = 0; z < this->gridSize.z; z++) {
			for (size_t x = 0; x < this->gridSize.x; x++) {
				if (!this->heightMaps[z][x]) {
					this->heightMaps[z][x] = fakeHeightmap;//reserve the slot: assign whatever to the pointer so it's not nullptr anymore
					chunks_lock.unlock(); // now we can build the data
					Math::Vector3	chunkWorldIndex(
						this->gridIndex.x - halfGrid.x + x,
						0,//useless here
						this->gridIndex.z - halfGrid.z + z);
					newHmap = new HeightMap(perlinSettings,
						chunkWorldIndex.x * chunk_size.x, chunkWorldIndex.z * chunk_size.z,
						chunk_size.x, chunk_size.z);

					std::cout << "[" << threadID << "] gen HeightMap: [" << z << "][" << x << "]";
					std::cout << newHmap->map << " ";
					std::cout << (chunkWorldIndex.x * chunk_size.x) << ":" << (chunkWorldIndex.z * chunk_size.z) << " ";
					std::cout << chunk_size.x << "x" << chunk_size.z << " ";
					std::cout << "\n";

					newHmap->glth_buildPanel();
					chunks_lock.lock(); //to assign the new data
					//std::cout << "[thread " << threadID << "] built HeightMap " << newHmap << "\n";
					this->heightMaps[z][x] = newHmap;
					newHmap = nullptr;
				}
			}
		}
		this->threadsReadyToBuildChunks++;
		std::cout << "[" << threadID << "] threadsReadyToBuildChunks : " << (int)threadsReadyToBuildChunks << "\n";
		glfwSetWindowTitle(context, (threadIDstr + "thread ready to build Chunks").c_str());
		if (this->threadsReadyToBuildChunks != this->builderAmount) {
			chunks_lock.unlock();
			this->cv.notify_all();//wakes up other threads
			glfwSetWindowTitle(context, (threadIDstr + "thread ready to build Chunks, waiting other threads").c_str());
			this->cv.wait(chunks_lock, [this] { return (this->threadsReadyToBuildChunks == this->builderAmount); });//unlock and wait
		}
		chunks_lock.unlock();
		this->cv.notify_all();//wakes up other threads
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
		chunks_lock.lock();

		std::cout << "[" << threadID << "] building chunks... \n";
		glfwSetWindowTitle(context, (threadIDstr + "building chunks...").c_str());
		for (size_t z = 0; z < this->gridSize.z; z++) {
			for (size_t y = 0; y < this->gridSize.y; y++) {
				for (size_t x = 0; x < this->gridSize.x; x++) {
					if (!this->grid[z][y][x]) {
						this->grid[z][y][x] = fakeChunk;//reserve the slot: assign whatever to the pointer so it's not nullptr anymore
						newHmap = this->heightMaps[z][x];
						Math::Vector3	chunkWorldIndex(
							this->gridIndex.x - halfGrid.x + x,
							this->gridIndex.y - halfGrid.y + y,
							this->gridIndex.z - halfGrid.z + z);
						chunks_lock.unlock();//now we can build a real chunk and heightmap
						newChunk = new Chunk(chunkWorldIndex, chunk_size, perlinSettings, newHmap);
						newChunk->buildMesh();
						chunks_lock.lock(); //to assign the new data
						this->grid[z][y][x] = newChunk;
						std::cout << "[" << threadID << "] built chunk " << z << " " << y << " " << x << " " << this->grid[z][y][x];
						if (newChunk->meshBP)
							std::cout << " meshBP " << newChunk->meshBP << " vao " << newChunk->meshBP->getVao();
						std::cout << "\n";

						newChunk = nullptr;
						newHmap = nullptr;
					}
				}
			}
		}
		//this->playerChangedChunk = false;
		this->_chunksReadyForMeshes = true;
		this->threadsReadyToBuildChunks = 0;
		std::cout << "[" << threadID << "] nomore chunks to build, _chunksReadyForMeshes true\n";
		glfwSetWindowTitle(context, (threadIDstr + "nomore chunks to build, _chunksReadyForMeshes true").c_str());
		chunks_lock.unlock();
	}
	std::cout << "[" << threadID << "] exiting...\n";
	glfwSetWindowShouldClose(context, GLFW_TRUE);
	glfwMakeContextCurrent(nullptr);
	this->terminateBuilders.unlock();
}