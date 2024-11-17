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
    //CageFaces.Add(TArray<int>{0,8,1,2});
    //CageFaces.Add(TArray<int>{0,2,3,4});
    //CageFaces.Add(TArray<int>{0,4,5,6});
    //CageFaces.Add(TArray<int>{0,6,7,8});
    //
    //CageFaces.Add(TArray<int>{9,16,8,1});
    //CageFaces.Add(TArray<int>{18,25,16,9});
    //CageFaces.Add(TArray<int>{25,24,15,16});
    //CageFaces.Add(TArray<int>{16,15,7,8});
//
    //CageFaces.Add(TArray<int>{15,14,6,7});
    //CageFaces.Add(TArray<int>{24,23,14,15});
    //CageFaces.Add(TArray<int>{23,22,13,14});
    //CageFaces.Add(TArray<int>{14,13,5,6});
    //
    //CageFaces.Add(TArray<int>{13,12,4,5});
    //CageFaces.Add(TArray<int>{22,21,12,13});
    //CageFaces.Add(TArray<int>{21,20,11,12});
    //CageFaces.Add(TArray<int>{12,11,3,4});
//
    //CageFaces.Add(TArray<int>{11,10,2,3});
    //CageFaces.Add(TArray<int>{20,19,10,11});
    //CageFaces.Add(TArray<int>{19,18,9,10});
    //CageFaces.Add(TArray<int>{10,9,1,2});

    CageFaces.Add(TArray<int>{0, 8, 1, 2});  // Face 1
    CageFaces.Add(TArray<int>{0, 2, 3, 4});  // Face 2
    CageFaces.Add(TArray<int>{0, 4, 5, 6});  // Face 3
    CageFaces.Add(TArray<int>{0, 6, 7, 8});  // Face 4

    // Side Faces (Connecting Bottom to Middle Layer)
    CageFaces.Add(TArray<int>{10, 9, 1, 2});     // Face 5
    CageFaces.Add(TArray<int>{11, 10, 2, 3});    // Face 6
    CageFaces.Add(TArray<int>{12, 11, 3, 4});    // Face 7
    CageFaces.Add(TArray<int>{13, 12, 4, 5});    // Face 8
    CageFaces.Add(TArray<int>{14, 13, 5, 6});    // Face 9
    CageFaces.Add(TArray<int>{15, 14, 6, 7});    // Face 10
    CageFaces.Add(TArray<int>{16, 15, 7, 8});    // Face 11
    CageFaces.Add(TArray<int>{9, 16, 8, 1});     // Face 12

    // Middle Layer Faces
    CageFaces.Add(TArray<int>{19, 18, 9, 10});   // Face 13
    CageFaces.Add(TArray<int>{20, 19, 10, 11});  // Face 14
    CageFaces.Add(TArray<int>{21, 20, 11, 12});  // Face 15
    CageFaces.Add(TArray<int>{22, 21, 12, 13});  // Face 16
    CageFaces.Add(TArray<int>{23, 22, 13, 14});  // Face 17
    CageFaces.Add(TArray<int>{24, 23, 14, 15});  // Face 18
    CageFaces.Add(TArray<int>{25, 24, 15, 16});  // Face 19
    CageFaces.Add(TArray<int>{18, 25, 16, 9});   // Face 20
//
    //// Top Faces (Connecting Middle Layer to Top Point)
    CageFaces.Add(TArray<int>{17, 19, 18, 25});      // Face 21
    CageFaces.Add(TArray<int>{17, 21, 20, 19});      // Face 22
    CageFaces.Add(TArray<int>{17, 23, 22, 21});      // Face 23
    CageFaces.Add(TArray<int>{17, 25, 24, 23});      // Face 24

    //for(int j = 0; j < 4; j++)
    //{
    //    CageFaces.Add(TArray<int>{CageFaces[j][0] + 17, CageFaces[j][1] + 17, CageFaces[j][2] + 17, CageFaces[j][3] + 17});
    //}

    for(int j = 4; j <= 19; j++)
    {
        CageFaces[j].Swap(1,3);
    }
    
    TArray<FVector> InCageVertices;
    InCageVertices.Emplace(-100.0f, 100.0f, -0.0f);
    InCageVertices.Emplace(100.0f, 100.0f, -0.0f);
    InCageVertices.Emplace(100.0f, -100.0f, -0.0f);
    InCageVertices.Emplace(-100.0f, -100.0f, -0.0f);
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
                InitialCageVertices.Add(InCageVertices[i] * (1.f - static_cast<float>(j) / 2.f) + InCageVertices[(i + 1) % InCageVertices.Num()] * (static_cast<float>(j) / 2.f) + FVector(0.f, 0.f, (static_cast<float>(k) / 2.f) * 200.0f));
            }
        }
    }
    InitialCageVertices.Insert(Center + FVector(0.f, 0.f, 200.0f), 17);
}

