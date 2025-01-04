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

bool UAllMeshData::ProcessMeshData(UWorld* World, const FVector& Center, const float Rotation, const UStaticMesh* StaticMesh, TArray<int>& EdgeCodesOut)
{
	if(!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid World!"));
		return false;
	}
	if (!StaticMesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid StaticMesh"));
		return false;
	}
	MeshEdges.Empty();
	
	if(MeshEdges.Contains(StaticMesh))
	{
		EdgeCodesOut.SetNumZeroed(6);
		int Cycle = (4 - (static_cast<int>((Rotation) / 90.f))) % 4;
		for(int i = 1; i < 5; i++)
		{
			int NewIndex = i - Cycle;
			if(NewIndex <= 0)
				NewIndex += 4;
			if(NewIndex >= 5)
				NewIndex -= 4;
			EdgeCodesOut[i] = MeshEdges[StaticMesh][NewIndex];
		}
		EdgeCodesOut[0] = MeshEdges[StaticMesh][0];
		EdgeCodesOut[5] = MeshEdges[StaticMesh][5];
		return true;
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

	MeshEdges.Add(StaticMesh);
	MeshEdges[StaticMesh].SetNum(6);

	const float PieceExtent = 100.f - 1.f;

	TArray<TArray<int>> MeshBorders;
	TArray<TArray<FIntVector>> MeshBordersVector;
	MeshBorders.SetNum(6);
	MeshBordersVector.SetNum(6);

	TArray<FVector> Mult = {FVector(1.f, 1.f, 0.f),
							FVector(1.f, 0.f, 1.f),
							FVector(0.f, 1.f, 1.f),
							FVector(1.f, 0.f, 1.f),
							FVector(0.f, 1.f, 1.f),
							FVector(1.f, 1.f, 0.f)};
	
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
			SidePoint *= Mult[Index];
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

	TArray<int> EdgeCodes;

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

		return true; // Adjust as needed
	};

	//Get or add border code
	for(int i = 0; i < MeshBordersVector.Num(); i++)
	{
		const EEdgeSide Side = static_cast<EEdgeSide>(i);
		MeshBordersVector[i].Sort(Comparator);
		TArray<FIntVector>* FoundVector = EdgeVariations.Find(MeshBordersVector[i]);
		if(FoundVector)
		{
			UE_LOG(LogTemp, Display, TEXT("Mesh: %s - Edge code found: %d"), *StaticMesh->GetPathName(), EdgeCode[*FoundVector]);
		}
		else
		{
			EdgeCode.Add(MeshBordersVector[i], EdgeCode.Num() + 1);
			TArray<FIntVector> FlippedVector = MeshBordersVector[i];
			TArray<FIntVector> RotatedVector = MeshBordersVector[i];
			TArray<FIntVector> FlippedRotatedVector = MeshBordersVector[i];
			if(Side == EEdgeSide::Front || Side == EEdgeSide::Back)
			{
				for(int j = 0; j < FlippedVector.Num(); j++)
				{
					FlippedVector[j].X *= -1;
				}
				
				for(int j = 0; j < RotatedVector.Num(); j++)
				{
					Swap(RotatedVector[j].X, RotatedVector[j].Y);
					Swap(FlippedRotatedVector[j].X, FlippedRotatedVector[j].Y);
					FlippedRotatedVector[j].Y *= -1;
				}
			}
			else if(Side == EEdgeSide::Left || Side == EEdgeSide::Right)
			{
				for(int j = 0; j < FlippedVector.Num(); j++)
				{
					FlippedVector[j].Y *= -1;
				}
				
				for(int j = 0; j < RotatedVector.Num(); j++)
				{
					Swap(RotatedVector[j].X, RotatedVector[j].Y);
					Swap(FlippedRotatedVector[j].X, FlippedRotatedVector[j].Y);
					FlippedRotatedVector[j].X *= -1;
				}
			}

			FlippedVector.Sort(Comparator);
			RotatedVector.Sort(Comparator);
			FlippedRotatedVector.Sort(Comparator);
			EdgeVariations.Add(MeshBordersVector[i], MeshBordersVector[i]);
			EdgeVariations.Add(FlippedVector, MeshBordersVector[i]);
			EdgeVariations.Add(RotatedVector, MeshBordersVector[i]);
			EdgeVariations.Add(FlippedRotatedVector, MeshBordersVector[i]);
		}
		EdgeCodes.Add(EdgeCode[EdgeVariations[MeshBordersVector[i]]]);
	}
	MeshEdges.Add(StaticMesh, EdgeCodes);
	
	EdgeCodesOut.SetNumZeroed(6);
	int Cycle = (4 - (static_cast<int>((Rotation) / 90.f))) % 4;
	for(int i = 1; i < 5; i++)
	{
		int NewIndex = i - Cycle;
		if(NewIndex <= 0)
			NewIndex += 4;
		if(NewIndex >= 5)
			NewIndex -= 4;
		EdgeCodesOut[i] = MeshEdges[StaticMesh][NewIndex];
	}
	EdgeCodesOut[0] = MeshEdges[StaticMesh][0];
	EdgeCodesOut[5] = MeshEdges[StaticMesh][5];
	

	//Debug
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

void UAllMeshData::GetAllEdgeCodes(TArray<FString>& MeshNames, TArray<FString>& MeshEdgeCodes, TArray<FString>& AllEdgeKeys, TArray<int>& AllEdgeValues)
{
	AllEdgeKeys.Empty();
	AllEdgeValues.Empty();
	MeshNames.Empty();
	MeshEdgeCodes.Empty();
	
	for(auto Edge : EdgeCode)
	{
		FString EdgeString = "";
		for(int i = 0; i < Edge.Key.Num(); i++)
		{
			EdgeString += "(";
			EdgeString += FString::FromInt(Edge.Key[i].X);
			EdgeString += ", ";
			EdgeString += FString::FromInt(Edge.Key[i].X);
			EdgeString += ", ";
			EdgeString += FString::FromInt(Edge.Key[i].X);
			EdgeString += "), ";
		}
		AllEdgeKeys.Add(EdgeString);
		AllEdgeValues.Add(Edge.Value);
	}

	for(auto Mesh : MeshEdges)
	{
		MeshNames.Add(Mesh.Key.GetAssetName());
		FString EdgesCodes = "";
		for(int i = 0; i < Mesh.Value.Num(); i++)
		{
			EdgesCodes += FString::FromInt(Mesh.Value[i]);
			EdgesCodes += "  ";
		}
		MeshEdgeCodes.Add(EdgesCodes);
	}
}

//
// const TArray<int>& UAllMeshData::GetMeshData(const UStaticMesh* StaticMesh) const
// {
// 	return TArray<int>();
// }

