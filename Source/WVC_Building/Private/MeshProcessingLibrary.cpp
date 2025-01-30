// Fill out your copyright notice in the Description page of Project Settings.


#include "MeshProcessingLibrary.h"
// Fill out your copyright notice in the Description page of Project Settings.

#include "EdgeAdjacencyData.h"
#include "IPropertyTable.h"
#include "MaterialDomain.h"
#include "StaticMeshOperations.h"
#include "WVC_Building/Public/BuildingMeshData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Math/UnrealMathUtility.h"

void UMeshProcessingLibrary::ProcessAllMeshes(UDataTable* MeshDataTable, UDataTable* OriginalMeshTable, UDataTable* VariationMeshTable, UDataTable* EdgeAdjacencyTable, const FString& FolderPath)
{
	TMap<TArray<FIntVector>, TArray<FIntVector>> EdgeVariations;
	TMap<TArray<FIntVector>, int> EdgeCodes;
	TMap<int, int> EdgeAdjacencies;
	
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
		if(ProcessMeshData(StaticMesh, MeshEdgeCodes, EdgeVariations, EdgeCodes, EdgeAdjacencies))
		{
			const FName MeshName = *StaticMesh->GetName();
			MeshNames.Add(MeshName);
			FBuildingMeshData* MeshRow = MeshDataTable->FindRow<FBuildingMeshData>(MeshName, TEXT("ProcessAllMeshes"));
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
				if(FBuildingMeshData* VariationRow = VariationMeshTable->FindRow<FBuildingMeshData>(MeshName, TEXT("ProcessAllMeshes")))
				{
					MeshData.Corners = VariationRow->Corners;
					MeshData.Priority = VariationRow->Priority;
				}

				if(FBuildingMeshData* OriginalRow = OriginalMeshTable->FindRow<FBuildingMeshData>(MeshName, TEXT("ProcessAllMeshes")))
				{
					MeshData.Corners = OriginalRow->Corners;
					MeshData.Priority = OriginalRow->Priority;
				}
				
				MeshDataTable->AddRow(MeshName, MeshData);
			}
		}
	}

	EdgeAdjacencyTable->EmptyTable();
	for(auto EdgeAdjacency : EdgeAdjacencies)
	{
		FEdgeAdjacencyData EdgeAdjacencyData;
		EdgeAdjacencyData.EdgeCode = EdgeAdjacency.Key;
		EdgeAdjacencyData.CorrespondingEdgeCode = EdgeAdjacency.Value;
		EdgeAdjacencyData.EdgePoints = *EdgeCodes.FindKey(EdgeAdjacency.Key);
		EdgeAdjacencyData.CorrespondingEdgePoints = *EdgeCodes.FindKey(EdgeAdjacency.Value);
		EdgeAdjacencyTable->AddRow(FName(*FString::FromInt(EdgeAdjacency.Key)), EdgeAdjacencyData);
	}

	TArray<FBuildingMeshData*> TableRows;
	MeshDataTable->GetAllRows<FBuildingMeshData>(TEXT("ProcessAllMeshes"), TableRows);
	
	for(const FBuildingMeshData* Row : TableRows)
	{
		if(!MeshNames.Contains(Row->Name))
			MeshDataTable->RemoveRow(Row->Name);		
	}
}

void UMeshProcessingLibrary::CreateRotationMeshes(UDataTable* OriginalMeshTable, UDataTable* VariationMeshTable)
{
	TArray<FBuildingMeshData*> TableRows;
	OriginalMeshTable->GetAllRows<FBuildingMeshData>(TEXT("ProcessAllMeshes"), TableRows);

	for(int i = 0; i < TableRows.Num(); i++)
	{
		for(int j = i; j < TableRows.Num(); j++)
		{
			CreateAdditionalMeshes(TableRows[i], TableRows[j], VariationMeshTable);
		}
	}

	//auto Row2 = OriginalMeshTable->FindRow<FBuildingMeshData>(Mesh2, TEXT("ProcessAllMeshes"));
//
	//CreateAdditionalMeshes(Row1, Row2);
}

