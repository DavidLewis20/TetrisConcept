// Fill out your copyright notice in the Description page of Project Settings.


#include "TetrisBlock.h"
#include "SpawnedBlock.h"
#include "BlueprintFunctionality.h"

// Sets default values
ATetrisBlock::ATetrisBlock()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	AutoPossessPlayer = EAutoReceiveInput::Player0;

	//initalises text elements
	ScoreText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("ScoreText"));
	LevelText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("LevelText"));

	//initialise overflow height, if not set in inspector
	overflowHeight = 1.f;
}

// Called when the game starts or when spawned
void ATetrisBlock::BeginPlay()
{
	Super::BeginPlay();

	//initialisation of important variables
	score = 0;

	level = 1;

	canRotate = true;

	bSoftDrop = false;

	bTBlock = false;

	recentlyRotated = false;
	tSpin = false;
	largeOffset = false;

	scoreMultiplier = 1.f;

	//gets a reference to the camera actor in the scene
	for (TObjectIterator<ACameraActor> act; act; ++act) {
		FString actorName = act->GetName();
		if (act->GetWorld() != this->GetWorld()) {
			continue;
		}

		if (actorName.Contains("MainCamera")) {
			ACameraActor* cameraActor = (ACameraActor*)*act;
			mainCamera = cameraActor->GetCameraComponent();
			break;
		}
	}

	//attach score text to main camera
	ScoreText->SetupAttachment(mainCamera);

	//set up position, rotation, scale and colour of score text
	ScoreText->SetRelativeLocation(FVector(-2380.f, -900.f, 1520.f));
	ScoreText->SetRelativeRotation(FRotator(0.f, 180.f, 0.f));
	ScoreText->SetRelativeScale3D(FVector(0.5f, 0.5f, 0.5f));
	ScoreText->SetTextRenderColor(FColor::Green);

	//set up position, rotation, scale and colour of level text
	LevelText->SetRelativeLocation(FVector(0.f, -650.f, 0.f));
	LevelText->SetRelativeRotation(FRotator(0.f, 0.f, 0.f));
	LevelText->SetRelativeScale3D(FVector(1.f, 1.f, 1.f));
	LevelText->SetTextRenderColor(FColor::Green);

	//intialise score text to current score (which should be 0)
	ScoreText->SetText(FText::FromString("Score = " + FString::FromInt(score)));

	//initialise level text to starting level of 1
	LevelText->SetText(FText::FromString("Level = " + FString::FromInt(level)));

	//get the blueprint functionality class in the game world
	GetBlueprintFunctionality();

	//set game over to false
	blueprintFunctionality->bGameOver = false;

	//set initial gravity based on tetris algorithm: gravity = (0.8 - (level - 1))^level - 1
	dropSpeed = FMath::Pow(0.8f -(((float)level - 1.f) * 0.007), (float)level - 1.f);

	//initialise input timer so player can move tetromino sideways
	inputTimer = 0.1f;

	//fill colour pool will all block colours
	colourPool = blockColours;

	//get next 3 tetromino colours to spawn
	for (int i = 0; i < 3; ++i) {
		GetNextColourIndex();
	}
	
	//spawn the first tetromino
	SpawnTetromino();
}

void ATetrisBlock::GetNextColourIndex()
{
	int nextIndex = FMath::RandRange(0, colourPool.Num() - 1);
	nextColours.Add(colourPool[nextIndex]);

	int nextBlockIndex = blockColours.Find(colourPool[nextIndex]);
	blueprintFunctionality->nextTetrominoNumbers.Add(nextBlockIndex);
	colourPool.RemoveAt(nextIndex);
}

