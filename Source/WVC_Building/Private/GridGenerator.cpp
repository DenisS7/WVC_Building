// Fill out your copyright notice in the Description page of Project Settings.


#include "GridGenerator.h"

#include "BuildingMeshData.h"
#include "BuildingPiece.h"
#include "DebugStrings.h"
#include "EdgeAdjacencyData.h"
#include "GridGeneratorVis.h"
#include "Polygon2.h"
#include "Components/DynamicMeshComponent.h"
#include "GeometryScript/MeshPrimitiveFunctions.h"
#include "GeometryScript/MeshNormalsFunctions.h"
#include "GeometryScript/MeshModelingFunctions.h"
#include "GeometryScript/MeshSelectionFunctions.h"

// Sets default values
AGridGenerator::AGridGenerator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	DebugStringsComp = CreateDefaultSubobject<UDebugStrings>(TEXT("DebugStringsComponent"));
	GridGeneratorVis = CreateDefaultSubobject<UGridGeneratorVis>(TEXT("GridGeneratorVis"));

	WholeGridMesh = CreateDefaultSubobject<UDynamicMeshComponent>(TEXT("WholeGridMesh"));
	HoveredShapeMesh = CreateDefaultSubobject<UDynamicMeshComponent>(TEXT("SelectedQuadMesh"));

	SecondRelaxDelegate = FTimerDelegate::CreateUObject(this, &AGridGenerator::Relax2);
	ThirdRelaxDelegate = FTimerDelegate::CreateUObject(this, &AGridGenerator::Relax3);
	CreateWholeGridMeshDelegate = FTimerDelegate::CreateUObject(this, &AGridGenerator::CreateWholeGridMesh);
	CreateSecondGridDelegate = FTimerDelegate::CreateUObject(this, &AGridGenerator::CreateSecondGrid);
}

void AGridGenerator::RunWVC(const int Elevation, const int MarchingBitUpdated)
{
	TArray<FCell*> BuildingCells = GetCellsToCheck(Elevation, MarchingBitUpdated);
	ResetCells(BuildingCells);
	CalculateCandidates(BuildingCells);
	LogSuperpositionOptions(BuildingCells);
	TArray<int> CellOrder;
	bool FoundSolution = false;
	TArray<FCell> CopyBuildingCells;
	for(int i = 0; i < BuildingCells.Num(); i++)
	{
		CopyBuildingCells.Add(*BuildingCells[i]);
	}
	UE_LOG(LogTemp, Error, TEXT("11111111111111111111111111111111111111"));
	SolveWVC(BuildingCells, CopyBuildingCells);
	CreateCellMeshes(BuildingCells);
}

UWorld* AGridGenerator::GetGridWorld() const
{
	return GetWorld();
}

// Called when the game starts or when spawned
void AGridGenerator::BeginPlay()
{
	Super::BeginPlay();

	for(int i = 0; i < MaxElevation; i++)
	{
		Elevations.Add(FElevationData(BaseGridPoints.Num()));
		for(int j = 0; j < BaseGridQuads.Num(); j++)
		{
			Elevations.Last().Cells.Add(BaseGridQuads[j].Index, FCell(i, BaseGridQuads[j].Index));
			if(i >= 1)
				Elevations.Last().Cells[BaseGridQuads[j].Index].Neighbours.Add(TPair<int, int>(i - 1, BaseGridQuads[j].Index));
			for(int k = 0; k < BaseGridQuads[j].Neighbours.Num(); k++)
				Elevations.Last().Cells[BaseGridQuads[j].Index].Neighbours.Add(TPair<int, int>(i, BaseGridQuads[j].Neighbours[k]));
			if(i < MaxElevation - 1)
				Elevations.Last().Cells[BaseGridQuads[j].Index].Neighbours.Add(TPair<int, int>(i + 1, BaseGridQuads[j].Index));
		}
	}
	//Elevations.Add(FElevationData(BaseGridPoints.Num()));
}

void AGridGenerator::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	
	//FlushPersistentDebugLines(GetWorld());

	FMath::RandInit(Seed);
	FMath::SRandInit(Seed);
	
	GenerateGrid();
}

// Called every frame
void AGridGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AGridGenerator::GenerateGrid()
{
	BaseGridPoints.Empty();
	BaseGridQuads.Empty();
	DebugOnlyPerfectQuads.Empty();
	Elevations.Empty();
	FlushPersistentDebugLines(GetWorld());
	
	BaseGridCenter = GetActorLocation();
	BaseGridPoints.Emplace(BaseGridPoints.Num(), BaseGridCenter);
	for(uint32 i = 0; i < GridExtent - 1; i++)
	{
		GenerateHexCoordinates(BaseGridCenter, FirstHexSize * (i + 1), i);
	}

	TArray<FGridTriangle> Triangles = DivideGridIntoTriangles(BaseGridCenter);
	DivideGridIntoQuads(BaseGridCenter, Triangles);
	RelaxAndCreateSecondGrid();
	ReorderQuadNeighbours();

	TArray<FColor> Colors = {FColor::Red, FColor::Green, FColor::Blue, FColor::Yellow, FColor::Purple, FColor::Emerald, FColor::Magenta};
	for(int i = 0; i < BuildingGridShapes.Num(); i++)
	{
		for(int j = 0; j < BuildingGridShapes[i].ComposingQuads.Num(); j++)
		{
			for(int k = 0; k < BuildingGridShapes[i].ComposingQuads[j].Points.Num(); k++)
			{
				//DrawDebugLine(GetWorld(), BuildingGridShapes[i].ComposingQuads[j].Points[k], BuildingGridShapes[i].ComposingQuads[j].Points[(k + 1) % 4], FColor::Purple, true, -1, 0, 3.f);
			}
		}
	}
	
	for(int i = 0; i < BaseGridQuads.Num(); i++)
	{
		//DrawDebugBox(GetWorld(), BaseGridQuads[i].Center, FVector(8.f), FColor::Black, true, -1, 0, 5.f);
		//for(int j = 0; j < 4; j++)
		//{
		for(int j = 0; j < 4; j++)
		{
			//DrawDebugSphere(GetWorld(), BaseGridPoints[BaseGridQuads[i].Points[j]].Location, 10.f + j * 3.f, 3, Colors[j], true, -1, 0, 3.f); 
			if(BaseGridQuads[i].OffsetNeighbours[j] != -1)
			{
				const FVector Direction = (BaseGridQuads[BaseGridQuads[i].OffsetNeighbours[j]].Center - BaseGridQuads[i].Center).GetSafeNormal();
				//DrawDebugLine(GetWorld(), BaseGridQuads[i].Center, BaseGridQuads[BaseGridQuads[i].OffsetNeighbours[j]].Center - Direction * 100.f, Colors[j], true, -1, 0, 5.f);
			}
		}

		const FGridQuad& Quad = BaseGridQuads[i];
		for(int j = 0; j < Quad.Points.Num(); j++)
		{
			FVector Pos = (GetBasePointCoordinates(Quad.Points[j]) + GetBasePointCoordinates(Quad.Points[(j + 1) % Quad.Points.Num()])) / 2.f;
			FVector Pos2 = (GetBasePointCoordinates(Quad.Points[(j + 1) % Quad.Points.Num()]) + GetBasePointCoordinates(Quad.Points[(j + 2) % Quad.Points.Num()])) / 2.f;
			Pos.Z = Pos2.Z = 0.f;
			//DrawDebugLine(GetWorld(), Pos, Pos2, Colors[j], true, -1, 0, 3.f);
			//DrawDebugString(GetWorld(), Pos - Quad.Center, FString::FromInt(EdgeCodes[i + 1]), this, FColor::Black, 1.f);
		}
	}
//
	//UE_LOG(LogTemp, Warning, TEXT("\n\n"));
	//for(int i = 0; i < BaseGridQuads[0].Points.Num(); i++)
	//{
	//	UE_LOG(LogTemp, Warning, TEXT("Point: %d  ----- Neighbour: %d"), BaseGridQuads[0].Points[i], BaseGridQuads[0].OffsetNeighbours[i]);
	//}
	//UE_LOG(LogTemp, Warning, TEXT("\n\n"));
}

void AGridGenerator::GenerateHexCoordinates(const FVector& GridCenter, const float HexSize, const uint32 HexIndex)
{
	const uint32 NextIndex = HexIndex + 1;
	const uint32 NumPoints = 6 * NextIndex;
	const int PrevNumPoints = BaseGridPoints.Num();
	BaseGridPoints.SetNum(PrevNumPoints + NumPoints);
	const bool IsEdgePoint = (HexIndex == GridExtent - 2);
	
	//Hex Corners
	for(uint32 i = 0; i < 6; i++)
	{
		const float AngleRad = FMath::DegreesToRadians(60.f * i - 150.f);
		BaseGridPoints[PrevNumPoints + i * NextIndex] = FGridPoint(PrevNumPoints + i * NextIndex,
			FVector(GridCenter.X + HexSize * FMath::Cos(AngleRad), GridCenter.Y + HexSize * FMath::Sin(AngleRad), GridCenter.Z),
			IsEdgePoint);
	}

	//Points on hex edges
	const float FractionDistanceBetween = 1.f / static_cast<float>(NextIndex);
	for(uint32 i = 0; i < 6; i++)
	{
		for(uint32 j = 0; j < HexIndex; j++)
		{
			const int PointIndex = i * NextIndex + j + 1;
			const float FractionDist = static_cast<float>(j + 1) * FractionDistanceBetween;
			BaseGridPoints[PrevNumPoints + PointIndex] = FGridPoint(PrevNumPoints + PointIndex,
				FractionDist * BaseGridPoints[PrevNumPoints + (((i + 1) % 6) * NextIndex)].Location +
				(1.f - FractionDist) * BaseGridPoints[PrevNumPoints + i * NextIndex].Location,
				IsEdgePoint);
		}
	}
}

TArray<FGridTriangle> AGridGenerator::DivideGridIntoTriangles(const FVector& GridCenter)
{
	TArray<FGridTriangle> Triangles;
	if(!BaseGridPoints.Num())
		return Triangles;

	int TriangleIndex = 0;
	for(uint32 i = 0; i < GridExtent - 1; i++)
	{
		const int SmallMaxCoordinate = GetNumberOfPointsOnHex(i);
		const int LargeMaxCoordinate = GetNumberOfPointsOnHex(i + 1);
		const int TrianglesPerSide = i * 2 + 1;
		for(int j = 0; j < 6; j++)
		{
			int SmallCoordinate = j * i;
			int LargeCoordinate = j * (i + 1);
			bool LargeTriangle = true;
			for(int k = 0; k < TrianglesPerSide; k++, LargeTriangle = !LargeTriangle)
			{
				TArray<int>TrianglePointsIndices;
				if(LargeTriangle)
				{
					TrianglePointsIndices.Add(GetIndexOfPointOnHex(i, SmallCoordinate));
					TrianglePointsIndices.Add(GetIndexOfPointOnHex(i + 1, LargeCoordinate));
					TrianglePointsIndices.Add(GetIndexOfPointOnHex(i + 1, (LargeCoordinate + 1) % LargeMaxCoordinate));
					
					LargeCoordinate = (LargeCoordinate + 1) % LargeMaxCoordinate;
				}
				else
				{
					TrianglePointsIndices.Add(GetIndexOfPointOnHex(i, SmallCoordinate));
					TrianglePointsIndices.Add(GetIndexOfPointOnHex(i, (SmallCoordinate + 1) % SmallMaxCoordinate));
					TrianglePointsIndices.Add(GetIndexOfPointOnHex(i + 1, LargeCoordinate));
					
					SmallCoordinate = (SmallCoordinate + 1) % SmallMaxCoordinate;
				}
				
				TArray<int> Neighbours;

				//Last triangle
				if(j == 5 && k == TrianglesPerSide - 1)
				{
					Neighbours.Add(GetFirstTriangleIndexOnHex(i));
				}
				else
				{
					Neighbours.Add(TriangleIndex + 1);
				}
				
				//First triangle
				if(j == 0 && k == 0)
				{
					Neighbours.Add(GetFirstTriangleIndexOnHex(i + 1) - 1);
				}
				else
				{
					Neighbours.Add(TriangleIndex - 1);
				}
				
				if(k % 2 == 0)
				{
					if(GetFirstPointIndexOnHex(i + 3) < BaseGridPoints.Num() - 1)
						Neighbours.Add(TriangleIndex + 6 * (2 * i + 1) + 1 + 2 * j);
				}
				else
				{
					if(i > 0)
						Neighbours.Add(TriangleIndex - 6 * (2 * i - 1) - 1 - 2 * j);
				}
				Triangles.Emplace(TrianglePointsIndices, TriangleIndex++, Neighbours);
			}
		}
	}
	
	return Triangles;
}

