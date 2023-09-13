// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine.h"
#include "GameFramework/Pawn.h"
#include "TetrisBlock.generated.h"

class ASpawnedBlock;
class ABlueprintFunctionality;

//works as a Game Manager and controls the in game behaviour
UCLASS()
class ASSIGNMENT2PROJECT_API ATetrisBlock : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ATetrisBlock();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	//controls the landing behaviour of the tetris blocks
	void RegisterAndCheckBlocks();

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	//controls horizontal movement by player
	void MoveHorizontally(float axisValue);

	//spawns a singular block based on parameters passed through
	void SpawnBlock(FVector position, UMaterial* blockColour, int blockIndex);

	//checks row of current block to see if line has been cleared
	void CheckRow(ASpawnedBlock* currentBlock);

	//deletes all blocks in blocks array from the game
	void RemoveBlocks(TArray<ASpawnedBlock*> blocks);

	//moves all blocks in blocks array down by 100 units (equivalent of 1 tetris unit)
	void ShiftBlocksDown(TArray<ASpawnedBlock*> blocks);

	//controls soft drop behaviours
	void SpeedUpDrop();

	//releases soft drop
	void SlowDownDrop();

	//spawns the entire tetromino
	void SpawnTetromino();

	//rotates the tetromino anti clockwise
	void RotateAntiClockwise();

	//rotates the tetromino clockwise
	void RotateClockwise();

	//wall kicks tetromino if rotation was illegal
	void WallKick(TArray<FVector>& newPositions, FVector wallKickOffsets[4]);

	//updates players score. Default of 100
	void UpdateScore(int scoreIncrease = 100);

	//increases the level by 1
	void UpdateLevel();

	//controls behaviour of a locking hard drop
	void HardDrop();

	//checks if player performed a t-spin after rotation
	void CheckForTSpin();

	//gets the blueprint functionality class in the game world
	void GetBlueprintFunctionality();

	//get the next tetromino colour due to spawn
	void GetNextColourIndex();

	//get position of lowest point of tetromino
	float GetLowestZPosition();

	//current score text component
	UPROPERTY(Category = Grid, VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UTextRenderComponent* ScoreText;

	//current level text component
	UPROPERTY(Category = Grid, VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UTextRenderComponent* LevelText;

	//Z value of ground level set in inspector
	UPROPERTY(EditAnywhere)
	float groundLevel;

	//distance that tetromino drops periodically (i.e., distance of 1 tetris unit)
	UPROPERTY(EditAnywhere)
	float dropDist;

	//furthest Y point to the left
	UPROPERTY(EditAnywhere)
	float leftBoundary;

	//furthest Y point to the right
	UPROPERTY(EditAnywhere)
	float rightBoundary;

	//X position of where tetromino should spawn
	UPROPERTY(EditAnywhere)
	float xSpawnPoint;

	//Z position (height) of where tetromino should spawn. Based on lowest position
	UPROPERTY(EditAnywhere)
	float zSpawnPoint;

	//possible colours of each tetromino
	UPROPERTY(EditAnywhere)
	TArray<UMaterial*> blockColours;

	//maximum Z value of a tetromino, beyond this will trigger game over
	UPROPERTY(EditAnywhere)
	float overflowHeight;

	//reference to the in game camera
	UPROPERTY(EditAnywhere)
	USceneComponent* camera;

	//wall kick offsets tested when rotated clockwise from position 0 (the starting rotation) excluding I block
	UPROPERTY(EditAnywhere)
	FVector clockwise0WallKickOffsets[4];

	//wall kick offsets tested when rotated clockwise from position R (rotation when rotated clockwise from start rotation) excluding I block
	UPROPERTY(EditAnywhere)
	FVector clockwiseRWallKickOffsets[4];

	//wall kick offsets tested when rotated clockwise from position 2 (rotation when rotated twice in same direction from start rotation) excluding I block
	UPROPERTY(EditAnywhere)
	FVector clockwise2WallKickOffsets[4];

	//wall kick offsets tested when rotated clockwise from position L (rotation when rotated anticlockwise from start rotation) excluding I block
	UPROPERTY(EditAnywhere)
	FVector clockwiseLWallKickOffsets[4];

	//wall kick offsets tested when rotated anti-clockwise from position 0 excluding I block
	UPROPERTY(EditAnywhere)
	FVector antiClockwise0WallKickOffsets[4];

	//wall kick offsets tested when rotated anti-clockwise from position L excluding I block
	UPROPERTY(EditAnywhere)
	FVector antiClockwiseLWallKickOffsets[4];

	//wall kick offsets tested when rotated anti-clockwise from position 2 excluding I block
	UPROPERTY(EditAnywhere)
	FVector antiClockwise2WallKickOffsets[4];

	//wall kick offsets tested when rotated anti-clockwise from position R excluding I block
	UPROPERTY(EditAnywhere)
	FVector antiClockwiseRWallKickOffsets[4];

	//wall kick offsets tested when I block is rotated clockwise from position 0
	UPROPERTY(EditAnywhere)
	FVector iClockwise0WallKickOffsets[4];

	//wall kick offsets tested when I block is rotated clockwise from position R
	UPROPERTY(EditAnywhere)
	FVector iClockwiseRWallKickOffsets[4];

	//wall kick offsets tested when I block is rotated clockwise from position 2
	UPROPERTY(EditAnywhere)
	FVector iClockwise2WallKickOffsets[4];

	//wall kick offsets tested when I block is rotated clockwise from position L
	UPROPERTY(EditAnywhere)
	FVector iClockwiseLWallKickOffsets[4];

	//wall kick offsets tested when I block is rotated anti-clockwise from position 0
	UPROPERTY(EditAnywhere)
	FVector iAntiClockwise0WallKickOffsets[4];

	//wall kick offsets tested when I block is rotated anti-clockwise from position L
	UPROPERTY(EditAnywhere)
	FVector iAntiClockwiseLWallKickOffsets[4];

	//wall kick offsets tested when I block is rotated anti-clockwise from position 2
	UPROPERTY(EditAnywhere)
	FVector iAntiClockwise2WallKickOffsets[4];

	//wall kick offsets tested when I block is rotated anti-clockwise from position R
	UPROPERTY(EditAnywhere)
	FVector iAntiClockwiseRWallKickOffsets[4];

private:
	//current velocity of tetromino
	FVector CurrentVelocity;

	//the position of each block in the current tetromino
	ASpawnedBlock* spawnedBlocks[4];

	//the position of all landed blocks which haven't been cleared
	TArray<FVector> landedBlockPos;

	//reference to all blocks currently in the scene
	TArray<ASpawnedBlock*> allBlocks;

	//possible tetromino colours that can spawn. Removed once selected but updated to full when array is empty
	TArray<UMaterial*> colourPool;

	//times when the tetromino last dropped 1 tetris unit
	float dropTimer;

	//times when the player last move the tetromino left or right
	float inputTimer;

	//the frequency that the tetromino should drop 1 tetris unit. Decreases in later levels
	float dropSpeed;

	//current level that the player is at, affects the drop speed
	int level;

	//total amount of lines cleared currently
	int linesCleared;

	//leftmost position of the tetromino
	int leftmostBlockIndex;

	//rightmost position of the tetromino
	int rightmostBlockIndex;

	//lowest position of the tetromino
	int lowestBlockIndex;

	//current rotation position of tetromino (0 = 0 pos, 1 = R pos, 2 = 2 pos, 3 = L pos, see wall kick offsets for position meanings)
	int rotationPos;

	//checks if tetromino is an I tetromino
	bool isIBlock;

	//rotation origin of the tetromino
	FVector tetriminoOrigin;

	//offset from block 1 position if center is not block 1 position (i.e., for I and O blocks)
	FVector tetriminoOriginOffset;

	//reference to the main camera in the scene
	UCameraComponent* mainCamera;

	//current player's score
	int score;

	//if true, player can hard lock tetromino
	bool canHardLock;

	//if false once all wall kick offsets were tested, blocks a rotation
	bool canRotate;

	//index of current block that is compared to check its landing position when hard locked
	int ZCompareIndex;

	//amount of rows cleared from the landed tetromino, used for scoring
	int rowsClearedInMove;

	//if true, will increase drop speed to soft drop speed
	bool bSoftDrop;

	//reports if current tetromino is a T block
	bool bTBlock;

	//reports if last move before locking tetromino was a rotation, if true, will check for a T spin
	bool recentlyRotated;

	//if true, player has performed a T spin
	bool tSpin;

	//if true, player has performed a mini T spin
	bool miniTSpin;

	//if true, when rotated, T block has wall kicked to a large offset (i.e., offset 4 in wall kick array)
	bool largeOffset;

	//checks if player performed a difficult move when block was landed (i.e., tetris, mini T Spin/T spin single, mini T spin/T spin double or T spin triple)
	bool difficultMovePerformed;

	//multiplies score earnt by multiplier. Used for back-to-back difficult moves
	float scoreMultiplier;

	//reference to the blueprint functionality class
	ABlueprintFunctionality* blueprintFunctionality;

	//colours of next 3 tetrominoes to spawn
	TArray<UMaterial*> nextColours;
};
