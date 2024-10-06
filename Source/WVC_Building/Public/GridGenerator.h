// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridGenerator.generated.h"

class UDebugStrings;

struct FGridQuad
{	
	int Index = -1;
	FVector Center;
	TArray<FInt32Point> Points;
	TArray<int> Neighbours;

	FGridQuad() {}

	FGridQuad(const TArray<FInt32Point>& InPoints, const int InIndex)
	{
		Points = InPoints;
		Index = InIndex;
	}
};

struct FGridTriangle
{
	int Index = -1;
	TArray<FInt32Point> Points;
	TArray<int> Neighbours;
	bool FormsQuad = false;
	FGridTriangle() {}

	FGridTriangle(const TArray<FInt32Point>& InPoints, const int InIndex)
	{
		Points = InPoints;
		Index = InIndex;
	}

	FGridTriangle(const TArray<FInt32Point>& InPoints, const int InIndex, const TArray<int>& InNeighbours)
	{
		Points = InPoints;
		Index = InIndex;
		Neighbours = InNeighbours;
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
	TArray<FGridTriangle> Triangles;
	TArray<FGridQuad> Quads;
	FVector Center;
	void GenerateHexCoordinates(const FVector& GridCenter, const float Size, const uint32 Index);
	void DivideGridIntoTriangles(const FVector& GridCenter);
	void DivideGridIntoQuads(const FVector& GridCenter);
public:
	UPROPERTY(EditAnywhere)
	float HexSize = 50.f;
	UPROPERTY(EditAnywhere)
	uint32 GridSize = 5;
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	virtual void OnConstruction(const FTransform& Transform) override;
	void GenerateGrid();
	
	int GetFirstTriangleIndexOnHex(const uint32 Hex) { return 6 * Hex * Hex; }
	UDebugStrings* DebugStringsComp;
	
	FVector GetGridCoordinate(const int Hex, const int Coordinate) {return GridCoordinates[Hex][Coordinate];}
	const TArray<FGridTriangle>& GetTriangles() { return Triangles; }
};
