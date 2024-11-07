// Fill out your copyright notice in the Description page of Project Settings.


#include "BuildingPiece.h"
#include "ProceduralMeshComponent.h"
#include "KismetProceduralMeshLibrary.h"
#include "MeshDeformerComponent.h"

// Sets default values
ABuildingPiece::ABuildingPiece()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>("StaticMeshComponent");
	ProceduralMeshComponent = CreateDefaultSubobject<UProceduralMeshComponent>("ProceduralMeshComponent");
	MeshDeformerComponent = CreateDefaultSubobject<UMeshDeformerComponent>("MeshDeformerComponent");
	MeshDeformerComponent->ProceduralMeshComp = ProceduralMeshComponent;
}

// Called when the game starts or when spawned
void ABuildingPiece::BeginPlay()
{
	Super::BeginPlay();

}

// Called every frame
void ABuildingPiece::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ABuildingPiece::DeformMesh(const TArray<FVector>& CageBase, const float CageHeight)
{
    if (!StaticMeshComponent || !StaticMeshComponent->GetStaticMesh())
    {
        UE_LOG(LogTemp, Error, TEXT("StaticMeshComponent does not have a static mesh assigned."));
        return;
    }
	
    UStaticMesh* StaticMesh = StaticMeshComponent->GetStaticMesh();
    if (StaticMesh)
    {
        FPositionVertexBuffer* VertexBuffer = nullptr;
        FStaticMeshVertexBuffer* StaticMeshVertexBuffer = nullptr;
        FIndexArrayView Indices;

        if (StaticMesh->GetRenderData() && StaticMesh->GetRenderData()->LODResources.Num() > 0)
        {
            const FStaticMeshLODResources& LODResources = StaticMesh->GetRenderData()->LODResources[0];

            VertexBuffer = const_cast<FPositionVertexBuffer*>(&LODResources.VertexBuffers.PositionVertexBuffer);
            StaticMeshVertexBuffer = const_cast<FStaticMeshVertexBuffer*>(&LODResources.VertexBuffers.StaticMeshVertexBuffer);
            Indices = LODResources.IndexBuffer.GetArrayView();

            TArray<FVector> Vertices;
            TArray<int32> Triangles;
            TArray<FVector> Normals;
            TArray<FVector2D> UVs;
            TArray<FColor> VertexColors;
            TArray<FProcMeshTangent> Tangents;

            // Copy vertex positions
            for (uint32 i = 0; i < VertexBuffer->GetNumVertices(); i++)
            {
                Vertices.Add(static_cast<FVector>(VertexBuffer->VertexPosition(i)));
            }

            // Copy indices
            for (int32 i = 0; i < Indices.Num(); i++)
            {
                Triangles.Add(Indices[i]);
            }
        	
            for (uint32 i = 0; i < StaticMeshVertexBuffer->GetNumVertices(); i++)
            {
                Normals.Add(static_cast<FVector>(StaticMeshVertexBuffer->VertexTangentZ(i)));
                UVs.Add(static_cast<FVector2D>(StaticMeshVertexBuffer->GetVertexUV(i, 0)));
            }
        	
            StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        	
            MeshDeformerComponent->InitializeMeshData(Vertices, Triangles, Normals, UVs, Tangents);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to access StaticMesh RenderData."));
            return;
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("StaticMeshComponent does not have a valid static mesh."));
        return;
    }
	TArray<FVector> CageVertices = CageBase;
	MeshDeformerComponent->DeformMesh(CageVertices, CageHeight);
}