// Called every frame
void ATetrisBlock::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//if game over, exit as block should no longer be functional
	if (blueprintFunctionality->bGameOver) {
		return;
	}

	//increase timers by DeltaTime
	dropTimer += DeltaTime;
	inputTimer += DeltaTime;

	//if 10 lines have been cleared, increase the level (and gravity)
	if (linesCleared >= 10) {
		linesCleared = 0;
		UpdateLevel();
	}

	//if the base of the tetromino isn't touching the ground
	if (spawnedBlocks[lowestBlockIndex]->GetActorLocation().Z > groundLevel) {
		//and the tetromino can move down 1 tetris unit
		if (dropTimer > dropSpeed) {
			//reset drop timer and set Z velocity to 1 tetris unit (100 Unreal units)
			dropTimer = 0.f;
			CurrentVelocity.Z = dropDist;
		}
		else
		{
			//otherwise, set Z velocity to 0
			CurrentVelocity.Z = 0.f;
		}
	}
	else {
		//otherwise, if landed for longer than lock delay (set at 0.5 seconds)
		if (dropTimer > 0.5f) {
			//lock the tetromino and spawn a new tetromino. Also, ensure gravity is reset to standard speed if soft dropped
			RegisterAndCheckBlocks();
			SlowDownDrop();
			SpawnTetromino();
			return;
		}
		else {
			//otherwise, set Z velocity to 0 and allow horizontal movement during lock delay
			CurrentVelocity.Z = 0.f;
		}
	}

	//if block has moved (horizontally or vertically)
	if (!CurrentVelocity.IsZero()) {
		//get leftmost and rightmost positions of new location
		FVector leftmostNewLoc = spawnedBlocks[leftmostBlockIndex]->GetActorLocation() + (CurrentVelocity);
		FVector rightmostNewLoc = spawnedBlocks[rightmostBlockIndex]->GetActorLocation() + (CurrentVelocity);

		FVector NewLocations[4];

		//get the new location of each tetromino block
		for (int i = 0; i < 4; ++i) {
			NewLocations[i] = spawnedBlocks[i]->GetActorLocation() + (CurrentVelocity);
		}

		//requires a new check as otherwise other blocks in tetromino may not move down and get registered if a line is cleared
		for (int i = 0; i < 4; ++i) {
			//if one of the new block positions after movement is the same as an already landed block
			if (landedBlockPos.Contains(NewLocations[i])) {
				//ignore if Z velocity is 0
				if (CurrentVelocity.Z == 0.f) {
					return;
				}

				//else, check for a game over
				if (spawnedBlocks[i]->GetActorLocation().Z > overflowHeight) {
					blueprintFunctionality->bGameOver = true;
					return;
				}
				
				//otherwise, land the blocks, reset the gravity to non soft drop speed and spawn a new tetromino
				RegisterAndCheckBlocks();
				SlowDownDrop();
				SpawnTetromino();
				return;
			}
		}

		//gets all blocks again. Seperate functionality so new loop is required
		for(int i = 0; i < 4; ++i){
			//if new horizontal position is within the level boundaries then move tetromino
			if (leftmostNewLoc.Y >= leftBoundary && rightmostNewLoc.Y <= rightBoundary) {
				spawnedBlocks[i]->MoveBlock(NewLocations[i]);

			}
			else {
				//otherwise, block horizontal movement
				CurrentVelocity.Y = 0.f;
				//then get the new location based on the changed speed and move the block
				NewLocations[i] = spawnedBlocks[i]->GetActorLocation() + CurrentVelocity;
				spawnedBlocks[i]->MoveBlock(NewLocations[i]);
			}
		}

		//sets recently rotated to false as last move was a drop/sideways movement
		recentlyRotated = false;

		//resets t spin bools if it was a T block
		if (bTBlock) {
			miniTSpin = false;
			tSpin = false;
		}

		//if movement was soft dropped, increase score by 1
		if (bSoftDrop) {
			UpdateScore(1);
		}
	}
}

void ATetrisBlock::RegisterAndCheckBlocks()
{
	//initialise relevant variables
	bool blocksRegistered = false;
	rowsClearedInMove = 0;

	//get all blocks in current tetromino
	for (int j = 0; j < 4; ++j) {
		//if not registered
		if (!blocksRegistered) {
			//add the current position of the block to landed block positions
			landedBlockPos.Add(spawnedBlocks[j]->GetActorLocation());

			//resets array so it can check the row of each landed block.
			if (j >= 3) {
				blocksRegistered = true;
				j = -1;
			}

			continue;
		}

		//checks row of each block in the tetromino for line clears
		CheckRow(spawnedBlocks[j]);
	}

	//if no lines were cleared
	if (rowsClearedInMove < 1) {
		//but it is a T tetromino
		if (bTBlock) {
			//if a mini T spin was performed, increase score by 100 * level
			if (miniTSpin) {
				UpdateScore(100 * level);
			}
			else if (tSpin) {
				//otherwise, if a T spin was performed, increase score by 400 * level
				UpdateScore(400 * level);
			}
		}
		//exit as scoring as been added
		return;
	}

	//the calculated score added based on the move performed
	float scoreIncrease = 0.f;

	//get the rows cleared in the move
	switch (rowsClearedInMove) {
	case 1:
		if (bTBlock) {
			//if 1 row was cleared and a mini T spin was performed
			if (miniTSpin) {
				//if another difficult move was performed before this move, set multiplier to 1.5
				if (difficultMovePerformed) {
					scoreMultiplier = 1.5f;
				}
				
				//get score increase based on base value of 200, multiplied by the current level and by the score multiplier and update the score
				scoreIncrease = 200.f * (float)level * scoreMultiplier;
				UpdateScore((int)scoreIncrease);
				//set difficult move performed to true, making player eligable for multiplier if they before another one next
				difficultMovePerformed = true;
				break;
			}
			//otherwise, if 1 row was cleared and a T spin was performed
			if (tSpin) {
				//increase multiplier if performed after another difficult move
				if (difficultMovePerformed) {
					scoreMultiplier = 1.5f;
				}

				//increase score by base value of 800, multiplied by current level and score multiplier
				scoreIncrease = 800.f * (float)level * scoreMultiplier;
				UpdateScore((int)scoreIncrease);
				//make player eligable for multiplier if they perform another difficult move next
				difficultMovePerformed = true;
				break;
			}
		}

		//otherwise, break the difficult move streak and reset multiplier
		difficultMovePerformed = false;
		scoreMultiplier = 1.f;
		//and update score by a base value of 100 multiplied by level
		UpdateScore(100 * level);
		break;
	case 2:
		if (bTBlock) {
			//if 2 rows were cleared and a mini T spin was performed
			if (miniTSpin) {
				//increase multiplier if back-to-back with another difficult move
				if (difficultMovePerformed) {
					scoreMultiplier = 1.5f;
				}

				//increase score by base value of 400, multiplied by level and score multiplier
				scoreIncrease = 400.f * (float)level * scoreMultiplier;
				UpdateScore((int)scoreIncrease);

				//make player eligable for multiplier if next move is difficult
				difficultMovePerformed = true;
				break;
			}
			//otherwise, if 2 rows were cleared and a T spin was performed
			if (tSpin) {
				//if back-to-back with another difficult move, increase multiplier
				if (difficultMovePerformed) {
					scoreMultiplier = 1.5f;
				}
				
				//increase score by base value of 1200 multiplied by level and score multiplier
				scoreIncrease = 1200.f * (float)level * scoreMultiplier;
				UpdateScore((int)scoreIncrease);

				//make player eligable for multiplier if they perform another difficult move next
				difficultMovePerformed = true;
				break;
			}
		}

		//otherwise, 2 rows were cleared so break difficult move streak and reset multiplier
		difficultMovePerformed = false;
		scoreMultiplier = 1.f;
		//then increase score by base value of 300 multiplied by level
		UpdateScore(300 * level);
		break;
	case 3:
		if (bTBlock) {
			//if 3 lines were cleared and a T spin was performed
			if (tSpin) {
				//if last move was also difficult then increase multiplier
				if (difficultMovePerformed) {
					scoreMultiplier = 1.5f;
				}

				//increase score by base value of 1600 multiplied by level and score multiplier
				scoreIncrease = 1600.f * (float)level * scoreMultiplier;
				UpdateScore((int)scoreIncrease);
				//make player eligable for back-to-back bonus if another difficult move is performed next
				difficultMovePerformed = true;
				break;
			}
		}

		//else, 3 lines were cleared so break difficult move streak and reset multiplier
		difficultMovePerformed = false;
		scoreMultiplier = 1.f;
		//then increase score by base value of 500 multiplied by the current level
		UpdateScore(500 * level);
		break;
	case 4:
		//if 4 lines were cleared (i.e., a tetris), if previous move was difficult, increase multiplier to 1.5
		if (difficultMovePerformed) {
			scoreMultiplier = 1.5f;
		}

		//then increase score by base value of 800, multiplied by level and score multiplier
		scoreIncrease = 800.f * (float)level * scoreMultiplier;
		UpdateScore(scoreIncrease);

		//and make player eligable for back-to-back multiplier if next move is difficult
		difficultMovePerformed = true;
		break;
	default:
		break;
	}
}

