// Fill out your copyright notice in the Description page of Project Settings.


#include "GridGenerator.h"

#include "DebugStrings.h"

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
		GridPoints[PrevNumPoints+ i * NextIndex] = FVector(GridCenter.X + Size * FMath::Cos(AngleRad), GridCenter.Y + Size * FMath::Sin(AngleRad), GridCenter.Z);
		
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
			GridPoints[PrevNumPoints + PointIndex] = FractionDist * GridPoints[PrevNumPoints + (((i + 1) % 6) * NextIndex)] +
									(1.f - FractionDist) * GridPoints[PrevNumPoints + i * NextIndex];
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
			TriangleCenter += GridPoints[Triangles[TrianglesLeft[i]].Points[j]];
		TriangleCenter /= 3.f;
		TArray<int> TriangleMidpoints;
		for(int j = 0; j < 3; j++)
		{
			TriangleMidpoints.Add(GetMidpointIndex(Midpoints, TrPoints[j], TrPoints[(j + 1) % 3]));
		}
		GridPoints.Add(TriangleCenter);

		TArray<int> Quad1Points = {TrPoints[0], TriangleMidpoints[0], GridPoints.Num() - 1, TriangleMidpoints[2]};
		TArray<int> Quad2Points = {TrPoints[1], TriangleMidpoints[1], GridPoints.Num() - 1, TriangleMidpoints[0]};
		TArray<int> Quad3Points = {TrPoints[2], TriangleMidpoints[2], GridPoints.Num() - 1, TriangleMidpoints[1]};
		TArray<TArray<int>> QuadsPoints = {Quad1Points, Quad2Points, Quad3Points};
		for(int j = 0; j < 3; j++)
		{
			FinalQuads.Add({QuadsPoints[j], FinalQuadIndex++});
			SortQuadPoints(FinalQuads.Last());
		}
	}
	for(int i = 0; i < Quads.Num(); i++)
	{
		TArray<int> QuadMidpoints;
		for(int j = 0; j < 4; j++)
		{
			QuadMidpoints.Add(GetMidpointIndex(Midpoints, Quads[i].Points[j], Quads[i].Points[(j + 1) % 4]));
			//GridPoints.Add((GridPoints[Quads[i].Points[j]] + GridPoints[Quads[i].Points[(j + 1) % 4]]) / 2.f);
		}
		GridPoints.Add(Quads[i].Center);
		
		TArray<int> Quad1Points = {Quads[i].Points[0], QuadMidpoints[0], GridPoints.Num() - 1, QuadMidpoints[3]};
		TArray<int> Quad2Points = {Quads[i].Points[1], QuadMidpoints[1], GridPoints.Num() - 1, QuadMidpoints[0]};
		TArray<int> Quad3Points = {Quads[i].Points[2], QuadMidpoints[2], GridPoints.Num() - 1, QuadMidpoints[1]};
		TArray<int> Quad4Points = {Quads[i].Points[3], QuadMidpoints[3], GridPoints.Num() - 1, QuadMidpoints[2]};
		TArray<TArray<int>> QuadsPoints = {Quad1Points, Quad2Points, Quad3Points, Quad4Points};
		for(int j = 0; j < 4; j++)
		{
			FinalQuads.Add({QuadsPoints[j], FinalQuadIndex++});
			SortQuadPoints(FinalQuads.Last());
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

void AGridGenerator::SortQuadPoints(FGridQuad& Quad)
{
	FVector QuadCenter = FVector::ZeroVector;
	for(int i = 0; i < Quad.Points.Num(); i++)
		QuadCenter += GridPoints[Quad.Points[i]];
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
		const FVector GridCoordinate = GridPoints[Quad.Points[i]];
		Angles.Add(FPointAngle(i, FMath::Atan2(GridCoordinate.Y - QuadCenter.Y, GridCoordinate.X - QuadCenter.X)));
	}
	Angles.Sort([](const FPointAngle& A, const FPointAngle& B)
	{
		return A.Angle < B.Angle; // Counterclockwise sorting
	});
	const TArray<int> CopyPoints = Quad.Points;
	for(int i = 0; i < Quad.Points.Num(); i++)
	{
		Quad.Points[i] = CopyPoints[Angles[i].Index];
	}
}

void AGridGenerator::RelaxGrid()
{
	for(int i = 0; i < FinalQuads.Num(); i++)
	{
		
	}
}

int AGridGenerator::GetMidpointIndex(TMap<TPair<int, int>, int>& Midpoints, int Point1, int Point2)
{
	if(Point1 > Point2)
		std::swap(Point1, Point2);
	const TPair<int, int> Key(Point1, Point2);
	if(Midpoints.Contains(Key))
	{
		return Midpoints[Key];
	}
	GridPoints.Add((GridPoints[Point1] + GridPoints[Point2]) / 2.f);
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
	GenerateGrid();
	//for(int i = 0; i < GridCoordinates.Num(); i++)
	//	for(int j = 0; j < GridCoordinates[i].Num(); j++)
	//	{
	//		DrawDebugPoint(GetWorld(), GridCoordinates[i][j], 4.f, FColor::Blue, true);
	//	}

	for(int i = 0; i < GridPoints.Num(); i++)
	{
		DrawDebugPoint(GetWorld(), GridPoints[i], 4.f, FColor::Blue, true);
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

	for(int i = 0; i < FinalQuads.Num(); i++)
	{
		if(FinalQuads[i].Index == -1)
			continue;
		FLinearColor Color = FColor::Red;
		FLinearColor Color2 = FColor::Green;
		FLinearColor Color3 = FColor::Blue;
		FLinearColor Color4 = FColor::Yellow;
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
		//DrawDebugString(GetWorld(), TriangleCenter, *FString::Printf(TEXT("T")), nullptr, FColor::Red, 100.f, true, 5.f);
	}
}

void AGridGenerator::GenerateGrid()
{
	GridPoints.Empty();
	Triangles.Empty();
	Quads.Empty();
	FinalQuads.Empty();
	Center = GetActorLocation();
	GridPoints.Add(Center);
	for(uint32 i = 0; i < GridSize; i++)
	{
		GenerateHexCoordinates(Center, HexSize * (i + 1), i);
	}
	DivideGridIntoTriangles(Center);
	DivideGridIntoQuads(Center);
	RelaxGrid();
}