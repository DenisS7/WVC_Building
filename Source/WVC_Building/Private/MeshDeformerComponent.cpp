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

    CageFaces.Add(TArray<int>{0, 7, 6, 5, 4, 3, 2, 1});
    CageFaces.Add(TArray<int>{12, 13, 14, 9, 2, 1, 0, 8});
    CageFaces.Add(TArray<int>{14, 15, 16, 10, 4, 3, 2, 9});
    CageFaces.Add(TArray<int>{16, 17, 18, 11, 6, 5, 4, 10});
    CageFaces.Add(TArray<int>{18, 19, 12, 8, 0, 7, 6, 11});
    CageFaces.Add(TArray<int>{12, 13, 14, 15, 16, 17, 18, 19});

    for(int j = 1; j <= 4; j++)
    {
        CageFaces[j].Swap(1,7);
        CageFaces[j].Swap(2,6);
        CageFaces[j].Swap(3,5);
    }
    
    TArray<UE::Math::TVector<double>> InCageVertices;
    InCageVertices.Emplace(-100.0, 100.0, -0.0);
    InCageVertices.Emplace(100.0, 100.0, -0.0);
    InCageVertices.Emplace(100.0, -100.0, -0.0);
    InCageVertices.Emplace(-100.0, -100.0, -0.0);
    
    for(int k = 0; k < 3; k++)
    {
        for(int i = 0; i < InCageVertices.Num(); i++)
        {
            for(int j = 0; j < 2; j++)
            {
                if(k == 0 || k == 2)
                    InitialCageVertices.Add(InCageVertices[i] * (1.f - static_cast<double>(j) / 2.f) + InCageVertices[(i + 1) % InCageVertices.Num()] * (static_cast<double>(j) / 2.f) + UE::Math::TVector<double>(0.0, 0.0, (static_cast<double>(k) / 2.0) * 200.0));
                else
                {
                    InitialCageVertices.Add(InCageVertices[i] + UE::Math::TVector<double>(0.0, 0.0, 100.0));
                    break;
                }
            }
        }
    }
    
}

void UMeshDeformerComponent::BeginPlay()
{
    Super::BeginPlay();
}

double UMeshDeformerComponent::GetAngleBetweenUnitVectors(const UE::Math::TVector<double>& U1, const UE::Math::TVector<double>& U2)
{
    return 2.0 * asin( (U1 - U2).Size() / 2.0 );
}

