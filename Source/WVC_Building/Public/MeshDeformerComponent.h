// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "Components/ActorComponent.h"
#include "MeshDeformerComponent.generated.h"

class UProceduralMeshComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class WVC_BUILDING_API UMeshDeformerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMeshDeformerComponent();
	
protected:
	virtual void BeginPlay() override;
	bool Log1 = false;
	bool Log2 = false;
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Deformer")
	UProceduralMeshComponent* ProceduralMeshComp = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Deformer")
	UStaticMesh* StaticMeshComp = nullptr;

	TArray<FVector> OriginalVertices;
	TArray<FVector> DeformedVertices;

	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FProcMeshTangent> Tangents;
	
	TArray<FVector> InitialCageVertices;
	TArray<FVector> CageVertices;
	TArray<TArray<int>> CageFaces;
	void InitializeMeshData(const TArray<FVector>& InVertices, const TArray<int32>& InTriangles,
							const TArray<FVector>& InNormals, const TArray<FVector2D>& InUVs,
							const TArray<FProcMeshTangent>& InTangents);

	void DeformMesh(const TArray<FVector>& InCageVertices, const float InHeight);
private:
	FVector ComputeSMVCCoordinate(const FVector& OriginalCoordinate);
	void ComputeSMVCWeights(const FVector& Point, TArray<double>& OutWeights);
	double GetAngleBetweenUnitVectors(const FVector& U1, const FVector& U2);
	double GetTangentOfHalfAngleBetweenUnitVectors(const FVector& U1, const FVector& U2);

};
