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
	TArray<FVector> HexPoints;
	HexPoints.SetNum(NumPoints);
	
	for(uint32 i = 0; i < 6; i++)
	{
		const float AngleRad = FMath::DegreesToRadians(60.f * i - 150.f);
		HexPoints[i * NextIndex] = FVector(GridCenter.X + Size * FMath::Cos(AngleRad), GridCenter.Y + Size * FMath::Sin(AngleRad), GridCenter.Z);
	}
	const float FractionDistanceBetween = 1.f / static_cast<float>(NextIndex);
	for(uint32 i = 0; i < 6; i++)
	{
		for(uint32 j = 0; j < Index; j++)
		{
			const int PointIndex = i * NextIndex + j + 1;
			const float FractionDist = static_cast<float>(j + 1) * FractionDistanceBetween;
			HexPoints[PointIndex] = FractionDist * HexPoints[(((i + 1) % 6) * NextIndex)] +
												(1.f - FractionDist) * HexPoints[i * NextIndex];
		}
	}
	GridCoordinates.Emplace(HexPoints);
}

void AGridGenerator::DivideGridIntoTriangles(const FVector& GridCenter)
{
	if(!GridCoordinates.Num())
		return;

	
	for(int i = 0; i < GridCoordinates.Num() - 1; i++)
	{
		TArray<FGridTriangle> TrianglesOnHex;
		const int SmallMaxCoordinate = GridCoordinates[i].Num();
		const int LargeMaxCoordinate = GridCoordinates[i + 1].Num();
		const int TrianglesPerSide = i * 2 + 1;
		const int NumTriangles = 6 * TrianglesPerSide;
		int TriangleIndex = 0;
		for(int j = 0; j < 6; j++)
		{
			int SmallCoordinate = j * i;
			int LargeCoordinate = j * (i + 1);
			bool LargeTriangle = true;
			for(int k = 0; k < TrianglesPerSide; k++, LargeTriangle = !LargeTriangle)
			{
				TArray<FIntPoint> TrianglePoints;
				if(LargeTriangle)
				{
					TrianglePoints.Emplace(i, SmallCoordinate);
					TrianglePoints.Emplace(i + 1, LargeCoordinate);
					TrianglePoints.Emplace(i + 1, (LargeCoordinate + 1) % LargeMaxCoordinate);
					LargeCoordinate = (LargeCoordinate + 1) % LargeMaxCoordinate;
				}
				else
				{
					TrianglePoints.Emplace(i, SmallCoordinate);
					TrianglePoints.Emplace(i, (SmallCoordinate + 1) % SmallMaxCoordinate);
					TrianglePoints.Emplace(i + 1, LargeCoordinate);
					SmallCoordinate = (SmallCoordinate + 1) % SmallMaxCoordinate;
				}
				TArray<FIntPoint> Neighbours;
				Neighbours.Add({i, (TriangleIndex + 1) % NumTriangles});
				if(j == 0 && k == 0) //first triangle
				{
					Neighbours.Add({i, NumTriangles - 1});
				}
				else
				{
					Neighbours.Add({i, TriangleIndex - 1});
				}
				if(k % 2 == 0)
				{
					if(i + 1 < GridCoordinates.Num() - 1)
						Neighbours.Add({i + 1, TriangleIndex + j * 2 + 1});
				}
				else if (k % 2 == 1 && i)
				{
					if(i > 0)
						Neighbours.Add({i - 1, TriangleIndex - j * 2 - 1});
				}
				TrianglesOnHex.Emplace(TrianglePoints, TriangleIndex++, Neighbours);
			}
		}
		Triangles.Emplace(TrianglesOnHex);
	}
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
	for(int i = 0; i < GridCoordinates.Num(); i++)
		for(int j = 0; j < GridCoordinates[i].Num(); j++)
		{
			DrawDebugPoint(GetWorld(), GridCoordinates[i][j], 4.f, FColor::Blue, true);
		}

	for(int i = 0; i < Triangles.Num(); i++)
	{
		for(int j = 0; j < Triangles[i].Num(); j++)
		{
			if(Triangles[i][j].Index == -1)
				continue;
			FLinearColor Color = FColor::Red;
			FLinearColor Color2 = FColor::Green;
			FLinearColor Color3 = FColor::Blue;
			float LineThickness = 2.f;
			//if(i != 24)
			//	continue;
			DrawDebugLine(GetWorld(), 
			GetGridCoordinate(Triangles[i][j].Points[0].X, Triangles[i][j].Points[0].Y),
				GetGridCoordinate(Triangles[i][j].Points[1].X, Triangles[i][j].Points[1].Y),
				Color.ToFColor(true),
				true);
			DrawDebugLine(GetWorld(), 
			GetGridCoordinate(Triangles[i][j].Points[1].X, Triangles[i][j].Points[1].Y),
				GetGridCoordinate(Triangles[i][j].Points[2].X, Triangles[i][j].Points[2].Y),
				Color2.ToFColor(true),
				true);
			DrawDebugLine(GetWorld(), 
			GetGridCoordinate(Triangles[i][j].Points[2].X, Triangles[i][j].Points[2].Y),
				GetGridCoordinate(Triangles[i][j].Points[0].X, Triangles[i][j].Points[0].Y),
				Color3.ToFColor(true),
				true);
		}
		
		//DrawDebugString(GetWorld(), TriangleCenter, *FString::Printf(TEXT("T")), nullptr, FColor::Red, 100.f, true, 5.f);
	}
}

void AGridGenerator::GenerateGrid()
{
	GridCoordinates.Empty();
	Triangles.Empty();
	Center = GetActorLocation();
	GridCoordinates.Emplace(TArray<FVector>{Center});
	for(uint32 i = 0; i < GridSize; i++)
	{
		GenerateHexCoordinates(Center, HexSize * (i + 1), i);
	}
	DivideGridIntoTriangles(Center);
}