// Called to bind functionality to input
void ATetrisBlock::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	//moves left when left key is pressed and right when right key is pressed
	InputComponent->BindAxis("MoveHorizontally", this, &ATetrisBlock::MoveHorizontally);

	//soft drops when down key is pressed
	InputComponent->BindAction("IncreaseGravity", IE_Pressed, this, &ATetrisBlock::SpeedUpDrop);

	//stops soft drop when down key is released
	InputComponent->BindAction("IncreaseGravity", IE_Released, this, &ATetrisBlock::SlowDownDrop);

	//rotates clockwise when up key is pressed
	InputComponent->BindAction("Rotate90Clockwise", IE_Pressed, this, &ATetrisBlock::RotateClockwise);

	//rotates anti-clockwise when Z or left control key is pressed
	InputComponent->BindAction("Rotate90AntiClockwise", IE_Pressed, this, &ATetrisBlock::RotateAntiClockwise);

	//hard drops when space bar is pressed
	InputComponent->BindAction("HardDrop", IE_Pressed, this, &ATetrisBlock::HardDrop);
}

void ATetrisBlock::MoveHorizontally(float axisValue) {
	//input only accepted every 0.1 seconds
	if (inputTimer >= 0.1f) {
		//if accepted, then set Y value of velocity to 100 or -100 based on direction
		CurrentVelocity.Y = FMath::Clamp(axisValue, -1.f, 1.f) * 100.f;
		inputTimer = 0.f;
	}
	else {
		//otherwise, set Y velocity to 0
		CurrentVelocity.Y = 0.f;
	}
}

