// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MeshProcessingLibrary.generated.h"

struct FBuildingMeshData;

UENUM(BlueprintType)
enum class EEdgeSide : uint8
{
	Bottom = 0,
	Front = 1,
	Left = 2,
	Back = 3,
	Right = 4,
	Top = 5
};

/**
 * 
 */
UCLASS()
class WVC_BUILDING_API UMeshProcessingLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "MeshDataLibrary")
    static void ProcessAllMeshes(UDataTable* AllMeshDataTable, UDataTable* OriginalMeshTable, UDataTable* VariationMeshTable, UDataTable* EdgeAdjacencyTable, const FString& FolderPath);

	UFUNCTION(BlueprintCallable, Category = "MeshDataLibrary")
	static void CreateRotationMeshes(UDataTable* OriginalMeshTable, UDataTable* VariationMeshTable);
	
    UFUNCTION(BlueprintCallable, Category = "MeshDataLibrary")
    static void GetMeshData(const FName& MeshName);

	static void NormalizeMarchingBits(TArray<int>& MarchingBits, int& RotationNeeded);

	UFUNCTION(BlueprintCallable, Category = "MeshDataLibrary")
	static void ClearDataTable(UDataTable* DataTable);
private:
	static TArray<int> RotateMarchingBits(const TArray<int>& MarchingBits, const int Rotation);
	static bool DoesMeshHaveInteriorBorders(const FMeshDescription& MeshDescription);
	static bool ProcessMeshData(const UStaticMesh* StaticMesh, TArray<int>& EdgeCodesOut, TMap<TArray<FIntVector>, TArray<FIntVector>>& EdgeVariations, TMap<TArray<FIntVector>, int>& EdgeCodes, TMap<int, int>& EdgeAdjacencies, UWorld* World = nullptr, const FVector& Center = FVector::ZeroVector);
	static UStaticMesh* CombineMeshes(const UStaticMesh* Mesh1, const UStaticMesh* Mesh2, const float SpecificMesh2Rotation, float BothMeshRotation);
	static void CreateAdditionalMeshes(const FBuildingMeshData* Row1, const FBuildingMeshData* Row2, UDataTable* VariationMeshTable);
};
