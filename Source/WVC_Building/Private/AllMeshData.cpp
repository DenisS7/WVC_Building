// Fill out your copyright notice in the Description page of Project Settings.


#include "AllMeshData.h"

UAllMeshData* UAllMeshData::Instance = nullptr;

UAllMeshData::UAllMeshData()
{
}

UAllMeshData* UAllMeshData::GetInstance()
{
	if(!Instance)
	{
		Instance = NewObject<UAllMeshData>();
		Instance->AddToRoot();
	}
	return Instance;
}

void UAllMeshData::ProcessMeshData(UWorld* World, const FVector& Center, const UStaticMesh* StaticMesh)
{
	if(!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid World!"));
		return;
	}
	if (!StaticMesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid StaticMesh"));
		return;
	}
	const FStaticMeshRenderData* RenderData = StaticMesh->GetRenderData();
	if(!RenderData || !RenderData->LODResources.Num())
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid RenderData"));
		return;
	}
	const FStaticMeshLODResources& LODResource = RenderData->LODResources[0];
	const FPositionVertexBuffer& VertexBuffer = LODResource.VertexBuffers.PositionVertexBuffer;

	const FRawStaticIndexBuffer& IndexBuffer = LODResource.IndexBuffer;
	const FRawStaticIndexBuffer& Index2Buffer = LODResource.AdditionalIndexBuffers->WireframeIndexBuffer;
	TMap<FIntPoint, int> Edges;
	TArray<int> All;

	
	TArray<int> RemoveIndices;
	TMap<int, int> CorrectVertex;
	
	for(int i = 0; i < IndexBuffer.GetNumIndices(); i++)
	{
		const int Index = IndexBuffer.GetIndex(i);
		bool Found = false;
		All.AddUnique(Index);
		if(RemoveIndices.Contains(Index))
			continue;
		for(int j = i + 1; j < IndexBuffer.GetNumIndices(); j++)
		{
			const int Index2 = IndexBuffer.GetIndex(j);
			if(RemoveIndices.Contains(Index2) || Index == Index2)
				continue;
			if(FVector3f::DistSquared(VertexBuffer.VertexPosition(Index), VertexBuffer.VertexPosition(Index2)) < 0.01f)
			{
				RemoveIndices.AddUnique(Index2);
				CorrectVertex.FindOrAdd(Index2, Index);
				CorrectVertex.FindOrAdd(Index, Index);
				Found = true;
			}
		}
		if(!Found)
			CorrectVertex.FindOrAdd(Index, Index);
	}

	for(int i = 0; i < RemoveIndices.Num(); i++)
		All.Remove(RemoveIndices[i]);

	TArray<FVector> VertexPositions;

	for(int i = 0; i < All.Num(); i++)
		VertexPositions.Add(static_cast<FVector>(VertexBuffer.VertexPosition(All[i])));
	
	for (int i = 0; i < IndexBuffer.GetNumIndices(); i += 3)
	{
		const int OGIndex0 = IndexBuffer.GetIndex(i);
		const int OGIndex1 = IndexBuffer.GetIndex(i + 1);
		const int OGIndex2 = IndexBuffer.GetIndex(i + 2);
		
		const int Index0 = CorrectVertex[OGIndex0];
		const int Index1 = CorrectVertex[OGIndex1];
		const int Index2 = CorrectVertex[OGIndex2];
		
		FIntPoint Edge1 = FIntPoint(FMath::Min(Index0, Index1), FMath::Max(Index0, Index1));
		FIntPoint Edge2 = FIntPoint(FMath::Min(Index1, Index2), FMath::Max(Index1, Index2));
		FIntPoint Edge3 = FIntPoint(FMath::Min(Index2, Index0), FMath::Max(Index2, Index0));

		Edges.FindOrAdd(Edge1)++;
		Edges.FindOrAdd(Edge2)++;
		Edges.FindOrAdd(Edge3)++;
	}

	TArray<int> EdgePoints;
	for(const auto& Edge : Edges)
	{
		if(Edge.Value == 1)
		{
			EdgePoints.AddUnique(Edge.Key.X);
			EdgePoints.AddUnique(Edge.Key.Y);
		}
	}

	for(int i = 0; i < EdgePoints.Num(); i++)
	{
		DrawDebugSphere(World, Center + static_cast<FVector>(VertexBuffer.VertexPosition(EdgePoints[i])), 2.f, 3, FColor::Yellow, true, -1, 0, 2.f);
	}

	//for(int i = 0; i < VertexPositions.Num(); i++)
	//{
	//	DrawDebugSphere(World, Center + VertexPositions[i], 2.f, 3, FColor::Yellow, true, -1, 0, 2.f);
	//}
}
//
// const TArray<int>& UAllMeshData::GetMeshData(const UStaticMesh* StaticMesh) const
// {
// 	return TArray<int>();
// }