void ATetrisBlock::SpawnTetromino() {
	//if colour pool is empty then reset it
	if (colourPool.Num() < 1) {
		colourPool = blockColours;
	}

	FVector blockPositions[4];
	//set position of centermost block to 5 tetris units from left boundary and 1 tetris unit above playfield
	blockPositions[0] = FVector(xSpawnPoint, leftBoundary + 500.0f, zSpawnPoint);
	//initialise variables
	rotationPos = 0;
	isIBlock = false;
	bTBlock = false;
	miniTSpin = false;
	tSpin = false;

	//get the next block colour
	UMaterial* blockColour = nextColours[0];

	//get the index of the colour from block colours
	int blockColourIndex = blockColours.Find(nextColours[0]);

	//remove the next colour index as it will be spawned from both index arrays
	nextColours.RemoveAt(0);
	blueprintFunctionality->nextTetrominoNumbers.RemoveAt(0);

	//get the colour 3rd in the spawn queue
	GetNextColourIndex();

	//set ui updated to false so next tetromino queue can be updated
	blueprintFunctionality->uiUpdated = false;

	//get the index of the block from blockColours
	switch (blockColourIndex) {
	case 0:
		//if 0, tetromino is blue so spawn a J tetromino
		blockPositions[1] = blockPositions[0] + FVector(0.f, -100.f, 0.f);
		blockPositions[2] = blockPositions[0] + FVector(0.f, -100.f, 100.f);
		blockPositions[3] = blockPositions[0] + FVector(0.f, 100.f, 0.f);

		//get leftmost, rightmost indexes and origin for rotations
		leftmostBlockIndex = 1;
		rightmostBlockIndex = 3;
		tetriminoOriginOffset = FVector(0.f, 0.f, 0.f);
		break;
	case 1:
		//if 1, tetromino is green so spawn an S tetromino
		blockPositions[1] = blockPositions[0] + FVector(0.f, -100.f, 0.f);
		blockPositions[2] = blockPositions[0] + FVector(0.f, 0.f, 100.f);
		blockPositions[3] = blockPositions[0] + FVector(0.f, 100.f, 100.f);
		leftmostBlockIndex = 1;
		rightmostBlockIndex = 3;
		tetriminoOriginOffset = FVector(0.f, 0.f, 0.f);
		break;
	case 2:
		//if 2, tetromino is red so spawn a Z tetromino
		blockPositions[1] = blockPositions[0] + FVector(0.f, 100.f, 0.f);
		blockPositions[2] = blockPositions[0] + FVector(0.f, 0.f, 100.f);
		blockPositions[3] = blockPositions[0] + FVector(0.f, -100.f, 100.f);
		leftmostBlockIndex = 3;
		rightmostBlockIndex = 1;
		tetriminoOriginOffset = FVector(0.f, 0.f, 0.f);
		break;
	case 3:
		//if 3, tetromino is yellow so spawn an O tetromino
		blockPositions[1] = blockPositions[0] + FVector(0.f, 0.f, 100.f);
		blockPositions[2] = blockPositions[0] + FVector(0.f, 100.f, 100.f);
		blockPositions[3] = blockPositions[0] + FVector(0.f, 100.f, 0.f);
		leftmostBlockIndex = 0;
		rightmostBlockIndex = 2;
		tetriminoOriginOffset = FVector(0.f, 50.f, 50.f);
		break;
	case 4:
		//if 4, tetromino is light blue so spawn an I tetromino
		blockPositions[1] = blockPositions[0] + FVector(0.f, -100.f, 0.f);
		blockPositions[2] = blockPositions[0] + FVector(0.f, 100.f, 0.f);
		blockPositions[3] = blockPositions[0] + FVector(0.f, 200.f, 0.f);
		leftmostBlockIndex = 1;
		rightmostBlockIndex = 3;
		tetriminoOriginOffset = FVector(0.f, 50.f, -50.f);
		//set to true so class will check for I wall kicks rather than standard wall kicks
		isIBlock = true;
		break;
	case 5:
		//if 5, tetromino is orange so spawn a L tetromino
		blockPositions[1] = blockPositions[0] + FVector(0.f, -100.f, 0.f);
		blockPositions[2] = blockPositions[0] + FVector(0.f, 100.f, 0.f);
		blockPositions[3] = blockPositions[0] + FVector(0.f, 100.f, 100.f);
		leftmostBlockIndex = 1;
		rightmostBlockIndex = 2;
		tetriminoOriginOffset = FVector(0.f, 0.f, 0.f);
		break;
	case 6:
		//if 6, tetromino is magenta so spawn a T tetromino
		blockPositions[1] = blockPositions[0] + FVector(0.f, -100.f, 0.f);
		blockPositions[2] = blockPositions[0] + FVector(0.f, 0.f, 100.f);
		blockPositions[3] = blockPositions[0] + FVector(0.f, 100.f, 0.f);
		leftmostBlockIndex = 1;
		rightmostBlockIndex = 3;
		tetriminoOriginOffset = FVector(0.f, 0.f, 0.f);
		//set to true so class will check for T spins
		bTBlock = true;
		break;
	default:
		break;
	}
	//set base block to 0 as center block is always at base from initial rotation
	lowestBlockIndex = 0;

	//spawn 4 blocks at the set positions based on the randomise colour
	for (int i = 0; i < 4; ++i) {
		SpawnBlock(blockPositions[i], blockColour, i);
	}
}

void ATetrisBlock::SpawnBlock(FVector position, UMaterial* blockColour, int blockIndex) {
	//spawns a block and adds it to current blocks array based on the current index
	spawnedBlocks[blockIndex] = (ASpawnedBlock*)GWorld->SpawnActor(ASpawnedBlock::StaticClass());
	//add to all blocks array
	allBlocks.Add(spawnedBlocks[blockIndex]);

	//set position and colour based on parameters passed through
	spawnedBlocks[blockIndex]->SetActorLocation(position);
	spawnedBlocks[blockIndex]->SetColour(blockColour);
}

