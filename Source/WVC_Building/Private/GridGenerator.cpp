// Fill out your copyright notice in the Description page of Project Settings.


#include "GridGenerator.h"

#include "BuildingPiece.h"
#include "DebugStrings.h"
#include "GridGeneratorVis.h"
#include "MeshCornersData.h"
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

// Called when the game starts or when spawned
void AGridGenerator::BeginPlay()
{
	Super::BeginPlay();

	Elevations.Add(FElevationData(BaseGridPoints.Num()));
	Elevations.Add(FElevationData(BaseGridPoints.Num()));
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
	
	BaseGridCenter = GetActorLocation();
	BaseGridPoints.Emplace(BaseGridPoints.Num(), BaseGridCenter);
	for(uint32 i = 0; i < GridExtent - 1; i++)
	{
		GenerateHexCoordinates(BaseGridCenter, FirstHexSize * (i + 1), i);
	}

	TArray<FGridTriangle> Triangles = DivideGridIntoTriangles(BaseGridCenter);
	DivideGridIntoQuads(BaseGridCenter, Triangles);
	RelaxAndCreateSecondGrid();
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
		int EdgePoints = 0;
		for(int j = 0; j < BaseGridQuads[i].Points.Num(); j++)
		{
			if(BaseGridPoints[BaseGridQuads[i].Points[j]].IsEdge)
			{
				EdgePoints++;
				if(EdgePoints >= 2)
				{
					IsEdge = true;
					break;
				}
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
	
	for(int i = 0; i < BuildingGridShapes.Num() - 1; i++)
	{
		for(int j = i + 1; j < BuildingGridShapes.Num(); j++)
		{
			int CommonPoints = 0;
			for(int k = 0; k < BuildingGridShapes[i].Points.Num(); k++)
			{
				bool Found = false;
				for(int p = 0; p < BuildingGridShapes[j].Points.Num(); p++)
				{
					if(BuildingGridShapes[i].Points[k] == BuildingGridShapes[j].Points[p])
					{
						if(++CommonPoints == 2)
						{
							BuildingGridShapes[i].Neighbours.Add(j);
							BuildingGridShapes[j].Neighbours.Add(i);
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

	for(int i = 0; i < BuildingGridShapes.Num(); i++)
	{
		TArray<int> ShapePoints;
		for(int j = 0; j < BuildingGridShapes[i].Neighbours.Num(); j++)
			ShapePoints.Add(BuildingGridShapes[BuildingGridShapes[i].Neighbours[j]].CorrespondingBaseGridPoint);
		FGridShape Shape(-1, ShapePoints, -1);
		SortShapePoints(Shape, false);
		TArray<int> NewNeighbourArray;
		for(int j = 0; j < Shape.Points.Num(); j++)
		{
			NewNeighbourArray.Add(BaseGridPoints[Shape.Points[j]].CorrespondingBuildingShape);
		}
		BuildingGridShapes[i].Neighbours = NewNeighbourArray;
		const FVector CenterPoint1 = BuildingGridPoints[BuildingGridShapes[i].Points[0]].Location - BuildingGridShapes[i].Center;
		float LeastAngle = -999999999999999.f;
		int Neighbour1 = -1;
 		for(int j = 0; j < BuildingGridShapes[i].Neighbours.Num(); j++)
		{
			const int NeighbourIndex = BuildingGridShapes[i].Neighbours[j];
			const FVector CenterNeighbour = BuildingGridShapes[NeighbourIndex].Center - BuildingGridShapes[i].Center;\
			const float Dot = UE::Geometry::Dot(CenterPoint1, CenterNeighbour);
			const FVector Cross = UE::Geometry::Cross(CenterPoint1, CenterNeighbour);

			float Angle = FMath::Atan2(Cross.Z, Dot);
			if(Angle < 0.f)
				Angle += 2.f * PI;

			if(Angle > LeastAngle)
			{
				LeastAngle = Angle;
				Neighbour1 = j;
			}
		}

		NewNeighbourArray.Empty();
		for(int j = Neighbour1; j < Shape.Points.Num() + Neighbour1; j++)
		{
			NewNeighbourArray.Add(BaseGridPoints[Shape.Points[j % Shape.Points.Num()]].CorrespondingBuildingShape);
		}
		BuildingGridShapes[i].Neighbours = NewNeighbourArray;
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
		const int PrevPoint = i - 1 < 0 ? 3 : 0;
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

	const bool SameSide1 = Sign(Point, GetBuildingPointCoordinates(Shape.Points[0]), GetBuildingPointCoordinates(Shape.Points[1])) > 0;
	for(int i = 1; i < Shape.Points.Num(); i++)
	{
		const bool SameSide2 = Sign(Point, GetBuildingPointCoordinates(Shape.Points[i]), GetBuildingPointCoordinates(Shape.Points[(i + 1) % Shape.Points.Num()])) > 0;
		if(SameSide1 != SameSide2)
			return false;
	}
		
	return true;
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

void AGridGenerator::UpdateMarchingBit(const int& ElevationLevel, const int& Index, const bool& Value)
{
	Elevations[ElevationLevel].MarchingBits[Index] = Value;
	for(int i = 0; i < BaseGridPoints[Index].PartOfQuads.Num(); i++)
	{
		UpdateBuildingPiece(ElevationLevel, BaseGridPoints[Index].PartOfQuads[i]);
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
	ABuildingPiece* BuildingPiece;
	if(Find)
	{
		BuildingPiece = *Find;
		BuildingPiece->Corners.Empty();
	}
	else
	{
		FActorSpawnParameters SpawnParameters;
		BuildingPiece = World->SpawnActor<ABuildingPiece>(BuildingPieceToSpawn, CorrespondingQuad.Center, FRotator(0, 0, 0));
		BuildingPiece->Index = BaseGridPoints[Index].CorrespondingBuildingShape;
		BuildingPiece->Grid = this;
		BuildingPiece->Elevation = ElevationLevel;
		Elevations[ElevationLevel].BuildingPieces.Add(Index, BuildingPiece);
		UE_LOG(LogTemp, Warning, TEXT("SpawnIndex: %d"), BuildingPiece->Index);
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

	if(!LowerCorners.Num() && !UpperCorners.Num())
	{
		Elevations[ElevationLevel].BuildingPieces.Remove(Index);
		World->DestroyActor(BuildingPiece);
		return;
	}
	
	FString RowNameString;
	BuildingPiece->Corners = LowerCorners;
	BuildingPiece->Corners.Append(UpperCorners);
	for(int j = 0; j < LowerCorners.Num(); j++)
		RowNameString.Append(FString::FromInt(LowerCorners[j]));
	for(int j = 0; j < UpperCorners.Num(); j++)
		RowNameString.Append(FString::FromInt(UpperCorners[j]));
	
	const FName RowName(RowNameString);
	const auto Row = BuildingPiece->DataTable->FindRow<FMeshCornersData>(RowName, "MeshCornersRow");
	BuildingPiece->SetStaticMesh(Row->Mesh);
	BuildingPiece->DeformMesh(CageBase, 200.f, Rotation);
}
