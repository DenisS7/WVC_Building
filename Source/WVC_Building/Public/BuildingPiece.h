// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BuildingPiece.generated.h"

class UMeshDeformerComponent;
class UProceduralMeshComponent;
struct FGridQuad;

UCLASS()
class WVC_BUILDING_API ABuildingPiece : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ABuildingPiece();
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UStaticMeshComponent* StaticMeshComponent;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UProceduralMeshComponent* ProceduralMeshComponent;
	UPROPERTY(EditAnywhere)
	UMeshDeformerComponent* MeshDeformerComponent;
	UPROPERTY(EditAnywhere)
	uint32 Elevation = 0;
	UPROPERTY(EditAnywhere)
	uint32 QuadIndex = 0;
public:	
	UPROPERTY(EditAnywhere)
	TArray<bool> MarchingCorners;
	UPROPERTY(EditAnywhere)
	TArray<int> Corners;
	UPROPERTY(EditAnywhere)
	UDataTable* DataTable;
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void SetStaticMesh(UStaticMesh* StaticMesh) { StaticMeshComponent->SetStaticMesh(StaticMesh); }
	void DeformMesh(const TArray<FVector>& CageBase, const float CageHeight, const float Rotation);
};