void ATetrisBlock::CheckRow(ASpawnedBlock* currentBlock) {
	int blocksOnRow = 0;
	TArray<ASpawnedBlock*> blocksToDestroy;
	TArray<ASpawnedBlock*> blocksToMoveDown;

	//gets every block currently in game
	for (int i = 0; i < allBlocks.Num(); ++i) {
		//if the Z position of the current block is the same as the current array block
		if (allBlocks[i]->GetActorLocation().Z == currentBlock->GetActorLocation().Z) {
			//increment row by 1 and add to blocks to destroy
			blocksOnRow++;
			blocksToDestroy.Add(allBlocks[i]);
		}
		else if (allBlocks[i]->GetActorLocation().Z > currentBlock->GetActorLocation().Z) {
			//if array block is above current blocks location, add to move down array
			blocksToMoveDown.Add(allBlocks[i]);
		}
	}

	//if 10 blocks are found in the row
	if (blocksOnRow >= 10) {
		//destroy all blocks in the row and move the above blocks down by 1
		RemoveBlocks(blocksToDestroy);
		ShiftBlocksDown(blocksToMoveDown);
		//and increment rows cleared in move
		rowsClearedInMove++;
	}
}

void ATetrisBlock::RemoveBlocks(TArray<ASpawnedBlock*> blocks) {
	//destroy all blocks passed through and remove them from respective arrays
	for (int i = 0; i < blocks.Num(); ++i) {
		allBlocks.Remove(blocks[i]);
		landedBlockPos.Remove(blocks[i]->GetActorLocation());
		blocks[i]->Destroy();
	}

	//increment lines cleared
	linesCleared++;
}

void ATetrisBlock::ShiftBlocksDown(TArray<ASpawnedBlock*> blocks) {
	//shift all blocks passed through down by 1 tetris unit and update location in respective arrays
	for (int i = 0; i < blocks.Num(); ++i) {
		int indexToChange = landedBlockPos.Find(blocks[i]->GetActorLocation());
		FVector NewLoc = blocks[i]->GetActorLocation() + FVector(0.f, 0.f, dropDist);
		blocks[i]->SetActorLocation(NewLoc);
		
		landedBlockPos[indexToChange] = blocks[i]->GetActorLocation();
		
	}
}

void ATetrisBlock::SpeedUpDrop() {
	//increase gravity to 0.01 (i.e., very fast drop) and set bool to true so score is increased
	dropSpeed = 0.01f;
	bSoftDrop = true;
}

void ATetrisBlock::SlowDownDrop() {
	//reset drop speed to current speed based on algorithm
	dropSpeed = FMath::Pow(0.8f - (((float)level - 1.f) * 0.007), (float)level - 1.f);
	bSoftDrop = false;
}

void ATetrisBlock::HardDrop() {
	//if not game over
	if (!blueprintFunctionality->bGameOver) {
		//calculate the lowest Z position that the tetromino can land at
		float lowestZPosition = GetLowestZPosition();

		//calculate the distance to drop based on the lowest Z position
		float distanceToDrop = lowestZPosition - spawnedBlocks[ZCompareIndex]->GetActorLocation().Z;

		//move blocks to landed position
		for (int i = 0; i < 4; ++i) {
			FVector NewLoc = spawnedBlocks[i]->GetActorLocation() + FVector(0.f, 0.f, distanceToDrop);
			spawnedBlocks[i]->SetActorLocation(NewLoc);
		}

		//check blocks for line clears
		RegisterAndCheckBlocks();

		//increase score based on tetris units moved multipled by 2
		UpdateScore(2 * (int)(-distanceToDrop / 100.f));
		
		//spawn a new tetromino
		SpawnTetromino();
	}
}

float ATetrisBlock::GetLowestZPosition() {
	FVector newLocation;

	//set to ground level initially
	float lowestZPosition = groundLevel;
	//the index of the tetromino block that is being compared, set to the lowest block initially
	ZCompareIndex = lowestBlockIndex;

	for (int i = 0; i < 4; ++i) {
		//start lowest position at overflow height
		float tempZPos = overflowHeight;

		//then get the Z compare index
		//if i = 0 and Y position is same as ZCompareIndex block, no check is needed as the lowest position was already found on this column
		if (i != 0 && spawnedBlocks[ZCompareIndex]->GetActorLocation().Y == spawnedBlocks[i]->GetActorLocation().Y) {
			//but if the Z position of the current block is lower than the block at ZCompareIndex, change Z compare index to this new position
			if (spawnedBlocks[i]->GetActorLocation().Z < spawnedBlocks[ZCompareIndex]->GetActorLocation().Z) {
				ZCompareIndex = i;
			}
			continue;
		}
		while (tempZPos >= groundLevel) {
			//increment lowest position downwards until the ground level is reached
			tempZPos -= 100.f;

			//but stop if a block is found on this Y position
			if (landedBlockPos.Contains(FVector(spawnedBlocks[i]->GetActorLocation().X, spawnedBlocks[i]->GetActorLocation().Y, tempZPos))) {
				break;
			}
		}

		//then, if the lowest Z position is lower than the lowest position found for the current block (+100 (1 tetris unit) to account for being above the block/ground)
		if (lowestZPosition < tempZPos + 100.f) {
			//update lowest Z position and Z compare index
			lowestZPosition = tempZPos + 100.f;
			ZCompareIndex = i;
		}
	}
	//return the new Z position of the lowest block in the tetromino
	return lowestZPosition;
}

