#include "UtilityLibrary.h"

#include "GridGenerator.h"

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
	//DrawDebugLine(World, WorldLocation, WorldLocation + WorldDirection * 10000.f, FColor::Red, true, 5.f, 0, 2.f);
	if(bHit)
	{
		Grid = Cast<AGridGenerator>(HitResult.GetActor());
		if(Grid)
		{
			ShapeIndex = Grid->DetermineWhichGridShapeAPointIsIn(HitResult.Location);
		}
	}
}