void UMeshDeformerComponent::BeginPlay()
{
    Super::BeginPlay();
    // Mesh data is initialized externally
}

double UMeshDeformerComponent::GetAngleBetweenUnitVectors(const FVector& U1, const FVector& U2)
{
    double DotProduct = UE::Geometry::Dot(U1, U2);
    DotProduct = FMath::Clamp(DotProduct, -1.0, 1.0); // Clamp to valid range
    double Angle = acos(DotProduct);
    if (isnan(Angle))
    {
        // Handle NaN angle
        Angle = 0.0;
    }
    return Angle;
    //return 2.0 * asin( (U1 - U2).Size() / 2.0 );
}

double UMeshDeformerComponent::GetTangentOfHalfAngleBetweenUnitVectors(const FVector& U1, const FVector& U2)
{
    double DotProduct = UE::Geometry::Dot(U1, U2);
    double CosTheta = FMath::Clamp(DotProduct, -1.0, 1.0);
    double SinTheta = (UE::Geometry::Cross(U1, U2)).Size();
    if (SinTheta < 0.000000001)
    {
        // Handle zero sine case
        return 0.0;
    }
    return SinTheta / (1.0 + CosTheta);
    //double Factor = (U1 - U2).Size() / 2.0;
   // return Factor / sqrt( FMath::Max(1.0 - Factor * Factor , 0.0) );
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
    double Epsilon = 0.001;
    for(int v = 0; v < InitialCageVertices.Num(); v++)
    {
        D[v] = (Point - InitialCageVertices[v]).Size();
        if(D[v] < Epsilon)//DBL_EPSILON)
        {
            OutWeights[v] = 1.0;
            return;
        }
        U[v] = (InitialCageVertices[v] - Point) / D[v];
    }
    struct Out
    {
        int Index = -1;
        int Face = -1;
        float Lambda = -1.f;
        float Denom = -1.f;
        float LambdaDenom = -1.f;
        FVector FaceMeanVector = FVector(-1.f);
        float Distance = -1.f;
        FVector UnitVector = FVector(-1.f);
        
        Out(int InIndex, int InFace, float InLambda, float InDenom, FVector InFaceMeanVector, float InDistance, FVector InUnitVector)
            : Index(InIndex), Face(InFace), Lambda(InLambda), Denom(InDenom), LambdaDenom(Lambda / InDenom), FaceMeanVector(InFaceMeanVector), Distance(InDistance), UnitVector(InUnitVector)
        {
            
        }
    };

    TMap<int, TArray<Out>> Outs;
    
    
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

            if(FaceMeanVectorSqrNorm > Epsilon)// DBL_EPSILON)
            {
                VFNormDividedBySinTheTai /= UE::Geometry::Cross(FaceMeanVector, Ui).Size();
            }

            FVector Cross1 = UE::Geometry::Cross(FaceMeanVector, Ui);
            FVector Cross2 = UE::Geometry::Cross(FaceMeanVector, UiPlus1);

            FVector Normal1 = (Cross1 * 100000.f).GetSafeNormal();
            FVector Normal2 = (Cross2 * 100000.f).GetSafeNormal();
            
            double TanAlphaIBy2 = GetTangentOfHalfAngleBetweenUnitVectors(Normal1, Normal2);

            double TanAlphaMinusIBy2 = GetTangentOfHalfAngleBetweenUnitVectors((UE::Geometry::Cross(FaceMeanVector, UiMinus1) * 100000.f).GetSafeNormal(),
                                                                        (UE::Geometry::Cross(FaceMeanVector, Ui) * 100000.f).GetSafeNormal());

            double TangentsSum = TanAlphaIBy2 + TanAlphaMinusIBy2;

            Lambdas[i] = VFNormDividedBySinTheTai * TangentsSum / D[Vi];
            Denominator += TangentsSum * UE::Geometry::Dot(FaceMeanVector, Ui) / UE::Geometry::Cross(FaceMeanVector, Ui).Size();
            if(Vi == 0 || Vi == 17)
            {
                TArray<Out>& Array = Outs.FindOrAdd(Vi);
                Array.Emplace(Vi, f, Lambdas[i], Denominator, FaceMeanVector, D[Vi], U[Vi]);
                //if((Vi == 1 || Vi == 18) && (f == 4 || f == 12))
                {
                    //UE_LOG(LogTemp, Warning, TEXT("OG Point: %s, Index: %d, Face: %d, Lambda: %f, Denominator: %f, Lambda/Denom: %f, FaceMeanVector: %s, Distance: %f, UnitVector: %s"), *Point.ToString(), Vi, f, Lambdas[i], Denominator, Lambdas[i] / Denominator, *FaceMeanVector.ToString(), D[Vi], *U[Vi].ToString());
                    //UE_LOG(LogTemp, Warning, TEXT("VFNorm: %f"), VFNormDividedBySinTheTai);
                    //UE_LOG(LogTemp, Warning, TEXT("Cross1: %s, Normal1: %s"), *Cross1.ToString(), *Normal1.ToString());
                    //UE_LOG(LogTemp, Warning, TEXT("Cross2: %s, Normal2: %s"), *Cross2.ToString(), *Normal2.ToString());
                    //UE_LOG(LogTemp, Warning, TEXT("TanAplha1By2: %f, TanAlphaMinus1By2: %f"), TanAlphaIBy2, TanAlphaMinusIBy2);
                    //UE_LOG(LogTemp, Warning, TEXT(" "));

                }
            }
        }
        UE_LOG(LogTemp, Warning, TEXT(" "));
        if(fabs(Denominator) < Epsilon)// DBL_EPSILON)
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
            if(Vi == 0 || Vi == 17)
                UE_LOG(LogTemp, Warning, TEXT("Index: %d, Lambda: %f, Denominator: %f, Lambda/Denom: %f, Weight: %f"), Vi, Lambdas[i], Denominator, Lambdai, W_Weights[Vi]);
        }
    }

    for(const auto& Key : Outs)
    {
        //UE_LOG(LogTemp, Warning, TEXT("Index: %d"), Key.Key);
        for(int i = 0; i < Key.Value.Num(); i++)
        {
            //UE_LOG(LogTemp, Warning, TEXT("OG Point: %s, Index: %d, Face: %d, Lambda: %f, Denominator: %f, Lambda/Denom: %f, FaceMeanVector: %s, Distance: %f, UnitVector: %s"), *Point.ToString(), Key.Value[i].Index, Key.Value[i].Face, Key.Value[i].Lambda, Key.Value[i].Denom, Key.Value[i].LambdaDenom, *Key.Value[i].FaceMeanVector.ToString(), Key.Value[i].Distance, *Key.Value[i].UnitVector.ToString());
        }

        //UE_LOG(LogTemp, Warning, TEXT(" "));
    }

    UE_LOG(LogTemp, Warning, TEXT(" "));
    for(int v = 0; v < InitialCageVertices.Num(); v++)
    {
        OutWeights[v] = W_Weights[v] / SumWeights;
        if(v == 0 || v == 17)
            UE_LOG(LogTemp, Warning, TEXT("Index: %d, Weight: %f, NormalizedWeight: %f, SumWeights: %f"), v, W_Weights[v], OutWeights[v], SumWeights);
    }
    UE_LOG(LogTemp, Warning, TEXT(" "));
}

