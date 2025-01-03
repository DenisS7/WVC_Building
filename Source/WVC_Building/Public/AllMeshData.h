// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "AllMeshData.generated.h"

/**
 * 
 */

UENUM(BlueprintType)
enum class EEdgeSide : uint8
{
	Bottom = 0,
	Left = 1,
	Back = 2,
	Right = 3,
	Front = 4,
	Top = 5
};

UCLASS(BlueprintType)
class WVC_BUILDING_API UAllMeshData : public UObject
{
	GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "MeshDataSingleton")
	static UAllMeshData* GetInstance();

	//UAllMeshData(const UAllMeshData&) = delete;

	UFUNCTION(BlueprintCallable, Category = "MeshDataSingleton")
	void ProcessMeshData(UWorld* World, const FVector& Center, float Rotation, const UStaticMesh* StaticMesh);

	//const TArray<int>& GetMeshData(const UStaticMesh* StaticMesh) const;
private:
	static UAllMeshData* Instance;

	TMap<TArray<FVector>, int> EdgeCode;
	TMap<TSoftObjectPtr<UStaticMesh*>, TArray<int>> MeshEdges;
	
	UAllMeshData(); 
};
