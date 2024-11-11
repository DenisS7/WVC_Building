// Fill out your copyright notice in the Description page of Project Settings.


#include "GridGenerator.h"

#include "DebugStrings.h"
#include "GridGeneratorVis.h"
#include "Polygon2.h"
#include "Components/DynamicMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Runtime/Core/Tests/Containers/TestUtils.h"
#include "GeometryScript/MeshPrimitiveFunctions.h"
#include "GeometryScript/MeshNormalsFunctions.h"

// Sets default values
AGridGenerator::AGridGenerator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	DebugStringsComp = CreateDefaultSubobject<UDebugStrings>(TEXT("DebugStringsComponent"));
	GridGeneratorVis = CreateDefaultSubobject<UGridGeneratorVis>(TEXT("GridGeneratorVis"));

	WholeGridMesh = CreateDefaultSubobject<UDynamicMeshComponent>(TEXT("WholeGridMesh"));
	HoveredShapeMesh = CreateDefaultSubobject<UDynamicMeshComponent>(TEXT("SelectedQuadMesh"));
	//UDynamicMesh* Mesh = new UDynamicMesh();
	//WholeGridMesh->SetDynamicMesh();	
	Delegate1 = FTimerDelegate::CreateUObject(this, &AGridGenerator::Relax2);
	Delegate2 = FTimerDelegate::CreateUObject(this, &AGridGenerator::Relax3);
	Delegate3 = FTimerDelegate::CreateUObject(this, &AGridGenerator::CreateSecondGrid);
}

// Called when the game starts or when spawned
void AGridGenerator::BeginPlay()
{
	Super::BeginPlay();
	TArray<bool> Elevation;
	for(int i = 0; i < GridPoints.Num(); i++)
		Elevation.Add(false);
	MarchingBits.Add(Elevation);
	MarchingBits.Add(Elevation);
	//for(int i = 0; i < FinalQuads.Num(); i++)
	//{
	//	FirstElevation.Emplace(i, 0);
	//}
	//VoxelConfig.Add(FirstElevation);
}

void AGridGenerator::GenerateHexCoordinates(const FVector& GridCenter, const float Size, const uint32 Index)
{
	const uint32 NextIndex = Index + 1;
	const uint32 NumPoints = 6 * NextIndex;
	//if(Index > 0)
		//NumPoints -= 1;
	//TArray<FVector> HexPoints;
	//HexPoints.SetNum(NumPoints);
	const int PrevNumPoints = GridPoints.Num();
	GridPoints.SetNum(PrevNumPoints + NumPoints);
	bool IsEdgePoint = (Index == GridSize - 2);
	for(uint32 i = 0; i < 6; i++)
	{
		const float AngleRad = FMath::DegreesToRadians(60.f * i - 150.f);
		//HexPoints[i * NextIndex] = FVector(GridCenter.X + Size * FMath::Cos(AngleRad), GridCenter.Y + Size * FMath::Sin(AngleRad), GridCenter.Z);
		GridPoints[PrevNumPoints+ i * NextIndex] = FGridPoint(PrevNumPoints+ i * NextIndex, FVector(GridCenter.X + Size * FMath::Cos(AngleRad), GridCenter.Y + Size * FMath::Sin(AngleRad), GridCenter.Z), IsEdgePoint);
	}
	const float FractionDistanceBetween = 1.f / static_cast<float>(NextIndex);
	for(uint32 i = 0; i < 6; i++)
	{
		for(uint32 j = 0; j < Index; j++)
		{
			const int PointIndex = i * NextIndex + j + 1;
			const float FractionDist = static_cast<float>(j + 1) * FractionDistanceBetween;
			//HexPoints[PointIndex] = FractionDist * HexPoints[(((i + 1) % 6) * NextIndex)] +
			//									(1.f - FractionDist) * HexPoints[i * NextIndex];
			GridPoints[PrevNumPoints + PointIndex] = FGridPoint(PrevNumPoints + PointIndex,
				FractionDist * GridPoints[PrevNumPoints + (((i + 1) % 6) * NextIndex)].Location +
									(1.f - FractionDist) * GridPoints[PrevNumPoints + i * NextIndex].Location, IsEdgePoint);
		}
	}
	//GridCoordinates.Emplace(HexPoints);
}

