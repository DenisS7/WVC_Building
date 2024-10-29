// MeshDeformerComponent.cpp

#include "MeshDeformerComponent.h"

#include "AudioMixerBlueprintLibrary.h"
#include "ProceduralMeshComponent.h"
#include "VectorTypes.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/KismetMathLibrary.h"

UMeshDeformerComponent::UMeshDeformerComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    CageFaces.Add(TArray<int>{0,8,1,2});
    CageFaces.Add(TArray<int>{0,2,3,4});
    CageFaces.Add(TArray<int>{0,4,5,6});
    CageFaces.Add(TArray<int>{0,6,7,8});
    
    CageFaces.Add(TArray<int>{9,16,8,1});
    CageFaces.Add(TArray<int>{18,25,16,9});
    CageFaces.Add(TArray<int>{25,24,15,16});
    CageFaces.Add(TArray<int>{16,15,7,8});

    CageFaces.Add(TArray<int>{15,14,6,7});
    CageFaces.Add(TArray<int>{24,23,14,15});
    CageFaces.Add(TArray<int>{23,22,13,14});
    CageFaces.Add(TArray<int>{14,13,5,6});
    
    CageFaces.Add(TArray<int>{13,12,4,5});
    CageFaces.Add(TArray<int>{22,21,12,13});
    CageFaces.Add(TArray<int>{21,20,11,12});
    CageFaces.Add(TArray<int>{12,11,3,4});

    CageFaces.Add(TArray<int>{11,10,2,3});
    CageFaces.Add(TArray<int>{20,19,10,11});
    CageFaces.Add(TArray<int>{19,18,9,10});
    CageFaces.Add(TArray<int>{10,9,1,2});

    for(int j = 0; j < 4; j++)
    {
        CageFaces.Add(TArray<int>{CageFaces[0][0] + 17, CageFaces[0][1] + 17, CageFaces[0][2] + 17, CageFaces[0][3] + 17});
    }
    TArray<FVector> InCageVertices;
    InCageVertices.Emplace(-100.f, -100.f, 0.f);
    InCageVertices.Emplace(-100.f, 100.f, 0.f);
    InCageVertices.Emplace(100.f, 100.f, 0.f);
    InCageVertices.Emplace(100.f, -100.f, 0.f);
    FVector Center = FVector::ZeroVector;
    for(int i = 0; i < 4; i++)
    {
        Center += InCageVertices[i];
    }
    Center /= 4.f;
    InitialCageVertices.Add(Center);
    
    for(int k = 0; k < 3; k++)
    {
        for(int i = 0; i < InCageVertices.Num(); i++)
        {
            for(int j = 0; j < 2; j++)
            {
                InitialCageVertices.Add(InCageVertices[i] * (1.f - static_cast<float>(j) / 2.f) + InCageVertices[(i + 1) % InCageVertices.Num()] * (static_cast<float>(j) / 2.f) + FVector(0.f, 0.f, (static_cast<float>(k) / 2.f) * 200.f));
            }
        }
    }
    InitialCageVertices.Insert(Center + FVector(0.f, 0.f, 200.f), 17);
}

void UMeshDeformerComponent::BeginPlay()
{
    Super::BeginPlay();
    // Mesh data is initialized externally
}

double UMeshDeformerComponent::GetAngleBetweenUnitVectors(const FVector& U1, const FVector& U2)
{
    return 2.0 * asin( (U1 - U2).Size() / 2.0 );
}

double UMeshDeformerComponent::GetTangentOfHalfAngleBetweenUnitVectors(const FVector& U1, const FVector& U2)
{
    double Factor = (U1 - U2).Size() / 2.0;
    return Factor / sqrt( FMath::Max(1.0 - Factor * Factor , 0.0) );
}

void UMeshDeformerComponent::InitializeMeshData(const TArray<FVector>& InVertices, const TArray<int32>& InTriangles,
                                                const TArray<FVector>& InNormals, const TArray<FVector2D>& InUVs,
                                                const TArray<FProcMeshTangent>& InTangents)
{
    OriginalVertices = InVertices;
    DeformedVertices = InVertices;
    Triangles = InTriangles;
    Normals = InNormals;
    UVs = InUVs;
    Tangents = InTangents;
}

