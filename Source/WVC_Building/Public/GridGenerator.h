// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridGenerator.generated.h"

struct Quad
{
	FVector Center;
	FVector Corners[4];
};

UCLASS()
class WVC_BUILDING_API AGridGenerator : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AGridGenerator();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	//
	//FVector GridCoordinates[10][60];
	TArray<TArray<FVector>> GridCoordinates;
	void GenerateHex(const FVector& Center, const float Size, const uint32 Index);
public:
	UPROPERTY(EditAnywhere)
	float HexSize = 50.f;
	UPROPERTY(EditAnywhere)
	uint32 GridSize = 5;
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	virtual void OnConstruction(const FTransform& Transform) override;
	void GenerateGrid();
};