void AGridGenerator::DivideGridIntoQuads(const FVector& GridCenter, TArray<FGridTriangle>& Triangles)
{
	TArray<int> AvailableTriangles;
	TArray<int> TrianglesLeft;
	for(int i = 0; i < Triangles.Num(); i++)
		AvailableTriangles.Add(i);
	int QuadIndex = 0;
	TArray<FGridQuad> Quads;
	while(AvailableTriangles.Num())
	{
		const int RandomTriangle = AvailableTriangles[FMath::RandRange(0, AvailableTriangles.Num() - 1)];
		FGridTriangle& CurrentTriangle = Triangles[RandomTriangle]; 
		TArray<int> Neighbours = CurrentTriangle.Neighbours;
		while(Neighbours.Num())
		{
			const int RandomNeighbour = Neighbours[FMath::RandRange(0, Neighbours.Num() - 1)];
			FGridTriangle& NeighbourTriangle = Triangles[RandomNeighbour]; 
			if(NeighbourTriangle.FormsQuad)
			{
				Neighbours.RemoveSingle(RandomNeighbour);
				continue;
			}
			TArray<int> QuadPoints;
			for(int j = 0; j < 3; j++)
			{
				QuadPoints.AddUnique(CurrentTriangle.Points[j]);
				QuadPoints.AddUnique(NeighbourTriangle.Points[j]);
			}
			Quads.Add({QuadPoints, QuadIndex++});
			SortQuadPoints(Quads.Last());
			CurrentTriangle.FormsQuad = true;
			NeighbourTriangle.FormsQuad = true;
			AvailableTriangles.RemoveSingle(NeighbourTriangle.Index);
			break;
		}
		if(!CurrentTriangle.FormsQuad)
			TrianglesLeft.Add(RandomTriangle);
		AvailableTriangles.RemoveSingle(RandomTriangle);
	}
	
	int FinalQuadIndex = 0;
	TMap<TPair<int, int>, int> Midpoints;
	for(int i = 0; i < TrianglesLeft.Num(); i++)
	{
		const TArray<int>& TrPoints = Triangles[TrianglesLeft[i]].Points;
		FVector TriangleCenter = FVector::ZeroVector;
		for(int j = 0; j < 3; j++)
			TriangleCenter += BaseGridPoints[Triangles[TrianglesLeft[i]].Points[j]].Location;
		TriangleCenter /= 3.f;
		TArray<int> TriangleMidpoints;
		for(int j = 0; j < 3; j++)
		{
			TriangleMidpoints.Add(GetOrAddMidpointIndexInBaseGrid(Midpoints, TrPoints[j], TrPoints[(j + 1) % 3]));
		}
		BaseGridPoints.Emplace(BaseGridPoints.Num(), TriangleCenter);

		TArray<int> Quad1Points = {TrPoints[0], TriangleMidpoints[0], BaseGridPoints.Num() - 1, TriangleMidpoints[2]};
		TArray<int> Quad2Points = {TrPoints[1], TriangleMidpoints[1], BaseGridPoints.Num() - 1, TriangleMidpoints[0]};
		TArray<int> Quad3Points = {TrPoints[2], TriangleMidpoints[2], BaseGridPoints.Num() - 1, TriangleMidpoints[1]};
		TArray<TArray<int>> QuadsPoints = {Quad1Points, Quad2Points, Quad3Points};
		for(int j = 0; j < 3; j++)
		{
			BaseGridQuads.Add({QuadsPoints[j], FinalQuadIndex++});
			SortQuadPoints(BaseGridQuads.Last());
			FindPointNeighboursInQuad(BaseGridQuads.Num() - 1);
			for(int k = 0; k < QuadsPoints[j].Num(); k++)
			{
				BaseGridPoints[QuadsPoints[j][k]].PartOfQuads.Add(FinalQuadIndex - 1);
			}
		}
	}
	for(int i = 0; i < Quads.Num(); i++)
	{
		TArray<int> QuadMidpoints;
		for(int j = 0; j < 4; j++)
		{
			QuadMidpoints.Add(GetOrAddMidpointIndexInBaseGrid(Midpoints, Quads[i].Points[j], Quads[i].Points[(j + 1) % 4]));
		}
		BaseGridPoints.Emplace(BaseGridPoints.Num(), Quads[i].Center);
		
		TArray<int> Quad1Points = {Quads[i].Points[0], QuadMidpoints[0], BaseGridPoints.Num() - 1, QuadMidpoints[3]};
		TArray<int> Quad2Points = {Quads[i].Points[1], QuadMidpoints[1], BaseGridPoints.Num() - 1, QuadMidpoints[0]};
		TArray<int> Quad3Points = {Quads[i].Points[2], QuadMidpoints[2], BaseGridPoints.Num() - 1, QuadMidpoints[1]};
		TArray<int> Quad4Points = {Quads[i].Points[3], QuadMidpoints[3], BaseGridPoints.Num() - 1, QuadMidpoints[2]};
		TArray<TArray<int>> QuadsPoints = {Quad1Points, Quad2Points, Quad3Points, Quad4Points};
		for(int j = 0; j < 4; j++)
		{
			BaseGridQuads.Add({QuadsPoints[j], FinalQuadIndex++});
			SortQuadPoints(BaseGridQuads.Last());
			FindPointNeighboursInQuad(BaseGridQuads.Num() - 1);
			for(int k = 0; k < QuadsPoints[j].Num(); k++)
			{
				BaseGridPoints[QuadsPoints[j][k]].PartOfQuads.Add(FinalQuadIndex - 1);
			}
		}
	}

	for(int i = 0; i < BaseGridPoints.Num(); i++)
	{
		FGridShape Shape(-1, BaseGridPoints[i].Neighbours, -1);
		SortShapePoints(Shape, false);
		BaseGridPoints[i].Neighbours = Shape.Points;
	}
	
	for(int i = 0; i < BaseGridQuads.Num() - 1; i++)
	{
		for(int j = i + 1; j < BaseGridQuads.Num(); j++)
		{
			int CommonPoints = 0;
			for(int k = 0; k < 4; k++)
			{
				bool Found = false;
				for(int p = 0; p < 4; p++)
				{
					if(BaseGridQuads[i].Points[k] == BaseGridQuads[j].Points[p])
					{
						if(++CommonPoints == 2)
						{
							BaseGridQuads[i].Neighbours.Add(j);
							BaseGridQuads[j].Neighbours.Add(i);
							Found = true;
							break;
						}
					}
				}
				if(Found)
				{
					break;
				}
			}
		}
	}
}

void AGridGenerator::CreateWholeGridMesh()
{
	FGridShape WholeMeshShape;
	for(int i = 0; i < BaseGridPoints.Num(); i++)
	{
		if(BaseGridPoints[i].IsEdge)
			WholeMeshShape.Points.Add(i);
	}
	SortShapePoints(WholeMeshShape, false);
	UE::Geometry::FPolygon2d MeshPolygon;
	for(int i = 0; i < WholeMeshShape.Points.Num(); i++)
	{
		MeshPolygon.AppendVertex(UE::Math::TVector2(BaseGridPoints[WholeMeshShape.Points[i]].Location));
	}
	if(MeshPolygon.IsClockwise())
		MeshPolygon.Reverse();
	WholeGridMesh->GetDynamicMesh()->Reset();
	float MeshHeight = 10.f;
	FTransform MeshTransform = FTransform();
	MeshTransform.SetLocation(FVector(MeshTransform.GetLocation().X, MeshTransform.GetLocation().Y, GetActorLocation().Z - MeshHeight));
	UGeometryScriptLibrary_MeshPrimitiveFunctions::AppendSimpleExtrudePolygon(WholeGridMesh->GetDynamicMesh(),
	FGeometryScriptPrimitiveOptions(),
	MeshTransform,
	MeshPolygon.GetVertices(),
	MeshHeight,
	5);
	WholeGridMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	UBodySetup* BodySetup = WholeGridMesh->GetBodySetup();
	BodySetup->CollisionTraceFlag = CTF_UseComplexAsSimple;
	WholeGridMesh->EnableComplexAsSimpleCollision();;
	BodySetup->AggGeom.ConvexElems.Empty();
	BodySetup->CreatePhysicsMeshes(); 
	WholeGridMesh->RecreatePhysicsState();
	UGeometryScriptLibrary_MeshNormalsFunctions::ComputeSplitNormals(WholeGridMesh->GetDynamicMesh(), FGeometryScriptSplitNormalsOptions(), FGeometryScriptCalculateNormalsOptions());
}

void AGridGenerator::CreateGridShapeMesh(const int ShapeIndex)
{
	UE::Geometry::FPolygon2d MeshPolygon;
	for(int i = 0; i < BuildingGridShapes[ShapeIndex].Points.Num(); i++)
	{
		MeshPolygon.AppendVertex(UE::Math::TVector2(BuildingGridPoints[BuildingGridShapes[ShapeIndex].Points[i]].Location));
	}
	if(MeshPolygon.IsClockwise())
		MeshPolygon.Reverse();
	HoveredShapeMesh->GetDynamicMesh()->Reset();
	float MeshHeight = 10.f;
	FTransform MeshTransform = FTransform();
	MeshTransform.SetLocation(FVector(MeshTransform.GetLocation().X, MeshTransform.GetLocation().Y, GetActorLocation().Z - MeshHeight));
	UGeometryScriptLibrary_MeshPrimitiveFunctions::AppendSimpleExtrudePolygon(HoveredShapeMesh->GetDynamicMesh(),
	FGeometryScriptPrimitiveOptions(),
	MeshTransform,
	MeshPolygon.GetVertices(),
	MeshHeight,
	5);
	HoveredShapeMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	UBodySetup* BodySetup = HoveredShapeMesh->GetBodySetup();
	BodySetup->CollisionTraceFlag = CTF_UseComplexAsSimple;
	HoveredShapeMesh->EnableComplexAsSimpleCollision();
	BodySetup->AggGeom.ConvexElems.Empty();
	BodySetup->CreatePhysicsMeshes(); 
	HoveredShapeMesh->RecreatePhysicsState();
	UGeometryScriptLibrary_MeshNormalsFunctions::ComputeSplitNormals(HoveredShapeMesh->GetDynamicMesh(), FGeometryScriptSplitNormalsOptions(), FGeometryScriptCalculateNormalsOptions());
}

void AGridGenerator::CreateAdjacentShapeMesh(const TArray<FVector>& Points)
{
	FVector Center;
	for(int i = 0; i < Points.Num(); i++)
		Center += Points[i];
	Center /= Points.Num();
	TArray<FVector> NewPoints;
	for(int i = 0; i < Points.Num(); i++)
		NewPoints.Add(Points[i] - Center);
	TArray<FVector> MeshPolygon;
	for(int i = 0; i < NewPoints.Num(); i++)
	{
		MeshPolygon.Add(NewPoints[i]);
	}
	HoveredShapeMesh->GetDynamicMesh()->Reset();
	//float MeshHeight = 10.f;
	FTransform MeshTransform = FTransform();
	MeshTransform.SetLocation(Center);//FVector(MeshTransform.GetLocation().X, MeshTransform.GetLocation().Y, GetActorLocation().Z));
	//UGeometryScriptLibrary_MeshPrimitiveFunctions::Append
	
	UGeometryScriptLibrary_MeshPrimitiveFunctions::AppendTriangulatedPolygon3D(HoveredShapeMesh->GetDynamicMesh(),
	FGeometryScriptPrimitiveOptions(),
	MeshTransform,
	MeshPolygon);
	FGeometryScriptMeshSelection Selection;
	
	UGeometryScriptLibrary_MeshSelectionFunctions::CreateSelectAllMeshSelection(HoveredShapeMesh->GetDynamicMesh(), Selection);
	FVector FaceNormal = UE::Geometry::Cross(Points[1] - Points[0], Points[2] - Points[0]);
	FaceNormal.Normalize();
	
	FGeometryScriptMeshLinearExtrudeOptions LinearExtrudeOptions(5.f, EGeometryScriptLinearExtrudeDirection::AverageFaceNormal, FaceNormal);
	UGeometryScriptLibrary_MeshModelingFunctions::ApplyMeshLinearExtrudeFaces(HoveredShapeMesh->GetDynamicMesh(), LinearExtrudeOptions, Selection);

	HoveredShapeMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	UBodySetup* BodySetup = HoveredShapeMesh->GetBodySetup();
	BodySetup->CollisionTraceFlag = CTF_UseComplexAsSimple;
	HoveredShapeMesh->EnableComplexAsSimpleCollision();
	BodySetup->AggGeom.ConvexElems.Empty();
	BodySetup->CreatePhysicsMeshes(); 
	HoveredShapeMesh->RecreatePhysicsState();
	
	UGeometryScriptLibrary_MeshNormalsFunctions::ComputeSplitNormals(HoveredShapeMesh->GetDynamicMesh(), FGeometryScriptSplitNormalsOptions(), FGeometryScriptCalculateNormalsOptions());

}

