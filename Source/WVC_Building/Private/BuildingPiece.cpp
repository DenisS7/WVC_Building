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
	// Ensure StaticMeshComponent has a static mesh assigned
    if (!StaticMeshComponent || !StaticMeshComponent->GetStaticMesh())
    {
        UE_LOG(LogTemp, Error, TEXT("StaticMeshComponent does not have a static mesh assigned."));
        return;
    }

    // Convert StaticMeshComponent to ProceduralMeshComponent
    UStaticMesh* StaticMesh = StaticMeshComponent->GetStaticMesh();
    if (StaticMesh)
    {
        // Get mesh data from static mesh
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

            // Copy normals and UVs
            for (uint32 i = 0; i < StaticMeshVertexBuffer->GetNumVertices(); i++)
            {
                Normals.Add(static_cast<FVector>(StaticMeshVertexBuffer->VertexTangentZ(i)));
                UVs.Add(static_cast<FVector2D>(StaticMeshVertexBuffer->GetVertexUV(i, 0)));
            }

            // Initialize the procedural mesh component
           // ProceduralMeshComponent->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, VertexColors, Tangents, true);

            // Disable collision on the static mesh component
            StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            //StaticMeshComponent->SetHiddenInGame(true);

            // Initialize the deformer component's mesh data
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
	for(int i = 0; i < 4; i++)
	{
	//	CageVertices.Add(CageVertices[i] + FVector(0, 0, CageHeight));
	}
    //SetupCage();
	MeshDeformerComponent->DeformMesh(CageVertices, CageHeight);
}

