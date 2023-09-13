// Fill out your copyright notice in the Description page of Project Settings.


#include "SpawnedBlock.h"

// Sets default values
ASpawnedBlock::ASpawnedBlock()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	//creates the block visual as a cube and sets its relative location and scale
	blockVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualRepresentation"));
	blockVisual->SetupAttachment(RootComponent);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> blockVisualAsset(TEXT("/Engine/BasicShapes/Cube.cube"));
	if (blockVisualAsset.Succeeded()) {
		blockVisual->SetStaticMesh(blockVisualAsset.Object);
		blockVisual->SetRelativeLocation(FVector(0.f, 0.f, 0.f));
		blockVisual->SetWorldScale3D(FVector(1.f));
	}

}

// Called when the game starts or when spawned
void ASpawnedBlock::BeginPlay()
{
	Super::BeginPlay();
	landed = false;
	
}

// Called every frame
void ASpawnedBlock::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ASpawnedBlock::MoveBlock(FVector NewLocation) {
	//move block to NewLocation
	SetActorLocation(NewLocation);
}

void ASpawnedBlock::SetColour(UMaterial* colour) {
	//change colour of block to colour variable
	blockVisual->SetMaterial(0, colour);
}