void UMeshProcessingLibrary::GetMeshData(const FName& MeshName)
{
}

void UMeshProcessingLibrary::NormalizeMarchingBits(TArray<int>& MarchingBits, int& RotationNeeded)
{
	TArray<int> LowerCorners;
	TArray<int> UpperCorners;

	for(int i = 0; i < MarchingBits.Num(); i++)
	{
		if(MarchingBits[i] < 4)
			LowerCorners.Add(MarchingBits[i]);
		else
			UpperCorners.Add(MarchingBits[i]);
	}

	LowerCorners.Sort();
	UpperCorners.Sort();
	
	if(LowerCorners.Num())
	{
		//Arranging so the first lower corner is the least number
		if(LowerCorners.Num() >= 2)
		{
			for(int j = 0; j < LowerCorners.Num() - 1; j++)
			{
				if(LowerCorners[j] != LowerCorners[j + 1] - 1)
				{
					for(int p = 0; p < LowerCorners.Num() - j - 1; p++)
					{
						const int LastElement = LowerCorners.Last();
						for(int k = LowerCorners.Num() - 1; k >= 1; k--)
						{
							LowerCorners[k] = LowerCorners[k - 1];
						}
						LowerCorners[0] = LastElement;
						
						if(UpperCorners.Num() >= 2)
						{
							const int UpperLastElement = UpperCorners.Last();
							for(int k = UpperCorners.Num() - 1; k >= 1; k--)
							{
								UpperCorners[k] = UpperCorners[k - 1];
							}
							UpperCorners[0] = UpperLastElement;
						}
					}
					break;
				}
			}
		}
		if(LowerCorners[0] != 0)
		{
			const int RotationAmount = 4 - LowerCorners[0];
			RotationNeeded = RotationAmount;
			for(int j = 0; j < LowerCorners.Num(); j++)
			{
				LowerCorners[j] = (LowerCorners[j] + RotationAmount) % 4;
			}
			
			for(int j = 0; j < UpperCorners.Num(); j++)
			{
				UpperCorners[j] = (UpperCorners[j] + RotationAmount) % 4 + 4;
			}
		}
	}
	if(UpperCorners.Num() && (!LowerCorners.Num() || LowerCorners.Num() == 4))
	{
		if(UpperCorners.Num() >= 2)
		{
			for(int j = 0; j < UpperCorners.Num() - 1; j++)
			{
				if(UpperCorners[j] != UpperCorners[j + 1] - 1)
				{
					for(int p = 0; p < UpperCorners.Num() - j - 1; p++)
					{
						const int LastElement = UpperCorners.Last();
						for(int k = UpperCorners.Num() - 1; k >= 1; k--)
						{
							UpperCorners[k] = UpperCorners[k - 1];
						}
						UpperCorners[0] = LastElement;
					}
					break;
				}
			}
		}
		if(UpperCorners[0] != 4)
		{
			const int RotationAmount = 8 - UpperCorners[0];
			RotationNeeded = RotationAmount;
			for(int j = 0; j < UpperCorners.Num(); j++)
			{
				UpperCorners[j] = (UpperCorners[j] + RotationAmount) % 4 + 4;
			}
		}
	}
	MarchingBits = LowerCorners;
	MarchingBits.Append(UpperCorners);
}

void UMeshProcessingLibrary::ClearDataTable(UDataTable* DataTable)
{
	if (!DataTable)
	{
		return;
	}
	
	DataTable->Modify();
	DataTable->EmptyTable();
	DataTable->MarkPackageDirty();
	DataTable->PostEditChange();
}

TArray<int> UMeshProcessingLibrary::RotateMarchingBits(const TArray<int>& MarchingBits, const int Rotation)
{
	TArray<int> Output;
	for(int i = 0; i < MarchingBits.Num(); i++)
	{
		Output.Add((MarchingBits[i] + Rotation) % 4);
		if(MarchingBits[i] >= 4)
			Output.Last() += 4;
	}
	return Output;
}