/*void ABuildingPiece::DeformMesh(const TArray<FVector>& CageBase, const float CageHeightt)
{
	CageBaseVertices = CageBase;
	CageHeight = CageHeightt;
	if (StaticMeshComponent)
	{
		// Create a procedural mesh component at runtime
		//ProceduralMeshComponent = NewObject<UProceduralMeshComponent>(GetOwner());
		//ProceduralMeshComponent->AttachToComponent(GetOwner()->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
		//ProceduralMeshComponent->RegisterComponent();

		InitializeProceduralMesh();
        
		OriginalMeshVertices = GetMeshVertices();
		MVCWeights = ComputeMVCWeights(OriginalMeshVertices, CageBase);
		UpdateDeformation(CageBaseVertices);
	}
	
	//CageControlPoints.Empty();
	//CageControlPoints.Append(CageBase);
	//for(int i = 0; i < CageBase.Num(); i++)
	//{
	//	CageControlPoints.Add(CageBase[i] + FVector(0.f, 0.f, CageHeight));		
	//}
	//UStaticMesh* StaticMesh = StaticMeshComponent->GetStaticMesh();
	//TArray<FVector> Vertices;
	//TArray<int32> Triangles;
	//TArray<FVector> Normals;
	//TArray<FVector2D> UV0;
	//TArray<FProcMeshTangent> Tangents;
    //
	//// Extract data from static mesh
	//UKismetProceduralMeshLibrary::GetSectionFromStaticMesh(StaticMeshComponent->GetStaticMesh(), 0, 0, Vertices, Triangles, Normals, UV0, Tangents);
    //
	//ProceduralMeshComponent->CreateMeshSection(0, Vertices, Triangles, Normals, UV0, TArray<FColor>(), Tangents, true);
	//TArray<FVector> FVertices;
	//ProceduralMeshComponent->GetProcMeshSection(0)->ProcVertexBuffer;//GetProcVertexBuffer(FVertices);
    //
	//for (const FProcMeshVertex& Vertex : ProceduralMeshComponent->GetProcMeshSection(0)->ProcVertexBuffer)
	//{
	//	Vertices.Add(Vertex.Position);
	//}
	//
	//for (FVector& Vertex : FVertices)
	//{
	//	Vertex = ApplyLaticeDeformer(Vertex);
	//}
    //
	//ProceduralMeshComponent->CreateMeshSection(0, FVertices, Triangles, Normals, UV0, TArray<FColor>(), Tangents, true);
	//ticMesh* StaticMesh = StaticMeshComponent->GetStaticMesh();
	//StaticMesh)
	//return;
	//ay<FVector> Vertices;
	//ay<int32> Triangles;
	//ay<FVector2D> UV0;
	//ay<FVector> Normals;
	//ay<FProcMeshTangent> Tangents;
	//taticMesh->GetRenderData() && StaticMesh->GetRenderData()->LODResources.Num() > 0)
	//
	////FPositionVertexBuffer* VertexBuffer = &StaticMesh->GetRenderData()->LODResources[0].VertexBuffers.PositionVertexBuffer;
	//FPositionVertexBuffer& VertexBuffer = StaticMesh->GetRenderData()->LODResources[0].VertexBuffers.PositionVertexBuffer;
	//FRawStaticIndexBuffer& IndexBuffer = StaticMesh->GetRenderData()->LODResources[0].IndexBuffer;
	//
	//// Get vertices
	//for (uint32 i = 0; i < VertexBuffer.GetNumVertices(); i++)
	//{
	//	Vertices.Add(static_cast<FVector>(VertexBuffer.VertexPosition(i)));
	//}
    //    
	//// Get triangles
	//for (int32 i = 0; i < IndexBuffer.GetNumIndices(); i++)
	//{
	//	Triangles.Add(IndexBuffer.GetIndex(i));
	//}

	//// Find min/max of X and Y from quad corners
	//const float MinX = FMath::Min(
	//	FMath::Min(CageBase[0].X, CageBase[1].X),
	//	FMath::Min(CageBase[2].X, CageBase[3].X)
	//);
	//const float MaxX = FMath::Max(
	//	FMath::Max(CageBase[0].X, CageBase[1].X),
	//	FMath::Max(CageBase[2].X, CageBase[3].X)
	//);

	//const float MinY = FMath::Min(
	//	FMath::Min(CageBase[0].Y, CageBase[1].Y),
	//	FMath::Min(CageBase[2].Y, CageBase[3].Y)
	//);
	//const float MaxY = FMath::Max(
	//	FMath::Max(CageBase[0].Y, CageBase[1].Y),
	//	FMath::Max(CageBase[2].Y, CageBase[3].Y)
	//);

	//// Z is straightforward since we know the height
	//const float MinZ = CageBase[0].Z;  // Assuming quad is on bottom
	//const float MaxZ = MinZ + CageHeight;

	//FVector CageMin(MinX, MinY, MinZ);
	//FVector CageMax(MaxX, MaxY, MaxZ);

	//for (uint32 VertexIndex = 0; VertexIndex < Vertices.Num(); VertexIndex++)
	//{
	//	FVector OriginalPosition = static_cast<FVector>(Vertices->VertexPosition(VertexIndex));

	//	// Map the original vertex position from [-0.5, 0.5] space to [0, 1] space for interpolation
	//	FVector NormalizedPosition = (OriginalPosition + FVector(0.5f, 0.5f, 0.5f));

	//	// Perform trilinear interpolation using the eight vertices of the cage
	//	FVector DeformedPosition = 
	//		CageBase[0] * (1 - NormalizedPosition.X) * (1 - NormalizedPosition.Y) * (1 - NormalizedPosition.Z) +
	//		CageBase[1] * NormalizedPosition.X * (1 - NormalizedPosition.Y) * (1 - NormalizedPosition.Z) +
	//		CageBase[2] * NormalizedPosition.X * NormalizedPosition.Y * (1 - NormalizedPosition.Z) +
	//		CageBase[3] * (1 - NormalizedPosition.X) * NormalizedPosition.Y * (1 - NormalizedPosition.Z) +
	//		(CageBase[0] + FVector(0.f, 0.f, 100.f)) * (1 - NormalizedPosition.X) * (1 - NormalizedPosition.Y) * NormalizedPosition.Z +
	//		(CageBase[1] + FVector(0.f, 0.f, 100.f)) * NormalizedPosition.X * (1 - NormalizedPosition.Y) * NormalizedPosition.Z +
	//		(CageBase[2] + FVector(0.f, 0.f, 100.f)) * NormalizedPosition.X * NormalizedPosition.Y * NormalizedPosition.Z +
	//		(CageBase[3] + FVector(0.f, 0.f, 100.f)) * (1 - NormalizedPosition.X) * NormalizedPosition.Y * NormalizedPosition.Z;

	//	// Update the vertex position to the newly calculated position
	//	VertexBuffer->VertexPosition(VertexIndex) = static_cast<FVector3f>(DeformedPosition);
	//}
		//StaticMeshComponent->MarkRenderStateDirty();
		//for (FVector& Vertex : Vertices)
		//{
		//	// Normalize position within original bounds
		//	FVector NormalizedPos = (Vertex - CageMin) / (CageMax - CageMin);
        //
		//	// Apply deformation based on normalized position
		//	// This is a simple linear interpolation - you might want something more sophisticated
		//	float u = NormalizedPos.X;
		//	float v = NormalizedPos.Y;
		//	float w = NormalizedPos.Z;
        //
		//	// Interpolate new position based on quad corners for bottom face
		//	FVector NewPos;
		//	//if (w < 0.5f) // Bottom half
		//	{
		//		// Bilinear interpolation on quad
		//		FVector P0 = CageBase[0];
		//		FVector P1 = CageBase[1];
		//		FVector P2 = CageBase[2];
		//		FVector P3 = CageBase[3];
        //    
		//		NewPos = (1-u)*(1-v)*P0 + u*(1-v)*P1 + u*v*P2 + (1-u)*v*P3;
		//		NewPos.Z = Vertex.Z; // Maintain original height
		//	}
		//	//else
		//	//{
		//	//	// Keep top half relatively unchanged or apply different deformation
		//	//	NewPos = Vertex;
		//	//}
        //
		//	Vertex = NewPos;
		//}
		//CalculateNormals(Vertices, Triangles, Normals);
		//ProceduralMeshComponent->CreateMeshSection(0, Vertices, Triangles, Normals, UV0, TArray<FColor>(), Tangents, true);
}

void ABuildingPiece::InitializeProceduralMesh()
{
    // Extract mesh data from StaticMeshComponent and apply it to ProceduralMeshComponent
    if (!StaticMeshComponent || !ProceduralMeshComponent) return;

    // Retrieve LOD 0 for simplicity; adjust if needed
    FStaticMeshLODResources& LODResource = StaticMeshComponent->GetStaticMesh()->GetRenderData()->LODResources[0];
    TArray<FVector> Vertices;
    TArray<int32> Triangles;

    FPositionVertexBuffer& PositionVertexBuffer = LODResource.VertexBuffers.PositionVertexBuffer;
    for (uint32 i = 0; i < PositionVertexBuffer.GetNumVertices(); i++)
    {
        Vertices.Add(static_cast<FVector>(PositionVertexBuffer.VertexPosition(i)));
    }

    // Get triangles (index buffer)
    FRawStaticIndexBuffer& IndexBuffer = LODResource.IndexBuffer;
    for (int32 i = 0; i < IndexBuffer.GetNumIndices(); i++)
    {
        Triangles.Add(IndexBuffer.GetIndex(i));
    }
	UStaticMesh* StaticMesh = StaticMeshComponent->GetStaticMesh();
	//TArray<FVector> Vertices;
	//TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UV0;
	TArray<FProcMeshTangent> Tangents;
	
	// Extract data from static mesh
	UKismetProceduralMeshLibrary::GetSectionFromStaticMesh(StaticMeshComponent->GetStaticMesh(), 0, 0, Vertices, Triangles, Normals, UV0, Tangents);
	
	ProceduralMeshComponent->CreateMeshSection(0, Vertices, Triangles, Normals, UV0, TArray<FColor>(), Tangents, true);
	
    // Set procedural mesh section
    //ProceduralMeshComponent->CreateMeshSection(0, Vertices, Triangles, TArray<FVector>(), TArray<FVector2D>(), TArray<FColor>(), TArray<FProcMeshTangent>(), true);
}

TArray<float> ABuildingPiece::ComputeMVCWeights(const TArray<FVector>& MeshVertices, const TArray<FVector>& CageVertices)
{
    TArray<float> Weights;

    for (const FVector& Vertex : MeshVertices)
    {
        float Weight = 0.0f;
        for (int32 i = 0; i < CageVertices.Num(); i++)
        {
            int32 Next = (i + 1) % CageVertices.Num();
            FVector Edge1 = CageVertices[i] - Vertex;
            FVector Edge2 = CageVertices[Next] - Vertex;

            float Theta = FMath::Acos(FVector::DotProduct(Edge1.GetSafeNormal(), Edge2.GetSafeNormal()));
            Weight += (FMath::Tan(Theta / 2.0f)) / Edge1.Size();
        }
        Weights.Add(Weight);
    }
    return Weights;
}

void ABuildingPiece::UpdateDeformation(const TArray<FVector>& CageDeformedVertices)
{
    if (!ProceduralMeshComponent || MVCWeights.Num() == 0) return;

    TArray<FVector> DeformedMeshVertices;
    for (int32 i = 0; i < OriginalMeshVertices.Num(); i++)
    {
        FVector NewPosition(0.0f);

        for (int32 j = 0; j < CageBaseVertices.Num(); j++)
        {
            float Weight = MVCWeights[(i * CageBaseVertices.Num() + j) % MVCWeights.Num()];
            NewPosition += Weight * CageDeformedVertices[j];
        }

        float HeightFactor = OriginalMeshVertices[i].Z / CageHeight;
        NewPosition.Z += HeightFactor * CageDeformedVertices.Last().Z;

        DeformedMeshVertices.Add(NewPosition);
    }

    ApplyVerticesToProceduralMesh(DeformedMeshVertices);
}

void ABuildingPiece::ApplyVerticesToProceduralMesh(const TArray<FVector>& Vertices)
{
    if (ProceduralMeshComponent && Vertices.Num() > 0)
    {
    	UStaticMesh* StaticMesh = StaticMeshComponent->GetStaticMesh();
    	TArray<FVector> Vertices2;
    	TArray<int32> Triangles;
    	TArray<FVector> Normals;
    	TArray<FVector2D> UV0;
    	TArray<FProcMeshTangent> Tangents;
	
    	// Extract data from static mesh
    	UKismetProceduralMeshLibrary::GetSectionFromStaticMesh(StaticMeshComponent->GetStaticMesh(), 0, 0, Vertices2, Triangles, Normals, UV0, Tangents);
	

        ProceduralMeshComponent->UpdateMeshSection(0, Vertices, Normals, UV0, TArray<FColor>(), Tangents);
    }
}

TArray<FVector> ABuildingPiece::GetMeshVertices()
{
	TArray<FVector> Vertices;
	if (!ProceduralMeshComponent) return Vertices;

	// Retrieve the section from the procedural mesh
	FProcMeshSection* MeshSection = ProceduralMeshComponent->GetProcMeshSection(0);
	if (MeshSection)
	{
		for (const FProcMeshVertex& Vertex : MeshSection->ProcVertexBuffer)
		{
			Vertices.Add(Vertex.Position);
		}
	}
    
	return Vertices;
}

TArray<FVector> ABuildingPiece::GetUpdatedCageVertices()
{
    // Implement logic to get the updated cage vertices at runtime
    TArray<FVector> UpdatedVertices = { /* updated cage vertices here #1# };
    return UpdatedVertices;
}

FVector ABuildingPiece::ApplyLaticeDeformer(const FVector& VertexPosition)
{
	FVector NewPosition = FVector::ZeroVector;
	float TotalWeight = 0.0f;

	// Apply lattice deformation with weighted control point influence
	for (const FVector& ControlPoint : CageControlPoints)
	{
		float Distance = FVector::Dist(ControlPoint, VertexPosition);
		//float Weight = FMath::Pow(FMath::Max(1.0f - (Distance / InfluenceFalloff), 0.0f), 2);

		//NewPosition += ControlPoint * Weight;
		//TotalWeight += Weight;
	}

	if (TotalWeight > 0.0f)
	{
		NewPosition /= TotalWeight;  // Normalize for smooth blending
	}

	return NewPosition;
}

void ABuildingPiece::CalculateNormals(const TArray<FVector>& InVertices, const TArray<int32>& InTriangles, TArray<FVector>& OutNormals)
{
	OutNormals.SetNum(InVertices.Num());
	OutNormals.Init(FVector::ZeroVector, InVertices.Num());
    
	// Calculate normals for each triangle and accumulate
	for (int32 i = 0; i < InTriangles.Num(); i += 3)
	{
		FVector V0 = InVertices[InTriangles[i]];
		FVector V1 = InVertices[InTriangles[i + 1]];
		FVector V2 = InVertices[InTriangles[i + 2]];

		// Calculate triangle edges
		FVector Edge1 = V1 - V0;
		FVector Edge2 = V2 - V0;
		
		const FVector Normal = FVector::CrossProduct(Edge1, Edge2);
        
		OutNormals[InTriangles[i]] += Normal;
		OutNormals[InTriangles[i + 1]] += Normal;
		OutNormals[InTriangles[i + 2]] += Normal;
	}
    
	// Normalize accumulated normals
	for (FVector& Normal : OutNormals)
	{
		if (!Normal.IsNearlyZero())
		{
			Normal = Normal.GetSafeNormal();
		}
		else
		{
			Normal = FVector::UpVector;  // Fallback
		}
	}
}*/
