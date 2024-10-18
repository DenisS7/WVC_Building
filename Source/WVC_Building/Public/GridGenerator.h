// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridGenerator.generated.h"

class UDebugStrings;

struct FGridPoint
{
	int Index = -1;
	FVector Location = FVector::ZeroVector;
	TArray<int> Neighbours;

	FGridPoint() {}

	FGridPoint(const int InIndex, const FVector& InLocation)
		: Index(InIndex), Location(InLocation) {};
	
	FGridPoint(const int InIndex, const FVector& InLocation, const TArray<int>& InNeighbours)
		: Index(InIndex), Location(InLocation), Neighbours(InNeighbours) {};
};

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
	TArray<FGridPoint> GridPoints;
	//TArray<TArray<int>> GridPointNeighbours;
	//UPROPERTY(BlueprintReadOnly)
	TArray<FGridTriangle> Triangles;
	TArray<FGridQuad> Quads;
	TArray<FGridQuad> FinalQuads;
	TArray<TArray<FVector>> PerfectQuads;
	FVector Center;
	void GenerateHexCoordinates(const FVector& GridCenter, const float Size, const uint32 Index);
	void DivideGridIntoTriangles(const FVector& GridCenter);
	void DivideGridIntoQuads(const FVector& GridCenter);
	void FindPointNeighboursInQuad(const int QuadIndex);
	void SortQuadPoints(FGridQuad& Quad);
	void RelaxGridBasedOnSquare(float SquareSideLength);
	void RelaxGridBasedOnSquare2();
	void RelaxGridBasedOnNeighbours();
	void Relax2();
	void Relax3();
	int GetOrAddMidpointIndex(TMap<TPair<int, int>, int>& Midpoints, int Point1, int Point2);

	uint32 IterationsUsed1 = 0;
	uint32 IterationsUsed2 = 0;
	
	FTimerHandle SquareHandle1;
	FTimerDelegate SquareDelegate1;
	FTimerHandle SquareHandle2;
	FTimerDelegate SquareDelegate2;
	FTimerHandle TimerHandle;
	FTimerHandle TimerHandle2;
	FTimerDelegate Delegate1;
	FTimerDelegate Delegate2;
public:
	UPROPERTY(EditAnywhere)
	bool ShowGrid = true;
	UPROPERTY(EditAnywhere)
	bool ShowSquares = false;
	UPROPERTY(EditAnywhere)
	float HexSize = 50.f;
	UPROPERTY(EditAnywhere)
	uint32 GridSize = 5;
	//UPROPERTY(EditAnywhere)
	//bool DoSquareRelaxationFirst = true;
	UPROPERTY(EditAnywhere)
	float SquareSize = 15.f;
	UPROPERTY(EditAnywhere)
	float ForceScale = 0.1f;
	UPROPERTY(EditAnywhere)
	uint32 Square1RelaxIterations = 10;
	UPROPERTY(EditAnywhere)
	uint32 Square2RelaxIterations = 10;
	UPROPERTY(EditAnywhere)
	uint32 NeighbourRelaxIterations = 10;
	UPROPERTY(EditAnywhere)
	uint32 Seed = 0;

	UPROPERTY(EditAnywhere)
	uint32 Square1Order = 1;
	UPROPERTY(EditAnywhere)
	uint32 Square2Order = 2;
	UPROPERTY(EditAnywhere)
	uint32 NeighbourOrder = 3;

	UPROPERTY(EditAnywhere)
	float Order1TimeRate = 0.1f;
	UPROPERTY(EditAnywhere)
	float Order2TimeRate = 0.1f;
	UPROPERTY(EditAnywhere)
	float Order3TimeRate = 0.1f;
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	virtual void OnConstruction(const FTransform& Transform) override;
	void DrawGrid();
	void GenerateGrid();

	static int GetFirstTriangleIndexOnHex(const uint32 Hex) { return 6 * Hex * Hex; }
	static int GetNumberOfPointsOnHex(const uint32 Hex) { if(Hex == 0) return 1; return 6 * Hex; }
	static int GetFirstPointIndexOnHex(const uint32 Hex) { if(Hex == 0) return 0; return Hex * (Hex - 1) / 2 * 6 + 1;}
;	static int GetIndexOfPointOnHex(const uint32 Hex, const uint32 Point) { if (Hex == 0) return 0; return GetFirstPointIndexOnHex(Hex) + Point; }
	const FVector& GetPointCoordinates(const uint32 Point) { return GridPoints[Point].Location; }
	virtual bool ShouldTickIfViewportsOnly() const override { return true; }
	UDebugStrings* DebugStringsComp;
	//FVector GetGridCoordinate(const int Hex, const int Coordinate) {return GridCoordinates[Hex][Coordinate];}
	const TArray<FGridTriangle>& GetTriangles() { return Triangles; }
	const TArray<FGridQuad>& GetQuads() { return Quads; }
	const TArray<FGridPoint>& GetGridPoints() { return GridPoints; }
	const TArray<FGridQuad>& GetFinalQuads() { return FinalQuads; }
};