FVector UMeshDeformerComponent::ComputeSMVCCoordinate(const FVector& OriginalCoordinate)
{
    TArray<double> Weights;
    ComputeSMVCWeights(OriginalCoordinate, Weights);
    FVector DeformedCoordinate = FVector::ZeroVector;
    
    UE_LOG(LogTemp, Error, TEXT("Name: %s, OG Coordinate: %s"), *GetOwner()->GetName(), *OriginalCoordinate.ToString());
    for(int i = 0; i < CageVertices.Num(); i++)
    {
        DeformedCoordinate += Weights[i] * CageVertices[i];
        UE_LOG(LogTemp, Warning, TEXT("Index: %d, Weight: %f, OG Cage Vertex: %s, Cage Vertex: %s, Influence: %s, Current Deformed Coordinate: %s"), i, Weights[i], *InitialCageVertices[i].ToString(), *CageVertices[i].ToString(), *(Weights[i] * CageVertices[i]).ToString(), *DeformedCoordinate.ToString());
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
    CageVertices.Add(Center - Center);
    
    for(int k = 0; k < 3; k++)
    {
        for(int i = 0; i < InCageVertices.Num(); i++)
        {
            for(int j = 0; j < 2; j++)
            {
                CageVertices.Add(InCageVertices[i] * (1.f - static_cast<float>(j) / 2.f) + InCageVertices[(i + 1) % InCageVertices.Num()] * (static_cast<float>(j) / 2.f) + FVector(0.f, 0.f, (static_cast<float>(k) / 2.f) * InHeight) - Center);
            }
        }
    }
    CageVertices.Insert(FVector(0, 0, InHeight), 17);
    
    //FlushPersistentDebugLines(GetWorld());
    //for(int i = 0; i < CageFaces.Num(); i++)
    //{
    //    for(int j = 0; j < CageFaces[i].Num(); j++)
    //    {
    //        DrawDebugLine(GetWorld(), CageVertices[CageFaces[i][j]], CageVertices[CageFaces[i][(j + 1) % CageFaces[i].Num()]], FColor::Green, false, 100.f, 0, 2.f);
    //    }
    //}
    for(int i = 0; i < OriginalVertices.Num(); i++)
    {
        DeformedVertices.Add(ComputeSMVCCoordinate(OriginalVertices[i]));
    }
    ProceduralMeshComp->CreateMeshSection(0, DeformedVertices, Triangles, Normals, UVs, TArray<FColor>(), Tangents, false);
}