void AGridGenerator::ResetShapeMesh()
{
	HoveredShapeMesh->GetDynamicMesh()->Reset();
}

void AGridGenerator::CreateSecondGrid()
{
	BuildingGridShapes.Empty();
	BuildingGridPoints.Empty();
	for(int i = 0; i < BaseGridQuads.Num(); i++)
	{
		bool IsEdge = false;
		for(int j = 0; j < BaseGridQuads[i].Points.Num(); j++)
		{
			if(BaseGridPoints[BaseGridQuads[i].Points[j]].IsEdge)
			{
				IsEdge = true;
				break;
			}
		}
		BuildingGridPoints.Emplace(i, BaseGridQuads[i].Center, IsEdge);
	}
	
	int NeighbourOffset = 0;
	for(int i = 0; i < BaseGridPoints.Num(); i++)
	{
		if(BaseGridPoints[i].IsEdge)
		{
			NeighbourOffset++;
			continue;
		}
		TArray<int> ShapePoints;
		for(int j = 0; j < BaseGridPoints[i].PartOfQuads.Num(); j++)
		{
			ShapePoints.Add(BaseGridPoints[i].PartOfQuads[j]);
		}
		BuildingGridShapes.Emplace(BuildingGridShapes.Num(), ShapePoints, i);
		BaseGridPoints[i].CorrespondingBuildingShape = BuildingGridShapes.Num() - 1;
		SortShapePoints(BuildingGridShapes.Last());
	}
	
	//Neighbours
	for(int i = 0; i < BuildingGridShapes.Num(); i++)
	{
		//Put shape points and neighbours in same order
		TArray<int> ShapePoints;
		for(int j = 0; j < BaseGridPoints[BuildingGridShapes[i].CorrespondingBaseGridPoint].Neighbours.Num(); j++)
		{
			//if(!BaseGridPoints[BaseGridPoints[BuildingGridShapes[i].CorrespondingBaseGridPoint].Neighbours[j]].IsEdge)
			ShapePoints.Add(BaseGridPoints[BuildingGridShapes[i].CorrespondingBaseGridPoint].Neighbours[j]);
		}
		FGridShape Shape(-1, ShapePoints, -1);
		SortShapePoints(Shape, false);
		//TArray<int> AllNeighbourArray;
		TArray<int> NewNeighbourArray;
		for(int j = 0; j < Shape.Points.Num(); j++)
		{
			//if(!BaseGridPoints[Shape.Points[j]].IsEdge)
				NewNeighbourArray.Add(BaseGridPoints[Shape.Points[j]].CorrespondingBuildingShape);
			//if(BaseGridPoints[Shape.Points[j]].IsEdge)
			//AllNeighbourArray.Add(BaseGridPoints[Shape.Points[j]].CorrespondingBuildingShape);
		}

		const FVector Point1 = BuildingGridPoints[BuildingGridShapes[i].Points[0]].Location - BuildingGridShapes[i].Center;
		float LeastAngle = -999999999999999.f;
		int Neighbour1 = -1;
		for(int j = 0; j < NewNeighbourArray.Num(); j++)
		{
			const int NeighbourIndex = NewNeighbourArray[j];

			if(NeighbourIndex == -1)
			{
				continue;
			}
			
			const FVector CenterNeighbour = BuildingGridShapes[NeighbourIndex].Center - BuildingGridShapes[i].Center;
			const float Dot = UE::Geometry::Dot(Point1.GetSafeNormal(), CenterNeighbour.GetSafeNormal());
			const FVector Cross = UE::Geometry::Cross(Point1.GetSafeNormal(), CenterNeighbour.GetSafeNormal());

			float Angle = FMath::Atan2(Cross.Z, Dot);
			if(Angle < 0.f)
				Angle += 2.f * PI;

			if(Angle > LeastAngle)
			{
				LeastAngle = Angle;
				Neighbour1 = j;
			}
		}
		//NewNeighbourArray.Empty();
		//AllNeighbourArray.Empty();

		BuildingGridShapes[i].Neighbours.Empty();
		BuildingGridShapes[i].OffsetNeighbours.Empty();
		for(int j = Neighbour1; j < Shape.Points.Num() + Neighbour1; j++)
		{
			const int PointIndex = Shape.Points[j % Shape.Points.Num()];
			if(PointIndex == -1)
				continue;
			const int NeighbourIndex = BaseGridPoints[PointIndex].CorrespondingBuildingShape;
			if(NeighbourIndex == -1)
				continue;
			BuildingGridShapes[i].Neighbours.Add(NeighbourIndex);
			BuildingGridShapes[i].OffsetNeighbours.Add(NeighbourIndex);
		}

		for(int j = 0; j < BuildingGridShapes[i].Points.Num(); j++)
		{
			if(BuildingGridPoints[BuildingGridShapes[i].Points[j]].IsEdge && BuildingGridPoints[BuildingGridShapes[i].Points[(j + 1) % BuildingGridShapes[i].Points.Num()]].IsEdge)
			{
				BuildingGridShapes[i].OffsetNeighbours.Insert(-1, j);
			}
		}
		//BuildingGridShapes[i].Neighbours = NewNeighbourArray;
		//BuildingGridShapes[i].OffsetNeighbours = AllNeighbourArray;
		
		//Create the composing quads
		for(int j = 0; j < BuildingGridShapes[i].Points.Num(); j++)
		{
			TArray<FVector> Quad;
			//Building is not on the edge of the grid
			if(BuildingGridShapes[i].Neighbours.Num() == BuildingGridShapes[i].Points.Num())
			{
				Quad.Add(BuildingGridPoints[BuildingGridShapes[i].Points[j]].Location);
				const FVector CurrentShapeLoc = BaseGridPoints[BuildingGridShapes[i].CorrespondingBaseGridPoint].Location;
				const FVector NextNeighbourLoc = BaseGridPoints[BuildingGridShapes[BuildingGridShapes[i].Neighbours[j]].CorrespondingBaseGridPoint].Location;
				int PrevNeighbourIndex = j - 1;
				if(PrevNeighbourIndex < 0)
					PrevNeighbourIndex += BuildingGridShapes[i].Neighbours.Num();
				const FVector PrevNeighbourLoc = BaseGridPoints[BuildingGridShapes[BuildingGridShapes[i].Neighbours[PrevNeighbourIndex]].CorrespondingBaseGridPoint].Location;
				Quad.Add((CurrentShapeLoc + PrevNeighbourLoc) / 2.f);
				Quad.Add(CurrentShapeLoc);
				Quad.Add((CurrentShapeLoc + NextNeighbourLoc) / 2.f);
			}
			else
			{
				Quad.Add(BuildingGridPoints[BuildingGridShapes[i].Points[j]].Location);
				const FVector CurrentShapeLoc = BaseGridPoints[BuildingGridShapes[i].CorrespondingBaseGridPoint].Location;
				int PrevInvalidNeighbours = 0;
				int Index = 0;
				while(Index <= j && BuildingGridShapes[i].OffsetNeighbours[Index] == -1)
				{
					PrevInvalidNeighbours++;
					Index++;
				}
				int NextNeighbourIndex = (j + Neighbour1 - PrevInvalidNeighbours) % Shape.Points.Num();
				if(NextNeighbourIndex < 0)
					NextNeighbourIndex += Shape.Points.Num();
				const FVector NextNeighbourLoc = BaseGridPoints[Shape.Points[NextNeighbourIndex]].Location;
				int PrevNeighbourIndex = j - 1 + Neighbour1 - PrevInvalidNeighbours;
				if(PrevNeighbourIndex < 0)
					PrevNeighbourIndex += Shape.Points.Num();
				else
					PrevNeighbourIndex %= Shape.Points.Num();
				const FVector PrevNeighbourLoc = BaseGridPoints[Shape.Points[PrevNeighbourIndex]].Location;
				Quad.Add((CurrentShapeLoc + PrevNeighbourLoc) / 2.f);
				Quad.Add(CurrentShapeLoc);
				Quad.Add((CurrentShapeLoc + NextNeighbourLoc) / 2.f);
			}
			BuildingGridShapes[i].ComposingQuads.Emplace(Quad);
		}
	}
}