void AGridGenerator::DivideGridIntoTriangles(const FVector& GridCenter)
{
	if(!GridPoints.Num())
		return;

	int TriangleIndex = 0;
	for(uint32 i = 0; i < GridSize - 1; i++)
	{
		const int SmallMaxCoordinate = GetNumberOfPointsOnHex(i);//GridCoordinates[i].Num();
		const int LargeMaxCoordinate = GetNumberOfPointsOnHex(i + 1);//GridCoordinates[i + 1].Num();
		const int TrianglesPerSide = i * 2 + 1;
		for(int j = 0; j < 6; j++)
		{
			int SmallCoordinate = j * i;
			int LargeCoordinate = j * (i + 1);
			bool LargeTriangle = true;
			for(int k = 0; k < TrianglesPerSide; k++, LargeTriangle = !LargeTriangle)
			{
				//TArray<FIntPoint> TrianglePoints;
				TArray<int>TrianglePointsIndices;
				if(LargeTriangle)
				{
					//TrianglePoints.Emplace(i, SmallCoordinate);
					//TrianglePoints.Emplace(i + 1, LargeCoordinate);
					//TrianglePoints.Emplace(i + 1, (LargeCoordinate + 1) % LargeMaxCoordinate);

					TrianglePointsIndices.Add(GetIndexOfPointOnHex(i, SmallCoordinate));
					TrianglePointsIndices.Add(GetIndexOfPointOnHex(i + 1, LargeCoordinate));
					TrianglePointsIndices.Add(GetIndexOfPointOnHex(i + 1, (LargeCoordinate + 1) % LargeMaxCoordinate));
					
					LargeCoordinate = (LargeCoordinate + 1) % LargeMaxCoordinate;
				}
				else
				{
					//TrianglePoints.Emplace(i, SmallCoordinate);
					//TrianglePoints.Emplace(i, (SmallCoordinate + 1) % SmallMaxCoordinate);
					//TrianglePoints.Emplace(i + 1, LargeCoordinate);

					TrianglePointsIndices.Add(GetIndexOfPointOnHex(i, SmallCoordinate));
					TrianglePointsIndices.Add(GetIndexOfPointOnHex(i, (SmallCoordinate + 1) % SmallMaxCoordinate));
					TrianglePointsIndices.Add(GetIndexOfPointOnHex(i + 1, LargeCoordinate));
					
					SmallCoordinate = (SmallCoordinate + 1) % SmallMaxCoordinate;
				}
				TArray<int> Neighbours;
				if(j == 5 && k == TrianglesPerSide - 1)
				{
					Neighbours.Add(GetFirstTriangleIndexOnHex(i));
				}
				else
				{
					Neighbours.Add(TriangleIndex + 1);
				}
				if(j == 0 && k == 0) //first triangle
				{
					Neighbours.Add(GetFirstTriangleIndexOnHex(i + 1) - 1);
				}
				else
				{
					Neighbours.Add(TriangleIndex - 1);
				}
				if(k % 2 == 0)
				{
					if(GetFirstPointIndexOnHex(i + 3) < GridPoints.Num() - 1)
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
}

void AGridGenerator::DivideGridIntoQuads(const FVector& GridCenter)
{
	TArray<int> AvailableTriangles;
	TArray<int> TrianglesLeft;
	for(int i = 0; i < Triangles.Num(); i++)
		AvailableTriangles.Add(i);
	int QuadIndex = 0;
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
			TriangleCenter += GridPoints[Triangles[TrianglesLeft[i]].Points[j]].Location;
		TriangleCenter /= 3.f;
		TArray<int> TriangleMidpoints;
		for(int j = 0; j < 3; j++)
		{
			TriangleMidpoints.Add(GetOrAddMidpointIndexInGrid1(Midpoints, TrPoints[j], TrPoints[(j + 1) % 3]));
		}
		GridPoints.Emplace(GridPoints.Num(), TriangleCenter);

		TArray<int> Quad1Points = {TrPoints[0], TriangleMidpoints[0], GridPoints.Num() - 1, TriangleMidpoints[2]};
		TArray<int> Quad2Points = {TrPoints[1], TriangleMidpoints[1], GridPoints.Num() - 1, TriangleMidpoints[0]};
		TArray<int> Quad3Points = {TrPoints[2], TriangleMidpoints[2], GridPoints.Num() - 1, TriangleMidpoints[1]};
		TArray<TArray<int>> QuadsPoints = {Quad1Points, Quad2Points, Quad3Points};
		for(int j = 0; j < 3; j++)
		{
			FinalQuads.Add({QuadsPoints[j], FinalQuadIndex++});
			SortQuadPoints(FinalQuads.Last());
			FindPointNeighboursInQuad(FinalQuads.Num() - 1);
			for(int k = 0; k < QuadsPoints[j].Num(); k++)
			{
				GridPoints[QuadsPoints[j][k]].PartOfQuads.Add(FinalQuadIndex - 1);
			}
		}
	}
	for(int i = 0; i < Quads.Num(); i++)
	{
		TArray<int> QuadMidpoints;
		for(int j = 0; j < 4; j++)
		{
			QuadMidpoints.Add(GetOrAddMidpointIndexInGrid1(Midpoints, Quads[i].Points[j], Quads[i].Points[(j + 1) % 4]));
		}
		GridPoints.Emplace(GridPoints.Num(), Quads[i].Center);
		
		TArray<int> Quad1Points = {Quads[i].Points[0], QuadMidpoints[0], GridPoints.Num() - 1, QuadMidpoints[3]};
		TArray<int> Quad2Points = {Quads[i].Points[1], QuadMidpoints[1], GridPoints.Num() - 1, QuadMidpoints[0]};
		TArray<int> Quad3Points = {Quads[i].Points[2], QuadMidpoints[2], GridPoints.Num() - 1, QuadMidpoints[1]};
		TArray<int> Quad4Points = {Quads[i].Points[3], QuadMidpoints[3], GridPoints.Num() - 1, QuadMidpoints[2]};
		TArray<TArray<int>> QuadsPoints = {Quad1Points, Quad2Points, Quad3Points, Quad4Points};
		for(int j = 0; j < 4; j++)
		{
			FinalQuads.Add({QuadsPoints[j], FinalQuadIndex++});
			SortQuadPoints(FinalQuads.Last());
			FindPointNeighboursInQuad(FinalQuads.Num() - 1);
			for(int k = 0; k < QuadsPoints[j].Num(); k++)
			{
				GridPoints[QuadsPoints[j][k]].PartOfQuads.Add(FinalQuadIndex - 1);
			}
		}
	}
	for(int i = 0; i < FinalQuads.Num() - 1; i++)
	{
		for(int j = i + 1; j < FinalQuads.Num(); j++)
		{
			int CommonPoints = 0;
			for(int k = 0; k < 4; k++)
			{
				bool Found = false;
				for(int p = 0; p < 4; p++)
				{
					if(FinalQuads[i].Points[k] == FinalQuads[j].Points[p])
					{
						if(++CommonPoints == 2)
						{
							FinalQuads[i].Neighbours.Add(j);
							FinalQuads[j].Neighbours.Add(i);
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

void AGridGenerator::FindPointNeighboursInQuad(const int QuadIndex)
{
	for(int i = 0; i < 4; i++)
	{
		const int PrevPoint = i - 1 < 0 ? 3 : 0;
		const int NextPoint = (i + 1) % 4;
		GridPoints[FinalQuads[QuadIndex].Points[i]].Neighbours.AddUnique(FinalQuads[QuadIndex].Points[PrevPoint]);
		GridPoints[FinalQuads[QuadIndex].Points[i]].Neighbours.AddUnique(FinalQuads[QuadIndex].Points[NextPoint]);
	}
}

void AGridGenerator::SortQuadPoints(FGridQuad& Quad)
{
	FVector QuadCenter = FVector::ZeroVector;
	for(int i = 0; i < Quad.Points.Num(); i++)
		QuadCenter += GridPoints[Quad.Points[i]].Location;
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
		const FVector& GridCoordinate = GridPoints[Quad.Points[i]].Location;
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
	TArray<FGridPoint>& Points = SecondGrid ? SecondGridPoints : GridPoints;
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

void AGridGenerator::RelaxGridBasedOnSquare(const float SquareSideLength)
{
	//for(uint32 Iterations = 0; Iterations < SquareRelaxIterations; Iterations++)
	if(Debug)
	{
		IterationsUsed1 = 0;
		{
			SquareDelegate1.BindWeakLambda(this, [this, SquareSideLength]()
			{
				if(IterationsUsed1 >= Square1RelaxIterations)
				{
					GetWorld()->GetTimerManager().ClearTimer(SquareHandle1);
					SquareHandle1.Invalidate();
					return;
				}
				const float r = (SquareSideLength * sqrt(2.f)) / 2.f;
				TArray<FVector> GridPointsForce;
				GridPointsForce.SetNumZeroed(GridPoints.Num());
				PerfectQuads.Empty();
				for(int i = 0; i < FinalQuads.Num(); i++)
				{
					TArray<int>& QuadPoints = FinalQuads[i].Points;
					FVector NewCenter = FVector::ZeroVector;
					for(int j = 0; j < 4; j++)
						NewCenter += GridPoints[QuadPoints[j]].Location;
					 NewCenter /= 4.f;
					 FinalQuads[i].Center = NewCenter;
					 float Alpha = 0.f;
					 for(int k = 0; k <= 1; k++)
					 {
						 float Numerator = GridPoints[QuadPoints[0]].Location.Y + GridPoints[QuadPoints[1]].Location.X - GridPoints[QuadPoints[2]].Location.Y - GridPoints[QuadPoints[3]].Location.X;
						  float Denominator = GridPoints[QuadPoints[0]].Location.X - GridPoints[QuadPoints[1]].Location.Y - GridPoints[QuadPoints[2]].Location.X + GridPoints[QuadPoints[3]].Location.Y;
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
					 SquarePoints.Emplace(FVector(r * CosAlpha, r * SinAlpha, 0.f) + FinalQuads[i].Center);
					 SquarePoints.Emplace(FVector(r * SinAlpha, -r * CosAlpha, 0.f) + FinalQuads[i].Center);
					 SquarePoints.Emplace(FVector(-r * CosAlpha, -r * SinAlpha, 0.f) + FinalQuads[i].Center);
					 SquarePoints.Emplace(FVector(-r * SinAlpha, r * CosAlpha, 0.f) + FinalQuads[i].Center);
		
					 for(int k = 0; k < 4; k++)
					 {	
						 FVector Diff = SquarePoints[k] - GridPoints[QuadPoints[k]].Location;
						 Diff.Z = 0.f;
						 GridPointsForce[QuadPoints[k]] += Diff;
					 }
					 PerfectQuads.Add(SquarePoints);
				 }
				 for(int i = 0; i < GridPoints.Num(); i++)
				 {
					 GridPoints[i].Location += GridPointsForce[i] * 0.1f;
					 GridPointsForce[i] = FVector::ZeroVector;
				 }
				 DrawGrid();
				 IterationsUsed1++;
			 });
		}
		GetWorld()->GetTimerManager().SetTimer(SquareHandle1, SquareDelegate1, Order1TimeRate, true);
	}
	else
	{
		for(uint32 Iteration = 0; Iteration < Square1RelaxIterations; Iteration++)
		{
			const float r = (SquareSideLength * sqrt(2.f)) / 2.f;
			TArray<FVector> GridPointsForce;
			GridPointsForce.SetNumZeroed(GridPoints.Num());
			PerfectQuads.Empty();
			for(int i = 0; i < FinalQuads.Num(); i++)
			{
				TArray<int>& QuadPoints = FinalQuads[i].Points;
				FVector NewCenter = FVector::ZeroVector;
				for(int j = 0; j < 4; j++)
					NewCenter += GridPoints[QuadPoints[j]].Location;
				NewCenter /= 4.f;
				FinalQuads[i].Center = NewCenter;
				float Alpha = 0.f;
				for(int k = 0; k <= 1; k++)
				{
				 float Numerator = GridPoints[QuadPoints[0]].Location.Y + GridPoints[QuadPoints[1]].Location.X - GridPoints[QuadPoints[2]].Location.Y - GridPoints[QuadPoints[3]].Location.X;
				  float Denominator = GridPoints[QuadPoints[0]].Location.X - GridPoints[QuadPoints[1]].Location.Y - GridPoints[QuadPoints[2]].Location.X + GridPoints[QuadPoints[3]].Location.Y;
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
				SquarePoints.Emplace(FVector(r * CosAlpha, r * SinAlpha, 0.f) + FinalQuads[i].Center);
				SquarePoints.Emplace(FVector(r * SinAlpha, -r * CosAlpha, 0.f) + FinalQuads[i].Center);
				SquarePoints.Emplace(FVector(-r * CosAlpha, -r * SinAlpha, 0.f) + FinalQuads[i].Center);
				SquarePoints.Emplace(FVector(-r * SinAlpha, r * CosAlpha, 0.f) + FinalQuads[i].Center);
	
				for(int k = 0; k < 4; k++)
				{	
				 FVector Diff = SquarePoints[k] - GridPoints[QuadPoints[k]].Location;
				 Diff.Z = 0.f;
				 GridPointsForce[QuadPoints[k]] += Diff.GetSafeNormal();
				}
				PerfectQuads.Add(SquarePoints);
			 }
			 for(int i = 0; i < GridPoints.Num(); i++)
			 {
				 GridPoints[i].Location += GridPointsForce[i];
				 GridPointsForce[i] = FVector::ZeroVector;
			 }
			 //DrawGrid();
		}
	}
}

void AGridGenerator::RelaxGridBasedOnSquare2()
{
	IterationsUsed2 = 0;
	if(Debug)
	{
		SquareDelegate2.BindWeakLambda(this, [this]()
		{
			if(IterationsUsed2 >= Square2RelaxIterations)
			{
				GetWorld()->GetTimerManager().ClearTimer(SquareHandle2);
				SquareHandle2.Invalidate();
				return;
			}
			TArray<FVector> GridPointsForce;
			GridPointsForce.SetNumZeroed(GridPoints.Num());
			for(int i = 0; i < FinalQuads.Num(); i++)
			{
				TArray<int>& QuadPoints = FinalQuads[i].Points;
				FVector NewCenter = FVector::ZeroVector;
				for(int j = 0; j < 4; j++)
					NewCenter += GridPoints[QuadPoints[j]].Location;
				NewCenter /= 4.f;
				FinalQuads[i].Center = NewCenter;
				FVector QuadForce = FVector::ZeroVector;
				for(int j = 0; j < 4; j++)
				{
					QuadForce += GetPointCoordinates(QuadPoints[j]) - NewCenter;
					QuadForce = FVector(QuadForce.Y, -QuadForce.X, 0.f);
				}
				QuadForce /= 4.f;
				for(int j = 0; j < 4; j++)
				{
					GridPointsForce[QuadPoints[j]] += NewCenter + QuadForce - GetPointCoordinates(QuadPoints[j]);
					QuadForce = FVector(QuadForce.Y, -QuadForce.X, 0.f);
				}
			}
			for(int i = 0; i < GridPoints.Num(); i++)
			{
				GridPoints[i].Location += GridPointsForce[i] * ForceScale;
		   	}
			DrawGrid();
			IterationsUsed2++;
		});
		
		GetWorld()->GetTimerManager().SetTimer(SquareHandle2, SquareDelegate2, Order2TimeRate, true);
	}
	else
	{
		for(uint32 Iteration = 0; Iteration < Square2RelaxIterations; Iteration++)
		{
			TArray<FVector> GridPointsForce;
			GridPointsForce.SetNumZeroed(GridPoints.Num());
			for(int i = 0; i < FinalQuads.Num(); i++)
			{
				TArray<int>& QuadPoints = FinalQuads[i].Points;
				FVector NewCenter = FVector::ZeroVector;
				for(int j = 0; j < 4; j++)
					NewCenter += GridPoints[QuadPoints[j]].Location;
				NewCenter /= 4.f;
				FinalQuads[i].Center = NewCenter;
				FVector QuadForce = FVector::ZeroVector;
				for(int j = 0; j < 4; j++)
				{
					QuadForce += GetPointCoordinates(QuadPoints[j]) - NewCenter;
					QuadForce = FVector(QuadForce.Y, -QuadForce.X, 0.f);
				}
				QuadForce /= 4.f;
				for(int j = 0; j < 4; j++)
				{
					GridPointsForce[QuadPoints[j]] += NewCenter + QuadForce - GetPointCoordinates(QuadPoints[j]);
					QuadForce = FVector(QuadForce.Y, -QuadForce.X, 0.f);
				}
			}
			for(int i = 0; i < GridPoints.Num(); i++)
			{
				GridPoints[i].Location += GridPointsForce[i] * ForceScale;
			}
		}
	}
}

void AGridGenerator::RelaxGridBasedOnNeighbours()
{
	for(uint32 Iteration = 0; Iteration < NeighbourRelaxIterations; Iteration++)
	{
		TArray<FGridPoint> NewGridPoints = GridPoints;
		NewGridPoints.SetNum(GridPoints.Num());
		for(int i = 0; i < GridPoints.Num(); i++)
		{
			FVector Sum = FVector::ZeroVector;
			for(int j = 0; j < GridPoints[i].Neighbours.Num(); j++)
			{
				Sum += GridPoints[GridPoints[i].Neighbours[j]].Location;
			}
			NewGridPoints[i].Location = Sum / static_cast<float>(GridPoints[i].Neighbours.Num());
		}
		GridPoints = NewGridPoints;
	}
}

void AGridGenerator::Relax2()
{
	if(Square1Order == 2)
		RelaxGridBasedOnSquare(SquareSize);
	else if (Square2Order == 2)
		RelaxGridBasedOnSquare2();
	else if (NeighbourOrder == 2)
		RelaxGridBasedOnNeighbours();
}

void AGridGenerator::Relax3()
{
	if(Square1Order == 3)
		RelaxGridBasedOnSquare(SquareSize);
	else if (Square2Order == 3)
		RelaxGridBasedOnSquare2();
	else if (NeighbourOrder == 3)
		RelaxGridBasedOnNeighbours();
}

int AGridGenerator::GetOrAddMidpointIndexInGrid1(TMap<TPair<int, int>, int>& Midpoints, int Point1, int Point2)
{
	if(Point1 > Point2)
		std::swap(Point1, Point2);
	const TPair<int, int> Key(Point1, Point2);
	if(Midpoints.Contains(Key))
	{
		return Midpoints[Key];
	}
	const bool IsEdgePoint = (GridPoints[Point1].IsEdge && GridPoints[Point2].IsEdge);
	GridPoints.Emplace(GridPoints.Num(), (GridPoints[Point1].Location + GridPoints[Point2].Location) / 2.f, IsEdgePoint);
	//Midpoints[Key] = GridPoints.Num() - 1;
	Midpoints.Add(Key, GridPoints.Num() - 1);
	return Midpoints[Key];
}

void AGridGenerator::CreateSecondGrid()
{
	SecondGridShapes.Empty();
	SecondGridPoints.Empty();
	for(int i = 0; i < FinalQuads.Num(); i++)
	{
		bool IsEdge = false;
		int EdgePoints = 0;
		for(int j = 0; j < FinalQuads[i].Points.Num(); j++)
		{
			if(GridPoints[FinalQuads[i].Points[j]].IsEdge)
			{
				EdgePoints++;
				if(EdgePoints >= 2)
				{
					IsEdge = true;
					break;
				}
			}
		}
		SecondGridPoints.Emplace(i, FinalQuads[i].Center, IsEdge);
	}
	
	int NeighbourOffset = 0;
	for(int i = 0; i < GridPoints.Num(); i++)
	{
		if(GridPoints[i].IsEdge)
		{
			NeighbourOffset++;
			continue;
		}
		TArray<int> ShapePoints;
		for(int j = 0; j < GridPoints[i].PartOfQuads.Num(); j++)
		{
			ShapePoints.Add(GridPoints[i].PartOfQuads[j]);
		}
		SecondGridShapes.Emplace(SecondGridShapes.Num(), ShapePoints, i);
		SortShapePoints(SecondGridShapes.Last());
	}
	
	for(int i = 0; i < SecondGridShapes.Num() - 1; i++)
	{
		for(int j = i + 1; j < SecondGridShapes.Num(); j++)
		{
			int CommonPoints = 0;
			for(int k = 0; k < SecondGridShapes[i].Points.Num(); k++)
			{
				bool Found = false;
				for(int p = 0; p < SecondGridShapes[j].Points.Num(); p++)
				{
					if(SecondGridShapes[i].Points[k] == SecondGridShapes[j].Points[p])
					{
						if(++CommonPoints == 2)
						{
							SecondGridShapes[i].Neighbours.Add(j);
							SecondGridShapes[j].Neighbours.Add(i);
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

	//TArray<FVoxelConfig> FirstElevation;

	//if(ShowSecondGrid)
		//DrawSecondGrid();
}

void AGridGenerator::CreateWholeGridMesh()
{
	FGridShape WholeMeshShape;
	for(int i = 0; i < GridPoints.Num(); i++)
	{
		if(GridPoints[i].IsEdge)
			WholeMeshShape.Points.Add(i);
	}
	SortShapePoints(WholeMeshShape, false);
	UE::Geometry::FPolygon2d MeshPolygon;
	for(int i = 0; i < WholeMeshShape.Points.Num(); i++)
	{
		MeshPolygon.AppendVertex(UE::Math::TVector2(GridPoints[WholeMeshShape.Points[i]].Location));
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
	MeshPolygon.GetVertices(), //Vertices2D,
	MeshHeight, // Height
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

bool AGridGenerator::IsPointInShape(const FVector& Point, const FGridShape& Shape) const
{
	auto Sign = [](const FVector& P1, const FVector& P2, const FVector& P3) {
		return (P1.X - P3.X) * (P2.Y - P3.Y) - 
			   (P2.X - P3.X) * (P1.Y - P3.Y);
	};

	const bool SameSide1 = Sign(Point, GetSecondPointCoordinates(Shape.Points[0]), GetSecondPointCoordinates(Shape.Points[1])) > 0;
	for(int i = 1; i < Shape.Points.Num(); i++)
	{
		const bool SameSide2 = Sign(Point, GetSecondPointCoordinates(Shape.Points[i]), GetSecondPointCoordinates(Shape.Points[(i + 1) % Shape.Points.Num()])) > 0;
		if(SameSide1 != SameSide2)
			return false;
	}
		
	return true;
}

int AGridGenerator::DetermineWhichGridShapeAPointIsIn(const FVector& Point)
{
	TArray<bool> Visited;
	//FVector<float> DistanceToQuad;
	//FVector<bool> IsDistanceCalculated;
	Visited.SetNumZeroed(SecondGridShapes.Num());
	//DistanceToQuad.SetNum(FinalQuads.Num());
	int CurrentShape = 0;
	int VisitedQuadNumber = 0;
	//DrawDebugSphere(GetWorld(), Point, 6.f, 8, FColor::Purple, false, 10.f);

	while(VisitedQuadNumber < SecondGridShapes.Num())
	{
		if(IsPointInShape(Point, SecondGridShapes[CurrentShape]))
		{
			UE_LOG(LogTemp, Warning, TEXT("Shape: %d"), CurrentShape);
			return CurrentShape;
		}
		
		Visited[CurrentShape] = true;
		VisitedQuadNumber++;

		float ClosestNeighbourDistance = FLT_MAX;
		int ClosestNeighbour = -1;

		for(int i = 0; i < SecondGridShapes[CurrentShape].Neighbours.Num(); i++)
		{
			const int NeighbourIndex = SecondGridShapes[CurrentShape].Neighbours[i];
			if(Visited[NeighbourIndex])
				continue;
			float DistanceBetween = FVector::DistSquared2D(Point, SecondGridShapes[NeighbourIndex].Center);
			if(DistanceBetween < ClosestNeighbourDistance)
			{
				ClosestNeighbourDistance = DistanceBetween;
				ClosestNeighbour = NeighbourIndex;
			}
		}
		//DrawDebugSphere(GetWorld(), SecondGridShapes[CurrentShape].Center, 5.f, 8, FColor::Yellow, false, 10.f);
		if(ClosestNeighbour == -1)
		{
			return -1;
		}

		CurrentShape = ClosestNeighbour;
	}
	return -1;
}

// Called every frame
void AGridGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AGridGenerator::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	
	//FlushPersistentDebugLines(GetWorld());

	FMath::RandInit(Seed);
	FMath::SRandInit(Seed);
	
	GenerateGrid();
}

void AGridGenerator::DrawGrid()
{
	FlushPersistentDebugLines(GetWorld());
	for(int i = 0; i < FinalQuads.Num(); i++)
	{
		if(FinalQuads[i].Index == -1)
			continue;
		FLinearColor Color = FColor::Red;
		FLinearColor Color2 = FColor::Green;
		FLinearColor Color3 = FColor::Blue;
		FLinearColor Color4 = FColor::Yellow;
		if(ShowGrid)
		{
			float LineThickness = 2.f;
			//if(i != 24)
			//	continue;
			DrawDebugLine(GetWorld(), 
			GetPointCoordinates(FinalQuads[i].Points[0]),
				GetPointCoordinates(FinalQuads[i].Points[1]),
				Color.ToFColor(true),
				true);
			DrawDebugLine(GetWorld(), 
			GetPointCoordinates(FinalQuads[i].Points[1]),
				GetPointCoordinates(FinalQuads[i].Points[2]),
				Color2.ToFColor(true),
				true);
			DrawDebugLine(GetWorld(), 
			GetPointCoordinates(FinalQuads[i].Points[2]),
				GetPointCoordinates(FinalQuads[i].Points[3]),
				Color3.ToFColor(true),
				true);
			DrawDebugLine(GetWorld(), 
			GetPointCoordinates(FinalQuads[i].Points[3]),
				GetPointCoordinates(FinalQuads[i].Points[0]),
				Color4.ToFColor(true),
				true);
		}
		if(!ShowSquares || !PerfectQuads.Num())
			continue;
		//DrawDebugString(GetWorld(), TriangleCenter, *FString::Printf(TEXT("T")), nullptr, FColor::Red, 100.f, true, 5.f);
		DrawDebugLine(GetWorld(), 
			PerfectQuads[i][0],
			PerfectQuads[i][1],
			Color.ToFColor(true),
			true);
		DrawDebugLine(GetWorld(), 
			PerfectQuads[i][1],
				PerfectQuads[i][2],
				Color2.ToFColor(true),
				true);
		DrawDebugLine(GetWorld(), 
			PerfectQuads[i][2],
				PerfectQuads[i][3],
				Color3.ToFColor(true),
				true);
		DrawDebugLine(GetWorld(), 
			PerfectQuads[i][3],
				PerfectQuads[i][0],
				Color4.ToFColor(true),
				true);
	}
}

void AGridGenerator::DrawSecondGrid()
{
	FlushPersistentDebugLines(GetWorld());
	TArray<FLinearColor> Colors = {FColor::Red, FColor::Green, FColor::Blue, FColor::Yellow, FColor::Purple, FColor::Emerald};
	for(int i = 0; i < SecondGridShapes.Num(); i++)
	{
		if(SecondGridShapes[i].Index == -1 || i != 1)
			continue;
		if(ShowSecondGrid)
		{
			float LineThickness = 2.f;
			for(int j = 0; j < SecondGridShapes[i].Points.Num(); j++)
			{
			DrawDebugLine(GetWorld(), 
			GetPointCoordinates(SecondGridShapes[i].Points[j]),
				GetPointCoordinates(SecondGridShapes[i].Points[(j + 1) % SecondGridShapes[i].Points.Num()]),
				Colors[j].ToFColor(true),
				true);
			}
		}
	}
}

void AGridGenerator::GenerateGrid()
{
	GridPoints.Empty();
	Triangles.Empty();
	Quads.Empty();
	FinalQuads.Empty();
	PerfectQuads.Empty();
	BuildingPieces.Empty();
	MarchingBits.Empty();
	//FirstElevation.Empty();
	//VoxelConfig.Empty();
	Center = GetActorLocation();
	GridPoints.Emplace(GridPoints.Num(), Center);
	for(uint32 i = 0; i < GridSize - 1; i++)
	{
		GenerateHexCoordinates(Center, HexSize * (i + 1), i);
	}
	DivideGridIntoTriangles(Center);
	DivideGridIntoQuads(Center);

	uint32 It1 = 0;
	if(Square1Order == 1)
		RelaxGridBasedOnSquare(SquareSize), It1 = Square1RelaxIterations;
	else if (Square2Order == 1)
		RelaxGridBasedOnSquare2(), It1 = Square2RelaxIterations;
	else if (NeighbourOrder == 1)
		RelaxGridBasedOnNeighbours(), It1 = NeighbourRelaxIterations;
	
	uint32 It2 = 0;
	if(Square1Order == 2)
		It2 = Square1RelaxIterations;
	else if (Square2Order == 2)
		It2 = Square2RelaxIterations;
	else if (NeighbourOrder == 2)
		It2 = NeighbourRelaxIterations;

	uint32 It3 = 0;
	if(Square1Order == 2)
		It3 = Square1RelaxIterations;
	else if (Square2Order == 2)
		It3 = Square2RelaxIterations;
	else if (NeighbourOrder == 2)
		It3 = NeighbourRelaxIterations;

	if(Debug)
	{
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, Delegate1, Order1TimeRate * (It1 + 1), false, Order1TimeRate * (It1 + 1));
		GetWorld()->GetTimerManager().SetTimer(TimerHandle2, Delegate2, Order1TimeRate * (It1 + 1) + Order2TimeRate * 0.01f * (It2 + 1), false, Order1TimeRate * (It1 + 1) + Order2TimeRate * 0.01f * (It2 + 1));
		GetWorld()->GetTimerManager().SetTimer(TimerHandle3, Delegate3, Order1TimeRate * (It1 + 1) + Order2TimeRate * 0.01f * (It2 + 1) + + Order3TimeRate * 0.01f * (It3 + 1), false, Order1TimeRate * (It1 + 1) + Order2TimeRate * 0.01f * (It2 + 1) + + Order3TimeRate * 0.01f * (It3 + 1));
	}
	else
	{
		Relax2();
		Relax3();
		CreateWholeGridMesh();
		CreateSecondGrid();
	}
}

void AGridGenerator::CreateShapeMesh(const int ShapeIndex)
{
	UE::Geometry::FPolygon2d MeshPolygon;
	for(int i = 0; i < SecondGridShapes[ShapeIndex].Points.Num(); i++)
	{
		MeshPolygon.AppendVertex(UE::Math::TVector2(SecondGridPoints[SecondGridShapes[ShapeIndex].Points[i]].Location));
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

void AGridGenerator::ResetShapeMesh()
{
	HoveredShapeMesh->GetDynamicMesh()->Reset();
}
