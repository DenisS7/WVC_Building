// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridGenerator.generated.h"

class UDebugStrings;

struct Quad
{
	FVector Center;
	FVector Corners[4];
};

USTRUCT(BlueprintType)
struct FGridTriangle
{
	GENERATED_BODY();
	
	TArray<FInt32Point> Points;

	int Index = -1;
	TArray<FInt32Point> Neighbours;

	FGridTriangle()
	{
	}

	FGridTriangle(const TArray<FInt32Point>& NPoints, const int NIndex)
	{
		Points = NPoints;
		Index = NIndex;
	}

	FGridTriangle(const TArray<FInt32Point>& NPoints, const int NIndex, const TArray<FInt32Point>& NNeighbours)
	{
		Points = NPoints;
		Index = NIndex;
		Neighbours = NNeighbours;
	}
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
	//UPROPERTY(BlueprintReadOnly)
	TArray<TArray<FGridTriangle>> Triangles;
	FVector Center;
	void GenerateHexCoordinates(const FVector& GridCenter, const float Size, const uint32 Index);
	void DivideGridIntoTriangles(const FVector& GridCenter);
public:
	UPROPERTY(EditAnywhere)
	float HexSize = 50.f;
	UPROPERTY(EditAnywhere)
	uint32 GridSize = 5;
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	virtual void OnConstruction(const FTransform& Transform) override;
	void GenerateGrid();
	
	int GetTriangleIndexAt(uint32 Hex, uint32 Number) { return Hex * 12 + Number; }
	UDebugStrings* DebugStringsComp;
	
	FVector GetGridCoordinate(const int Hex, const int Coordinate) {return GridCoordinates[Hex][Coordinate];}
	const TArray<TArray<FGridTriangle>>& GetTriangles() { return Triangles; }
};
