// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine.h"
#include "GameFramework/Actor.h"
#include "SpawnedBlock.generated.h"

//contains the details of each individual spawned block
UCLASS()
class ASSIGNMENT2PROJECT_API ASpawnedBlock : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASpawnedBlock();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	//moves block to the new location (i.e., NewLocation)
	void MoveBlock(FVector NewLocation);

	//changes block colour to colour passed through
	void SetColour(UMaterial* colour);

	//if true, player will no longer control block
	bool landed;

private:
	//visual component of block
	UStaticMeshComponent* blockVisual;

};