void ATetrisBlock::RotateAntiClockwise() {
	//if game over, disable controls by returning
	if (blueprintFunctionality->bGameOver) {
		return;
	}
	//set points very high/low to make it easy to change them when comparing
	float lowestYPoint = 1e+10;
	float highestYPoint = -1e+10;
	float lowestZpoint = 1e+10;
	TArray<FVector> newPositions;
	//initialise bools to false
	bool shouldWallKick = false;
	tSpin = false;
	miniTSpin = false;
	//calculate origin of rotation based on the first block and its offset to the center (usually 0 with exception of I and O blocks)
	tetriminoOrigin = spawnedBlocks[0]->GetActorLocation() + tetriminoOriginOffset;
	//allow rotations for now
	canRotate = true;
	for (int i = 0; i < 4; ++i) {
		//code adapted from McGuire (2017)
		//get offset from rotation origin of each block
		FVector originOffset = spawnedBlocks[i]->GetActorLocation() - tetriminoOrigin;
		//get position that block will rotate to, relative to x axis
		FVector rotatedPos = originOffset.RotateAngleAxis(90.f, FVector(1.f, 0.f, 0.f));
		//get final position of block, relative to the rotation origin of the tetrimino
		FVector newPosition = rotatedPos + tetriminoOrigin;
		//round new positions to nearest int, as rotation often offsets block positions slightly, creating collision issues
		FMath::RoundToInt(newPosition.Y);
		FMath::RoundToInt(newPosition.Z);

		//if the rotation would result in the block clipping through walls or blocks then wall kick
		if (landedBlockPos.Contains(newPosition) || newPosition.Y < leftBoundary || newPosition.Y > rightBoundary || newPosition.Z < groundLevel) {
			shouldWallKick = true;
		}

		//get the rotated position of the block
		newPositions.Add(newPosition);
	}

	//if wall kick = true
	if (shouldWallKick) {
		switch(rotationPos){
		case 0:
			//if 0, block was rotated from 0 (initial rotation), so wall kick based on 0 anti-clockwise offsets. If I block, wall kick based on respective I offsets
			if (isIBlock) {
				WallKick(newPositions, iAntiClockwise0WallKickOffsets);
				break;
			}

			WallKick(newPositions, antiClockwise0WallKickOffsets);
			break;
		case 1:
			//if 1, block was rotated from R (clockwise position), so wall kick based on R anti-clockwise offsets. If I block, wall kick based on respective I offsets
			if (isIBlock) {
				WallKick(newPositions, iAntiClockwiseRWallKickOffsets);
				break;
			}

			WallKick(newPositions, antiClockwiseRWallKickOffsets);
			break;
		case 2:
			if (isIBlock) {
				//if 2, block was rotated from 2 (opposite rotation), so wall kick based on 2 anti-clockwise offsets. If I block, wall kick based on respective I offsets
				WallKick(newPositions, iAntiClockwise2WallKickOffsets);
				break;
			}

			WallKick(newPositions, antiClockwise2WallKickOffsets);
			break;
		case 3:
			//if 1, block was rotated from L (anti-clockwise position), so wall kick based on L anti-clockwise offsets. If I block, wall kick based on respective I offsets
			if (isIBlock) {
				WallKick(newPositions, iAntiClockwiseLWallKickOffsets);
				break;
			}

			WallKick(newPositions, antiClockwiseLWallKickOffsets);
			break;
		default:
			break;
		}
	}

	//if canRotate bool is still true
	if (canRotate) {
		//then find the leftmost, rightmost and lowest blocks in the tetromino after the rotation
		for (int i = 0; i < 4; ++i) {
			if (newPositions[i].Y < lowestYPoint) {
				lowestYPoint = newPositions[i].Y;
				leftmostBlockIndex = i;
			}
			if (newPositions[i].Y > highestYPoint) {
				highestYPoint = newPositions[i].Y;
				rightmostBlockIndex = i;
			}
			if (newPositions[i].Z < lowestZpoint) {
				lowestZpoint = newPositions[i].Z;
				lowestBlockIndex = i;
			}
			//and set the new location of the block
			spawnedBlocks[i]->SetActorLocation(newPositions[i]);
			//end of adapted code
		}

		//change origin offset based on new position of tetromino (only changes for I and O tetrominoes)
		tetriminoOriginOffset = tetriminoOrigin - spawnedBlocks[0]->GetActorLocation();

		//update the rotated position. As anti-clockwise rotation, increment downwards
		rotationPos--;

		//but reset to 3 if it drops below 0
		if (rotationPos < 0) {
			rotationPos = 3;
		}

		//check for a t spin if a T tetromino
		if (bTBlock) {
			CheckForTSpin();
		}

		//set bool to true in case of T spin/mini T spin
		recentlyRotated = true;
	}
}

void ATetrisBlock::WallKick(TArray<FVector>& newPositions, FVector wallKickOffsets[4]) {
	TArray<FVector> tempNewPositions;
	bool validPosition = true;
	FVector tempOrigin; 
	//for each new position of the tetromino blocks
	for (int i = 0; i < 4; ++i) {
		tempOrigin = tetriminoOrigin;
		validPosition = true;
		tempNewPositions.Empty();

		//get the origin of the tetromino when moved to new offset
		tempOrigin += wallKickOffsets[i];

		for (int j = 0; j < newPositions.Num(); ++j) {
			//calculate new position of the tetromino block based on the wall kick
			FVector tempNewPos = newPositions[j] + wallKickOffsets[i];

			//but if position is same as a landed block or outside the playfield then set valid position to false
			if (landedBlockPos.Contains(tempNewPos) || tempNewPos.Y < leftBoundary || tempNewPos.Y > rightBoundary || tempNewPos.Z < groundLevel) {
				validPosition = false;
			}
			tempNewPositions.Add(tempNewPos);
		}

		//if all positions are valid
		if (validPosition) {
			//if final offset was used, then it was a large wall kick, making player eligable for T spin
			if(i == 3 && bTBlock){
				largeOffset = true;
			}
			//break the loop as can wall kick
			break;
		}
	}

	//if new tetromino position is valid
	if (validPosition) {
		//set new rotated positions for each tetromino block and the new origin
		newPositions = tempNewPositions;
		tetriminoOrigin = tempOrigin;
	}
	else {
		//otherwise, no wall kick was possible, so block the rotation
		canRotate = false;
	}
}

