#include "UtilityLibrary.h"

#include "BuildingPiece.h"
#include "GridGenerator.h"
#include "MeshCornersData.h"
#include "ProceduralMeshComponent.h"
#include "VectorTypes.h"

void UtilityLibrary::GetGridAndShapeMouseIsHoveringOver(const UWorld* World, AGridGenerator*& Grid, int& ShapeIndex)
{
	FHitResult HitResult;
	FVector WorldLocation;
	FVector WorldDirection;
	World->GetFirstPlayerController()->DeprojectMousePositionToWorld(WorldLocation, WorldDirection);
	const bool bHit = World->LineTraceSingleByChannel(HitResult,
		WorldLocation,
		WorldLocation + WorldDirection * 10000.f,
		ECC_Visibility);

	if(bHit)
	{
		Grid = Cast<AGridGenerator>(HitResult.GetActor());
		if(Grid)
		{
			ShapeIndex = Grid->DetermineWhichGridShapeAPointIsIn(HitResult.Location);
		}
	}
}

bool UtilityLibrary::GetGridAndBuildingMouseIsHoveringOver(const UWorld* World, AGridGenerator*& Grid,
                                                           int& HitBuildingIndex, int& HitBuildingElevation, int& AdjacentHitBuildingIndex, int& AdjacentHitBuildingElevation)
{
	if(!World)
		return false;
	FHitResult HitResult;
	FVector WorldLocation;
	FVector WorldDirection;
	World->GetFirstPlayerController()->DeprojectMousePositionToWorld(WorldLocation, WorldDirection);
	const bool bHit = World->LineTraceSingleByChannel(HitResult,
		WorldLocation,
		WorldLocation + WorldDirection * 10000.f,
		ECC_Visibility);

	Grid = Cast<AGridGenerator>(HitResult.GetActor());
	if(Grid)
	{
		//Hits grid
		HitBuildingIndex = -1;
		HitBuildingIndex = Grid->DetermineWhichGridShapeAPointIsIn(HitResult.Location);
		HitBuildingElevation = 0;
		if(HitBuildingIndex != -1)
			return true;
	}
	else
	{
		ABuildingPiece* BuildingPiece = Cast<ABuildingPiece>(HitResult.GetActor());
		if(BuildingPiece)
		{
			Grid = BuildingPiece->GetGrid();
			if(!Grid)
				return false;
			
			const int BaseShapeIndex = Grid->DetermineWhichGridShapeAPointIsIn(FVector(HitResult.Location.X, HitResult.Location.Y, Grid->GetActorLocation().Z));
			if(BaseShapeIndex < 0)
				return false;
			HitBuildingIndex = BaseShapeIndex;
			HitBuildingElevation = AdjacentHitBuildingElevation = BuildingPiece->GetElevation();
			const UProceduralMeshComponent* MeshComp = BuildingPiece->GetProceduralMeshComponent();
			FBoxSphereBounds MeshBounds = MeshComp->GetLocalBounds();
			//UE_LOG(LogTemp, Warning, TEXT("MeshBounds: %s, HitBuildingElevation: %d"), *MeshBounds.GetBox().GetExtent().ToString(), HitBuildingElevation);
			if(HitResult.Location.Z < static_cast<float>(AdjacentHitBuildingElevation) * 200.f + 10.f)
			{
				--AdjacentHitBuildingElevation;
				if(AdjacentHitBuildingElevation < 0)
					return false;
				AdjacentHitBuildingIndex = BaseShapeIndex;
				//UE_LOG(LogTemp, Warning, TEXT("BOTTOM MeshBounds: %s, HitBuildingElevation: %d"), *MeshBounds.GetBox().GetExtent().ToString(), HitBuildingElevation);
				return true;
			}
			if(HitResult.Location.Z > static_cast<float>(AdjacentHitBuildingElevation) * 200.f + MeshBounds.Origin.Z + MeshBounds.GetBox().GetExtent().Z - 10.f
				|| !Grid->GetElevationData(AdjacentHitBuildingElevation + 1).MarchingBits[Grid->GetBuildingGridShapes()[HitBuildingIndex].CorrespondingBaseGridPoint])
			{
				++AdjacentHitBuildingElevation;
				if(AdjacentHitBuildingElevation >= Grid->GetMaxElevation())
					return false;
				AdjacentHitBuildingIndex = BaseShapeIndex;
				//UE_LOG(LogTemp, Warning, TEXT("TOP MeshBounds: %s, HitBuildingElevation: %d"), *MeshBounds.GetBox().GetExtent().ToString(), HitBuildingElevation);
				return true;
			}
			
			const FGridShape GridShape = Grid->GetBuildingGridShapes()[HitBuildingIndex];
			float LeastDotDiff = 9999999999999999999999.f;

			for(int i = 0; i < GridShape.Points.Num(); i++)
			{
				const int Index1 = GridShape.Points[i];
				const int Index2 = GridShape.Points[(i + 1) % GridShape.Points.Num()];

				const FVector Point1 = Grid->GetBuildingPointCoordinates(Index1);
				const FVector Point2 = Grid->GetBuildingPointCoordinates(Index2);

				const FVector Segment = Point2 - Point1;
				FVector Perpendicular = FVector(-Segment.Y, Segment.X, 0.f);
				Perpendicular.Normalize();

				FVector Direction = GridShape.Center - Point1;
				Direction.Normalize();
				//const float Dot = 
				if(UE::Geometry::Dot(Perpendicular, Direction) < 0.f)
					Perpendicular *= -1.f;
				
				const float DotProduct = UE::Geometry::Dot(Perpendicular, (FVector(HitResult.Location.X, HitResult.Location.Y, 0.f) - FVector(GridShape.Center.X, GridShape.Center.Y, 0.f)).GetSafeNormal());

				if(DotProduct < LeastDotDiff)
				{
					LeastDotDiff = DotProduct;
					if(GridShape.OffsetNeighbours.Num() <= i)
						return false;
					AdjacentHitBuildingIndex = GridShape.OffsetNeighbours[i];
					AdjacentHitBuildingElevation = BuildingPiece->GetElevation();
				}
			}
			//UE_LOG(LogTemp, Warning, TEXT("BuildingIndex: %d ----- Adjacent: %d\n"), HitBuildingIndex, AdjacentHitBuildingIndex);
			//UE_LOG(LogTemp, Warning, TEXT("          Points: %s\n"), *PointsArray);
			//UE_LOG(LogTemp, Warning, TEXT("          Neighb: %s\n"), *NeighboursArray);
			//UE_LOG(LogTemp, Warning, TEXT("          OffsetNeighb: %s\n"), *OffsetNeighboursArray);
			if(AdjacentHitBuildingIndex == -1)
				return false;
			return true;
		}
	}
	return false;
}

