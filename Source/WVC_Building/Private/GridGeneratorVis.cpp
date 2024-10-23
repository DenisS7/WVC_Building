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
		if(Owner->ShowGrid)
		{
			const TArray<FGridQuad>& FinalQuads = Owner->GetFinalQuads();
			for(int i = 0; i < FinalQuads.Num(); i++)
			{
				if(FinalQuads[i].Index == -1)
					continue;

				float LineThickness = 2.f;
				//if(i != 24)
				//	continue;
				for(int j = 0; j < 4; j++)
				{
					DSceneProxy->Lines.Emplace(Owner->GetPointCoordinates(FinalQuads[i].Points[j]),
						Owner->GetPointCoordinates(FinalQuads[i].Points[(j + 1) % FinalQuads[i].Points.Num()]),
						Colors[1].ToFColor(true),
						1.5f);
				}
			}
		}
		if(Owner->ShowSecondGrid)
		{
			const TArray<FGridShape>& SecondGrid = Owner->GetSecondGrid();
			for(int i = 0; i < SecondGrid.Num(); i++)
			{
				if(SecondGrid[i].Index == -1)
					continue;
				for(int j = 0; j < SecondGrid[i].Points.Num(); j++)
				{
					DSceneProxy->Lines.Emplace( 
					Owner->GetSecondPointCoordinates(SecondGrid[i].Points[j]),
						Owner->GetSecondPointCoordinates(SecondGrid[i].Points[(j + 1) % SecondGrid[i].Points.Num()]),
						Colors[0].ToFColor(true),
						1.5f);
				}
			}
		}

		if(Owner->ShowGrid1Points)
		{
			const TArray<FGridPoint>& Grid1Points = Owner->GetGridPoints();
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

		if(Owner->ShowGrid2Points)
		{
			const TArray<FGridPoint>& Grid2Points = Owner->GetSecondGridPoints();
			for(int i = 0; i < Grid2Points.Num(); i++)
			{
				if(Grid2Points[i].Index == -1)
					continue;
				DSceneProxy->Texts.Emplace(FString::FromInt(Grid2Points[i].Index), Grid2Points[i].Location, Colors[3].ToFColor(true));
			}
		}

		if(Owner->ShowGrid1QuadIndices)
		{
			const TArray<FGridQuad>& FinalQuads = Owner->GetFinalQuads();
			for(int i = 0; i < FinalQuads.Num(); i++)
			{
				if(FinalQuads[i].Index == -1)
					continue;
	
				DSceneProxy->Texts.Emplace(FString::FromInt(FinalQuads[i].Index), FinalQuads[i].Center, Colors[2].ToFColor(true));
			}
		}

		if(Owner->ShowGrid2QuadIndices)
		{
			const TArray<FGridShape>& SecondGrid = Owner->GetSecondGrid();
			for(int i = 0; i < SecondGrid.Num(); i++)
			{
				if(SecondGrid[i].Index == -1)
					continue;
	
				DSceneProxy->Texts.Emplace(FString::FromInt(SecondGrid[i].Index), SecondGrid[i].Center, Colors[2].ToFColor(true));
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
