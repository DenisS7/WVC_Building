// Fill out your copyright notice in the Description page of Project Settings.


#include "GridGenerator.h"

// Sets default values
AGridGenerator::AGridGenerator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AGridGenerator::BeginPlay()
{
	Super::BeginPlay();
	
}

void AGridGenerator::GenerateHex(const FVector& Center, const float Size, const uint32 Index)
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
		HexPoints[i * NextIndex] = FVector(Center.X + Size * FMath::Cos(AngleRad), Center.Y + Size * FMath::Sin(AngleRad), Center.Z);
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
	for(uint32 i = 0; i < GridSize; i++)
		for(uint32 j = 0; j < 6 * (i + 1); j++)
		{
			int NextPointIndex = j;
			if(NextPointIndex == 6 * (i + 1) - 1)
				NextPointIndex = 0;
			//DrawDebugLine(GetWorld(), GridCoordinates[i][j], GridCoordinates[i][NextPointIndex], FColor::Red, true, -1, 0, 5.f);
			DrawDebugPoint(GetWorld(), GridCoordinates[i][j], 4.f, FColor::Blue, true);
		}
}

void AGridGenerator::GenerateGrid()
{
	GridCoordinates.Empty();
	FVector ActorLocation = GetActorLocation();
	for(uint32 i = 0; i < GridSize; i++)
	{
		GenerateHex(ActorLocation, HexSize * (i + 1), i);
	}
}

