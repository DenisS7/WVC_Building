// Fill out your copyright notice in the Description page of Project Settings.


#include "MeshProcessingLibrary.h"
// Fill out your copyright notice in the Description page of Project Settings.

#include "WVC_Building/Public/BuildingMeshData.h"
#include "AssetRegistry/AssetRegistryModule.h"

void UMeshProcessingLibrary::ProcessAllMeshes(UDataTable* DataTable, const FString& FolderPath)
{
	TMap<TArray<FIntVector>, TArray<FIntVector>> EdgeVariations;
	TMap<TArray<FIntVector>, int> EdgeCodes;
	
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	
	FARFilter Filter;
	Filter.PackagePaths.Add(FName("/Game/" + FolderPath));
	Filter.ClassPaths.Add(UStaticMesh::StaticClass()->GetClassPathName());
	Filter.bRecursivePaths = true;
	
	TArray<FAssetData> OutAssetData;
	TArray<FName> MeshNames;
	AssetRegistryModule.Get().GetAssets(Filter, OutAssetData);
	
	for (const FAssetData& AssetData : OutAssetData)
	{
		UStaticMesh* StaticMesh = Cast<UStaticMesh>(AssetData.GetAsset());
		TArray<int> MeshEdgeCodes;
		if(ProcessMeshData(StaticMesh, MeshEdgeCodes, EdgeVariations, EdgeCodes))
		{
			const FName MeshName = *StaticMesh->GetName();
			MeshNames.Add(MeshName);
			FBuildingMeshData* MeshRow = DataTable->FindRow<FBuildingMeshData>(MeshName, TEXT("ProcessAllMeshes"));
			if(MeshRow)
			{
				MeshRow->EdgeCodes = MeshEdgeCodes;
				MeshRow->StaticMesh = StaticMesh;
			}
			else
			{
				FBuildingMeshData MeshData;
				MeshData.Name = MeshName;
				MeshData.EdgeCodes = MeshEdgeCodes;
				MeshData.StaticMesh = StaticMesh;
				DataTable->AddRow(MeshName, MeshData);
			}
		}
	}

	TArray<FBuildingMeshData*> TableRows;
	DataTable->GetAllRows<FBuildingMeshData>(TEXT("ProcessAllMeshes"), TableRows);
	
	for(const FBuildingMeshData* Row : TableRows)
	{
		if(!MeshNames.Contains(Row->Name))
			DataTable->RemoveRow(Row->Name);		
	}
}

void UMeshProcessingLibrary::GetMeshData(const FName& MeshName)
{
}