void AGridGenerator::RelaxGridBasedOnPerfectEqualSquare(const float SquareSideLength)
{
	if(Debug)
	{
		IterationsUsed1 = 0;
		{
			PerfectEqualSquareDelegate.BindWeakLambda(this, [this, SquareSideLength]()
			{
				if(IterationsUsed1 >= PerfectEqualSquareRelaxIterations)
				{
					GetWorld()->GetTimerManager().ClearTimer(PerfectEqualSquareTimerHandle);
					PerfectEqualSquareTimerHandle.Invalidate();
					return;
				}
				const float r = (SquareSideLength * sqrt(2.f)) / 2.f;
				TArray<FVector> GridPointsForce;
				GridPointsForce.SetNumZeroed(BaseGridPoints.Num());
				DebugOnlyPerfectQuads.Empty();
				for(int i = 0; i < BaseGridQuads.Num(); i++)
				{
					TArray<int>& QuadPoints = BaseGridQuads[i].Points;
					FVector NewCenter = FVector::ZeroVector;
					for(int j = 0; j < 4; j++)
						NewCenter += BaseGridPoints[QuadPoints[j]].Location;
					 NewCenter /= 4.f;
					 BaseGridQuads[i].Center = NewCenter;
					 float Alpha = 0.f;
					 for(int k = 0; k <= 1; k++)
					 {
						 float Numerator = BaseGridPoints[QuadPoints[0]].Location.Y + BaseGridPoints[QuadPoints[1]].Location.X - BaseGridPoints[QuadPoints[2]].Location.Y - BaseGridPoints[QuadPoints[3]].Location.X;
						  float Denominator = BaseGridPoints[QuadPoints[0]].Location.X - BaseGridPoints[QuadPoints[1]].Location.Y - BaseGridPoints[QuadPoints[2]].Location.X + BaseGridPoints[QuadPoints[3]].Location.Y;
						  Alpha = FMath::Atan( Numerator / Denominator) + static_cast<float>(k) * PI;
						  float SecondDerivative = 2.f * r * FMath::Cos(Alpha) * Denominator + 2.f * r * FMath::Sin(Alpha) * Numerator;
						  if(SecondDerivative > 0.f)
						  {
							  break;
						  }
					 }
					 TArray<FVector> SquarePoints;
					 float CosAlpha = FMath::Cos(Alpha);
					 float SinAlpha = FMath::Sin(Alpha);
					 SquarePoints.Emplace(FVector(r * CosAlpha, r * SinAlpha, 0.f) + BaseGridQuads[i].Center);
					 SquarePoints.Emplace(FVector(r * SinAlpha, -r * CosAlpha, 0.f) + BaseGridQuads[i].Center);
					 SquarePoints.Emplace(FVector(-r * CosAlpha, -r * SinAlpha, 0.f) + BaseGridQuads[i].Center);
					 SquarePoints.Emplace(FVector(-r * SinAlpha, r * CosAlpha, 0.f) + BaseGridQuads[i].Center);
		
					 for(int k = 0; k < 4; k++)
					 {	
						 FVector Diff = SquarePoints[k] - BaseGridPoints[QuadPoints[k]].Location;
						 Diff.Z = 0.f;
						 GridPointsForce[QuadPoints[k]] += Diff;
					 }
					 DebugOnlyPerfectQuads.Add(SquarePoints);
				 }
				 for(int i = 0; i < BaseGridPoints.Num(); i++)
				 {
					 BaseGridPoints[i].Location += GridPointsForce[i] * 0.1f;
					 GridPointsForce[i] = FVector::ZeroVector;
				 }
				 DrawGrid();
				 IterationsUsed1++;
			 });
		}
		GetWorld()->GetTimerManager().SetTimer(PerfectEqualSquareTimerHandle, PerfectEqualSquareDelegate, Order1TimeRate, true);
	}
	else
	{
		for(uint32 Iteration = 0; Iteration < PerfectEqualSquareRelaxIterations; Iteration++)
		{
			const float r = (SquareSideLength * sqrt(2.f)) / 2.f;
			TArray<FVector> GridPointsForce;
			GridPointsForce.SetNumZeroed(BaseGridPoints.Num());
			DebugOnlyPerfectQuads.Empty();
			for(int i = 0; i < BaseGridQuads.Num(); i++)
			{
				TArray<int>& QuadPoints = BaseGridQuads[i].Points;
				FVector NewCenter = FVector::ZeroVector;
				for(int j = 0; j < 4; j++)
					NewCenter += BaseGridPoints[QuadPoints[j]].Location;
				NewCenter /= 4.f;
				BaseGridQuads[i].Center = NewCenter;
				float Alpha = 0.f;
				for(int k = 0; k <= 1; k++)
				{
				 float Numerator = BaseGridPoints[QuadPoints[0]].Location.Y + BaseGridPoints[QuadPoints[1]].Location.X - BaseGridPoints[QuadPoints[2]].Location.Y - BaseGridPoints[QuadPoints[3]].Location.X;
				  float Denominator = BaseGridPoints[QuadPoints[0]].Location.X - BaseGridPoints[QuadPoints[1]].Location.Y - BaseGridPoints[QuadPoints[2]].Location.X + BaseGridPoints[QuadPoints[3]].Location.Y;
				  Alpha = FMath::Atan( Numerator / Denominator) + static_cast<float>(k) * PI;
				  float SecondDerivative = 2.f * r * FMath::Cos(Alpha) * Denominator + 2.f * r * FMath::Sin(Alpha) * Numerator;
				  if(SecondDerivative > 0.f)
				  {
					  break;
				  }
				}
				TArray<FVector> SquarePoints;
				float CosAlpha = FMath::Cos(Alpha);
				float SinAlpha = FMath::Sin(Alpha);
				SquarePoints.Emplace(FVector(r * CosAlpha, r * SinAlpha, 0.f) + BaseGridQuads[i].Center);
				SquarePoints.Emplace(FVector(r * SinAlpha, -r * CosAlpha, 0.f) + BaseGridQuads[i].Center);
				SquarePoints.Emplace(FVector(-r * CosAlpha, -r * SinAlpha, 0.f) + BaseGridQuads[i].Center);
				SquarePoints.Emplace(FVector(-r * SinAlpha, r * CosAlpha, 0.f) + BaseGridQuads[i].Center);
	
				for(int k = 0; k < 4; k++)
				{	
				 FVector Diff = SquarePoints[k] - BaseGridPoints[QuadPoints[k]].Location;
				 Diff.Z = 0.f;
				 GridPointsForce[QuadPoints[k]] += Diff.GetSafeNormal();
				}
				DebugOnlyPerfectQuads.Add(SquarePoints);
			 }
			 for(int i = 0; i < BaseGridPoints.Num(); i++)
			 {
				 BaseGridPoints[i].Location += GridPointsForce[i];
				 GridPointsForce[i] = FVector::ZeroVector;
			 }
			 //DrawGrid();
		}
	}
}

void AGridGenerator::RelaxGridBasedOnPerfectLocalSquare()
{
	IterationsUsed2 = 0;
	if(Debug)
	{
		PerfectLocalSquareDelegate.BindWeakLambda(this, [this]()
		{
			if(IterationsUsed2 >= PerfectLocalSquareRelaxIterations)
			{
				GetWorld()->GetTimerManager().ClearTimer(PerfectLocalSquareTimerHandle);
				PerfectLocalSquareTimerHandle.Invalidate();
				return;
			}
			TArray<FVector> GridPointsForce;
			GridPointsForce.SetNumZeroed(BaseGridPoints.Num());
			for(int i = 0; i < BaseGridQuads.Num(); i++)
			{
				TArray<int>& QuadPoints = BaseGridQuads[i].Points;
				FVector NewCenter = FVector::ZeroVector;
				for(int j = 0; j < 4; j++)
					NewCenter += BaseGridPoints[QuadPoints[j]].Location;
				NewCenter /= 4.f;
				BaseGridQuads[i].Center = NewCenter;
				FVector QuadForce = FVector::ZeroVector;
				for(int j = 0; j < 4; j++)
				{
					QuadForce += GetBasePointCoordinates(QuadPoints[j]) - NewCenter;
					QuadForce = FVector(QuadForce.Y, -QuadForce.X, 0.f);
				}
				QuadForce /= 4.f;
				for(int j = 0; j < 4; j++)
				{
					GridPointsForce[QuadPoints[j]] += NewCenter + QuadForce - GetBasePointCoordinates(QuadPoints[j]);
					QuadForce = FVector(QuadForce.Y, -QuadForce.X, 0.f);
				}
			}
			for(int i = 0; i < BaseGridPoints.Num(); i++)
			{
				BaseGridPoints[i].Location += GridPointsForce[i] * ForceScale;
		   	}
			DrawGrid();
			IterationsUsed2++;
		});
		
		GetWorld()->GetTimerManager().SetTimer(PerfectLocalSquareTimerHandle, PerfectLocalSquareDelegate, Order2TimeRate, true);
	}
	else
	{
		for(uint32 Iteration = 0; Iteration < PerfectLocalSquareRelaxIterations; Iteration++)
		{
			TArray<FVector> GridPointsForce;
			GridPointsForce.SetNumZeroed(BaseGridPoints.Num());
			for(int i = 0; i < BaseGridQuads.Num(); i++)
			{
				TArray<int>& QuadPoints = BaseGridQuads[i].Points;
				FVector NewCenter = FVector::ZeroVector;
				for(int j = 0; j < 4; j++)
					NewCenter += BaseGridPoints[QuadPoints[j]].Location;
				NewCenter /= 4.f;
				BaseGridQuads[i].Center = NewCenter;
				FVector QuadForce = FVector::ZeroVector;
				for(int j = 0; j < 4; j++)
				{
					QuadForce += GetBasePointCoordinates(QuadPoints[j]) - NewCenter;
					QuadForce = FVector(QuadForce.Y, -QuadForce.X, 0.f);
				}
				QuadForce /= 4.f;
				for(int j = 0; j < 4; j++)
				{
					GridPointsForce[QuadPoints[j]] += NewCenter + QuadForce - GetBasePointCoordinates(QuadPoints[j]);
					QuadForce = FVector(QuadForce.Y, -QuadForce.X, 0.f);
				}
			}
			for(int i = 0; i < BaseGridPoints.Num(); i++)
			{
				BaseGridPoints[i].Location += GridPointsForce[i] * ForceScale;
			}
		}
	}
}

void AGridGenerator::RelaxGridBasedOnNeighbours()
{
	for(uint32 Iteration = 0; Iteration < NeighbourRelaxIterations; Iteration++)
	{
		TArray<FGridPoint> NewGridPoints = BaseGridPoints;
		NewGridPoints.SetNum(BaseGridPoints.Num());
		for(int i = 0; i < BaseGridPoints.Num(); i++)
		{
			FVector Sum = FVector::ZeroVector;
			for(int j = 0; j < BaseGridPoints[i].Neighbours.Num(); j++)
			{
				Sum += BaseGridPoints[BaseGridPoints[i].Neighbours[j]].Location;
			}
			NewGridPoints[i].Location = Sum / static_cast<float>(BaseGridPoints[i].Neighbours.Num());
		}
		BaseGridPoints = NewGridPoints;
	}
}

void AGridGenerator::RelaxAndCreateSecondGrid()
{
	Relax1();
	if(Debug)
	{
		uint32 It1 = 0;
		if(PerfectEqualSquareOrder == 1)
			It1 = PerfectEqualSquareRelaxIterations;
		else if (PerfectLocalSquareOrder == 1)
			It1 = PerfectLocalSquareRelaxIterations;
		else if (NeighbourOrder == 1)
			It1 = NeighbourRelaxIterations;
		
		uint32 It2 = 0;
		if(PerfectEqualSquareOrder == 2)
			It2 = PerfectEqualSquareRelaxIterations;
		else if (PerfectLocalSquareOrder == 2)
			It2 = PerfectLocalSquareRelaxIterations;
		else if (NeighbourOrder == 2)
			It2 = NeighbourRelaxIterations;
	
		uint32 It3 = 0;
		if(PerfectEqualSquareOrder == 2)
			It3 = PerfectEqualSquareRelaxIterations;
		else if (PerfectLocalSquareOrder == 2)
			It3 = PerfectLocalSquareRelaxIterations;
		else if (NeighbourOrder == 2)
			It3 = NeighbourRelaxIterations;
		
		const float TimerDuration1 = Order1TimeRate * (static_cast<float>(It1) + 1.f);
		const float TimerDuration2 = TimerDuration1 + Order2TimeRate * (static_cast<float>(It2) + 1.f);
		const float TimerDuration3 = TimerDuration2 + Order3TimeRate * (static_cast<float>(It3) + 1.f);
		
		GetWorld()->GetTimerManager().SetTimer(
			SecondRelaxTimerHandle,
			SecondRelaxDelegate,
			TimerDuration1,
			false,
			TimerDuration1);
		
		GetWorld()->GetTimerManager().SetTimer(
			ThirdRelaxTimerHandle,
			ThirdRelaxDelegate,
			TimerDuration2,
			false,
			TimerDuration2);
		
		GetWorld()->GetTimerManager().SetTimer(
			CreateWholeGridMeshTimerHandle,
			CreateWholeGridMeshDelegate,
			TimerDuration3,
			false,
			TimerDuration3);

		GetWorld()->GetTimerManager().SetTimer(
			CreateSecondGridTimerHandle,
			CreateSecondGridDelegate,
			TimerDuration3 + 0.1f,
			false,
			TimerDuration3);
	}
	else
	{
		Relax2();
		Relax3();
		CreateWholeGridMesh();
		CreateSecondGrid();
	}
}

void AGridGenerator::ReorderQuadNeighbours()
{
	for(int i = 0; i < BaseGridQuads.Num(); i++)
	{
		//Ordering neighbours in counter clockwise order
		FGridQuad& CurrentQuad = BaseGridQuads[i];
		TArray<FVector> NeighbourCenters;
		TArray<int> NewNeighbourOrder;

		for(int j = 0; j < CurrentQuad.Neighbours.Num(); j++)
			NeighbourCenters.Add(BaseGridQuads[CurrentQuad.Neighbours[j]].Center);

		SortPoints(NeighbourCenters, NewNeighbourOrder);
		TArray<int> NeighbourCopy;
		
		for(int j = 0; j < NewNeighbourOrder.Num(); j++)
		{
			NeighbourCopy.Add(CurrentQuad.Neighbours[NewNeighbourOrder[j]]);
		}

		const FVector Point1 = BaseGridPoints[CurrentQuad.Points[0]].Location - CurrentQuad.Center;
		float LeastAngle = -9999999999999.f;
		int Neighbour1 = 0;

		for(int j = 0; j < NeighbourCopy.Num(); j++)
		{
			if(NeighbourCopy[j] == -1)
				continue;
			const int NeighbourIndex = NeighbourCopy[j];
			const FVector CenterNeighbour = BaseGridQuads[NeighbourIndex].Center - CurrentQuad.Center;
			const float Dot = UE::Geometry::Dot(Point1.GetSafeNormal(), CenterNeighbour.GetSafeNormal());
			const FVector Cross = UE::Geometry::Cross(Point1.GetSafeNormal(), CenterNeighbour.GetSafeNormal());

			float Angle = FMath::Atan2(Cross.Z, Dot);
			if(Angle < 0.f)
				Angle += 2.f * PI;

			if(Angle > LeastAngle)
			{
				LeastAngle = Angle;
				Neighbour1 = j;
			}
		}

		CurrentQuad.Neighbours.Empty();
		CurrentQuad.OffsetNeighbours.Empty();
		for(int j = Neighbour1; j < NeighbourCopy.Num() + Neighbour1; j++)
		{
			const int NeighbourIndex = NeighbourCopy[j % NeighbourCopy.Num()];
			if(NeighbourIndex != -1)
				CurrentQuad.Neighbours.Add(NeighbourIndex);
			CurrentQuad.OffsetNeighbours.Add(NeighbourIndex);
		}

		for(int j = 0; j < CurrentQuad.Points.Num(); j++)
		{
			if(BaseGridPoints[CurrentQuad.Points[j]].IsEdge && BaseGridPoints[CurrentQuad.Points[(j + 1) % CurrentQuad.Points.Num()]].IsEdge)
			{
				CurrentQuad.OffsetNeighbours.Insert(-1, j);
			}
		}
	}
}

