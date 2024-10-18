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
	
	//TArray<FGridTriangle> Triangles = GridGen->GetTriangles();
	//for(int i = 0; i < Triangles.Num(); i++)
	//{
	//	if(Triangles[i].Index == -1)
	//		continue;
	//	FVector Sum = FVector::Zero();
	//	for(int k = 0; k < 3; k++)
	//		Sum += GridGen->GetPointCoordinates(Triangles[i].Points[k]);
	//	FVector TriangleCenter = Sum / 3.f;
	//	ProxyData.DebugLabels.Add({TriangleCenter, FString::FromInt(Triangles[i].Index)});
	//	
	//	//DrawDebugString(GetWorld(), TriangleCenter, *FString::Printf(TEXT("T")), nullptr, FColor::Red, 100.f, true, 5.f);
	//}

	TArray<FGridQuad> Quads = GridGen->GetQuads();
	for(int i = 0; i < Quads.Num(); i++)
	{

		if(Quads[i].Index == -1)
			continue;
		//FVector Sum = FVector::Zero();
		//for(int k = 0; k < 4; k++)
		//	Sum += GridGen->GetPointCoordinates(Quads[i].Points[k]);
		//FVector QuadCenter = Sum / 4.f;
		//ProxyData.DebugLabels.Add({Quads[i].Center, FString::FromInt(Quads[i].Index)});
		
		//DrawDebugString(GetWorld(), TriangleCenter, *FString::Printf(TEXT("T")), nullptr, FColor::Red, 100.f, true, 5.f);
	}

	TArray<FGridQuad> FinalQuads = GridGen->GetFinalQuads();
	for(int i = 0; i < FinalQuads.Num(); i++)
	{

		if(FinalQuads[i].Index == -1)
			continue;

		for(int k = 0; k < 4; k++)
		{
			//FVector TowardCenter = FinalQuads[i].Center - GridGen->GetPointCoordinates(FinalQuads[i].Points[k]);
			//TowardCenter.Normalize();
			//ProxyData.DebugLabels.Emplace(GridGen->GetPointCoordinates(FinalQuads[i].Points[k]) + TowardCenter * 20.f, FString::FromInt(k));
		}
		
		//FVector Sum = FVector::Zero();
		//for(int k = 0; k < 4; k++)
		//	Sum += GridGen->GetPointCoordinates(Quads[i].Points[k]);
		//FVector QuadCenter = Sum / 4.f;
		//ProxyData.DebugLabels.Add({FinalQuads[i].Center, FString::FromInt(FinalQuads[i].Index)});
		
		//DrawDebugString(GetWorld(), TriangleCenter, *FString::Printf(TEXT("T")), nullptr, FColor::Red, 100.f, true, 5.f);
	}
	
	const TArray<FGridPoint> Points = GridGen->GetGridPoints();
	for(int i = 0; i < Points.Num(); i++)
	{
		//ProxyData.DebugLabels.Add({Points[i], FString::FromInt(i)});
		
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
