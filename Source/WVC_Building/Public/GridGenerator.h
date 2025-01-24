// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridGenerator.generated.h"

class ABuildingPiece;
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

	UPROPERTY(BlueprintReadOnly)
	TArray<int> Neighbours;

	UPROPERTY(BlueprintReadOnly)
	TArray<int> PartOfQuads;

	UPROPERTY(BlueprintReadOnly)
	bool IsEdge = false;

	UPROPERTY(BlueprintReadOnly)
	int CorrespondingBuildingShape = -1;
	
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
	FVector Center = FVector::ZeroVector;
	
	UPROPERTY(BlueprintReadOnly)
	TArray<int> Points;
	
	UPROPERTY(BlueprintReadOnly)
	TArray<int> Neighbours;
	
	UPROPERTY(BlueprintReadOnly)
	TArray<int> OffsetNeighbours;
	
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

USTRUCT(BlueprintType)
struct FShapeQuadPointsForBP
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	TArray<FVector> Points;

	FShapeQuadPointsForBP () {}
	
	explicit FShapeQuadPointsForBP(const TArray<FVector>& InPoints) : Points(InPoints) {}
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
	TArray<int> OffsetNeighbours;
	
	UPROPERTY(BlueprintReadOnly)
	int CorrespondingBaseGridPoint = 0;
	
	UPROPERTY(BlueprintReadOnly)
	FVector Center = FVector::ZeroVector;
	
	UPROPERTY(BlueprintReadOnly)
	TArray<FShapeQuadPointsForBP> ComposingQuads;
	
	FGridShape() {}
	
	FGridShape(const int InIndex, const TArray<int>& InPoints, const int InCorrespondingGrid1Point)
		: Index(InIndex), Points(InPoints), CorrespondingBaseGridPoint(InCorrespondingGrid1Point)
	{
		
	}
};

USTRUCT(BlueprintType)
struct FVoxelConfig
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int CorrespondingQuadIndex = -1;
	
	UPROPERTY(BlueprintReadOnly)
	TArray<bool> ConfigurationCorners;
	
	UPROPERTY(BlueprintReadOnly)
	int Elevation = -1;
	
	UPROPERTY(BlueprintReadOnly)
	ABuildingPiece* BuildingPiece = nullptr;
	
	FVoxelConfig() {}
	FVoxelConfig(const int InCorrespondingQuadIndex, const int InElevation)
		: CorrespondingQuadIndex(InCorrespondingQuadIndex), Elevation(InElevation)
	{
		ConfigurationCorners.Init(false, 8);
	}
};

USTRUCT(BlueprintType)
struct FCell
{
	GENERATED_BODY()
	
	int Elevation = -1;
	int Index = -1;

	TArray<FName> Candidates;
	TArray<TArray<int>> CandidateBorders;

	TArray<FName> DiscardedCandidates;
	TArray<TArray<int>> DiscardedCandidateBorders;
	
	int RotationAmount = 0;
	bool HasChosenCandidate = false;

	FName ChosenCandidate = FName("");
	
	TArray<TPair<int, int>> Neighbours;
	
	TArray<int> MarchingBits;

	FCell() {}
	
	FCell(const int InElevation, const int InIndex)
		: Elevation(InElevation), Index(InIndex)
	{}

	bool operator==(const FCell& Other) const
	{
		return Elevation == Other.Elevation && Index == Other.Index;
	}
};

USTRUCT(BlueprintType)
struct FElevationData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	TArray<bool> MarchingBits;

	UPROPERTY(BlueprintReadOnly)
	TMap<int, FCell> Cells;

	UPROPERTY(BlueprintReadOnly)
	TMap<int, TObjectPtr<ABuildingPiece>> BuildingPieces;

	FElevationData() {}

	FElevationData(const int& NumBits)
	{
		MarchingBits.Init(false, NumBits);
	}
};

typedef TPair<int, int> FCellIndex;


UCLASS()
class WVC_BUILDING_API AGridGenerator : public AActor
{
	GENERATED_BODY()
	
public:	
	UPROPERTY(EditAnywhere, Category = "Show Debug")
	bool Debug = false;
	
	UPROPERTY(EditAnywhere, Category = "Show Debug")
	bool ShowBaseGrid = true;
	
	UPROPERTY(EditAnywhere, Category = "Show Debug")
	bool ShowSquares = false;
	