void AGridGenerator::LogSuperpositionOptions(const FCell& Cell)
{
	FString MarchingString = "";
	for(int i = 0; i < Cell.MarchingBits.Num(); i++)
	{
		MarchingString += FString::FromInt(Cell.MarchingBits[i]);
		MarchingString += ", ";
	}
	UE_LOG(LogTemp, Error, TEXT("Elevation: %d - Cell: %d - Marching Bits: %s"), Cell.Elevation, Cell.Index, *MarchingString);
	for(int i = 0; i < Cell.Candidates.Num(); i++)
		UE_LOG(LogTemp, Warning, TEXT("Option: %s"), *Cell.Candidates[i].ToString());
	UE_LOG(LogTemp, Warning, TEXT("--------------------------------------"));

}

void AGridGenerator::LogSuperpositionOptions(const TArray<FCell*>& Cells)
{
	for(int i = 0; i < Cells.Num(); i++)
		LogSuperpositionOptions(*Cells[i]);
}

void AGridGenerator::GetMarchingBitsForCell(FCell& Cell)
{
	int MinCorner = -1;
	TArray<int> LowerCorners;
	TArray<int> UpperCorners;
	TArray<bool> CornersMask;
	const FGridQuad& CorrespondingQuad = BaseGridQuads[Cell.Index];
	for(int j = 0; j < 8; j++)
	{
		CornersMask.Add(Elevations[Cell.Elevation + j / 4].MarchingBits[CorrespondingQuad.Points[j % 4]]);
		if(Elevations[Cell.Elevation + j / 4].MarchingBits[CorrespondingQuad.Points[j % 4]])
		{
			if(j < 4)
				LowerCorners.Add(j);
			else
				UpperCorners.Add(j);
			if(MinCorner == -1)
				MinCorner = j;
		}
	}

	Cell.RotationAmount = 0;
	
	if(LowerCorners.Num())
	{
		//Arranging so the first lower corner is the least number
		if(LowerCorners.Num() >= 2)
		{
			for(int j = 0; j < LowerCorners.Num() - 1; j++)
			{
				if(LowerCorners[j] != LowerCorners[j + 1] - 1)
				{
					for(int p = 0; p < LowerCorners.Num() - j - 1; p++)
					{
						const int LastElement = LowerCorners.Last();
						for(int k = LowerCorners.Num() - 1; k >= 1; k--)
						{
							LowerCorners[k] = LowerCorners[k - 1];
						}
						LowerCorners[0] = LastElement;
						
						if(UpperCorners.Num() >= 2)
						{
							const int UpperLastElement = UpperCorners.Last();
							for(int k = UpperCorners.Num() - 1; k >= 1; k--)
							{
								UpperCorners[k] = UpperCorners[k - 1];
							}
							UpperCorners[0] = UpperLastElement;
						}
					}
					break;
				}
			}
		}
		if(LowerCorners[0] != 0)
		{
			const int RotationAmount = 4 - LowerCorners[0];
			Cell.RotationAmount = RotationAmount;
			for(int j = 0; j < LowerCorners.Num(); j++)
			{
				LowerCorners[j] = (LowerCorners[j] + RotationAmount) % 4;
			}
			
			for(int j = 0; j < UpperCorners.Num(); j++)
			{
				UpperCorners[j] = (UpperCorners[j] + RotationAmount) % 4 + 4;
			}
		}
	}
	if(UpperCorners.Num() && (!LowerCorners.Num() || LowerCorners.Num() == 4))
	{
		if(UpperCorners.Num() >= 2)
		{
			for(int j = 0; j < UpperCorners.Num() - 1; j++)
			{
				if(UpperCorners[j] != UpperCorners[j + 1] - 1)
				{
					for(int p = 0; p < UpperCorners.Num() - j - 1; p++)
					{
						const int LastElement = UpperCorners.Last();
						for(int k = UpperCorners.Num() - 1; k >= 1; k--)
						{
							UpperCorners[k] = UpperCorners[k - 1];
						}
						UpperCorners[0] = LastElement;
					}
					break;
				}
			}
		}
		if(UpperCorners[0] != 4)
		{
			const int RotationAmount = 7 - UpperCorners[0];
			Cell.RotationAmount = RotationAmount;
			for(int j = 0; j < UpperCorners.Num(); j++)
			{
				UpperCorners[j] = (UpperCorners[j] + RotationAmount) % 4 + 4;
			}
		}
	}

	Cell.MarchingBits = LowerCorners;
	Cell.MarchingBits.Append(UpperCorners);
}

TArray<FCell*> AGridGenerator::GetCellsToCheck(const int Elevation, const int MarchingBitUpdated)
{
	TArray<FCell*> BuildingCells;
	
	//Add initial cells (the ones the marching bit updated is part of)
	TArray<FCell*> CellsToCheck;
	for(int i = 0; i < BaseGridPoints[MarchingBitUpdated].PartOfQuads.Num(); i++)
		CellsToCheck.Add(&Elevations[Elevation].Cells[BaseGridPoints[MarchingBitUpdated].PartOfQuads[i]]);
	
	TArray<FCell*> CheckedCells;
	
	while(CellsToCheck.Num())
	{
		//Get first cell and remove it from check array
		FCell* CurrentCell = CellsToCheck[0];
		CellsToCheck.RemoveAt(0);
		CheckedCells.Add(CurrentCell);
		//Check cell neighbours
		const TArray<int> CurrentCellsPoints = BaseGridQuads[CurrentCell->Index].Points;
		
		//should always be 4, but just in case
		for(int i = 0; i < CurrentCellsPoints.Num(); i++)
		{
			//Add neighbours containing that point (if they're valid)
			if(Elevations[CurrentCell->Elevation].MarchingBits[CurrentCellsPoints[i]])
			{
				BuildingCells.AddUnique(CurrentCell);
				BuildingCells.Last()->HasChosenCandidate = false;
				const int PreviousNeighbourIndex = i - 1 < 0 ? CurrentCellsPoints.Num() - 1 : i - 1;
				const int NextNeighbourIndex = i;

				const int PreviousNeighbour = BaseGridQuads[CurrentCell->Index].OffsetNeighbours[PreviousNeighbourIndex];
				const int NextNeighbour = BaseGridQuads[CurrentCell->Index].OffsetNeighbours[NextNeighbourIndex];
				if(PreviousNeighbour != -1)
				{
					FCell* PreviousNeighbourCell = &Elevations[CurrentCell->Elevation].Cells[PreviousNeighbour];
					if(!CheckedCells.Contains(PreviousNeighbourCell))
					{
						CellsToCheck.AddUnique(PreviousNeighbourCell);
					}
				}

				if(NextNeighbour != -1)
				{
					FCell* NextNeighbourCell = &Elevations[CurrentCell->Elevation].Cells[NextNeighbour];
					if(!CheckedCells.Contains(NextNeighbourCell))
					{
						CellsToCheck.AddUnique(NextNeighbourCell);
					}
				}
			}
		}

		//Add above and bellow cells
		const int Bellow = CurrentCell->Elevation - 1;
		const int AboveElevation = CurrentCell->Elevation + 1;
		// might break if there are upper marching bits and no lower ones
		if(Bellow >= 0 && Bellow < MaxElevation - 1)
		{
			FCell* BelowCell = &Elevations[Bellow].Cells[CurrentCell->Index];
			for(int i = 0; i < CurrentCellsPoints.Num(); i++)
			{
				if(Elevations[CurrentCell->Elevation].MarchingBits[CurrentCellsPoints[i]])
				{
					if(!CheckedCells.Contains(BelowCell))
						CellsToCheck.AddUnique(BelowCell);
					break;
				}
				if(Elevations[CurrentCell->Elevation + 1].MarchingBits[CurrentCellsPoints[i]])
				{
					if(!CheckedCells.Contains(BelowCell))
						CellsToCheck.AddUnique(BelowCell);
					break;
				}
			}
		}

		if(AboveElevation >= 0 && AboveElevation < MaxElevation - 1)
		{
			FCell* AboveCell = &Elevations[AboveElevation].Cells[CurrentCell->Index];
			for(int i = 0; i < CurrentCellsPoints.Num(); i++)
			{
				if(Elevations[AboveElevation].MarchingBits[CurrentCellsPoints[i]])
				{
					if(!CheckedCells.Contains(AboveCell))
						CellsToCheck.AddUnique(AboveCell);
					break;
				}
				if(Elevations[AboveElevation + 1].MarchingBits[CurrentCellsPoints[i]])
				{
					if(!CheckedCells.Contains(AboveCell))
						CellsToCheck.AddUnique(AboveCell);
					break;
				}
			}
		}
	}

	for(int i = 0; i < CheckedCells.Num(); i++)
	{
		GetMarchingBitsForCell(*CheckedCells[i]);
		if(!CheckedCells[i]->MarchingBits.Num())
		{
			TObjectPtr<ABuildingPiece>* BuildingPiece = Elevations[CheckedCells[i]->Elevation].BuildingPieces.Find(CheckedCells[i]->Index);
			if(BuildingPiece)
			{
				Elevations[CheckedCells[i]->Elevation].BuildingPieces.Remove(CheckedCells[i]->Index);
				GetWorld()->DestroyActor(*BuildingPiece);
			}
		}
	}

	return BuildingCells;
	
	//for(int i = 0; i < BuildingCells.Num(); i++)
	//{
	//	UE_LOG(LogTemp, Warning, TEXT("Cell: %d, %d"), BuildingCells[i].Elevation, BuildingCells[i].Index);
	//}
	//UE_LOG(LogTemp, Warning, TEXT("----------------------\n"));
}

void AGridGenerator::CalculateCandidates(TArray<FCell*>& Cells)
{
	TArray<FBuildingMeshData*> TableRows;
	//Optimize to only calculate for the ones had their bit changed
	MeshTable->GetAllRows(TEXT("CalculateSuperposition"), TableRows);
	for(int i = 0; i < Cells.Num(); i++)
	{
		for(int j = 0; j < TableRows.Num(); j++)
		{
			//Sanity check
			if(TableRows[j] == nullptr)
				return;
			TSet<int> Corners = TSet<int>(TableRows[j]->Corners);
			TSet<int> MarchingBits = TSet<int>(Cells[i]->MarchingBits);
			if(!Corners.Difference(MarchingBits).Num())
			{
				Cells[i]->Candidates.Add(TableRows[j]->Name);
				//Shift borders
				TArray<int> MeshBorders = TableRows[j]->EdgeCodes;
				TArray<int> SideMeshBorders = MeshBorders;

				const int Cycle = (4 - Cells[i]->RotationAmount) % 4;
				for(int k = 1; k < 5; k++)
				{
					int NewIndex = k - Cycle;
					if(NewIndex <= 0)
						NewIndex += 4;
					if(NewIndex >= 5)
						NewIndex -= 4;
					MeshBorders[k] = SideMeshBorders[NewIndex];
				}
				
				Cells[i]->CandidateBorders.Add(MeshBorders);
			}
		}
	}
}

