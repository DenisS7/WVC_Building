#pragma once

#include "MeshCornersData.generated.h"

USTRUCT(BlueprintType)
struct WVC_BUILDING_API FMeshCornersData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
	UStaticMesh* Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
	TArray<int> Corners;
};
