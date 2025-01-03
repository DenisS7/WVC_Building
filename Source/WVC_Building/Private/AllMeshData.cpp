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

void UAllMeshData::ProcessMeshData(UWorld* World, const FVector& Center, const float Rotation, const UStaticMesh* StaticMesh)
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
	//if(MeshEdges.Contains(StaticMesh))
	//	return;
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
	TMap<int, int> CorrectVertex;
	TArray<int> All;

	
	TArray<int> RemoveIndices;
	
	for(int i = 0; i < IndexBuffer.GetNumIndices(); i++)
	{
		const int Index = IndexBuffer.GetIndex(i);
		bool Found = false;
		All.AddUnique(Index);
		if(CorrectVertex.Contains(Index))
			continue;
		for(int j = i + 1; j < IndexBuffer.GetNumIndices(); j++)
		{
			const int Index2 = IndexBuffer.GetIndex(j);
			if(CorrectVertex.Contains(Index2) || Index == Index2)
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
		//DrawDebugSphere(World, Center + static_cast<FVector>(VertexBuffer.VertexPosition(EdgePoints[i])), 2.f, 3, FColor::Yellow, true, -1, 0, 2.f);
	}

	MeshEdges.Add(StaticMesh);
	MeshEdges[StaticMesh].SetNum(6);

	const float PieceExtent = 75.f - 1.f;
	
	const FVector Up = FVector(0.f, 0.f, 1.f);
	const FVector Right = FVector(0.f, 1.f, 0.f);
	const FVector Forward = FVector(1.f, 0.f, 0.f);

	const TArray<FVector> Direction = {-Up, -Right,	-Forward, Right, Forward, Up};

	const TArray<float> PieceExtents = {-PieceExtent, -PieceExtent, -PieceExtent, PieceExtent, PieceExtent, PieceExtent};

	TArray<TArray<int>> MeshBorders;
	MeshBorders.SetNum(6);
	
	for(int i = 0; i < EdgePoints.Num(); i++)
	{
		FVector PointCoord = static_cast<FVector>(VertexBuffer.VertexPosition(EdgePoints[i]));
		//for(int j = 0; j < Direction.Num(); j++)
		{
			if(PointCoord.X > PieceExtent)
			{
				MeshBorders[static_cast<int>(EEdgeSide::Front)].Add(EdgePoints[i]);
			}
			else if(PointCoord.X < -PieceExtent)
			{
				MeshBorders[static_cast<int>(EEdgeSide::Back)].Add(EdgePoints[i]);
			}

			if(PointCoord.Y > PieceExtent)
			{
				MeshBorders[static_cast<int>(EEdgeSide::Right)].Add(EdgePoints[i]);
			}
			else if(PointCoord.Y < -PieceExtent)
			{
				MeshBorders[static_cast<int>(EEdgeSide::Left)].Add(EdgePoints[i]);
			}
			
			if(PointCoord.Z > 2.f * PieceExtent - 1.f)
			{
				MeshBorders[static_cast<int>(EEdgeSide::Top)].Add(EdgePoints[i]);
			}
			else if(PointCoord.Z < 1.f)
			{
				MeshBorders[static_cast<int>(EEdgeSide::Bottom)].Add(EdgePoints[i]);
			}
		}
	}

	int Cycle = Rotation / 90.f;
	TArray<TArray<int>> CopyMeshBorders = MeshBorders;
	for(int i = 1; i < 5; i++)
	{
		int NewIndex = i - Cycle;
		if(NewIndex <= 0)
			NewIndex += 4;
		if(NewIndex >= 5)
			NewIndex -= 4;
		MeshBorders[i] = CopyMeshBorders[NewIndex];
		for(int j = 0; j < MeshBorders[i].Num(); j++)
		{
			//MeshBorders[i][j] =MeshBorders[i][j]
		}
	}

	TArray<FColor> PieceColors = {FColor::Black, FColor::Red, FColor::Blue, FColor::Green, FColor::Yellow, FColor::Purple};
	
	for(int i = 0; i < MeshBorders.Num(); i++)
	{
		for(int j = 0; j < MeshBorders[i].Num(); j++)
		{
			FVector Point = static_cast<FVector>(VertexBuffer.VertexPosition(MeshBorders[i][j]));
			//if(i >= 1 && i <= 4)
				//Point = Point.RotateAngleAxis(Rotation, FVector(0.f, 0.f, 1.f));
			DrawDebugSphere(World, Center + Point, 1.5f + static_cast<float>(i) * 0.5f, 3, PieceColors[i], true, -1, 0, 2.f);
		}}
}

//
// const TArray<int>& UAllMeshData::GetMeshData(const UStaticMesh* StaticMesh) const
// {
// 	return TArray<int>();
// }

