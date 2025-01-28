#include "GridGeneratorVis.h"

#include "GridDebugSceneProxy.h"
#include "GridGenerator.h"

namespace GridVis
{
	namespace Editor
	{
		TCustomShowFlag<> ShowGridVis(TEXT("GridGen"), true, SFG_Developer, FText::FromString("GridGen"));
	}
}

UGridGeneratorVis::UGridGeneratorVis()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetCastShadow(false);
	SetHiddenInGame(true);
	bVisibleInReflectionCaptures = false;
	bVisibleInRayTracing = false;
	bVisibleInRealTimeSkyCaptures = false;

	bIsEditorOnly = true;

#if WITH_EDITORONLY_DATA
	SetIsVisualizationComponent(true);
#endif
}

FDebugRenderSceneProxy* UGridGeneratorVis::CreateDebugSceneProxy()
{
	FGridDebugSceneProxy* DSceneProxy = new FGridDebugSceneProxy(this);
	const AGridGenerator* Owner = Cast<AGridGenerator>(GetOwner());
	if(!Owner)
		return DSceneProxy;

	const UWorld* World = GEditor->GetEditorWorldContext().World();
	if(!World)
		World = GEditor->PlayWorld;
	if(World)
	{
		TArray<FLinearColor> Colors = {FColor::Red, FColor::Green, FColor::Blue, FColor::Yellow, FColor::Purple, FColor::Emerald, FColor::Magenta};
		if(Owner->ShowBaseGrid)
		{
			const TArray<FGridQuad>& FinalQuads = Owner->GetBaseGridQuads();
			for(int i = 0; i < FinalQuads.Num(); i++)
			{
				if(FinalQuads[i].Index == -1)
					continue;

				float LineThickness = 2.f;
				//if(i != 24)
				//	continue;
				for(int j = 0; j < 4; j++)
				{
					DSceneProxy->Lines.Emplace(Owner->GetBasePointCoordinates(FinalQuads[i].Points[j]),
						Owner->GetBasePointCoordinates(FinalQuads[i].Points[(j + 1) % FinalQuads[i].Points.Num()]),
						Colors[1].ToFColor(true),
						1.5f);
				}
			}
		}
		if(Owner->ShowBuildingGrid)
		{
			const TArray<FGridShape>& SecondGrid = Owner->GetBuildingGridShapes();
			for(int i = 0; i < SecondGrid.Num(); i++)
			{
				if(SecondGrid[i].Index == -1)
					continue;
				for(int j = 0; j < SecondGrid[i].Points.Num(); j++)
				{
					DSceneProxy->Lines.Emplace( 
					Owner->GetBuildingPointCoordinates(SecondGrid[i].Points[j]),
						Owner->GetBuildingPointCoordinates(SecondGrid[i].Points[(j + 1) % SecondGrid[i].Points.Num()]),
						Colors[0].ToFColor(true),
						1.5f);
				}
			}
		}

		if(Owner->ShowBaseGridPoints)
		{
			const TArray<FGridPoint>& Grid1Points = Owner->GetBaseGridPoints();
			for(int i = 0; i < Grid1Points.Num(); i++)
			{
				if(Grid1Points[i].Index == -1)
					continue;
				int ColorIndex = 1;
				if(Grid1Points[i].IsEdge)
					ColorIndex = 2;
				DSceneProxy->Texts.Emplace(FString::FromInt(Grid1Points[i].Index), Grid1Points[i].Location, Colors[ColorIndex].ToFColor(true));
			}
		}

		if(Owner->ShowBuildingGridPoints)
		{
			const TArray<FGridPoint>& Grid2Points = Owner->GetBuildingGridPoints();
			for(int i = 0; i < Grid2Points.Num(); i++)
			{
				if(Grid2Points[i].Index == -1)
					continue;
				DSceneProxy->Texts.Emplace(FString::FromInt(Grid2Points[i].Index), Grid2Points[i].Location, Colors[3].ToFColor(true));
			}
		}

		if(Owner->ShowBaseGridQuadIndices)
		{
			const TArray<FGridQuad>& FinalQuads = Owner->GetBaseGridQuads();
			for(int i = 0; i < FinalQuads.Num(); i++)
			{
				if(FinalQuads[i].Index == -1)
					continue;
	
				DSceneProxy->Texts.Emplace(FString::FromInt(FinalQuads[i].Index), FinalQuads[i].Center, Colors[2].ToFColor(true));
			}
		}

		if(Owner->ShowBuildingGridQuadIndices)
		{
			const TArray<FGridShape>& SecondGrid = Owner->GetBuildingGridShapes();
			for(int i = 0; i < SecondGrid.Num(); i++)
			{
				if(SecondGrid[i].Index == -1)
					continue;
	
				DSceneProxy->Texts.Emplace(FString::FromInt(SecondGrid[i].Index), SecondGrid[i].Center, Colors[2].ToFColor(true));
			}
		}

		if(Owner->ShowMarchingBits)
		{
			const TArray<FElevationData>& Elevations = Owner->GetElevations();
			for(int i = 0; i < Elevations.Num(); i++)
			{
				for(int j = 0; j < Elevations[i].MarchingBits.Num(); j++)
				{
					if(Elevations[i].MarchingBits[j])
					{
						DSceneProxy->Spheres.Emplace(5.f, Owner->GetBaseGridPoints()[j].Location + FVector(0.f, 0.f, 200.f * static_cast<float>(i)), FLinearColor::Green);
					}
				}
			}
		}

		if(Owner->ShowQuadNeighbours)
		{
			const TArray<FGridQuad>& Quads = Owner->GetBaseGridQuads();
			for(int i = 0; i < Quads.Num(); i++)
			{
				for(int j = 0; j < Quads[i].OffsetNeighbours.Num(); j++)
				{
					bool ColorCheck = false;
					if(j >= 2)
						ColorCheck = true;
					if(Quads[i].OffsetNeighbours[j] == -1)
						continue;
					FVector ToNeighbour = Quads[Quads[i].OffsetNeighbours[j]].Center - Quads[i].Center;
					ToNeighbour.Normalize();
					DSceneProxy->Texts.Emplace(FString::FromInt(Quads[i].OffsetNeighbours[j]), Quads[i].Center + ToNeighbour * 50.f, Colors[j + static_cast<int>(ColorCheck)].ToFColor(true));

					FVector ToCenter = Quads[i].Center - Owner->GetBasePointCoordinates(Quads[i].Points[(j + 2) % Quads[i].Points.Num()]);
					ToCenter.Normalize();
					DSceneProxy->Spheres.Emplace(5.f, Owner->GetBasePointCoordinates(Quads[i].Points[(j + 2) % Quads[i].Points.Num()]) - ToCenter * 15.f, Colors[j + static_cast<int>(ColorCheck)].ToFColor(true));
				}
			}
		}
	}
	return DSceneProxy;
}

FBoxSphereBounds UGridGeneratorVis::CalcBounds(const FTransform& InTransform) const
{
	FBoxSphereBounds::Builder BoundsBuilder;
	BoundsBuilder += Super::CalcBounds(InTransform);
	// Add initial sphere bounds so if we have no TestChildren our bounds will still be non-zero
	BoundsBuilder += FSphere(GetComponentLocation(), 50.f);
	return BoundsBuilder;
}