void UMeshDeformerComponent::ComputeSMVCWeights(const FVector& Point, TArray<double>& OutWeights)
{
    TArray<double> W_Weights;
    double SumWeights = 0.0;
    TArray<double> D;
    TArray<FVector> U;
    OutWeights.Empty();
    OutWeights.SetNumZeroed(InitialCageVertices.Num());
    W_Weights.SetNumZeroed(InitialCageVertices.Num());
    D.SetNumZeroed(InitialCageVertices.Num());
    U.SetNumZeroed(InitialCageVertices.Num());

    for(int v = 0; v < InitialCageVertices.Num(); v++)
    {
        D[v] = (Point - InitialCageVertices[v]).Size();
        if(D[v] < DBL_EPSILON)
        {
            OutWeights[v] = 1.0;
            return;
        }
        U[v] = (InitialCageVertices[v] - Point) / D[v];
    }

    for(int f = 0; f < CageFaces.Num(); f++)
    {
        FVector FaceMeanVector = FVector::ZeroVector;
        for(int i = 0; i < CageFaces[f].Num(); i++)
        {
            const int V0 = CageFaces[f][i];
            const int V1 = CageFaces[f][(i + 1) % CageFaces[f].Num()];

            const FVector U0 = U[V0];
            const FVector U1 = U[V1];
            const double Angle = GetAngleBetweenUnitVectors(U0, U1);
            const FVector Direction = UE::Geometry::Cross(U0, U1);
            const FVector N = Direction.GetSafeNormal();

            FaceMeanVector += Angle / 2.0 * N;
        }

        double Denominator = 0.0;
        TArray<double> Lambdas;
        Lambdas.SetNumZeroed(CageFaces.Num());

        for(int i = 0; i < CageFaces[f].Num(); i++)
        {
            const int Vi = CageFaces[f][i];
            const int ViPlus1 = CageFaces[f][(i + 1) % CageFaces[f].Num()];
            const int ViMinus1 = CageFaces[f][(i - 1 + CageFaces[f].Num()) % CageFaces[f].Num()];

            const FVector Ui = U[Vi];
            const FVector UiPlus1 = U[ViPlus1];
            const FVector UiMinus1 = U[ViMinus1];

            double FaceMeanVectorSqrNorm = FaceMeanVector.SizeSquared();
            double VFNormDividedBySinTheTai = FaceMeanVectorSqrNorm;

            if(FaceMeanVectorSqrNorm > DBL_EPSILON)
            {
                VFNormDividedBySinTheTai /= UE::Geometry::Cross(FaceMeanVector, Ui).Size();
            }

            FVector Cross1 = UE::Geometry::Cross(FaceMeanVector, Ui);
            FVector Cross2 = UE::Geometry::Cross(FaceMeanVector, UiPlus1);

            FVector Normal1 = (Cross1 * 100000.f).GetSafeNormal();
            FVector Normal2 = (Cross2 * 100000.f).GetSafeNormal();
            
            double TanAlphaIBy2 = GetTangentOfHalfAngleBetweenUnitVectors(Normal1, Normal2);

            double TanAlphaMinusIBy2 = GetTangentOfHalfAngleBetweenUnitVectors(UE::Geometry::Cross(FaceMeanVector, UiMinus1).GetSafeNormal(),
                                                                        UE::Geometry::Cross(FaceMeanVector, Ui).GetSafeNormal());

            double TangentsSum = TanAlphaIBy2 + TanAlphaMinusIBy2;

            Lambdas[i] = VFNormDividedBySinTheTai * TangentsSum / D[Vi];
            Denominator += TangentsSum * UE::Geometry::Dot(FaceMeanVector, Ui) / UE::Geometry::Cross(FaceMeanVector, Ui).Size();
        }

        if(fabs(Denominator) < DBL_EPSILON)
        {
            W_Weights.Empty();
            OutWeights.Empty();
            OutWeights.SetNumZeroed(InitialCageVertices.Num());
            W_Weights.SetNumZeroed(InitialCageVertices.Num());
            SumWeights = 0.0;
            for(int i = 0; i < CageFaces[f].Num(); i++)
            {
                const int Vi = CageFaces[f][i];
                const double Lambdai = Lambdas[i];
                OutWeights[Vi] = Lambdai;
                SumWeights += Lambdai;
            }
            for(int i = 0; i < CageFaces[f].Num(); i++)
            {
                const int Vi = CageFaces[f][i];
                OutWeights[Vi] /= SumWeights;
            }
            return;
        }

        for(int i = 0; i < CageFaces[f].Num(); i++)
        {
            const int Vi = CageFaces[f][i];
            double Lambdai = Lambdas[i] / Denominator;
            W_Weights[Vi] += Lambdai;
            SumWeights += Lambdai;
        }
    }

    for(int v = 0; v < InitialCageVertices.Num(); v++)
    {
        OutWeights[v] = W_Weights[v] / SumWeights;
    }
}

