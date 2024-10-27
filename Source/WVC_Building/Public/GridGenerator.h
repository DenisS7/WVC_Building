// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridGenerator.generated.h"

class UDynamicMeshComponent;
class UGridGeneratorVis;
class UDebugStrings;

USTRUCT(BlueprintType)
struct FGridPoint
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly)
	int Index = -1;
	UPROPERTY(BlueprintReadOnly)
	FVector Location = FVector::ZeroVector;
	TArray<int> Neighbours;
	TArray<int> PartOfQuads;
	bool IsEdge = false;
	
	FGridPoint() {}

	FGridPoint(const int InIndex, const FVector& InLocation, const bool InIsEdge = false)
		: Index(InIndex), Location(InLocation), IsEdge(InIsEdge) {};
	
	FGridPoint(const int InIndex, const FVector& InLocation, const TArray<int>& InNeighbours, const bool InIsEdge = false)
		: Index(InIndex), Location(InLocation), Neighbours(InNeighbours), IsEdge(InIsEdge) {};
};

USTRUCT(BlueprintType)
struct FGridQuad
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly)
	int Index = -1;
	UPROPERTY(BlueprintReadOnly)
	FVector Center;
	UPROPERTY(BlueprintReadOnly)
	TArray<int> Points;
	UPROPERTY(BlueprintReadOnly)
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

UENUM(BlueprintType)
enum class EShape : uint8
{
	Triangle = 3,
	Quad = 4,
	Pentagon = 5,
	Hexagon = 6,
	None = 0
};

USTRUCT(BlueprintType)
struct FGridShape
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly)
	int Index = -1;
	UPROPERTY(BlueprintReadOnly)
	TArray<int> Points;
	UPROPERTY(BlueprintReadOnly)
	TArray<int> Neighbours;
	UPROPERTY(BlueprintReadOnly)
	FVector Center = FVector::ZeroVector;
	EShape Shape = EShape::None;

	FGridShape() {}
	FGridShape(const int InIndex, const TArray<int>& InPoints)
		: Index(InIndex), Points(InPoints)
	{
		Shape = static_cast<EShape>(InPoints.Num());
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
	UDebugStrings* DebugStringsComp;
	UGridGeneratorVis* GridGeneratorVis;
	UPROPERTY(BlueprintReadOnly)
	TArray<FGridPoint> GridPoints;
	TArray<FGridTriangle> Triangles;
	TArray<FGridQuad> Quads;
	UPROPERTY(BlueprintReadOnly)
	TArray<FGridQuad> FinalQuads;
	TArray<TArray<FVector>> PerfectQuads;

	UPROPERTY(BlueprintReadOnly)
	TArray<FGridPoint> SecondGridPoints;
	UPROPERTY(BlueprintReadOnly)
	TArray<FGridShape> SecondGridShapes;

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	UDynamicMeshComponent* WholeGridMesh;
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	UDynamicMeshComponent* HoveredShapeMesh;
	
	FVector Center;
	void GenerateHexCoordinates(const FVector& GridCenter, const float Size, const uint32 Index);
	void DivideGridIntoTriangles(const FVector& GridCenter);
	void DivideGridIntoQuads(const FVector& GridCenter);
	void FindPointNeighboursInQuad(const int QuadIndex);
	void SortQuadPoints(FGridQuad& Quad);
	void SortShapePoints(FGridShape& Shape, bool SecondGrid = true);
	void RelaxGridBasedOnSquare(float SquareSideLength)	;
	void RelaxGridBasedOnSquare2();
	void RelaxGridBasedOnNeighbours();
	void Relax2();
	void Relax3();
	int GetOrAddMidpointIndexInGrid1(TMap<TPair<int, int>, int>& Midpoints, int Point1, int Point2);
	void CreateSecondGrid();
	void CreateWholeGridMesh();
	bool IsPointInShape(const FVector& Point, const FGridShape& Quad) const;
	uint32 IterationsUsed1 = 0;
	uint32 IterationsUsed2 = 0;
	
	FTimerHandle SquareHandle1;
	FTimerHandle SquareHandle2;
	FTimerHandle TimerHandle;
	FTimerHandle TimerHandle2;
	FTimerHandle TimerHandle3;
	FTimerDelegate SquareDelegate1;
	FTimerDelegate SquareDelegate2;
	FTimerDelegate Delegate1;
	FTimerDelegate Delegate2;
	FTimerDelegate Delegate3;
public:
	UPROPERTY(EditAnywhere, Category = "Show Debug")
	bool Debug = false;
	UPROPERTY(EditAnywhere, Category = "Show Debug")
	bool ShowGrid = true;
	UPROPERTY(EditAnywhere, Category = "Show Debug")
	bool ShowSquares = false;
	UPROPERTY(EditAnywhere, Category = "Show Debug")
	bool ShowSecondGrid = false;
	UPROPERTY(EditAnywhere, Category = "Show Debug")
	bool ShowGrid1Points = false;
	UPROPERTY(EditAnywhere, Category = "Show Debug")
	bool ShowGrid2Points = false;
	UPROPERTY(EditAnywhere, Category = "Show Debug")
	bool ShowGrid1QuadIndices = false;
	UPROPERTY(EditAnywhere, Category = "Show Debug")
	bool ShowGrid2QuadIndices = false;
	
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
	void DrawSecondGrid();
	void GenerateGrid();
	void CreateShapeMesh(const int ShapeIndex);
	void ResetShapeMesh();
	
	int DetermineWhichGridShapeAPointIsIn(const FVector& Point);
	static int GetFirstTriangleIndexOnHex(const uint32 Hex) { return 6 * Hex * Hex; }
	static int GetNumberOfPointsOnHex(const uint32 Hex) { if(Hex == 0) return 1; return 6 * Hex; }
	static int GetFirstPointIndexOnHex(const uint32 Hex) { if(Hex == 0) return 0; return Hex * (Hex - 1) / 2 * 6 + 1;}
;	static int GetIndexOfPointOnHex(const uint32 Hex, const uint32 Point) { if (Hex == 0) return 0; return GetFirstPointIndexOnHex(Hex) + Point; }
	const FVector& GetPointCoordinates(const uint32 Point) const { return GridPoints[Point].Location; }
	const FVector& GetSecondPointCoordinates(const uint32 Point) const { return SecondGridPoints[Point].Location; }
	virtual bool ShouldTickIfViewportsOnly() const override { return true; }
	//FVector GetGridCoordinate(const int Hex, const int Coordinate) {return GridCoordinates[Hex][Coordinate];}
	const TArray<FGridTriangle>& GetTriangles() const { return Triangles; }
	const TArray<FGridQuad>& GetQuads() const { return Quads; }
	const TArray<FGridPoint>& GetGridPoints() const { return GridPoints; }
	const TArray<FGridPoint>& GetSecondGridPoints() const { return SecondGridPoints; }
	const TArray<FGridQuad>& GetFinalQuads() const { return FinalQuads; }
	const TArray<FGridShape>& GetSecondGrid() const { return SecondGridShapes; }
	//const TArray<FGridQuad>& GetFinalSquares() const { return Square}
};
