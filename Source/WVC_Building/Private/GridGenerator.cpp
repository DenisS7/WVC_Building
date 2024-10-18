// Fill out your copyright notice in the Description page of Project Settings.


#include "GridGenerator.h"

#include "DebugStrings.h"
#include "Runtime/Core/Tests/Containers/TestUtils.h"

// Sets default values
AGridGenerator::AGridGenerator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	DebugStringsComp = CreateDefaultSubobject<UDebugStrings>(TEXT("DebugStringsComponent"));
}

// Called when the game starts or when spawned
void AGridGenerator::BeginPlay()
{
	Super::BeginPlay();
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
	
	for(uint32 i = 0; i < 6; i++)
	{
		const float AngleRad = FMath::DegreesToRadians(60.f * i - 150.f);
		//HexPoints[i * NextIndex] = FVector(GridCenter.X + Size * FMath::Cos(AngleRad), GridCenter.Y + Size * FMath::Sin(AngleRad), GridCenter.Z);
		GridPoints[PrevNumPoints+ i * NextIndex] = FGridPoint(PrevNumPoints+ i * NextIndex, FVector(GridCenter.X + Size * FMath::Cos(AngleRad), GridCenter.Y + Size * FMath::Sin(AngleRad), GridCenter.Z));
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
									(1.f - FractionDist) * GridPoints[PrevNumPoints + i * NextIndex].Location);
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
	int ok = 0;
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
			TriangleMidpoints.Add(GetOrAddMidpointIndex(Midpoints, TrPoints[j], TrPoints[(j + 1) % 3]));
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
		}
	}
	for(int i = 0; i < Quads.Num(); i++)
	{
		TArray<int> QuadMidpoints;
		for(int j = 0; j < 4; j++)
		{
			QuadMidpoints.Add(GetOrAddMidpointIndex(Midpoints, Quads[i].Points[j], Quads[i].Points[(j + 1) % 4]));
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

void AGridGenerator::RelaxGridBasedOnSquare(const float SquareSideLength)
{
	const float r = (SquareSideLength * sqrt(2.f)) / 2.f;
	TArray<FVector> GridPointsForce;
	GridPointsForce.SetNumZeroed(GridPoints.Num());
	for(uint32 Iterations = 0; Iterations < SquareRelaxIterations; Iterations++)
	{
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
	}
}

void AGridGenerator::RelaxGridBasedOnSquare2()
{
	for(uint32 Iteration = 0; Iteration < SquareRelaxIterations; Iteration++)
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

int AGridGenerator::GetOrAddMidpointIndex(TMap<TPair<int, int>, int>& Midpoints, int Point1, int Point2)
{
	if(Point1 > Point2)
		std::swap(Point1, Point2);
	const TPair<int, int> Key(Point1, Point2);
	if(Midpoints.Contains(Key))
	{
		return Midpoints[Key];
	}
	GridPoints.Emplace(GridPoints.Num(), (GridPoints[Point1].Location + GridPoints[Point2].Location) / 2.f);
	//Midpoints[Key] = GridPoints.Num() - 1;
	Midpoints.Add(Key, GridPoints.Num() - 1);
	return Midpoints[Key];
}

// Called every frame
void AGridGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AGridGenerator::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	
	FlushPersistentDebugLines(GetWorld());

	FMath::RandInit(Seed);
	FMath::SRandInit(Seed);
	
	GenerateGrid();
	//for(int i = 0; i < GridCoordinates.Num(); i++)
	//	for(int j = 0; j < GridCoordinates[i].Num(); j++)
	//	{
	//		DrawDebugPoint(GetWorld(), GridCoordinates[i][j], 4.f, FColor::Blue, true);
	//	}

	for(int i = 0; i < GridPoints.Num(); i++)
	{
		DrawDebugPoint(GetWorld(), GridPoints[i].Location, 4.f, FColor::Blue, true);
	}

	//for(int i = 0; i < Triangles.Num(); i++)
	//{
	//	if(Triangles[i].Index == -1)
	//		continue;
	//	FLinearColor Color = FColor::Red;
	//	FLinearColor Color2 = FColor::Green;
	//	FLinearColor Color3 = FColor::Blue;
	//	float LineThickness = 2.f;
	//	//if(i != 24)
	//	//	continue;
	//	DrawDebugLine(GetWorld(), 
	//	GridPoints[Triangles[i].Points[0]],
	//		GridPoints[Triangles[i].Points[1]],
	//		Color.ToFColor(true),
	//		true);
	//	DrawDebugLine(GetWorld(), 
	//	GridPoints[Triangles[i].Points[1]],
	//		GridPoints[Triangles[i].Points[2]],
	//		Color2.ToFColor(true),
	//		true);
	//	DrawDebugLine(GetWorld(), 
	//	GridPoints[Triangles[i].Points[2]],
	//		GridPoints[Triangles[i].Points[0]],
	//		Color3.ToFColor(true),
	//		true);
	//	//DrawDebugString(GetWorld(), TriangleCenter, *FString::Printf(TEXT("T")), nullptr, FColor::Red, 100.f, true, 5.f);
	//}

	//for(int i = 0; i < Quads.Num(); i++)
	//{
	//	if(Quads[i].Index == -1)
	//		continue;
	//	FLinearColor Color = FColor::Red;
	//	FLinearColor Color2 = FColor::Green;
	//	FLinearColor Color3 = FColor::Blue;
	//	FLinearColor Color4 = FColor::Yellow;
	//	float LineThickness = 2.f;
	//	//if(i != 24)
	//	//	continue;
	//	DrawDebugLine(GetWorld(), 
	//	GetPointCoordinates(Quads[i].Points[0]),
	//		GetPointCoordinates(Quads[i].Points[1]),
	//		Color.ToFColor(true),
	//		true);
	//	DrawDebugLine(GetWorld(), 
	//	GetPointCoordinates(Quads[i].Points[1]),
	//		GetPointCoordinates(Quads[i].Points[2]),
	//		Color2.ToFColor(true),
	//		true);
	//	DrawDebugLine(GetWorld(), 
	//	GetPointCoordinates(Quads[i].Points[2]),
	//		GetPointCoordinates(Quads[i].Points[3]),
	//		Color3.ToFColor(true),
	//		true);
	//	DrawDebugLine(GetWorld(), 
	//	GetPointCoordinates(Quads[i].Points[3]),
	//		GetPointCoordinates(Quads[i].Points[0]),
	//		Color4.ToFColor(true),
	//		true);
	//	//DrawDebugString(GetWorld(), TriangleCenter, *FString::Printf(TEXT("T")), nullptr, FColor::Red, 100.f, true, 5.f);
	//}
	DrawGrid();

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

void AGridGenerator::GenerateGrid()
{
	GridPoints.Empty();
	Triangles.Empty();
	Quads.Empty();
	FinalQuads.Empty();
	PerfectQuads.Empty();
	Center = GetActorLocation();
	GridPoints.Emplace(GridPoints.Num(), Center);
	for(uint32 i = 0; i < GridSize; i++)
	{
		GenerateHexCoordinates(Center, HexSize * (i + 1), i);
	}
	DivideGridIntoTriangles(Center);
	DivideGridIntoQuads(Center);
	if(DoSquareRelaxationFirst)
	{
		//RelaxGridBasedOnSquare(SquareSize);
		RelaxGridBasedOnSquare2();
		RelaxGridBasedOnNeighbours();
	}
	else
	{
		RelaxGridBasedOnNeighbours();
		RelaxGridBasedOnSquare2();
		//RelaxGridBasedOnSquare(SquareSize);
	}
}
