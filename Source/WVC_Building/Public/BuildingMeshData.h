﻿#pragma once

#include "BuildingMeshData.generated.h"

USTRUCT(BlueprintType)
struct WVC_BUILDING_API FBuildingMeshData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BuildingMeshData")
	FName Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BuildingMeshData")
	UStaticMesh* StaticMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BuildingMeshData")
	int Priority = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BuildingMeshData")
	TArray<int> Corners;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BuildingMeshData")
	TArray<int> EdgeCodes;
};
