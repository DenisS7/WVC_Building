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
	StaticMeshComponent->SetHiddenInGame(true);
	SetRootComponent(StaticMeshComponent);
	ProceduralMeshComponent = CreateDefaultSubobject<UProceduralMeshComponent>("ProceduralMeshComponent");
	MeshDeformerComponent = CreateDefaultSubobject<UMeshDeformerComponent>("MeshDeformerComponent");
	MeshDeformerComponent->ProceduralMeshComp = ProceduralMeshComponent;
	MarchingCorners.Init(false, 8);
	ProceduralMeshComponent->SetupAttachment(StaticMeshComponent);
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

	//if(CorrespondingQuadIndex != 45)
	//	return;
	const FGridQuad& Quad = Grid->GetBaseGridQuads()[CorrespondingQuadIndex];
				
	for(int i = 0; i < Quad.Points.Num() && (i + 1) < EdgeCodes.Num(); i++)
	{
		FVector Pos = (Grid->GetBasePointCoordinates(Quad.Points[i]) + Grid->GetBasePointCoordinates(Quad.Points[(i + 1) % Quad.Points.Num()])) / 2.f;
		Pos.Z = 0.f;
		FVector Direction = (Quad.Center - Pos).GetSafeNormal();
		Pos += Direction * 30.f; 
		Pos.Z = 200.f;
		DrawDebugString(GetWorld(), Pos - Quad.Center, FString::FromInt(EdgeCodes[i + 1]), this, FColor::Black, 1.f);
	}
}

void ABuildingPiece::DeformMesh(const TArray<FVector>& CageBase, const float CageHeight, const float Rotation)
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
                Vertices.Add(static_cast<FVector>(VertexBuffer->VertexPosition(i)).RotateAngleAxis(Rotation, UE::Math::TVector<double>(0.f, 0.f, 1.f)));
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
        	ProceduralMeshComponent->SetMaterial(0, StaticMeshComponent->GetMaterial(0));
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