	UPROPERTY(EditAnywhere, Category = "Show Debug")
	bool ShowBuildingGrid = false;
	
	UPROPERTY(EditAnywhere, Category = "Show Debug")
	bool ShowBaseGridPoints = false;
	
	UPROPERTY(EditAnywhere, Category = "Show Debug")
	bool ShowBuildingGridPoints = false;
	
	UPROPERTY(EditAnywhere, Category = "Show Debug")
	bool ShowBaseGridQuadIndices = false;
	
	UPROPERTY(EditAnywhere, Category = "Show Debug")
	bool ShowBuildingGridQuadIndices = false;

	UPROPERTY(EditAnywhere, Category = "Show Debug")
	bool ShowMeshEdgeCodes = false;
	
	UPROPERTY(EditAnywhere)
	float FirstHexSize = 50.f;
	
	UPROPERTY(EditAnywhere)
	uint32 GridExtent = 5;
	
	UPROPERTY(EditAnywhere)
	float PerfectEqualSquareSize = 15.f;
	
	UPROPERTY(EditAnywhere)
	float ForceScale = 0.1f;

	UPROPERTY(EditAnywhere)
	uint32 PerfectEqualSquareRelaxIterations = 10;

	UPROPERTY(EditAnywhere)
	uint32 PerfectLocalSquareRelaxIterations = 10;

	UPROPERTY(EditAnywhere)
	uint32 NeighbourRelaxIterations = 10;

	UPROPERTY(EditAnywhere)
	uint32 Seed = 0;

	
	UPROPERTY(EditAnywhere)
	uint32 PerfectEqualSquareOrder = 1;

	UPROPERTY(EditAnywhere)
	uint32 PerfectLocalSquareOrder = 2;
	
	UPROPERTY(EditAnywhere)
	uint32 NeighbourOrder = 3;

	
	UPROPERTY(EditAnywhere)
	float Order1TimeRate = 0.1f;

	UPROPERTY(EditAnywhere)
	float Order2TimeRate = 0.1f;

	UPROPERTY(EditAnywhere)
	float Order3TimeRate = 0.1f;
	
	// Sets default values for this actor's properties
	AGridGenerator();
	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	//virtual void override;
	virtual void OnConstruction(const FTransform& Transform) override;
	
	virtual bool ShouldTickIfViewportsOnly() const override { return true; }
	
	void DrawGrid();
	void DrawSecondGrid();
	void GenerateGrid();
	void CreateGridShapeMesh(const int ShapeIndex);
	void CreateAdjacentShapeMesh(const TArray<FVector>& Points);
	void ResetShapeMesh();

	void UpdateMarchingBit(int ElevationLevel, int Index, bool Value, bool IsAdjacent);
	void UpdateBuildingPiece(const int& ElevationLevel, const int& Index);
	int DetermineWhichGridShapeAPointIsIn(const FVector& Point);
	
	static int GetFirstTriangleIndexOnHex(const uint32 Hex) { return 6 * Hex * Hex; }
	static int GetNumberOfPointsOnHex(const uint32 Hex) { if(Hex == 0) return 1; return 6 * Hex; }
	static int GetFirstPointIndexOnHex(const uint32 Hex) { if(Hex == 0) return 0; return Hex * (Hex - 1) / 2 * 6 + 1;}
;	static int GetIndexOfPointOnHex(const uint32 Hex, const uint32 Point) { if (Hex == 0) return 0; return GetFirstPointIndexOnHex(Hex) + Point; }

	int GetMaxElevation() const { return MaxElevation; }
	const FVector& GetBasePointCoordinates(const uint32 Point) const { return BaseGridPoints[Point].Location; }
	const FVector& GetBuildingPointCoordinates(const uint32 Point) const { return BuildingGridPoints[Point].Location; }
	const TArray<FGridPoint>& GetBaseGridPoints() const { return BaseGridPoints; }
	const TArray<FGridPoint>& GetBuildingGridPoints() const { return BuildingGridPoints; }
	const TArray<FGridQuad>& GetBaseGridQuads() const { return BaseGridQuads; }
	const TArray<FGridShape>& GetBuildingGridShapes() const { return BuildingGridShapes; }
	const FElevationData& GetElevationData(const int& Level) const { return Elevations[Level]; }
	void RunWVC(const int Elevation, const int MarchingBitUpdated);
	
