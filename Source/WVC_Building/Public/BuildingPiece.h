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
	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* StaticMeshComponent;
	UPROPERTY(EditAnywhere)
	UProceduralMeshComponent* ProceduralMeshComponent;
	UPROPERTY(EditAnywhere)
	UMeshDeformerComponent* MeshDeformerComponent;
	//TArray<FVector> CageControlPoints;
	///** Original vertices of the target mesh */
	//TArray<FVector> OriginalMeshVertices;
//
	//TArray<FVector> CageBaseVertices;
	//float CageHeight;
	///** MVC weights for each mesh vertex */
	//TArray<float> MVCWeights;
public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void DeformMesh(const TArray<FVector>& CageBase, const float CageHeight);
	
	//void InitializeProceduralMesh();
	//void UpdateDeformation(const TArray<FVector>& CageDeformedVertices);
	///** Computes Mean Value Coordinates (MVC) weights for the mesh vertices */
	//TArray<float> ComputeMVCWeights(const TArray<FVector>& MeshVertices, const TArray<FVector>& CageVertices);
//
	///** Extracts the vertices from the procedural mesh */
	//TArray<FVector> GetMeshVertices();
//
	///** Apply the updated vertices to the procedural mesh */
	//void ApplyVerticesToProceduralMesh(const TArray<FVector>& Vertices);
//
	///** Gets the updated cage vertices (you'll need to implement this as per your logic) */
	//TArray<FVector> GetUpdatedCageVertices();
	//
	//void DeformMesh(const TArray<FVector>& CageBase, const float CageHeight);
	//FVector ApplyLaticeDeformer(const FVector& VertexPosition);
	//void CalculateNormals(const TArray<FVector>& InVertices, const TArray<int32>& InTriangles, TArray<FVector>& OutNormals);
};