bool UMeshProcessingLibrary::DoesMeshHaveInteriorBorders(const FMeshDescription& MeshDescription)
{
	//TSet<FVertexID> BorderVertexIDs;
	//
	//for (const FEdgeID& EdgeID : MeshDescription.Edges().GetElementIDs())
	//{
	//	if (MeshDescription.GetEdgeConnectedPolygons(EdgeID).Num() == 1)
	//	{
	//		TArrayView<const FVertexID> EdgeVertices = MeshDescription.GetEdgeVertices(EdgeID);
	//		BorderVertexIDs.Add(EdgeVertices[0]);
	//		BorderVertexIDs.Add(EdgeVertices[1]);
	//	}
	//}
//
	//const TVertexAttributesConstRef<FVector> VertexPositions = MeshDescription.VertexAttributes().GetAttributesRef<FVector>(MeshAttribute::Vertex::Position);
	//const float PieceExtent = 100.f - 1.f;
	//for (const FVertexID& VertexID : BorderVertexIDs)
	//{
	//	const FVector Position = VertexPositions[VertexID];
	//	//Point is on one of the borders
	//	if(Position.X < -PieceExtent || Position.X > PieceExtent)
	//		continue; 
	//	if(Position.Y < -PieceExtent || Position.Y > PieceExtent)
	//		continue;
	//	if(Position.Z < 1.f || Position.Z > 2.f * PieceExtent + 1.f)
	//		continue;
//
	//	//Point is somewhere in the middle
	//	return true;
	//}
	
	return false;
}