	UFUNCTION(BlueprintCallable, Category = "GridGenerator")
	UWorld* GetGridWorld() const;
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	void FindPointNeighboursInQuad(const int QuadIndex);
	void SortQuadPoints(FGridQuad& Quad);
	void SortShapePoints(FGridShape& Shape, bool SecondGrid = true);
	TArray<FVector> SortPoints(const TArray<FVector>& Points, TArray<int>& NewOrder);
	int GetOrAddMidpointIndexInBaseGrid(TMap<TPair<int, int>, int>& Midpoints, int Point1, int Point2);
	bool IsPointInShape(const FVector& Point, const FGridShape& Quad) const;
	
	void RelaxGridBasedOnPerfectEqualSquare(float SquareSideLength);
	void RelaxGridBasedOnPerfectLocalSquare();
	void RelaxGridBasedOnNeighbours();
	
	void Relax1();
	void Relax2();
	void Relax3();
	void CreateWholeGridMesh();
	void CreateSecondGrid();

	void GenerateHexCoordinates(const FVector& GridCenter, const float HexSize, const uint32 HexIndex);
	TArray<FGridTriangle> DivideGridIntoTriangles(const FVector& GridCenter);
	void DivideGridIntoQuads(const FVector& GridCenter, TArray<FGridTriangle>& Triangles);
	void RelaxAndCreateSecondGrid();
	void ReorderQuadNeighbours();

	void LogSuperpositionOptions(const FCell& Cell);
	void LogSuperpositionOptions(const TArray<FCell*>& Cells);
	void GetMarchingBitsForCell(FCell& Cell);
	int GetLowestEntropyCell(const TArray<FCell>& Cells);
	bool CheckNeighbourCandidates(const FCell& Cell, FCell& NeighbourCell, const int CellBorderIndex, const int NeighbourCellBorderIndex);

	void ResetCells(TArray<FCell*>& Cells);
	TArray<FCell*> GetCellsToCheck(const int Elevation, const int MarchingBitUpdated);
	void CalculateCandidates(TArray<FCell*>& Cells);
	void PropagateChoice(TArray<FCell>& Cells, const FCell& UpdatedCell);
	bool SolveWVC(TArray<FCell*>& OriginalCells, TArray<FCell>& CopyCells, TArray<int> CellOrder = TArray<int>());
	void CreateCellMeshes(const TArray<FCell*>& Cells);
	
	
	UPROPERTY(BlueprintReadOnly)
	FVector BaseGridCenter = FVector::ZeroVector;
	
	UPROPERTY(BlueprintReadOnly)
	TArray<FGridPoint> BaseGridPoints;
	
	UPROPERTY(BlueprintReadOnly)
	TArray<FGridQuad> BaseGridQuads;
	
	TArray<TArray<FVector>> DebugOnlyPerfectQuads;
	
	UPROPERTY(BlueprintReadOnly)
	TArray<FGridPoint> BuildingGridPoints;
	
	UPROPERTY(BlueprintReadOnly)
	TArray<FGridShape> BuildingGridShapes;

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	UDebugStrings* DebugStringsComp = nullptr;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	UGridGeneratorVis* GridGeneratorVis = nullptr;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	UDynamicMeshComponent* WholeGridMesh = nullptr;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	UDynamicMeshComponent* HoveredShapeMesh = nullptr;

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	UDataTable* OriginalMeshTable;

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	UDataTable* MeshTable;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	UDataTable* BorderAdjacencyTable;
	
	uint32 IterationsUsed1 = 0;
	uint32 IterationsUsed2 = 0;

	FTimerHandle PerfectEqualSquareTimerHandle;
	FTimerHandle PerfectLocalSquareTimerHandle;
	FTimerHandle SecondRelaxTimerHandle;
	FTimerHandle ThirdRelaxTimerHandle;
	FTimerHandle CreateWholeGridMeshTimerHandle;
	FTimerHandle CreateSecondGridTimerHandle;

	FTimerDelegate PerfectEqualSquareDelegate;
	FTimerDelegate PerfectLocalSquareDelegate;
	FTimerDelegate SecondRelaxDelegate;
	FTimerDelegate ThirdRelaxDelegate;
	FTimerDelegate CreateWholeGridMeshDelegate;
	FTimerDelegate CreateSecondGridDelegate;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int MaxElevation = 10;
	
	UPROPERTY(BlueprintReadOnly)
	TArray<FElevationData> Elevations;
	
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
	TSubclassOf<ABuildingPiece> BuildingPieceToSpawn;


public:
	
};
