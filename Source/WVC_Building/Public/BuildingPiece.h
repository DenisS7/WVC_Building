// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GridGenerator.h"
#include "GameFramework/Actor.h"
#include "BuildingPiece.generated.h"

class AGridGenerator;
class UMeshDeformerComponent;
class UProceduralMeshComponent;
struct FGridQuad;

UCLASS()
class WVC_BUILDING_API ABuildingPiece : public AActor
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	TArray<bool> MarchingCorners;
	
	UPROPERTY(EditAnywhere)
	TArray<int> Corners;
	
	UPROPERTY(EditAnywhere)
	UDataTable* DataTable;

	UPROPERTY(EditAnywhere)
	TObjectPtr<AGridGenerator> Grid;
	
	// Sets default values for this actor's properties
	ABuildingPiece();
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	void SetStaticMesh(UStaticMesh* StaticMesh) { StaticMeshComponent->SetStaticMesh(StaticMesh); }
	void DeformMesh(const TArray<FVector>& CageBase, const float CageHeight, const float Rotation);

	TObjectPtr<AGridGenerator> GetGrid() const { return Grid; }
	int GetIndex() const { return Index; }
	int GetElevation() const { return Elevation; }
	const UProceduralMeshComponent* GetProceduralMeshComponent() const { return ProceduralMeshComponent; }
protected:
	friend AGridGenerator;
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UStaticMeshComponent* StaticMeshComponent = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UProceduralMeshComponent* ProceduralMeshComponent = nullptr;
	
	UPROPERTY(EditAnywhere)
	UMeshDeformerComponent* MeshDeformerComponent = nullptr;
	
	UPROPERTY(EditAnywhere)
	uint32 Elevation = 0;
	
	UPROPERTY(EditAnywhere)
	uint32 QuadIndex = 0;

	UPROPERTY(EditAnywhere)
	int Index = -1;
public:	
};
