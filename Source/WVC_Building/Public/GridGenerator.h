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
	TArray<int> Points;
	TArray<int> Neighbours;

	FGridQuad() {}

	FGridQuad(const TArray<int>& InPoints, const int InIndex)
	{
		Points = InPoints;
		Index = InIndex;
	}
	
};

struct FGridTriangle
{
	int Index = -1;
	TArray<int> Points;
	TArray<int> Neighbours;
	bool FormsQuad = false;
	FGridTriangle() {}
	
	FGridTriangle(const TArray<int>& InPoints, const int InIndex)
	{
		Points = InPoints;
		Index = InIndex;
	}

	FGridTriangle(const TArray<int>& InPoints, const int InIndex, const TArray<int>& InNeighbours)
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
	
	//TArray<TArray<FVector>> GridCoordinates;
	TArray<FVector> GridPoints;
	//UPROPERTY(BlueprintReadOnly)
	TArray<FGridTriangle> Triangles;
	TArray<FGridQuad> Quads;
	TArray<FGridQuad> FinalQuads;
	FVector Center;
	void GenerateHexCoordinates(const FVector& GridCenter, const float Size, const uint32 Index);
	void DivideGridIntoTriangles(const FVector& GridCenter);
	void DivideGridIntoQuads(const FVector& GridCenter);
	void SortQuadPoints(FGridQuad& Quad);
public:
	UPROPERTY(EditAnywhere)
	float HexSize = 50.f;
	UPROPERTY(EditAnywhere)
	uint32 GridSize = 5;
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	virtual void OnConstruction(const FTransform& Transform) override;
	void GenerateGrid();

	static int GetFirstTriangleIndexOnHex(const uint32 Hex) { return 6 * Hex * Hex; }
	static int GetNumberOfPointsOnHex(const uint32 Hex) { if(Hex == 0) return 1; return 6 * Hex; }
	static int GetFirstPointIndexOnHex(const uint32 Hex) { if(Hex == 0) return 0; return Hex * (Hex - 1) / 2 * 6 + 1;}
;	static int GetIndexOfPointOnHex(const uint32 Hex, const uint32 Point) { if (Hex == 0) return 0; return GetFirstPointIndexOnHex(Hex) + Point; }
	const FVector& GetPointCoordinates(const uint32 Point) { return GridPoints[Point]; }
	//virtual bool ShouldTickIfViewportsOnly() const override { return true; }
	UDebugStrings* DebugStringsComp;
	//FVector GetGridCoordinate(const int Hex, const int Coordinate) {return GridCoordinates[Hex][Coordinate];}
	const TArray<FGridTriangle>& GetTriangles() { return Triangles; }
	const TArray<FGridQuad>& GetQuads() { return Quads; }
	const TArray<FVector>& GetGridPoints() { return GridPoints; }
	const TArray<FGridQuad>& GetFinalQuads() { return FinalQuads; }
};
