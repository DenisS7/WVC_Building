#pragma once

#include "EdgeAdjacencyData.generated.h"

USTRUCT(BlueprintType)
struct WVC_BUILDING_API FEdgeAdjacencyData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EdgeAdjacencyData")
	int EdgeCode = -1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EdgeAdjacencyData")
	int CorrespondingEdgeCode = -1;
};