void ATetrisBlock::RotateClockwise() {
	//if game over then disable rotation by returning
	if (blueprintFunctionality->bGameOver) {
		return;
	}

	//set comparison points high/low to make it easy to change
	float lowestYPoint = 1e+10;
	float highestYPoint = -1e+10;
	float lowestZpoint = 1e+10;
	TArray<FVector> newPositions;
	bool shouldWallKick = false;
	tSpin = false;
	miniTSpin = false;
	//calculate origin based on offset
	tetriminoOrigin = spawnedBlocks[0]->GetActorLocation() + tetriminoOriginOffset;
	for (int i = 0; i < 4; ++i) {
		//code adapted from McGuire (2017)
		//get offset from rotation origin of each block
		FVector originOffset = spawnedBlocks[i]->GetActorLocation() - tetriminoOrigin;
		//get position that block will rotate to, relative to x axis
		FVector rotatedPos = originOffset.RotateAngleAxis(-90.f, FVector(1.f, 0.f, 0.f));
		//get final position of block, relative to the rotation origin of the tetrimino
		FVector newPosition = rotatedPos + tetriminoOrigin;

		//round to nearest int as rotated position can sometimes be incorrect by ~ 0.1, creating collision bugs
		FMath::RoundToInt(newPosition.Y);
		FMath::RoundToInt(newPosition.Z);

		//if new position of tetromino would result in clipping through wall or landed blocks then allow wall kick
		if (landedBlockPos.Contains(newPosition) || newPosition.Y < leftBoundary || newPosition.Y > rightBoundary || newPosition.Z < groundLevel) {
			shouldWallKick = true;
		}

		//update the new rotated positions
		newPositions.Add(newPosition);
	}

	//if wall kicking is enabled
	if (shouldWallKick) {
		//wall kick the tetromino based on its current rotation position and whether it is an I block
		//see anti-clockwise offsets for more details
		switch (rotationPos) {
		case 0:
			if (isIBlock) {
				WallKick(newPositions, iClockwise0WallKickOffsets);
				break;
			}

			WallKick(newPositions, clockwise0WallKickOffsets);
			break;
		case 1:
			if (isIBlock) {
				WallKick(newPositions, iClockwiseRWallKickOffsets);
				break;
			}

			WallKick(newPositions, clockwiseRWallKickOffsets);
			break;
		case 2:
			if (isIBlock) {
				WallKick(newPositions, iClockwise2WallKickOffsets);
				break;
			}

			WallKick(newPositions, clockwise2WallKickOffsets);
			break;
		case 3:
			if (isIBlock) {
				WallKick(newPositions, iClockwiseLWallKickOffsets);
				break;
			}

			WallKick(newPositions, clockwiseLWallKickOffsets);
			break;
		default:
			break;
		}
	}

	//if rotating is enabled
	if (canRotate) {
		//update the new leftmost, rightmost and lowest positions of the tetromino
		for (int i = 0; i < 4; ++i) {
			if (newPositions[i].Y < lowestYPoint) {
				lowestYPoint = newPositions[i].Y;
				leftmostBlockIndex = i;
			}
			if (newPositions[i].Y > highestYPoint) {
				highestYPoint = newPositions[i].Y;
				rightmostBlockIndex = i;
			}
			if (newPositions[i].Z < lowestZpoint) {
				lowestZpoint = newPositions[i].Z;
				lowestBlockIndex = i;
			}
			//set the new location of the block
			spawnedBlocks[i]->SetActorLocation(newPositions[i]);
			//end of adapted code
		}
	}

	//update the origin offset based on the new position of the 1st block
	tetriminoOriginOffset = tetriminoOrigin - spawnedBlocks[0]->GetActorLocation();

	//increment rotation position, increase as rotation is clockwise
	rotationPos++;

	//if position becomes greater than 3, reset to 0
	if (rotationPos > 3) {
		rotationPos = 0;
	}

	//check to see if a T spin/mini T spin was performed
	if (bTBlock) {
		CheckForTSpin();
	}

	//set last move as a rotation
	recentlyRotated = true;
}

