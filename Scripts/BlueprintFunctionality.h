// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BlueprintFunctionality.generated.h"

//controls c++ functionality of blueprints used for UI
UCLASS()
class ASSIGNMENT2PROJECT_API ABlueprintFunctionality : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABlueprintFunctionality();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	//if true, game will finish
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game Over")
	bool bGameOver;

	//contains the indexes of the next 3 tetrominoes to spawn. Indexes based on blockColours array position
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Next Tetromino")
	TArray<int> nextTetrominoNumbers;

	//if false, will update the next tetromino UI in blueprint class
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Next Tetromino")
	bool uiUpdated;
};
