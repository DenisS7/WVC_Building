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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Deformer")
	UProceduralMeshComponent* ProceduralMeshComp = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Deformer")
	UStaticMesh* StaticMeshComp = nullptr;

	TArray<UE::Math::TVector<double>> OriginalVertices;
	TArray<UE::Math::TVector<double>> DeformedVertices;

	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FProcMeshTangent> Tangents;
	
	TArray<UE::Math::TVector<double>> InitialCageVertices;
	TArray<UE::Math::TVector<double>> CageVertices;
	TArray<TArray<int>> CageFaces;
	
	UMeshDeformerComponent();
	
	void InitializeMeshData(const TArray<FVector>& InVertices, const TArray<int32>& InTriangles,
							const TArray<FVector>& InNormals, const TArray<FVector2D>& InUVs,
							const TArray<FProcMeshTangent>& InTangents);

	void DeformMesh(const TArray<UE::Math::TVector<double>>& InCageVertices, const float InHeight);

protected:
	virtual void BeginPlay() override;
	
	FVector ComputeSMVCCoordinate(const FVector& OriginalCoordinate);
	void ComputeSMVCWeights(const FVector& Point, TArray<double>& OutWeights);
	double GetAngleBetweenUnitVectors(const UE::Math::TVector<double>& U1, const UE::Math::TVector<double>& U2);
	double GetTangentOfHalfAngleBetweenUnitVectors(const UE::Math::TVector<double>& U1, const UE::Math::TVector<double>& U2);

};