bool AGridGenerator::PropagateChoice(TArray<FCell>& Cells, const FCell& UpdatedCell)
{
	UE_LOG(LogTemp, Error, TEXT("NOW PROPAGATING: %d - %d"), UpdatedCell.Elevation, UpdatedCell.Index);
	FString NeighboursString = "";
	TArray<FCell*> CellsToPropagate;
	for(int i = 0; i < UpdatedCell.Neighbours.Num(); i++)
	{
		int CellArrayIndex = -1;
		for(int j = 0; j < Cells.Num(); j++)
		{
			if(Cells[j].Elevation == UpdatedCell.Neighbours[i].Key && Cells[j].Index == UpdatedCell.Neighbours[i].Value)
			{
				CellArrayIndex = j;
				break;
			}
		}
		if(CellArrayIndex == -1)
			continue;
		FCell& Neighbour = Cells[CellArrayIndex];// = Elevations[UpdatedCell.Neighbours[i].Key].Cells[UpdatedCell.Neighbours[i].Value];
		NeighboursString += TEXT("            Elevation: ") + FString::FromInt(Neighbour.Elevation) + TEXT(" - Cell:") + FString::FromInt(Neighbour.Index) + TEXT("\n");
		bool IsNeigbhourCellStillValid = false;
		//Bottom neighbour
		if(Neighbour.Elevation < UpdatedCell.Elevation)
		{
			//Neighbour candidates were modified
			if(CheckNeighbourCandidates(UpdatedCell, Neighbour, 0, 5, IsNeigbhourCellStillValid))
			{
				CellsToPropagate.Add(&Neighbour);
			}
		}
		else if(Neighbour.Elevation > UpdatedCell.Elevation)
		{
			//Neighbour candidates were modified
			if(CheckNeighbourCandidates(UpdatedCell, Neighbour, 5, 0, IsNeigbhourCellStillValid))
			{
				CellsToPropagate.Add(&Neighbour);
			}
		}
		else
		{
			int NeighbourToCellBorderIndex = Neighbour.Neighbours.Find(TPair<int, int>(UpdatedCell.Elevation, UpdatedCell.Index));
			int CellToNeighbourBorderIndex = i;
			if(Neighbour.Neighbours[0].Key == UpdatedCell.Elevation)
				NeighbourToCellBorderIndex++;
			if(UpdatedCell.Neighbours[0].Key == UpdatedCell.Elevation)
				CellToNeighbourBorderIndex++;
			
			if(CheckNeighbourCandidates(UpdatedCell, Neighbour, CellToNeighbourBorderIndex, NeighbourToCellBorderIndex, IsNeigbhourCellStillValid))
			{
				CellsToPropagate.Add(&Neighbour);
			}
		}

		if(!IsNeigbhourCellStillValid)
		{
			UE_LOG(LogTemp, Warning, TEXT("Invalid Neighbour: Elevation: %d - Cell: %d"), Neighbour.Elevation, Neighbour.Index);
			return false;
		}
	}

	UE_LOG(LogTemp, Error, TEXT("AFTER PROPAGATING CELL: Elevation: %d - Cell: %d to NEIGHBOURS: %s"), UpdatedCell.Elevation, UpdatedCell.Index, *NeighboursString);
	for(int i = 0; i < Cells.Num(); i++)
	{
		bool IsNeighbour = false;
		for(int j = 0; j < UpdatedCell.Neighbours.Num(); j++)
		{
			if(Cells[i].Elevation == UpdatedCell.Neighbours[j].Key && Cells[i].Index == UpdatedCell.Neighbours[j].Value)
			{
				IsNeighbour = true;
				break;
			}
		}
		if(!IsNeighbour)
			continue;
		
		UE_LOG(LogTemp, Warning, TEXT("    Elevation: %d - Cell: %d"), Cells[i].Elevation, Cells[i].Index);
		for(int j = 0; j < Cells[i].Candidates.Num(); j++)
			UE_LOG(LogTemp, Log, TEXT("    Option: %s"), *Cells[i].Candidates[j].ToString());
		UE_LOG(LogTemp, Warning, TEXT("    --------------------------------------"));
	}
	
	for(int i = 0; i < CellsToPropagate.Num(); i++)
	{
		UE_LOG(LogTemp, Error, TEXT("NOW PROPAGATING FROM CELL: Elevation: %d - Cell: %d to NEIGHBOUR: Neighbour Elevation: %d - Neighbour Cell: %d"), UpdatedCell.Elevation, UpdatedCell.Index, CellsToPropagate[i]->Elevation, CellsToPropagate[i]->Index);

		if(!PropagateChoice(Cells, *CellsToPropagate[i]))
		{
			return false;
		}
	}
	
	return true;
}

bool AGridGenerator::SolveWVC(TArray<FCell*>& OriginalCells, TArray<FCell>& CopyCells, TArray<int> CellOrder)
{
	int LowestEntropy = GetLowestEntropyCell(CopyCells);
	UE_LOG(LogTemp, Log, TEXT("00000000000000000000000000000000000000000000"));
	while(LowestEntropy != -1)
    {
    	CellOrder.Add(LowestEntropy);
	    const int CandidateChosen = FMath::RandRange(0, CopyCells[LowestEntropy].Candidates.Num() - 1);
    	CopyCells[LowestEntropy].ChosenCandidate = CopyCells[LowestEntropy].Candidates[CandidateChosen];
    	const TArray<int> CandidateBorders = CopyCells[LowestEntropy].CandidateBorders[CandidateChosen];

		CopyCells[LowestEntropy].DiscardedCandidates.Append(CopyCells[LowestEntropy].Candidates);
		CopyCells[LowestEntropy].DiscardedCandidateBorders.Append(CopyCells[LowestEntropy].CandidateBorders);

		const int RemoveIndex = CopyCells[LowestEntropy].DiscardedCandidates.Find(CopyCells[LowestEntropy].ChosenCandidate);
		CopyCells[LowestEntropy].DiscardedCandidates.RemoveAt(RemoveIndex);
		CopyCells[LowestEntropy].DiscardedCandidateBorders.RemoveAt(RemoveIndex);
		
    	CopyCells[LowestEntropy].Candidates = TArray<FName>({CopyCells[LowestEntropy].ChosenCandidate});
    	CopyCells[LowestEntropy].CandidateBorders = TArray<TArray<int>>({CandidateBorders});

		UE_LOG(LogTemp, Error, TEXT("NEW CHOSEN MESH: Elevation: %d - Cell: %d WITH MESH: %s"), CopyCells[LowestEntropy].Elevation, CopyCells[LowestEntropy].Index, *CopyCells[LowestEntropy].ChosenCandidate.ToString());
		for(int i = 0; i < CopyCells.Num(); i++)
		{
			UE_LOG(LogTemp, Warning, TEXT("Elevation: %d - Cell: %d"), CopyCells[i].Elevation, CopyCells[i].Index);
			for(int j = 0; j < CopyCells[i].Candidates.Num(); j++)
				UE_LOG(LogTemp, Log, TEXT("Option: %s"), *CopyCells[i].Candidates[j].ToString());
			UE_LOG(LogTemp, Warning, TEXT("--------------------------------------"));
		}
		
    	if(!PropagateChoice(CopyCells, CopyCells[LowestEntropy]))
    	{
    		break;
    	}
		LowestEntropy = GetLowestEntropyCell(CopyCells);
    	//CopyBuildingCells[LowestEntropy]->ChosenMesh = true;
    }

	int Retry = false;
    for(int i = 0; i < CopyCells.Num(); i++)
    {
    	if(CopyCells[i].Candidates.Num() == 0 && CopyCells[i].MarchingBits.Num() != 8)
    	{
    		for(int j = CellOrder.Num() - 1; j >= 0; j--)
    		{
    			if(OriginalCells[CellOrder[j]]->Candidates.Num() == 1)
    			{
    				if(j == 0)
    				{
    					UE_LOG(LogTemp, Error, TEXT("CAN'T FIND SOLUTION"));
    					return false;
    				}
				    else 
				    {
				    	//Go back to previous decision and remove that option since it lead to a non solution
						OriginalCells[CellOrder[j]]->Candidates.Append(OriginalCells[CellOrder[j]]->DiscardedCandidates);
						OriginalCells[CellOrder[j]]->CandidateBorders.Append(OriginalCells[CellOrder[j]]->DiscardedCandidateBorders);

				    	OriginalCells[CellOrder[j]]->DiscardedCandidates.Empty();
				    	OriginalCells[CellOrder[j]]->DiscardedCandidateBorders.Empty();
				    	
				    	CopyCells[CellOrder[j]].Candidates = OriginalCells[CellOrder[j]]->Candidates;
				    	CopyCells[CellOrder[j]].CandidateBorders = OriginalCells[CellOrder[j]]->CandidateBorders;

				    	CopyCells[CellOrder[j]].DiscardedCandidates.Empty();
				    	CopyCells[CellOrder[j]].DiscardedCandidateBorders.Empty();
				    	CellOrder.Pop();
				    	//j--;
				    	//Retry = true;
				    }
    			}
    			else
    			{
    				const int IndexToRemove = OriginalCells[CellOrder[j]]->Candidates.Find(CopyCells[CellOrder[j]].ChosenCandidate);
    				OriginalCells[CellOrder[j]]->DiscardedCandidates.Add(OriginalCells[CellOrder[j]]->Candidates[IndexToRemove]);
    				OriginalCells[CellOrder[j]]->DiscardedCandidateBorders.Add(OriginalCells[CellOrder[j]]->CandidateBorders[IndexToRemove]);
    				
    				OriginalCells[CellOrder[j]]->Candidates.RemoveAt(IndexToRemove);
    				OriginalCells[CellOrder[j]]->CandidateBorders.RemoveAt(IndexToRemove);
    				
    				CopyCells[CellOrder[j]].Candidates = OriginalCells[CellOrder[j]]->Candidates;
    				CopyCells[CellOrder[j]].CandidateBorders = OriginalCells[CellOrder[j]]->CandidateBorders;

    				CopyCells[CellOrder[j]].DiscardedCandidates = OriginalCells[CellOrder[j]]->DiscardedCandidates;
    				CopyCells[CellOrder[j]].DiscardedCandidateBorders = OriginalCells[CellOrder[j]]->DiscardedCandidateBorders;
    				Retry = true;

    				for(int k = 0; k < CopyCells.Num(); k++)
    				{
    					if(CellOrder.Contains(k) && k != CellOrder[j])
    					{
    						int Index = OriginalCells[k]->Candidates.Find(CopyCells[k].ChosenCandidate);
    						check(Index != INDEX_NONE)

    						CopyCells[k].Candidates = TArray<FName>({OriginalCells[k]->Candidates[Index]});
    						CopyCells[k].CandidateBorders = TArray<TArray<int>>({OriginalCells[k]->CandidateBorders[Index]});

    						
    						continue;
    					}
    					CopyCells[k].Candidates = OriginalCells[k]->Candidates;
    					CopyCells[k].CandidateBorders = OriginalCells[k]->CandidateBorders;

    					CopyCells[k].DiscardedCandidates.Empty();
    					CopyCells[k].DiscardedCandidateBorders.Empty();
    				}
    				CellOrder.Pop();
    				
    				break;
    			}
    		}
    		break;
    	}
    }

	if(Retry)
	{
		UE_LOG(LogTemp, Warning, TEXT("Retry SOLVE WVC"));
		return SolveWVC(OriginalCells, CopyCells, CellOrder);
	}
	for(int i = 0; i < OriginalCells.Num(); i++)
	{
		OriginalCells[i]->Candidates = CopyCells[i].Candidates;
		OriginalCells[i]->CandidateBorders = CopyCells[i].CandidateBorders;
	}

	for(int i = 0; i < CopyCells.Num(); i++)
	{
		FGridQuad& Quad = BaseGridQuads[CopyCells[i].Index];
		if(CopyCells[i].CandidateBorders.Num() == 1 && CopyCells[i].Elevation == 1)
		{
			for(int k = 0; k < Quad.Points.Num() && (k + 1) < CopyCells[i].CandidateBorders[0].Num(); k++)
			{
				FVector Pos = (GetBasePointCoordinates(Quad.Points[k]) + GetBasePointCoordinates(Quad.Points[(k + 1) % Quad.Points.Num()])) / 2.f;
				Pos.Z = 0.f;
				FVector Direction = (Quad.Center - Pos).GetSafeNormal();
				Pos += Direction * 30.f; 
				Pos.Z = 200.f;
				//DrawDebugString(GetWorld(), Pos, FString::FromInt(CopyCells[i].CandidateBorders[0][k + 1]), this, FColor::Black, 10.f);
			}
		}
	}
	
	return true;
}

