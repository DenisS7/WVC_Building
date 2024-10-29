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

public:
	// Reference to the Procedural Mesh Component
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Deformer")
	UProceduralMeshComponent* ProceduralMeshComp = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Deformer")
	UStaticMesh* StaticMeshComp = nullptr;
	// Original and deformed vertices
	TArray<FVector> OriginalVertices;
	TArray<FVector> DeformedVertices;
//
	//// Triangles, normals, UVs, etc.
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FProcMeshTangent> Tangents;
//
	//// QMVC weights for each vertex
	//TArray<TArray<float>> Weights;
//
	//// Initial and current cage vertices
	///
	TArray<FVector> InitialCageVertices;
	TArray<FVector> CageVertices;
	TArray<TArray<int>> CageFaces;
	//TArray<FVector> CageVerticesCurrent;
//
	//// Function to initialize mesh data
	void InitializeMeshData(const TArray<FVector>& InVertices, const TArray<int32>& InTriangles,
							const TArray<FVector>& InNormals, const TArray<FVector2D>& InUVs,
							const TArray<FProcMeshTangent>& InTangents);

	void DeformMesh(const TArray<FVector>& InCageVertices, const float InHeight);
	// Function to initialize deformation
	//UFUNCTION(BlueprintCallable, Category = "Mesh Deformer")
	//void InitializeDeformation(const TArray<FVector>& CageVertices);

	// Function to update deformation
	//UFUNCTION(BlueprintCallable, Category = "Mesh Deformer")
	//void UpdateDeformation(const TArray<FVector>& NewCageVertices);
private:
	FVector ComputeSMVCCoordinate(const FVector& OriginalCoordinate);
	void ComputeSMVCWeights(const FVector& Point, TArray<double>& OutWeights);
	double GetAngleBetweenUnitVectors(const FVector& U1, const FVector& U2);
	double GetTangentOfHalfAngleBetweenUnitVectors(const FVector& U1, const FVector& U2);
	// Compute QMVC weights
	//void ComputeQMVCWeights();
//
	//// Update mesh vertices based on cage deformation
	//// Compute the QMVC weight for a vertex
//
	//// Helper functions for QMVC
	//FVector ComputeEdgeVector(const FVector& Vi, const FVector& Vj){return FVector::ZeroVector;};
	//float ComputeEdgeWeight(const FVector& Pi, const FVector& Pj, const FVector& PiMinusPj);
	//TArray<int32> GetAdjacentVertices(int32 VertexIndex);
	// ... (Other necessary helper functions)
};