bool UMeshProcessingLibrary::ProcessMeshData(const UStaticMesh* StaticMesh, TArray<int>& EdgeCodesOut, TMap<TArray<FIntVector>, TArray<FIntVector>>& EdgeVariations, TMap<TArray<FIntVector>, int>& EdgeCodes, UWorld* World, const FVector& Center)
{
	if (!StaticMesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid StaticMesh"));
		return false;
	}

	const FStaticMeshRenderData* RenderData = StaticMesh->GetRenderData();
	if(!RenderData || !RenderData->LODResources.Num())
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid RenderData"));
		return false;
	}
	const FStaticMeshLODResources& LODResource = RenderData->LODResources[0];
	const FPositionVertexBuffer& VertexBuffer = LODResource.VertexBuffers.PositionVertexBuffer;

	const FRawStaticIndexBuffer& IndexBuffer = LODResource.IndexBuffer;
	const FRawStaticIndexBuffer& Index2Buffer = LODResource.AdditionalIndexBuffers->WireframeIndexBuffer;
	TMap<FIntPoint, int> Edges;
	TMap<int, int> CorrectVertex;

	//Duplicate vertex
	for(int i = 0; i < IndexBuffer.GetNumIndices(); i++)
	{
		const int Index = IndexBuffer.GetIndex(i);
		bool Found = false;
		if(CorrectVertex.Contains(Index))
			continue;
		for(int j = i + 1; j < IndexBuffer.GetNumIndices(); j++)
		{
			const int Index2 = IndexBuffer.GetIndex(j);
			if(CorrectVertex.Contains(Index2) || Index == Index2)
				continue;
			if(FVector3f::DistSquared(VertexBuffer.VertexPosition(Index), VertexBuffer.VertexPosition(Index2)) < 0.01f)
			{
				CorrectVertex.FindOrAdd(Index2, Index);
				CorrectVertex.FindOrAdd(Index, Index);
				Found = true;
			}
		}
		if(!Found)
			CorrectVertex.FindOrAdd(Index, Index);
	}

	//Edge face count
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

	//Border points
	TArray<int> EdgePoints;
	for(const auto& Edge : Edges)
	{
		if(Edge.Value == 1)
		{
			EdgePoints.AddUnique(Edge.Key.X);
			EdgePoints.AddUnique(Edge.Key.Y);
		}
	}
	
	const float PieceExtent = 100.f - 1.f;

	TArray<TArray<int>> MeshBorders;
	TArray<TArray<FIntVector>> MeshBordersVector;
	MeshBorders.SetNum(6);
	MeshBordersVector.SetNum(6);
	
	//Face borders
	for(int i = 0; i < EdgePoints.Num(); i++)
	{
		const FVector PointCoord = static_cast<FVector>(VertexBuffer.VertexPosition(EdgePoints[i]));

		TArray<EEdgeSide> Sides;
		//Forward
		if(PointCoord.Y > PieceExtent)
		{
			Sides.Add(EEdgeSide::Front);
		}
		else if(PointCoord.Y < -PieceExtent)
		{
			Sides.Add(EEdgeSide::Back);
		}

		//Sides
		if(PointCoord.X < -PieceExtent)
		{
			Sides.Add(EEdgeSide::Right);
		}
		else if(PointCoord.X > PieceExtent)
		{
			Sides.Add(EEdgeSide::Left);
		}

		//Vertical
		if(PointCoord.Z > 2.f * PieceExtent + 1.f)
		{
			Sides.Add(EEdgeSide::Top);
		}
		else if(PointCoord.Z < 1.f)
		{
			Sides.Add(EEdgeSide::Bottom);
		}

		for(const EEdgeSide& Side : Sides)
		{
			int Index = static_cast<int>(Side);
			FVector SidePoint = PointCoord;
			const float X = FMath::RoundHalfFromZero(SidePoint.X * 100.f);
			const float Y = FMath::RoundHalfFromZero(SidePoint.Y * 100.f);
			const float Z = FMath::RoundHalfFromZero(SidePoint.Z * 100.f);
			const FIntVector IntPoint = FIntVector(static_cast<int>(X / 10.f), static_cast<int>(Y / 10.f), static_cast<int>(Z / 10.f));
			MeshBorders[Index].Add(EdgePoints[i]);
			MeshBordersVector[Index].Add(IntPoint);
		}
	}

	TArray<TArray<int>> ExtraPoints;

	//Detect extra points
	for(int i = 0; i < MeshBorders.Num(); i++)
	{
		const EEdgeSide Side = static_cast<EEdgeSide>(i);
		ExtraPoints.Add(TArray<int>());
		for(int j = 0; j < MeshBorders[i].Num(); j++)
		{
			FVector CurrentPoint = static_cast<FVector>(VertexBuffer.VertexPosition(MeshBorders[i][j]));
			bool Found = false;
			for(int k = 0; k < MeshBorders[i].Num(); k++)
			{
				if(k == j)
					continue;
				FVector PrevPoint = static_cast<FVector>(VertexBuffer.VertexPosition(MeshBorders[i][k]));
				const float Dist1 = FVector::Distance(CurrentPoint, PrevPoint);
				for(int p = k + 1; p < MeshBorders[i].Num(); p++)
				{
					if(p == j)
						continue;
					FVector NextPoint = static_cast<FVector>(VertexBuffer.VertexPosition(MeshBorders[i][p]));
					const float Dist2 = FVector::Distance(CurrentPoint, NextPoint);
					const float Dist3 = FVector::Distance(PrevPoint, NextPoint);
					
					if(FMath::Abs(Dist1 + Dist2 - Dist3) < 0.0001f)
					{
						ExtraPoints[i].Add(j);
						Found = true;
						break;
					}
				}
				if(Found == true)
				{
					break;
				}
			}
		}
	}

	//Remove extra points
	for(int i = 0; i < ExtraPoints.Num(); i++)
	{
		for(int j = 0; j < ExtraPoints[i].Num(); j++)
		{
			MeshBorders[i].RemoveAt(ExtraPoints[i][j]);
			MeshBordersVector[i].RemoveAt(ExtraPoints[i][j]);
		}
	}

	auto Comparator = [](const FIntVector& A, const FIntVector& B)
	{
		if (A.X > B.X)
			return true;
		if (A.X < B.X)
			return false;

		if (A.Y > B.Y)
			return true;
		if (A.Y < B.Y)
			return false;

		if (A.Z > B.Z)
			return true;
		if (A.Z < B.Z)
			return false;

		return true;
	};

	//Get or add border code
	for(int i = 0; i < MeshBordersVector.Num(); i++)
	{
		const EEdgeSide Side = static_cast<EEdgeSide>(i);
		MeshBordersVector[i].Sort(Comparator);
		TArray<FIntVector>* FoundVector = EdgeVariations.Find(MeshBordersVector[i]);
		if(FoundVector)
		{
			UE_LOG(LogTemp, Display, TEXT("Mesh: %s - Edge code found: %d"), *StaticMesh->GetPathName(), EdgeCodes[*FoundVector]);
		}
		else
		{
			EdgeCodes.Add(MeshBordersVector[i], EdgeCodes.Num() + 1);
			EdgeVariations.Add(MeshBordersVector[i], MeshBordersVector[i]);
			TArray<FIntVector> RotatedVector = MeshBordersVector[i];
			for(int j = 0; j < 3; j++)
			{
				for(int k = 0; k < RotatedVector.Num(); k++)
				{
					Swap(RotatedVector[k].X, RotatedVector[k].Y);
					RotatedVector[k].X *= -1;
				}
				RotatedVector.Sort(Comparator);
				EdgeVariations.Add(RotatedVector, MeshBordersVector[i]);
			}
			TArray<FIntVector> FlippedVector = MeshBordersVector[i];
			for(int j = 0; j < FlippedVector.Num(); j++)
				Swap(FlippedVector[j].X, FlippedVector[j].Y);
			FlippedVector.Sort(Comparator);
			TArray<FIntVector>* FoundFlippedVector = EdgeVariations.Find(FlippedVector);
			if(FoundFlippedVector)
			{
				UE_LOG(LogTemp, Display, TEXT("Mesh: %s - Flipped Edge code found: %d"), *StaticMesh->GetPathName(), EdgeCodes[*FoundFlippedVector]);
			}
			else
			{
				EdgeCodes.Add(FlippedVector, EdgeCodes.Num() + 1);
				EdgeVariations.Add(FlippedVector, FlippedVector);
				UE_LOG(LogTemp, Warning, TEXT("Mesh: %s - Connection: %d <-> %d"), *StaticMesh->GetPathName(), EdgeCodes[MeshBordersVector[i]], EdgeCodes[FlippedVector]);
				RotatedVector = FlippedVector;
				for(int j = 0; j < 3; j++)
				{
					for(int k = 0; k < RotatedVector.Num(); k++)
					{
						Swap(RotatedVector[k].X, RotatedVector[k].Y);
						RotatedVector[k].X *= -1;
					}
					RotatedVector.Sort(Comparator);
					EdgeVariations.Add(RotatedVector, FlippedVector);
				}
			}
		}
		EdgeCodesOut.Add(EdgeCodes[EdgeVariations[MeshBordersVector[i]]]);
	}

	//Debug
	if(!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid World!"));
		return true;
	}
	
	TArray<FColor> PieceColors = {FColor::Black, FColor::Red, FColor::Blue, FColor::Green, FColor::Yellow, FColor::Purple};
	
	for(int i = 0; i < MeshBorders.Num(); i++)
	{
		for(int j = 0; j < MeshBorders[i].Num(); j++)
		{
			FVector Point = static_cast<FVector>(VertexBuffer.VertexPosition(MeshBorders[i][j]));
			//if(i >= 1 && i <= 4)
			//Point = Point.RotateAngleAxis(Rotation, FVector(0.f, 0.f, 1.f));
			//DrawDebugSphere(World, Center + Point, 1.5f + static_cast<float>(i) * 0.5f, 3, PieceColors[i], true, -1, 0, 2.f);
		}
	}
	
	return true;
}

void UMeshProcessingLibrary::SaveMeshData()
{
}