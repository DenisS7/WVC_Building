// Fill out your copyright notice in the Description page of Project Settings.


#include "DebugStrings.h"

#include "GridGenerator.h"
#include "Engine/Canvas.h"

FDebugSceneProxy::FDebugSceneProxy(const UPrimitiveComponent* InComponent, FDebugSceneProxyData* ProxyData)
	: FDebugRenderSceneProxy(InComponent)
{
	DrawType = EDrawType::SolidAndWireMeshes;
	ViewFlagName = TEXT("DebugText");

	this->ProxyData = *ProxyData;

	for(const auto& Text : ProxyData->DebugLabels)
	{
		this->Texts.Add({Text.Text, Text.Location, FColor::White});
	}
	
}

void FDebugTextDelegateHelper::DrawDebugLabels(UCanvas* Canvas, APlayerController* PlayerController)
{
	if(!Canvas) return;

	const FColor OldDrawColor = Canvas->DrawColor;
	Canvas->SetDrawColor(FColor::White);
	const FSceneView* View = Canvas->SceneView;
	const UFont* Font = GEngine->GetSmallFont();
	const FDebugSceneProxyData::FDebugText* DebugText = DebugLabels.GetData();

	for(int32 i = 0; i < DebugLabels.Num(); ++i, ++DebugText)
	{
		if(View->ViewFrustum.IntersectSphere(DebugText->Location, 1.f))
		{
			const FVector ScreenLoc = Canvas->Project(DebugText->Location);
			Canvas->DrawText(Font, DebugText->Text, ScreenLoc.X, ScreenLoc.Y);
		}
	}

	Canvas->SetDrawColor(OldDrawColor);
}

void FDebugTextDelegateHelper::SetupFromProxy(const FDebugSceneProxy* InSceneProxy)
{
	DebugLabels.Reset();
	DebugLabels.Append(InSceneProxy->ProxyData.DebugLabels);
}

UDebugStrings::UDebugStrings(const FObjectInitializer& ObjectInitializer)
{
	FEngineShowFlags::RegisterCustomShowFlag(TEXT("DebugText"), false, SFG_Normal, FText::FromString("Debug Text"));
}

FDebugRenderSceneProxy* UDebugStrings::CreateDebugSceneProxy()
{
	FDebugSceneProxyData ProxyData;
	AGridGenerator* GridGen = Cast<AGridGenerator>(GetOwner());

	if(!GridGen)
	{
		FDebugSceneProxy* DebugSceneProxy = new FDebugSceneProxy(this, &ProxyData);
		DebugDrawDelegateManager.SetupFromProxy(DebugSceneProxy);
		return DebugSceneProxy;
	}
	
	TArray<TArray<FGridTriangle>> Triangles = GridGen->GetTriangles();
	for(int i = 0; i < Triangles.Num(); i++)
	{
		for(int j = 0; j < Triangles[i].Num(); j++)
		{
			if(Triangles[i][j].Index == -1)
				continue;
			FVector Sum = FVector::Zero();
			for(int k = 0; k < 3; k++)
				Sum += GridGen->GetGridCoordinate(Triangles[i][j].Points[k].X, Triangles[i][j].Points[k].Y);
			FVector TriangleCenter = Sum / 3.f;
			ProxyData.DebugLabels.Add({TriangleCenter, FString::FromInt(Triangles[i][j].Index)});
		}
		
		//DrawDebugString(GetWorld(), TriangleCenter, *FString::Printf(TEXT("T")), nullptr, FColor::Red, 100.f, true, 5.f);
	}
	
	FDebugSceneProxy* DebugSceneProxy = new FDebugSceneProxy(this, &ProxyData);

	
	
	DebugDrawDelegateManager.SetupFromProxy(DebugSceneProxy);
	return DebugSceneProxy;
}

FBoxSphereBounds UDebugStrings::CalcBounds(const FTransform& LocalToWorld) const
{
	return FBoxSphereBounds(FBox(FVector(-1000.f, -1000.f, -1000.f), FVector(1000.f, 1000.f, 1000.f)));
}