bool UMeshProcessingLibrary::ProcessMeshData(const UStaticMesh* StaticMesh, TArray<int>& EdgeCodesOut, TMap<TArray<FIntVector>, TArray<FIntVector>>& EdgeVariations, TMap<TArray<FIntVector>, int>& EdgeCodes, TMap<int, int>& EdgeAdjacencies, UWorld* World, const FVector& Center)
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
	TArray<int> UniqueVertices;

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
				UniqueVertices.AddUnique(Index);
				Found = true;
			}
		}
		if(!Found)
		{
			CorrectVertex.FindOrAdd(Index, Index);
			UniqueVertices.AddUnique(Index);
		}
	}

	TArray<TArray<int>> Triangles;

	//Go through Correct Vertices
		//Determine what side they are on (if any)
	//Go through triangles
		//Check if the vertices have a side in common
		//Check if the vertices is on at least 2 sides
			//Yes? add them to a Vector that would exclude them from being borders on that side

	const float PieceExtent = 100.f - 1.f;
	TMap<int, TArray<EEdgeSide>> UniqueVerticesSides;

	for(int i = 0; i < UniqueVertices.Num(); i++)
	{
		const FVector PointCoord = static_cast<FVector>(VertexBuffer.VertexPosition(UniqueVertices[i]));

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

		UniqueVerticesSides.Add(UniqueVertices[i], Sides);
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

		Triangles.Add(TArray<int>({Index0, Index1, Index2}));
		
		FIntPoint Edge1 = FIntPoint(FMath::Min(Index0, Index1), FMath::Max(Index0, Index1));
		FIntPoint Edge2 = FIntPoint(FMath::Min(Index1, Index2), FMath::Max(Index1, Index2));
		FIntPoint Edge3 = FIntPoint(FMath::Min(Index2, Index0), FMath::Max(Index2, Index0));

		Edges.FindOrAdd(Edge1)++;
		Edges.FindOrAdd(Edge2)++;
		Edges.FindOrAdd(Edge3)++;
	}

	//Go through triangles
		//Check if the vertices have a side in common
		//Check if the vertices is on at least 2 sides
			//Yes? add them to a Vector that would exclude them from being borders on that side

	TArray<TArray<int>> SideExcludeVertices;
	SideExcludeVertices.SetNum(6);
	for(int i = 0; i < Triangles.Num(); i++)
	{
		const FVector From01 = static_cast<FVector>(VertexBuffer.VertexPosition(Triangles[i][1])) - static_cast<FVector>(VertexBuffer.VertexPosition(Triangles[i][0]));
		const FVector From02 = static_cast<FVector>(VertexBuffer.VertexPosition(Triangles[i][2])) - static_cast<FVector>(VertexBuffer.VertexPosition(Triangles[i][0]));
		const FVector Cross = From01.Cross(From02);
		const float SizeSquared = Cross.Size();
		const float Tolerance = 0.01f;
		const bool AreCollinear = SizeSquared < Tolerance;
		if(AreCollinear)
			continue;
		
		EEdgeSide CommonSide = EEdgeSide::Bottom;
		bool HaveCommonSide = false;
		
		TArray<EEdgeSide> Point0Sides = UniqueVerticesSides[Triangles[i][0]];
		TArray<EEdgeSide> Point1Sides = UniqueVerticesSides[Triangles[i][1]];
		TArray<EEdgeSide> Point2Sides = UniqueVerticesSides[Triangles[i][2]];
		for(int j = 0; j < Point0Sides.Num(); j++)
		{
			if(Point1Sides.Contains(Point0Sides[j]) && Point2Sides.Contains(Point0Sides[j]))
			{
				CommonSide = Point0Sides[j];
				HaveCommonSide = true;
				break;
			}
		}

		if(HaveCommonSide)
		{
			if(Point0Sides.Num() >= 2)
			{
				SideExcludeVertices[static_cast<int>(CommonSide)].Add(Triangles[i][0]);
			}
			if(Point1Sides.Num() >= 2)
			{
				SideExcludeVertices[static_cast<int>(CommonSide)].Add(Triangles[i][1]);
			}
			if(Point2Sides.Num() >= 2)
			{
				SideExcludeVertices[static_cast<int>(CommonSide)].Add(Triangles[i][2]);
			}
		}
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
			if(SideExcludeVertices[static_cast<int>(Side)].Contains(EdgePoints[i]))
				continue;
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
		ExtraPoints[i].Sort();
		for(int j = ExtraPoints[i].Num() - 1; j >= 0; j--)
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
			if(Side == EEdgeSide::Bottom)
			{
				for(int j = 0; j < FlippedVector.Num(); j++)
					FlippedVector[j].Z = 2000;
			}
			else if (Side == EEdgeSide::Top)
			{
				for(int j = 0; j < FlippedVector.Num(); j++)
					FlippedVector[j].Z = 0;
			}
			else
			{
				for(int j = 0; j < FlippedVector.Num(); j++)
					Swap(FlippedVector[j].X, FlippedVector[j].Y);
			}
			FlippedVector.Sort(Comparator);
			TArray<FIntVector>* FoundFlippedVector = EdgeVariations.Find(FlippedVector);
			if(FoundFlippedVector)
			{
				UE_LOG(LogTemp, Display, TEXT("Mesh: %s - Flipped Edge code found: %d"), *StaticMesh->GetPathName(), EdgeCodes[*FoundFlippedVector]);
				EdgeAdjacencies.Add(EdgeCodes[MeshBordersVector[i]], EdgeCodes[EdgeVariations[FlippedVector]]);

			}
			else
			{
				EdgeCodes.Add(FlippedVector, EdgeCodes.Num() + 1);
				EdgeVariations.Add(FlippedVector, FlippedVector);
				UE_LOG(LogTemp, Warning, TEXT("Mesh: %s - Connection: %d <-> %d"), *StaticMesh->GetPathName(), EdgeCodes[MeshBordersVector[i]], EdgeCodes[FlippedVector]);
				EdgeAdjacencies.Add(EdgeCodes[MeshBordersVector[i]], EdgeCodes[FlippedVector]);
				EdgeAdjacencies.Add(EdgeCodes[FlippedVector], EdgeCodes[MeshBordersVector[i]]);
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

void UMeshProcessingLibrary::CreateAdditionalMeshes(const FBuildingMeshData* Row1, const FBuildingMeshData* Row2, UDataTable* VariationMeshTable)
{
	if(!Row1 || !Row2)
		return;

	UStaticMesh* Mesh1 = Row1->StaticMesh;
	UStaticMesh* Mesh2 = Row2->StaticMesh;

	if(!Mesh1 || !Mesh2)
		return;
	
	const TPair<bool, bool> Row1Pair = TPair<bool, bool>(Row1->Corners[0] < 4, Row1->Corners.Last() >= 4);
	const TPair<bool, bool> Row2Pair = TPair<bool, bool>(Row2->Corners[0] < 4, Row2->Corners.Last() >= 4);
	const TSet<int> Row1Set = TSet<int>(Row1->Corners);

	for(int i = 0; i < 3; i++)
	{
		//90 and 270 degrees rotations 
		if(i % 2 == 0 && Row1Pair == Row2Pair)
			continue;

		
		const TArray<int> RotatedMesh2Bits = RotateMarchingBits(Row2->Corners, i + 1);
		const TSet<int> Row2Set = TSet<int>(RotatedMesh2Bits);

		//Row1 mesh and rotated row2 mesh overlap marching bits
		if(Row1Set.Intersect(Row2Set).Num())
			continue;

		//don't want to combine meshes that are on top of each other as they should form a wall normally
		bool ValidCombination = true;
		for(int j = 0; j < Row1->Corners.Num(); j++)
		{
			int VerticalAdjacentBit = -1;
			int NextAdjacentBit = -1;
			int PreviousAdjacentBit = -1;
			if(Row1->Corners[j] < 4)
			{
				VerticalAdjacentBit = Row1->Corners[j] + 4;
				NextAdjacentBit = (Row1->Corners[j] + 1) % 4;
				PreviousAdjacentBit = Row1->Corners[j] - 1;
				if(PreviousAdjacentBit < 0)
					PreviousAdjacentBit += 4;
			}
			else
			{
				VerticalAdjacentBit = Row1->Corners[j] - 4;
				NextAdjacentBit = (Row1->Corners[j] + 1) % 4 + 4;
				PreviousAdjacentBit = Row1->Corners[j] - 1;
				if(PreviousAdjacentBit < 4)
					PreviousAdjacentBit += 4;
			}
			if(Row2Set.Contains(VerticalAdjacentBit) || Row2Set.Contains(NextAdjacentBit) || Row2Set.Contains(PreviousAdjacentBit))
			{
				ValidCombination = false;
				break;
			}
		}

		if(!ValidCombination)
			continue;

		int RotationNeeded = 0;
		TArray<int> CombinedMarchingBits = Row1->Corners;
		CombinedMarchingBits.Append(RotatedMesh2Bits);
		NormalizeMarchingBits(CombinedMarchingBits, RotationNeeded);

		UStaticMesh* CombinedMesh = CombineMeshes(Row1->StaticMesh, Row2->StaticMesh, -static_cast<float>(i + 1) * 90.f, -static_cast<float>(RotationNeeded) * 90.f);
		if(!CombinedMesh)
			continue;

		FBuildingMeshData CombinedMeshData;
		CombinedMeshData.Name = *CombinedMesh->GetName();
		CombinedMeshData.StaticMesh = CombinedMesh;
		CombinedMeshData.Corners = CombinedMarchingBits;
		CombinedMeshData.Priority = FMath::Min(Row1->Priority, Row2->Priority);
		FBuildingMeshData* ExistingRow = VariationMeshTable->FindRow<FBuildingMeshData>(*CombinedMesh->GetName(), TEXT("Create Additional Meshes"));
		if(ExistingRow)
		{
			ExistingRow->StaticMesh = CombinedMesh;
			ExistingRow->Corners = CombinedMeshData.Corners;
			ExistingRow->Priority = CombinedMeshData.Priority;
		}
		else
		{
			VariationMeshTable->AddRow(*CombinedMesh->GetName(), CombinedMeshData);
		}
	}
}

UStaticMesh* UMeshProcessingLibrary::CombineMeshes(const UStaticMesh* Mesh1, const UStaticMesh* Mesh2,
                                                   const float SpecificMesh2Rotation, const float BothMeshRotation)
{
	const FString PackagePath = TEXT("/Game/Assets/Meshes/GeneratedMeshes");
	const FString MeshName   = FString::Printf(TEXT("%s_%s_%d"), *Mesh1->GetName(), *Mesh2->GetName(), FMath::Abs(static_cast<int>(SpecificMesh2Rotation)));
	const FString FullPath = PackagePath / MeshName;

	UPackage* Package = CreatePackage(*FullPath);
	if (!Package)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not create or find package %s"), *FullPath);
		return nullptr;
	}
	
	UStaticMesh* ExistingMesh = FindObject<UStaticMesh>(Package, *MeshName);
	UStaticMesh* CombinedMesh = nullptr;

	if (ExistingMesh)
	{
		CombinedMesh = ExistingMesh;
		UE_LOG(LogTemp, Warning, TEXT("Will overwrite existing asset: %s"), *FullPath);
	}
	else
	{
		CombinedMesh = NewObject<UStaticMesh>(Package, *MeshName, RF_Public | RF_Standalone);
		if (!CombinedMesh)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to create new mesh asset: %s"), *FullPath);
			return nullptr;
		}

		CombinedMesh->AddSourceModel();
		FAssetRegistryModule::AssetCreated(CombinedMesh);
	}

	if (CombinedMesh->GetStaticMaterials().Num() == 0)
	{
		// Retrieve the engine’s default material for opaque surfaces:
		UMaterialInterface* DefaultMat = UMaterial::GetDefaultMaterial(EMaterialDomain::MD_Surface);

		// Add it as a single material slot:
		CombinedMesh->GetStaticMaterials().Add(FStaticMaterial(DefaultMat));
	}

	FMeshDescription Mesh1Desc = *Mesh1->GetMeshDescription(0);
	FMeshDescription Mesh2Desc = *Mesh2->GetMeshDescription(0);

	FTransform Mesh1RotateTransform(FRotator(0.f, BothMeshRotation, 0.f));
	FTransform Mesh2RotateTransform(FRotator(0.f, SpecificMesh2Rotation + BothMeshRotation, 0.f));
	FStaticMeshOperations::ApplyTransform(Mesh1Desc, Mesh1RotateTransform.ToMatrixWithScale());
	FStaticMeshOperations::ApplyTransform(Mesh2Desc, Mesh2RotateTransform.ToMatrixWithScale());
	FStaticMeshOperations::FAppendSettings AppendSettings;
	for (int32 ChannelIdx = 0; ChannelIdx < FStaticMeshOperations::FAppendSettings::MAX_NUM_UV_CHANNELS; ++ChannelIdx)
	{
		AppendSettings.bMergeUVChannels[ChannelIdx] = true;
	}

	FStaticMeshOperations::AppendMeshDescription(Mesh2Desc, Mesh1Desc, AppendSettings);

	CombinedMesh->ClearMeshDescriptions();
	CombinedMesh->CreateMeshDescription(0, Mesh1Desc);
	CombinedMesh->CommitMeshDescription(0);

	FStaticMeshSourceModel& SourceModel = CombinedMesh->GetSourceModel(0);
	SourceModel.BuildSettings.bRecomputeNormals = true;
	SourceModel.BuildSettings.bRecomputeTangents = true;
	SourceModel.BuildSettings.bUseFullPrecisionUVs = false;
	SourceModel.BuildSettings.bGenerateLightmapUVs = true;

	CombinedMesh->Build(false);
	CombinedMesh->MarkPackageDirty();
	
	FAssetRegistryModule::AssetCreated(CombinedMesh);

	const FString PackageFileName = FPackageName::LongPackageNameToFilename( FullPath, FPackageName::GetAssetPackageExtension());

	UPackage::SavePackage(
		Package,
		CombinedMesh,
		EObjectFlags::RF_Public | EObjectFlags::RF_Standalone,
		*PackageFileName,
		GError,
		nullptr,
		false,
		true,
		SAVE_NoError
	);

	UE_LOG(LogTemp, Log, TEXT("Successfully combined meshes into: %s"), *PackageFileName);
	return CombinedMesh;
}