FVector UMeshDeformerComponent::ComputeSMVCCoordinate(const FVector& OriginalCoordinate)
{
    TArray<double> Weights;
    ComputeSMVCWeights(OriginalCoordinate, Weights);
    FVector DeformedCoordinate = FVector::ZeroVector;
    for(int i = 0; i < CageVertices.Num(); i++)
    {
        DeformedCoordinate += Weights[i] * CageVertices[i];
    }
    return DeformedCoordinate;
}

void UMeshDeformerComponent::DeformMesh(const TArray<FVector>& InCageVertices, const float InHeight)
{
    DeformedVertices.Empty();
    CageVertices.Empty();
    //CageVertices = InCageVertices;
    
    FVector Center = FVector::ZeroVector;
    for(int i = 0; i < 4; i++)
    {
        Center += InCageVertices[i];
    }
    Center /= 4.f;
    CageVertices.Add(Center);
    
    for(int k = 0; k < 3; k++)
    {
        for(int i = 0; i < InCageVertices.Num(); i++)
        {
            for(int j = 0; j < 2; j++)
            {
                CageVertices.Add(InCageVertices[i] * (1.f - static_cast<float>(j) / 2.f) + InCageVertices[(i + 1) % InCageVertices.Num()] * (static_cast<float>(j) / 2.f) + FVector(0.f, 0.f, (static_cast<float>(k) / 2.f) * InHeight));
            }
        }
    }
    CageVertices.Insert(Center + FVector(0, 0, InHeight), 17);
    
    FlushPersistentDebugLines(GetWorld());
    for(int i = 0; i < CageFaces.Num(); i++)
    {
        for(int j = 0; j < CageFaces[i].Num(); j++)
        {
            DrawDebugLine(GetWorld(), CageVertices[CageFaces[i][j]], CageVertices[CageFaces[i][(j + 1) % CageFaces[i].Num()]], FColor::Green, false, 100.f, 0, 2.f);
        }
    }
    for(int i = 0; i < OriginalVertices.Num(); i++)
    {
        DeformedVertices.Add(ComputeSMVCCoordinate(OriginalVertices[i]));
        //DeformedVertices.Last() += GetOwner()->GetActorLocation();
    }
    ProceduralMeshComp->CreateMeshSection(0, DeformedVertices, Triangles, Normals, UVs, TArray<FColor>(), Tangents, false);
}


