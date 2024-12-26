#include "UtilityLibrary.h"

#include "BuildingPiece.h"
#include "GridGenerator.h"
#include "MeshCornersData.h"

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