void ATetrisBlock::CheckForTSpin() {
	FVector positionsToCheck[4];
	//get the positions diagonally above and below the T tetromino origin (i.e., block 1 position)
	positionsToCheck[0] = spawnedBlocks[0]->GetActorLocation() + FVector(0.f, -100.f, 100.f);
	positionsToCheck[1] = spawnedBlocks[0]->GetActorLocation() + FVector(0.f, 100.f, 100.f);
	positionsToCheck[2] = spawnedBlocks[0]->GetActorLocation() + FVector(0.f, 100.f, -100.f);
	positionsToCheck[3] = spawnedBlocks[0]->GetActorLocation() + FVector(0.f, -100.f, -100.f);

	//based on the current rotation position of the tetromino (0 = 0, 1 = R, 2 = 2, 3= L)
	switch (rotationPos) {
	case 0:
		//if rotation is 0, tetrominoes above are position 0 and 1 while below are position 2 and 3
		if ((landedBlockPos.Contains(positionsToCheck[0]) && landedBlockPos.Contains(positionsToCheck[1])) && (landedBlockPos.Contains(positionsToCheck[2]) || landedBlockPos.Contains(positionsToCheck[3]))) {
			//if 2 blocks are diagonally above and 1 is diagonally behind the origin, it is a T spin
			tSpin = true;
			return;
		}
		else if ((landedBlockPos.Contains(positionsToCheck[2]) && landedBlockPos.Contains(positionsToCheck[3])) && (landedBlockPos.Contains(positionsToCheck[0]) || landedBlockPos.Contains(positionsToCheck[1]))) {
			//if 2 blocks are diagonally behind while 1 is above
			//if it wall kicked by a large offset, it is still a T spin
			if (largeOffset) {
				tSpin = true;
				return;
			}
			//but otherwise, it is a mini T spin
			miniTSpin = true;
			return;
		}

		break;
	case 1:
		//if rotation position is R, then positions 1 and 2 are in front of the tetromino and vice versa
		if ((landedBlockPos.Contains(positionsToCheck[1]) && landedBlockPos.Contains(positionsToCheck[2])) && (landedBlockPos.Contains(positionsToCheck[0]) || landedBlockPos.Contains(positionsToCheck[3]))) {
			//if 2 blocks in front and 1 behind, it is a T spin
			tSpin = true;
			return;
		}
		else if ((landedBlockPos.Contains(positionsToCheck[0]) && landedBlockPos.Contains(positionsToCheck[3])) && (landedBlockPos.Contains(positionsToCheck[1]) || landedBlockPos.Contains(positionsToCheck[2]))) {
			//if 2 blocks behind and 1 in front but tetromino was wall kicked by a large offset, it is still a T spin
			if (largeOffset) {
				tSpin = true;
				return;
			}
			//otherwise, it is a mini T spin
			miniTSpin = true;
			return;
		}

		break;
	case 2:
		//if rotation position is 2, then positions 2 and 3 are in front and vice versa
		if ((landedBlockPos.Contains(positionsToCheck[2]) && landedBlockPos.Contains(positionsToCheck[3])) && (landedBlockPos.Contains(positionsToCheck[0]) || landedBlockPos.Contains(positionsToCheck[1]))) {
			//if 2 blocks are in front and at least 1 is behind, it is a T spin
			tSpin = true;
			return;
		}
		else if ((landedBlockPos.Contains(positionsToCheck[0]) && landedBlockPos.Contains(positionsToCheck[1])) && (landedBlockPos.Contains(positionsToCheck[2]) || landedBlockPos.Contains(positionsToCheck[3]))) {
			//else if 2 blocks are behind and 1 is in front
			//if it wall kicked by a large offset, it is a T spin
			if (largeOffset) {
				tSpin = true;
				return;
			}
			//otherwise, it is a mini T spin
			miniTSpin = true;
			return;
		}

		break;
	case 3:
		//if rotation position is 3, then positions 0 and 3 are in front and vice versa
		if ((landedBlockPos.Contains(positionsToCheck[0]) && landedBlockPos.Contains(positionsToCheck[3])) && (landedBlockPos.Contains(positionsToCheck[1]) || landedBlockPos.Contains(positionsToCheck[2]))) {
			//if 2 blocks are in front and at least 1 is behind, it is a T spin
			tSpin = true;
			return;
		}
		else if ((landedBlockPos.Contains(positionsToCheck[1]) && landedBlockPos.Contains(positionsToCheck[2])) && (landedBlockPos.Contains(positionsToCheck[0]) || landedBlockPos.Contains(positionsToCheck[3]))) {
			//else if 2 blocks are in behind and 1 is in front
			//if it wall kicked by a large offset, it is a T spin
			if (largeOffset) {
				tSpin = true;
				return;
			}
			//otherwise, it is a mini T spin
			miniTSpin = true;
			return;
		}

		break;
	default:
		break;
	}
}

void ATetrisBlock::UpdateScore(int scoreIncrease) {
	//increase score by scoreIncrease parameter
	score += scoreIncrease;

	//update the score text
	ScoreText->SetText(FText::FromString("Score = " + FString::FromInt(score)));
}

void ATetrisBlock::UpdateLevel() {
	//increment level
	level++;

	//update level text
	LevelText->SetText(FText::FromString("Level = " + FString::FromInt(level)));
}

void ATetrisBlock::GetBlueprintFunctionality() {
	//find blueprint class in game world and set variable
	for (TObjectIterator<ABlueprintFunctionality> act; act; ++act) {
		FString actorName = act->GetName();
		if (act->GetWorld() != this->GetWorld()) {
			continue;
		}

		if (actorName.Contains("UIFunctionality")) {
			blueprintFunctionality = (ABlueprintFunctionality*)*act;
			break;
		}
	}
}