/*
void UMeshDeformerComponent::InitializeDeformation(const TArray<FVector>& CageVertices)
{
    if (CageVertices.Num() != 8)
    {
        UE_LOG(LogTemp, Error, TEXT("Cage must have 8 vertices."));
        return;
    }

    CageVerticesInitial = CageVertices;
    CageVerticesCurrent = CageVertices;

    // Compute QMVC weights for each vertex
    ComputeQMVCWeights();
}

void UMeshDeformerComponent::ComputeQMVCWeights()
{
    int32 VertexCount = OriginalVertices.Num();
    Weights.SetNum(VertexCount);

    for (int32 i = 0; i < VertexCount; ++i)
    {
        Weights[i].SetNum(8); // Cage has 8 vertices
        FVector Point = OriginalVertices[i];

        // Compute weights for each cage vertex
        float WeightSum = 0.0f;
        for (int32 j = 0; j < 8; ++j)
        {
            float Weight = ComputeQMVCWeight(Point, j);
            Weights[i][j] = Weight;
            WeightSum += Weight;
        }

        // Normalize the weights
        if (WeightSum > KINDA_SMALL_NUMBER)
        {
            for (int32 j = 0; j < 8; ++j)
            {
                Weights[i][j] /= WeightSum;
            }
        }
    }
}

float UMeshDeformerComponent::ComputeQMVCWeight(const FVector& Point, int VertexIndex)
{
    // Implement the QMVC weight computation for the given point and cage vertex

    FVector Vi = CageVerticesInitial[VertexIndex];
    FVector Ri = Vi - Point;
    float Di = Ri.Size();

    if (Di < KINDA_SMALL_NUMBER)
    {
        // Point coincides with a cage vertex
        return 1.0f;
    }

    float Numerator = 0.0f;
    float Denominator = 0.0f;

    // Get adjacent vertices
    TArray<int32> AdjacentVertices = GetAdjacentVertices(VertexIndex);

    // For each adjacent edge (Vi, Vj)
    for (int32 AdjIndex : AdjacentVertices)
    {
        FVector Vj = CageVerticesInitial[AdjIndex];
        FVector Rj = Vj - Point;
        float Dj = Rj.Size();

        FVector Eij = Vj - Vi; // Edge vector
        FVector Si = FVector::CrossProduct(Ri, Eij); // Face area at Vi
        FVector Sj = FVector::CrossProduct(Eij, Rj); // Face area at Vj

        float SiNorm = Si.Size();
        float SjNorm = Sj.Size();

        if (SiNorm < KINDA_SMALL_NUMBER || SjNorm < KINDA_SMALL_NUMBER)
        {
            continue; // Avoid division by zero
        }

        float Ai = SiNorm / 2.0f; // Area at Vi
        float Aj = SjNorm / 2.0f; // Area at Vj

        float WeightTerm = (Ai / (Di * Di)) + (Aj / (Dj * Dj));
        Numerator += WeightTerm;
    }

    // The denominator is the sum of all Numerators
    // We'll normalize the weights later
    Denominator = Numerator;

    return Numerator;
}

float UMeshDeformerComponent::ComputeEdgeWeight(const FVector& Pi, const FVector& Pj, const FVector& PiMinusPj)
{
    // Quadratic Mean Value Coordinates edge weight calculation

    float PiLengthSquared = Pi.SizeSquared();
    float PjLengthSquared = Pj.SizeSquared();
    float PiPjDot = FVector::DotProduct(Pi, Pj);

    float Denominator = PiLengthSquared * PjLengthSquared - PiPjDot * PiPjDot;

    if (FMath::Abs(Denominator) < KINDA_SMALL_NUMBER)
    {
        // Avoid division by zero
        return 0.0f;
    }

    float Numerator = FVector::DotProduct(Pi, PiMinusPj) / Denominator;

    return Numerator;
}

TArray<int32> UMeshDeformerComponent::GetAdjacentVertices(int32 VertexIndex)
{
    // Return a list of vertex indices adjacent to the given vertex
    // For a cuboid, each vertex is connected to three edges (since it's a corner)

    switch (VertexIndex)
    {
    case 0: return {1, 3, 4};
    case 1: return {0, 2, 5};
    case 2: return {1, 3, 6};
    case 3: return {0, 2, 7};
    case 4: return {0, 5, 7};
    case 5: return {1, 4, 6};
    case 6: return {2, 5, 7};
    case 7: return {3, 4, 6};
    default: return {};
    }
}

void UMeshDeformerComponent::UpdateDeformation(const TArray<FVector>& NewCageVertices)
{
    if (NewCageVertices.Num() != 8)
    {
        UE_LOG(LogTemp, Error, TEXT("Cage must have 8 vertices."));
        return;
    }

    CageVerticesCurrent = NewCageVertices;
    DeformMesh();
}

void UMeshDeformerComponent::DeformMesh()
{
    int32 VertexCount = OriginalVertices.Num();

    for (int32 i = 0; i < VertexCount; ++i)
    {
        FVector DeformedPosition = FVector::ZeroVector;

        for (int32 j = 0; j < 8; ++j)
        {
            DeformedPosition += Weights[i][j] * CageVerticesCurrent[j] * 100.f;// * 1000000000.f;
        }

        DeformedVertices[i] = DeformedPosition;
    }

    // Update the procedural mesh
    ProceduralMeshComp->UpdateMeshSection(0, DeformedVertices, Normals, UVs, TArray<FColor>(), Tangents);
}Tangents*/