void AGridGenerator::CreateCellMeshes(const TArray<FCell*>& Cells)
{
	for(int i = 0; i < Cells.Num(); i++)
	{
		if(Cells[i]->Candidates.Num() == 1)
		{
			const auto Find = Elevations[Cells[i]->Elevation].BuildingPieces.Find(Cells[i]->Index);
			TObjectPtr<ABuildingPiece> BuildingPiece;
	
			if(Find)
			{
				BuildingPiece = *Find;
			}
			else
			{
				const FGridQuad& CorrespondingQuad = BaseGridQuads[Cells[i]->Index];
				FActorSpawnParameters SpawnParameters;
				BuildingPiece = GetWorld()->SpawnActor<ABuildingPiece>(BuildingPieceToSpawn, CorrespondingQuad.Center, FRotator(0, 0, 0));
				BuildingPiece->CorrespondingQuadIndex = Cells[i]->Index;
				BuildingPiece->Grid = this;
				BuildingPiece->Elevation = Cells[i]->Elevation;
				Elevations[Cells[i]->Elevation].BuildingPieces.Add(Cells[i]->Index, BuildingPiece);
				//DrawDebugBox(GetWorld(), BaseGridPoints[CorrespondingQuad.Points[0]].Location + FVector(0.f, 0.f, 160.f), FVector(5.f), FColor::Red, false, 10, 0, 2.f);
			}
			TArray<FVector> CageBase;
			for(int k = 0; k < 4; k++)
				CageBase.Add(GetBasePointCoordinates(BaseGridQuads[Cells[i]->Index].Points[k]));
			
			const auto Row = BuildingPiece->DataTable->FindRow<FBuildingMeshData>(Cells[i]->Candidates[0], "MeshCornersRow");
			check(Row);
			BuildingPiece->SetStaticMesh(Row->StaticMesh);
			BuildingPiece->DeformMesh(CageBase, 200.f, Cells[i]->RotationAmount * 90.f);
			FVector BuildingPieceLocation = BuildingPiece->GetActorLocation();
			BuildingPieceLocation.Z = 200.f * static_cast<float>(Cells[i]->Elevation);
			BuildingPiece->SetActorLocation(BuildingPieceLocation);
		}
	}
}

int AGridGenerator::GetLowestEntropyCell(const TArray<FCell>& Cells)
{
	if(!Cells.Num())
		return -1;
	int LowestEntropyIndex = -1;
	int LowestEntropy = 9999999;
	for(int i = 0; i < Cells.Num(); i++)
	{
		if(1 < Cells[i].Candidates.Num() && Cells[i].Candidates.Num() < LowestEntropy)
		{
			LowestEntropy = Cells[i].Candidates.Num();
			LowestEntropyIndex = i;
		}
	}
	if(LowestEntropy <= 1)
		return -1;
	return LowestEntropyIndex;
}

bool AGridGenerator::CheckNeighbourCandidates(const FCell& Cell, FCell& NeighbourCell, const int CellBorderIndex,
                                              const int NeighbourCellBorderIndex, bool& IsCellStillValid)
{
	bool Changed = false;
	TArray<int> RemainingCandidates;
	for(int i = 0; i < Cell.Candidates.Num(); i++)
	{
		const FEdgeAdjacencyData* AdjacencyRow = BorderAdjacencyTable->FindRow<FEdgeAdjacencyData>(FName(FString::FromInt(Cell.CandidateBorders[i][CellBorderIndex])), TEXT("Propagate Choice"));
		check(AdjacencyRow);

		const int AdjacencyRequired = AdjacencyRow->CorrespondingEdgeCode;
		for(int j = 0; j < NeighbourCell.Candidates.Num(); j++)
		{
			if(NeighbourCell.CandidateBorders[j][NeighbourCellBorderIndex] == AdjacencyRequired)
			{
				RemainingCandidates.AddUnique(j);
			}
		}
	}
	
	for(int i = NeighbourCell.Candidates.Num() - 1; i >= 0; i--)
	{
		if(!RemainingCandidates.Contains(i))
		{
			NeighbourCell.DiscardedCandidateBorders.Add(NeighbourCell.CandidateBorders[i]);
			NeighbourCell.DiscardedCandidates.Add(NeighbourCell.Candidates[i]);

			NeighbourCell.Candidates.RemoveAt(i);
			NeighbourCell.CandidateBorders.RemoveAt(i);
			Changed = true;
		}
	}

	if(!NeighbourCell.Candidates.Num() && NeighbourCell.MarchingBits.Num() < 8)
		IsCellStillValid = false;
	else
		IsCellStillValid = true;
	
	return Changed;
}

void AGridGenerator::ResetCells(TArray<FCell*>& Cells)
{
	for(int i = 0; i < Cells.Num(); i++)
	{
		Cells[i]->Candidates.Empty();
		Cells[i]->CandidateBorders.Empty();
		Cells[i]->DiscardedCandidates.Empty();
		Cells[i]->DiscardedCandidateBorders.Empty();
		Cells[i]->HasChosenCandidate = false;
		Cells[i]->ChosenCandidate = FName("");
	}
}

void AGridGenerator::Relax1()
{
	if(PerfectEqualSquareOrder == 1)
		RelaxGridBasedOnPerfectEqualSquare(PerfectEqualSquareSize);
	else if (PerfectLocalSquareOrder == 1)
		RelaxGridBasedOnPerfectLocalSquare();
	else if (NeighbourOrder == 1)
		RelaxGridBasedOnNeighbours();
}

void AGridGenerator::Relax2()
{
	if(PerfectEqualSquareOrder == 2)
		RelaxGridBasedOnPerfectEqualSquare(PerfectEqualSquareSize);
	else if (PerfectLocalSquareOrder == 2)
		RelaxGridBasedOnPerfectLocalSquare();
	else if (NeighbourOrder == 2)
		RelaxGridBasedOnNeighbours();
}

void AGridGenerator::Relax3()
{
	if(PerfectEqualSquareOrder == 3)
		RelaxGridBasedOnPerfectEqualSquare(PerfectEqualSquareSize);
	else if (PerfectLocalSquareOrder == 3)
		RelaxGridBasedOnPerfectLocalSquare();
	else if (NeighbourOrder == 3)
		RelaxGridBasedOnNeighbours();
}

void AGridGenerator::FindPointNeighboursInQuad(const int QuadIndex)
{
	for(int i = 0; i < 4; i++)
	{
		const int PrevPoint = i - 1 < 0 ? 3 : i - 1;
		const int NextPoint = (i + 1) % 4;
		BaseGridPoints[BaseGridQuads[QuadIndex].Points[i]].Neighbours.AddUnique(BaseGridQuads[QuadIndex].Points[PrevPoint]);
		BaseGridPoints[BaseGridQuads[QuadIndex].Points[i]].Neighbours.AddUnique(BaseGridQuads[QuadIndex].Points[NextPoint]);
	}
}

void AGridGenerator::SortQuadPoints(FGridQuad& Quad)
{
	FVector QuadCenter = FVector::ZeroVector;
	for(int i = 0; i < Quad.Points.Num(); i++)
		QuadCenter += BaseGridPoints[Quad.Points[i]].Location;
	QuadCenter /= 4.f;
	Quad.Center = QuadCenter;
	struct FPointAngle
	{
		int Index;
		float Angle;

		FPointAngle(const int InIndex, const float InAngle)
		{
			Index = InIndex;
			Angle = InAngle;
		}
	};
	TArray<FPointAngle> Angles;
	for(int i = 0; i < Quad.Points.Num(); i++)
	{
		const FVector& GridCoordinate = BaseGridPoints[Quad.Points[i]].Location;
		Angles.Add(FPointAngle(i, FMath::Atan2(GridCoordinate.Y - QuadCenter.Y, GridCoordinate.X - QuadCenter.X)));
	}
	Angles.Sort([](const FPointAngle& A, const FPointAngle& B)
	{
		return A.Angle > B.Angle; // Clockwise sorting
	});
	const TArray<int> CopyPoints = Quad.Points;
	for(int i = 0; i < Quad.Points.Num(); i++)
	{
		Quad.Points[i] = CopyPoints[Angles[i].Index];
	}
}

void AGridGenerator::SortShapePoints(FGridShape& Shape, const bool SecondGrid)
{
	FVector ShapeCenter = FVector::ZeroVector;
	TArray<FGridPoint>& Points = SecondGrid ? BuildingGridPoints : BaseGridPoints;
	for(int i = 0; i < Shape.Points.Num(); i++)
		ShapeCenter += Points[Shape.Points[i]].Location;
	ShapeCenter /= static_cast<float>(Shape.Points.Num());
	Shape.Center = ShapeCenter;
	struct FPointAngle
	{
		int Index;
		float Angle;

		FPointAngle(const int InIndex, const float InAngle)
		{
			Index = InIndex;
			Angle = InAngle;
		}
	};
	TArray<FPointAngle> Angles;
	for(int i = 0; i < Shape.Points.Num(); i++)
	{
		const FVector& GridCoordinate = Points[Shape.Points[i]].Location;
		Angles.Add(FPointAngle(i, FMath::Atan2(GridCoordinate.Y - ShapeCenter.Y, GridCoordinate.X - ShapeCenter.X)));
	}
	Angles.Sort([](const FPointAngle& A, const FPointAngle& B)
	{
		return A.Angle > B.Angle; // Clockwise sorting
	});
	const TArray<int> CopyPoints = Shape.Points;
	for(int i = 0; i < Shape.Points.Num(); i++)
	{
		Shape.Points[i] = CopyPoints[Angles[i].Index];
	}
}

TArray<FVector> AGridGenerator::SortPoints(const TArray<FVector>& Points, TArray<int>& NewOrder)
{
	FVector Center = FVector::ZeroVector;
	for(int i = 0; i < Points.Num(); i++)
		Center += Points[i];
	Center /= Points.Num();
	
	struct FPointAngle
	{
		int Index;
		float Angle;

		FPointAngle(const int InIndex, const float InAngle)
		{
			Index = InIndex;
			Angle = InAngle;
		}
	};
	
	TArray<FPointAngle> Angles;
	for(int i = 0; i < Points.Num(); i++)
	{
		const FVector& GridCoordinate = Points[i];
		Angles.Add(FPointAngle(i, FMath::Atan2(GridCoordinate.Y - Center.Y, GridCoordinate.X - Center.X)));
	}
	Angles.Sort([](const FPointAngle& A, const FPointAngle& B)
	{
		return A.Angle > B.Angle; // Clockwise sorting
	});
	
	TArray<FVector> Output;
	NewOrder.Empty();
	for(int i = 0; i < Points.Num(); i++)
	{
		Output.Add(Points[Angles[i].Index]);
		NewOrder.Add(Angles[i].Index);
	}

	return Output;
}

int AGridGenerator::GetOrAddMidpointIndexInBaseGrid(TMap<TPair<int, int>, int>& Midpoints, int Point1, int Point2)
{
	if(Point1 > Point2)
		std::swap(Point1, Point2);
	const TPair<int, int> Key(Point1, Point2);
	if(Midpoints.Contains(Key))
	{
		return Midpoints[Key];
	}
	const bool IsEdgePoint = (BaseGridPoints[Point1].IsEdge && BaseGridPoints[Point2].IsEdge);
	BaseGridPoints.Emplace(BaseGridPoints.Num(), (BaseGridPoints[Point1].Location + BaseGridPoints[Point2].Location) / 2.f, IsEdgePoint);
	Midpoints.Add(Key, BaseGridPoints.Num() - 1);
	return Midpoints[Key];
}