// void UtilityLibrary::UpdateBuildingPiece(UWorld* World, AGridGenerator* Grid, const int QuadIndex, const TSubclassOf<ABuildingPiece>& BuildingPieceToSpawn)
// {
// 	const FGridQuad& CorrespondingQuad = Grid->GetFinalQuads()[QuadIndex];
// 	int MinCorner = -1;
// 	TArray<FVector> CageBase;
// 	for(int k = 0; k < 4; k++)
// 		CageBase.Add(Grid->GetPointCoordinates(CorrespondingQuad.Points[k]));
// 	auto Find = Grid->BuildingPieces.Find(TPair<int, int>(0, QuadIndex));
// 	ABuildingPiece* BuildingPiece;
// 	if(!Find)
// 	{
// 		BuildingPiece = World->SpawnActor<ABuildingPiece>(BuildingPieceToSpawn, CorrespondingQuad.Center, FRotator(0, 0, 0));
// 		Grid->BuildingPieces.Add(TPair<int, int>(0, QuadIndex), BuildingPiece);
// 	}
// 	else
// 	{
// 		BuildingPiece = *Find;
// 		BuildingPiece->Corners.Empty();
// 	}
// 	int TileConfig = 0;
// 	TArray<int> LowerCorners;
// 	TArray<int> UpperCorners;
// 	TArray<bool> CornersMask;
// 	for(int j = 0; j < 8; j++)
// 	{
// 		TileConfig += Grid->MarchingBits[j / 4][CorrespondingQuad.Points[j % 4]] * pow(10, 8 - j - 1);
// 		CornersMask.Add(Grid->MarchingBits[j / 4][CorrespondingQuad.Points[j % 4]]);
// 		if(Grid->MarchingBits[j / 4][CorrespondingQuad.Points[j % 4]])
// 		{
// 			if(j < 4)
// 				LowerCorners.Add(j);
// 			else
// 				UpperCorners.Add(j);
// 			if(MinCorner == -1)
// 				MinCorner = j;
// 		}
// 	}
// 	float Rotation = 0.f;
// 	
// 	if(LowerCorners.Num())
// 	{
// 		if(LowerCorners.Num() >= 2)
// 		{
// 			for(int j = 0; j < LowerCorners.Num() - 1; j++)
// 			{
// 				if(LowerCorners[j] != LowerCorners[j + 1] - 1)
// 				{
// 					for(int p = 0; p < LowerCorners.Num() - j - 1; p++)
// 					{
// 						const int LastElement = LowerCorners.Last();
// 						for(int k = LowerCorners.Num() - 1; k >= 1; k--)
// 						{
// 							LowerCorners[k] = LowerCorners[k - 1];
// 						}
// 						LowerCorners[0] = LastElement;
// 					}
// 					break;
// 				}
// 			}
// 		}
// 		if(LowerCorners[0] != 0)
// 		{
// 			const int RotationAmount = 4 - LowerCorners[0];
// 			Rotation = static_cast<float>(RotationAmount) * 90.f;
// 			for(int j = 0; j < LowerCorners.Num(); j++)
// 			{
// 				LowerCorners[j] = (LowerCorners[j] + RotationAmount) % 4;
// 			}
// 		}
// 	}
//
// 	if(!LowerCorners.Num() && !UpperCorners.Num())
// 	{
// 		Grid->BuildingPieces.Remove(TPair<int, int>(0, QuadIndex));
// 		World->DestroyActor(BuildingPiece);
// 		return;
// 	}
// 	
// 	FString RowNameString;
// 	BuildingPiece->Corners = LowerCorners;
// 	BuildingPiece->Corners.Append(UpperCorners);
// 	for(int j = 0; j < LowerCorners.Num(); j++)
// 		RowNameString.Append(FString::FromInt(LowerCorners[j]));
// 	for(int j = 0; j < UpperCorners.Num(); j++)
// 		RowNameString.Append(FString::FromInt(UpperCorners[j]));
// 	
// 	const FName RowName(RowNameString);
// 	auto Row = BuildingPiece->DataTable->FindRow<FMeshCornersData>(RowName, "MeshCornersRow");
// 	BuildingPiece->SetStaticMesh(Row->Mesh);
// 	BuildingPiece->DeformMesh(CageBase, 200.f, Rotation);
// }