double UMeshDeformerComponent::GetTangentOfHalfAngleBetweenUnitVectors(const UE::Math::TVector<double>& U1, const UE::Math::TVector<double>& U2)
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
    UE::Math::TVector<double> CorrectPoint = UE::Math::TVector<double>(std::roundf(Point.X), std::roundf(Point.Y), std::roundf(Point.Z));
    TArray<double> W_Weights;
    double SumWeights = 0.0;
    TArray<double> D;
    TArray<UE::Math::TVector<double>> U;
    OutWeights.Empty();
    //FVector<double> t;
    OutWeights.SetNumZeroed(InitialCageVertices.Num());
    W_Weights.SetNumZeroed(InitialCageVertices.Num());
    D.SetNumZeroed(InitialCageVertices.Num());
    U.SetNumZeroed(InitialCageVertices.Num());
    double Epsilon = 0.000000001;
    
    for(int v = 0; v < InitialCageVertices.Num(); v++)
    {
        D[v] = (CorrectPoint - InitialCageVertices[v]).Size();
        if(D[v] < DBL_EPSILON)
        {
            OutWeights[v] = 1.0;
            return;
        }
        U[v] = (InitialCageVertices[v] - CorrectPoint) / D[v];
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
    
    TArray<int> IndicesToWatch = {8, 11};
    
    for(int f = 0; f < CageFaces.Num(); f++)
    {
        UE::Math::TVector<double> FaceMeanVector = FVector::ZeroVector;
        for(int i = 0; i < CageFaces[f].Num(); i++)
        {
            const int V0 = CageFaces[f][i];
            const int V1 = CageFaces[f][(i + 1) % CageFaces[f].Num()];

            const UE::Math::TVector<double> U0 = U[V0];
            const UE::Math::TVector<double> U1 = U[V1];
            const double Angle = GetAngleBetweenUnitVectors(U0, U1);
            const UE::Math::TVector<double> Direction = UE::Geometry::Cross(U0, U1);
            const UE::Math::TVector<double> N = Direction.GetSafeNormal(FLT_EPSILON);
            FaceMeanVector += Angle / 2.0 * N;
        }

        double Denominator = 0.0;
        TArray<double> Lambdas;
        Lambdas.SetNumZeroed(CageFaces[f].Num());

        for(int i = 0; i < CageFaces[f].Num(); i++)
        {
            const int Vi = CageFaces[f][i];
            const int ViPlus1 = CageFaces[f][(i + 1) % CageFaces[f].Num()];
            const int ViMinus1 = CageFaces[f][(i - 1 + CageFaces[f].Num()) % CageFaces[f].Num()];

            const UE::Math::TVector<double> Ui = U[Vi];
            const UE::Math::TVector<double> UiPlus1 = U[ViPlus1];
            const UE::Math::TVector<double> UiMinus1 = U[ViMinus1];

            double FaceMeanVectorSqrNorm = FaceMeanVector.SizeSquared();
            double VFNormDividedBySinTheTai = FaceMeanVectorSqrNorm;
            UE::Math::TVector<double> d;
            if(FaceMeanVectorSqrNorm > DBL_EPSILON)// DBL_EPSILON)
            {
                VFNormDividedBySinTheTai /= UE::Geometry::Cross(FaceMeanVector, Ui).Size();
            }

            UE::Math::TVector<double> Cross1 = UE::Geometry::Cross(FaceMeanVector, Ui);
            UE::Math::TVector<double> Cross2 = UE::Geometry::Cross(FaceMeanVector, UiPlus1);

            UE::Math::TVector<double> Normal1 = (Cross1).GetSafeNormal(DBL_EPSILON);
            UE::Math::TVector<double> Normal2 = (Cross2).GetSafeNormal(DBL_EPSILON);
            
            double TanAlphaIBy2 = GetTangentOfHalfAngleBetweenUnitVectors(Normal1, Normal2);

            UE::Math::TVector<double> Cross3 = UE::Geometry::Cross(FaceMeanVector, UiMinus1);
            UE::Math::TVector<double> Cross4 = UE::Geometry::Cross(FaceMeanVector, Ui);

            UE::Math::TVector<double> Normal3 = (Cross3).GetSafeNormal(DBL_EPSILON);
            UE::Math::TVector<double> Normal4 = (Cross4).GetSafeNormal(DBL_EPSILON);
            
            double TanAlphaMinusIBy2 = GetTangentOfHalfAngleBetweenUnitVectors(Normal3, Normal4);

            double TangentsSum = TanAlphaIBy2 + TanAlphaMinusIBy2;
            if(std::isinf(TangentsSum))
                TangentsSum = 9999999999.999999;
            Lambdas[i] = VFNormDividedBySinTheTai * TangentsSum / D[Vi];
            Denominator += TangentsSum * UE::Geometry::Dot(FaceMeanVector, Ui) / Cross1.Size();
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

int CurrentPointIndex = -1;

FVector UMeshDeformerComponent::ComputeSMVCCoordinate(const FVector& OriginalCoordinate)
{
    TArray<double> Weights;
    ComputeSMVCWeights(OriginalCoordinate, Weights);
    UE::Math::TVector<double> DeformedCoordinate = FVector::ZeroVector;
    
    for(int i = 0; i < CageVertices.Num(); i++)
    {
        DeformedCoordinate += Weights[i] * CageVertices[i];
    }
    return DeformedCoordinate;
}

void UMeshDeformerComponent::DeformMesh(const TArray<UE::Math::TVector<double>>& InCageVertices, const float InHeight)
{
    DeformedVertices.Empty();
    CageVertices.Empty();
    
    FVector Center = FVector::ZeroVector;
    for(int i = 0; i < 4; i++)
    {
        Center += InCageVertices[i];
    }
    Center /= 4.f;
    
    for(int k = 0; k < 3; k++)
    {
        for(int i = 0; i < InCageVertices.Num(); i++)
        {
            for(int j = 0; j < 2; j++)
            {
                if(k == 0 || k == 2)
                    CageVertices.Add(InCageVertices[i] * (1.f - static_cast<float>(j) / 2.f) + InCageVertices[(i + 1) % InCageVertices.Num()] * (static_cast<float>(j) / 2.f) + FVector(0.f, 0.f, (static_cast<float>(k) / 2.f) * InHeight) - Center);
                else
                {
                    CageVertices.Add(InCageVertices[i] + UE::Math::TVector<double>(0.0, 0.0, 100.0) - Center);
                    break;
                }
            }
        }
    }
    
    TArray<FColor> Colors = {FColor::Red, FColor::Green, FColor::Blue, FColor::Yellow, FColor::Purple, FColor::White};
    
    //FlushPersistentDebugLines(GetWorld());
    for(int i = 0; i < CageFaces.Num(); i++)
    {
        for(int j = 0; j < CageFaces[i].Num(); j++)
        {
            //DrawDebugLine(GetWorld(), CageVertices[CageFaces[i][j]], CageVertices[CageFaces[i][(j + 1) % CageFaces[i].Num()]], Colors[i], false, 100.f, 0, 2.f);
        }
    }
    for(int i = 0; i < OriginalVertices.Num(); i++)
    {
        CurrentPointIndex = i;
        DeformedVertices.Add(ComputeSMVCCoordinate(OriginalVertices[i]));
    }
    ProceduralMeshComp->CreateMeshSection(0, DeformedVertices, Triangles, Normals, UVs, TArray<FColor>(), Tangents, true);
}