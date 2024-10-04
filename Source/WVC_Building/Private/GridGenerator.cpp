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
		const float AngleRad = FMath::DegreesToRadians(60.f * i - 30.f);
		HexPoints[i * NextIndex] = FVector(GridCenter.X + Size * FMath::Cos(AngleRad), GridCenter.Y + Size * FMath::Sin(AngleRad), GridCenter.Z);
	}
	const float FractionDistanceBetween = 1.f / static_cast<float>(NextIndex);
	for(uint32 i = 0; i < 6; i++)
	{
		for(uint32 j = 0; j < Index; j++)
		{
			const int PointIndex = i * NextIndex + j + 1;
			const float FractionDist = static_cast<float>(j + 1) * FractionDistanceBetween;
			HexPoints[PointIndex] = FractionDist * HexPoints[i * NextIndex] +
												(1.f - FractionDist) * HexPoints[(((i + 1) % 6) * NextIndex)];
		}
	}
	GridCoordinates.Emplace(HexPoints);
}

void AGridGenerator::DivideGridIntoTriangles(const FVector& GridCenter)
{
	if(!GridCoordinates.Num())
		return;

	int TriangleIndex = 0;
	
	for(int i = 0; i < GridCoordinates.Num() - 1; i++)
	{
		for(int j = 0; j < 6; j++)
		{
			const int SmallMaxCoordinate = GridCoordinates[i].Num();
			const int LargeMaxCoordinate = GridCoordinates[i + 1].Num();
			int SmallCoordinate = j * i;
			int LargeCoordinate = j * (i + 1);
			const int TrianglesPerSide = i * 2 + 1;
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
				Triangles.Emplace(TrianglePoints, TriangleIndex++);
			}
		}
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
		if(Triangles[i].Index == -1)
			continue;
		FLinearColor Color = FColor::Red;
		FLinearColor Color2 = FColor::Green;
		FLinearColor Color3 = FColor::Blue;
		float LineThickness = 2.f;
		DrawDebugLine(GetWorld(), 
		GetGridCoordinate(Triangles[i].Points[0].X, Triangles[i].Points[0].Y),
			GetGridCoordinate(Triangles[i].Points[1].X, Triangles[i].Points[1].Y),
			Color.ToFColor(true),
			true);
		DrawDebugLine(GetWorld(), 
		GetGridCoordinate(Triangles[i].Points[1].X, Triangles[i].Points[1].Y),
			GetGridCoordinate(Triangles[i].Points[2].X, Triangles[i].Points[2].Y),
			Color2.ToFColor(true),
			true);
		DrawDebugLine(GetWorld(), 
		GetGridCoordinate(Triangles[i].Points[2].X, Triangles[i].Points[2].Y),
			GetGridCoordinate(Triangles[i].Points[0].X, Triangles[i].Points[0].Y),
			Color3.ToFColor(true),
			true);
		
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