bool AGridGenerator::IsPointInShape(const FVector& Point, const FGridShape& Shape) const
{
	auto Sign = [](const FVector& P1, const FVector& P2, const FVector& P3) {
		return (P1.X - P3.X) * (P2.Y - P3.Y) - 
			   (P2.X - P3.X) * (P1.Y - P3.Y);
	};

	for(int i = 0; i < Shape.ComposingQuads.Num(); i++)
	{
		const bool SameSide1 = Sign(Point, Shape.ComposingQuads[i].Points[0], Shape.ComposingQuads[i].Points[1]) > 0;
		bool Found = true;
		for(int j = 1; j < Shape.ComposingQuads[i].Points.Num(); j++)
		{
			const bool SameSide2 = Sign(Point, Shape.ComposingQuads[i].Points[j], Shape.ComposingQuads[i].Points[(j + 1) % Shape.ComposingQuads[i].Points.Num()]) > 0;
			if(SameSide1 != SameSide2)
			{
				Found = false;
				break;
			}
		}
		if(Found)
			return true;
	}
		
	return false;
}

int AGridGenerator::DetermineWhichGridShapeAPointIsIn(const FVector& Point)
{
	TArray<bool> Visited;
	Visited.SetNumZeroed(BuildingGridShapes.Num());
	int CurrentShape = 0;
	int VisitedQuadNumber = 0;

	while(VisitedQuadNumber < BuildingGridShapes.Num())
	{
		if(IsPointInShape(Point, BuildingGridShapes[CurrentShape]))
		{
			return CurrentShape;
		}
		
		Visited[CurrentShape] = true;
		VisitedQuadNumber++;

		float ClosestNeighbourDistance = FLT_MAX;
		int ClosestNeighbour = -1;

		for(int i = 0; i < BuildingGridShapes[CurrentShape].Neighbours.Num(); i++)
		{
			const int NeighbourIndex = BuildingGridShapes[CurrentShape].Neighbours[i];
			if(Visited[NeighbourIndex])
				continue;
			const float DistanceBetween = FVector::DistSquared2D(Point, BuildingGridShapes[NeighbourIndex].Center);
			if(DistanceBetween < ClosestNeighbourDistance)
			{
				ClosestNeighbourDistance = DistanceBetween;
				ClosestNeighbour = NeighbourIndex;
			}
		}
		if(ClosestNeighbour == -1)
		{
			return -1;
		}

		CurrentShape = ClosestNeighbour;
	}
	return -1;
}

void AGridGenerator::DrawGrid()
{
	FlushPersistentDebugLines(GetWorld());
	for(int i = 0; i < BaseGridQuads.Num(); i++)
	{
		if(BaseGridQuads[i].Index == -1)
			continue;
		FLinearColor Color = FColor::Red;
		FLinearColor Color2 = FColor::Green;
		FLinearColor Color3 = FColor::Blue;
		FLinearColor Color4 = FColor::Yellow;
		if(ShowBaseGrid)
		{
			float LineThickness = 2.f;
			DrawDebugLine(GetWorld(), 
			GetBasePointCoordinates(BaseGridQuads[i].Points[0]),
				GetBasePointCoordinates(BaseGridQuads[i].Points[1]),
				Color.ToFColor(true),
				true);
			DrawDebugLine(GetWorld(), 
			GetBasePointCoordinates(BaseGridQuads[i].Points[1]),
				GetBasePointCoordinates(BaseGridQuads[i].Points[2]),
				Color2.ToFColor(true),
				true);
			DrawDebugLine(GetWorld(), 
			GetBasePointCoordinates(BaseGridQuads[i].Points[2]),
				GetBasePointCoordinates(BaseGridQuads[i].Points[3]),
				Color3.ToFColor(true),
				true);
			DrawDebugLine(GetWorld(), 
			GetBasePointCoordinates(BaseGridQuads[i].Points[3]),
				GetBasePointCoordinates(BaseGridQuads[i].Points[0]),
				Color4.ToFColor(true),
				true);
		}
		if(!ShowSquares || !DebugOnlyPerfectQuads.Num())
			continue;
		
		DrawDebugLine(GetWorld(), 
			DebugOnlyPerfectQuads[i][0],
			DebugOnlyPerfectQuads[i][1],
			Color.ToFColor(true),
			true);
		DrawDebugLine(GetWorld(), 
			DebugOnlyPerfectQuads[i][1],
				DebugOnlyPerfectQuads[i][2],
				Color2.ToFColor(true),
				true);
		DrawDebugLine(GetWorld(), 
			DebugOnlyPerfectQuads[i][2],
				DebugOnlyPerfectQuads[i][3],
				Color3.ToFColor(true),
				true);
		DrawDebugLine(GetWorld(), 
			DebugOnlyPerfectQuads[i][3],
				DebugOnlyPerfectQuads[i][0],
				Color4.ToFColor(true),
				true);
	}
}

void AGridGenerator::DrawSecondGrid()
{
	FlushPersistentDebugLines(GetWorld());
	TArray<FLinearColor> Colors = {FColor::Red, FColor::Green, FColor::Blue, FColor::Yellow, FColor::Purple, FColor::Emerald};
	for(int i = 0; i < BuildingGridShapes.Num(); i++)
	{
		if(BuildingGridShapes[i].Index == -1 || i != 1)
			continue;
		if(ShowBuildingGrid)
		{
			float LineThickness = 2.f;
			for(int j = 0; j < BuildingGridShapes[i].Points.Num(); j++)
			{
			DrawDebugLine(GetWorld(), 
			GetBasePointCoordinates(BuildingGridShapes[i].Points[j]),
				GetBasePointCoordinates(BuildingGridShapes[i].Points[(j + 1) % BuildingGridShapes[i].Points.Num()]),
				Colors[j].ToFColor(true),
				true);
			}
		}
	}
}

void AGridGenerator::UpdateMarchingBit(const int ElevationLevel, const int Index, const bool Value, const bool IsAdjacent)
{
	int ActualElevationLevel = ElevationLevel;
	if(ElevationLevel == 0 || IsAdjacent)
	{
		Elevations[ActualElevationLevel + 1].MarchingBits[Index] = Value;
	}
	Elevations[ActualElevationLevel].MarchingBits[Index] = Value;
	RunWVC(ActualElevationLevel, Index);
	for(int i = 0; i < BaseGridPoints[Index].PartOfQuads.Num(); i++)
	{
		//UpdateBuildingPiece(ActualElevationLevel, BaseGridPoints[Index].PartOfQuads[i]);
	}
}

void AGridGenerator::UpdateBuildingPiece(const int& ElevationLevel, const int& Index)
{
	UWorld* World = GetWorld();
	if(!World)
		return;
	const FGridQuad& CorrespondingQuad = GetBaseGridQuads()[Index];
	int MinCorner = -1;
	TArray<FVector> CageBase;
	for(int k = 0; k < 4; k++)
		CageBase.Add(GetBasePointCoordinates(CorrespondingQuad.Points[k]));
	const auto Find = Elevations[ElevationLevel].BuildingPieces.Find(Index);
	TObjectPtr<ABuildingPiece> BuildingPiece;
	
	if(Find)
	{
		BuildingPiece = *Find;
		BuildingPiece->Corners.Empty();
	}
	else
	{
		FActorSpawnParameters SpawnParameters;
		BuildingPiece = World->SpawnActor<ABuildingPiece>(BuildingPieceToSpawn, CorrespondingQuad.Center, FRotator(0, 0, 0));
		BuildingPiece->CorrespondingQuadIndex = Index;
		BuildingPiece->Grid = this;
		BuildingPiece->Elevation = ElevationLevel;
		Elevations[ElevationLevel].BuildingPieces.Add(Index, BuildingPiece);
		//UE_LOG(LogTemp, Warning, TEXT("SpawnIndex: %d"), BuildingPiece->CorrespondingQuadIndex);

		//DrawDebugBox(GetWorld(), BaseGridPoints[CorrespondingQuad.Points[0]].Location + FVector(0.f, 0.f, 160.f), FVector(5.f), FColor::Red, false, 10, 0, 2.f);
	}
	int TileConfig = 0;
	TArray<int> LowerCorners;
	TArray<int> UpperCorners;
	TArray<bool> CornersMask;
	for(int j = 0; j < 8; j++)
	{
		TileConfig += Elevations[ElevationLevel + j / 4].MarchingBits[CorrespondingQuad.Points[j % 4]] * pow(10, 8 - j - 1);
		CornersMask.Add(Elevations[ElevationLevel + j / 4].MarchingBits[CorrespondingQuad.Points[j % 4]]);
		if(Elevations[ElevationLevel + j / 4].MarchingBits[CorrespondingQuad.Points[j % 4]])
		{
			if(j < 4)
				LowerCorners.Add(j);
			else
				UpperCorners.Add(j);
			if(MinCorner == -1)
				MinCorner = j;
		}
	}
	float Rotation = 0.f;
	
	if(LowerCorners.Num())
	{
		//Arranging so the first lower corner is the least number
		if(LowerCorners.Num() >= 2)
		{
			for(int j = 0; j < LowerCorners.Num() - 1; j++)
			{
				if(LowerCorners[j] != LowerCorners[j + 1] - 1)
				{
					for(int p = 0; p < LowerCorners.Num() - j - 1; p++)
					{
						const int LastElement = LowerCorners.Last();
						for(int k = LowerCorners.Num() - 1; k >= 1; k--)
						{
							LowerCorners[k] = LowerCorners[k - 1];
						}
						LowerCorners[0] = LastElement;
					}
					break;
				}
			}
		}
		if(LowerCorners[0] != 0)
		{
			const int RotationAmount = 4 - LowerCorners[0];
			Rotation = static_cast<float>(RotationAmount) * 90.f;
			for(int j = 0; j < LowerCorners.Num(); j++)
			{
				LowerCorners[j] = (LowerCorners[j] + RotationAmount) % 4;
			}
		}
	}

	BuildingPiece->MeshRotation = Rotation;

	FCell Cell(ElevationLevel, Index);
	GetMarchingBitsForCell(Cell);
	
	if(!Cell.MarchingBits.Num() || Cell.MarchingBits.Num() == 8)
	{
		Elevations[ElevationLevel].BuildingPieces.Remove(Index);
		World->DestroyActor(BuildingPiece);
		return;
	}

	TArray<FName> Candidates;
	TArray<int> AllCorners = LowerCorners;
	AllCorners.Append(UpperCorners);
	AllCorners = Cell.MarchingBits;
	BuildingPiece->Corners = AllCorners;

	TArray<FBuildingMeshData*> TableRows;
	BuildingPiece->DataTable->GetAllRows(TEXT("UpdateBuildingPiece"), TableRows);
	for(const FBuildingMeshData* Row : TableRows)
	{
		//Change to bitmasking
		if(Row->Corners.Num() != BuildingPiece->Corners.Num())
			continue;

		bool Candidate = true;
		for(int i = 0; i < BuildingPiece->Corners.Num(); i++)
		{
			bool Found = false;
			for(int j = 0; j < Row->Corners.Num(); j++)
			{
				if(BuildingPiece->Corners[i] == Row->Corners[j])
				{
					Found = true;
					break;
				}
			}
			if(!Found)
			{
				Candidate = false;
				break;
			}
		}

		if(Candidate)
			Candidates.Add(Row->Name);
	}

	FName ChosenMeshName;

	check(Candidates.Num() > 0);
	
	ChosenMeshName = Candidates[0];
	
	const auto Row = BuildingPiece->DataTable->FindRow<FBuildingMeshData>(ChosenMeshName, "MeshCornersRow");
	check(Row);
	BuildingPiece->SetStaticMesh(Row->StaticMesh);
	BuildingPiece->DeformMesh(CageBase, 200.f, Rotation);

	BuildingPiece->EdgeCodes.Empty();
	BuildingPiece->EdgeCodes.SetNum(6);
	int Cycle = (4 - (static_cast<int>((Rotation) / 90.f))) % 4;
	for(int i = 1; i < 5; i++)
	{
		int NewIndex = i - Cycle;
		if(NewIndex <= 0)
			NewIndex += 4;
		if(NewIndex >= 5)
			NewIndex -= 4;
		BuildingPiece->EdgeCodes[i] = Row->EdgeCodes[NewIndex];
	}
	BuildingPiece->EdgeCodes[0] = BuildingPiece->EdgeCodes[0];
	BuildingPiece->EdgeCodes[5] = BuildingPiece->EdgeCodes[5];
}
