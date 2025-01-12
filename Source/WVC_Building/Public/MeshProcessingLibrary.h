// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MeshProcessingLibrary.generated.h"

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
	UFUNCTION(BlueprintCallable, Category = "MeshDataSingleton")
    static void ProcessAllMeshes(UDataTable* DataTable, const FString& FolderPath);
   
    UFUNCTION(BlueprintCallable, Category = "MeshDataSingleton")
    static void GetMeshData(const FName& MeshName);

private:

	static bool ProcessMeshData(const UStaticMesh* StaticMesh, TArray<int>& EdgeCodesOut, TMap<TArray<FIntVector>, TArray<FIntVector>>& EdgeVariations, TMap<TArray<FIntVector>, int>& EdgeCodes, UWorld* World = nullptr, const FVector& Center = FVector::ZeroVector);
   
    UFUNCTION(BlueprintCallable, Category = "MeshDataSingleton")
    static void SaveMeshData();
};
