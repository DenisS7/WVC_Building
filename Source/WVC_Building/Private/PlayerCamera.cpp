// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerCamera.h"

#include "BuildingPiece.h"
#include "EngineUtils.h"
#include "GridGenerator.h"
#include "MeshCornersData.h"
#include "UtilityLibrary.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Generators/MarchingCubes.h"

// Sets default values
APlayerCamera::APlayerCamera()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	SpringArm = CreateDefaultSubobject<USpringArmComponent>("Spring Arm");
	Camera = CreateDefaultSubobject<UCameraComponent>("Camera");
	Camera->SetupAttachment(SpringArm);
	SpringArm->TargetArmLength = 2000.f;
	SpringArm->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));

	SquareHoverDelegate = FTimerDelegate::CreateUObject(this, &APlayerCamera::HoverOverShape);
	//DataTable = LoadObject<UDataTable>(nullptr, TEXT("/Game/DATA/Blueprints/DT_MeshCorners.DT_MeshCorners"));
	//FSoftObjectPath TablePath = FSoftObjectPath(TEXT("Game/DATA/Blueprints/DT_MeshCorners.DT_MeshCorners"));
	//FString TablePath = "Game/Blueprints/DT_MeshCorners";
	//TArray<UObject*> DataTables;
	//EngineUtils::FindOrLoadAssetsByPath(TablePath, DataTables, EngineUtils::ATL_Regular);
	//DataTable = Cast<UDataTable>(TablePath.TryLoad());
	//if(!DataTable)
	//{
	//	bool ok = true;
	//}
	//GroundTiles.Add(1000000, )
}

// Called when the game starts or when spawned
void APlayerCamera::BeginPlay()
{
	Super::BeginPlay();
	GetWorld()->GetTimerManager().SetTimer(SquareHoverTimerHandle, SquareHoverDelegate, 0.01f, true, 0.f);
}

// Called every frame
void APlayerCamera::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	DragCamera();
	RotatePanCamera();
}

// Called to bind functionality to input
void APlayerCamera::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
    
	PlayerInputComponent->BindAction("LeftMouseButton", IE_Pressed, this, &APlayerCamera::OnLeftMouseButtonPressed);
	PlayerInputComponent->BindAction("LeftMouseButton", IE_Released, this, &APlayerCamera::OnLeftMouseButtonReleased);
	
	PlayerInputComponent->BindAction("RightMouseButton", IE_Pressed, this, &APlayerCamera::OnRightMouseButtonPressed);
	PlayerInputComponent->BindAction("RightMouseButton", IE_Released, this, &APlayerCamera::OnRightMouseButtonReleased);

	PlayerInputComponent->BindAction("MiddleMouseButton", IE_Pressed, this, &APlayerCamera::OnMiddleMouseButtonPressed);
	PlayerInputComponent->BindAction("MiddleMouseButton", IE_Released, this, &APlayerCamera::OnMiddleMouseButtonReleased);

	PlayerInputComponent->BindAxis("MouseWheelAxis", this, &APlayerCamera::OnMouseWheelAxis);
}

void APlayerCamera::OnLeftMouseButtonPressed()
{
	AGridGenerator* Grid = nullptr;
	int Shape = -1;
	UtilityLibrary::GetGridAndShapeMouseIsHoveringOver(GetWorld(), Grid, Shape);
	if(!Grid || Shape < 0)
	{
		return;
	}
	
	//if(Grid->GetSecondGrid()[Shape].Points.Num() != 4)
	//	return;
	
	const FGridShape& GridShape = Grid->GetSecondGrid()[Shape];
	const TArray<int>& ShapePoints = GridShape.Points;
	Grid->MarchingBits[0][GridShape.CorrespondingGrid1Point] = true;
	for(int i = 0; i < ShapePoints.Num(); i++)
	{
		const FGridQuad& CorrespondingQuad = Grid->GetFinalQuads()[ShapePoints[i]];
		bool Has0 = Grid->MarchingBits[0][CorrespondingQuad.Points[0]];
		int MinCorner = -1;
		int MaxMinCorner = -1;
		TArray<FVector> CageBase;
		for(int k = 0; k < 4; k++)
			CageBase.Add(Grid->GetPointCoordinates(CorrespondingQuad.Points[k]));

		//if()
		auto Find = Grid->BuildingPieces.Find(TPair<int, int>(0, ShapePoints[i]));
		ABuildingPiece* BuildingPiece;
		if(!Find)
		{
			BuildingPiece = GetWorld()->SpawnActor<ABuildingPiece>(BuildingPieceToSpawn, CorrespondingQuad.Center, FRotator(0, 0, 0));
			Grid->BuildingPieces.Add(TPair<int, int>(0, ShapePoints[i]), BuildingPiece);
		}
		else
		{
			BuildingPiece = *Find;
			BuildingPiece->Corners.Empty();
			//BuildingPiece->MarchingCorners
		}
		//Grid->FirstElevation[ShapePoints[i]].ConfigurationCorners[i] = true;
		//Grid->VoxelConfig[0][ShapePoints[i]].ConfigurationCorners[i] = true;
		int TileConfig = 0;
		//int Corners = 0;
		//float Direction = 0.f;
		TArray<int> LowerCorners;
		TArray<int> UpperCorners;
		TArray<bool> CornersMask;
		for(int j = 0; j < 8; j++)
		{
			//TileConfig += Grid->FirstElevation[ShapePoints[i]].ConfigurationCorners[j] * pow(10, 8 - j);
			TileConfig += Grid->MarchingBits[j / 4][CorrespondingQuad.Points[j % 4]] * pow(10, 8 - j - 1);
			CornersMask.Add(Grid->MarchingBits[j / 4][CorrespondingQuad.Points[j % 4]]);
			//DELETE WHEN UPPER PART IS USED
			if(Grid->MarchingBits[j / 4][CorrespondingQuad.Points[j % 4]])
			{
				if(j < 4)
					LowerCorners.Add(j);
				else
					UpperCorners.Add(j);
				//Corners.Add((j / 4) * 4 + j % 4);
				if(MinCorner == -1)
					MinCorner = j;
				if(j < 4)
					MaxMinCorner = j;
			}
			
			float Height = 0.f;
			if(j >= 4)
				Height = 200.f;
			//DrawDebugBox(GetWorld(), Grid->GetPointCoordinates(CorrespondingQuad.Points[FMath::Min(j, 3)]) + FVector(0.f, 0.f, Height), FVector(10.f), Grid->MarchingBits[j / 4][CorrespondingQuad.Points[j % 4]] ? FColor::Green : FColor::Red, false, 100.f);
		}
		
		//if(Has0)
		//{
		//	while(Has0)
		//	{
		//		for(int j = 0; j < Corners.Num(); j++)
		//		{
		//			Corners[j]++;
		//			Corners[j];
		//		}
		//	}
		//}

		float Rotation = 0.f;
		
		if(LowerCorners.Num())
		{
			if(LowerCorners.Num() >= 2)
			{
				for(int j = 0; j < LowerCorners.Num() - 1; j++)
				{
					if(LowerCorners[j] != LowerCorners[j + 1] - 1)
					{
						for(int p = 0; p < LowerCorners.Num() - j - 1; p++)
						{
							const int LastElement = LowerCorners.Last();
							for(int k = LowerCorners.Num() - 1; k >= 1; k--)
							{
								LowerCorners[k] = LowerCorners[k - 1];
							}
							LowerCorners[0] = LastElement;
						}
						break;
					}
				}
			}

			if(LowerCorners[0] != 0)
			{
				int RotationAmount = 4 - LowerCorners[0];
				Rotation = static_cast<float>(RotationAmount) * 90.f;
				for(int j = 0; j < LowerCorners.Num(); j++)
				{
					LowerCorners[j] = (LowerCorners[j] + RotationAmount) % 4;
				}
			}
			//if(MinCorner == 0 && CornersMask[3])
			//{
			//	while(MaxMinCorner != 0)
			//	{
			//		for(int j = 0; j < 8; j++)
			//		{
			//			Corners[j]++;
			//			if(j < 4)
			//				Corners[j] %= 4;
			//			else
			//			{
			//				Corners[j] %= 4;
			//				Corners[j] += 4;
			//			}
			//		}
			//		//CageBase.Swap(0,3);
			//		//CageBase.Swap(1,3);
			//		//CageBase.Swap(2,3);
			//		MaxMinCorner++;
			//		MaxMinCorner %= 4;
			//		Rotation += 90.f;
			//	}
			//}
			//else
			//{
			//	while(MinCorner != 0)
			//	{
			//		for(int j = 0; j < 8; j++)
			//		{
			//			Corners[j]--;
			//			if(j >= 4 && Corners[j] < 4)
			//				Corners[j] += 4;
			//		}
			//		//CageBase.Swap(0,3);
			//		//CageBase.Swap(0,2);
			//		//CageBase.Swap(0,1);
			//		MinCorner--;
			//		Rotation -= 90.f;
			//	}
			//	for(int j = 0; j < 4; j++)
			//	{
			//		Corners[j] = FMath::Abs(Corners[j]);
			//	}
			//}
		}
		
		FString RowName;
		//TArray<int> UsedCorners;
		//for(int j = 0; j < 8; j++)
		//	if(CornersMask[j])
		//		UsedCorners.Add(Corners[j]), BuildingPiece->Corners.Add(j);
		//UsedCorners.Sort();
		BuildingPiece->Corners = LowerCorners;
		BuildingPiece->Corners.Append(UpperCorners);
		for(int j = 0; j < LowerCorners.Num(); j++)
			RowName.Append(FString::FromInt(LowerCorners[j]));
		for(int j = 0; j < UpperCorners.Num(); j++)
			RowName.Append(FString::FromInt(UpperCorners[j]));
		
		FName RowNameName(RowName);
		//RowNameName = "01";
		auto Row = BuildingPiece->DataTable->FindRow<FMeshCornersData>(RowNameName, "MeshCornersRow");
		BuildingPiece->SetStaticMesh(Row->Mesh);
		//UStaticMesh* Mesh2;
		//BuildingPiece->AddActorWorldRotation(FRotator(0.f, Rotation, 0.f));
		BuildingPiece->DeformMesh(CageBase, 200.f, Rotation);
	}
	
	//WVC Step to determine the pool
	
	
	//TArray<FVector> CageBase;
	//for(int i = 0; i < 4; i++)
	//	CageBase.Add(Grid->GetSecondPointCoordinates(Grid->GetSecondGrid()[Shape].Points[i]));
	//ABuildingPiece* NewBuildingPiece = GetWorld()->SpawnActor<ABuildingPiece>(BuildingPieceToSpawn, Grid->GetSecondGrid()[Shape].Center, FRotator(0, 0, 0));
	//NewBuildingPiece->DeformMesh(CageBase, 200.f);
}

void APlayerCamera::OnLeftMouseButtonReleased()
{
	
}

void APlayerCamera::OnRightMouseButtonPressed()
{
	IsDragging = true;
}

void APlayerCamera::OnRightMouseButtonReleased()
{
	IsDragging = false;
}

void APlayerCamera::OnMiddleMouseButtonPressed()
{
	IsRotating = true;
}

void APlayerCamera::OnMiddleMouseButtonReleased()
{
	IsRotating = false;
}

void APlayerCamera::OnMouseWheelAxis(float AxisValue)
{
	SpringArm->TargetArmLength = FMath::Clamp(AxisValue * -1.f * ZoomFactor + SpringArm->TargetArmLength, 1500.f, 5000.f);
}

void APlayerCamera::HoverOverShape()
{
	AGridGenerator* Grid = nullptr;
	int Shape = -1;
	UtilityLibrary::GetGridAndShapeMouseIsHoveringOver(GetWorld(), Grid, Shape);
	if(!Grid || Shape < 0)
	{
		if(HoveredGrid)
			HoveredGrid->ResetShapeMesh();
		return;
	}
	HoveredGrid = Grid;
	Grid->CreateShapeMesh(Shape);
}

void APlayerCamera::DragCamera()
{
	if(!IsDragging)
		return;
	float MouseX, MouseY;
	GetWorld()->GetFirstPlayerController()->GetInputMouseDelta(MouseX, MouseY);
	FVector Forward = GetActorForwardVector();
	Forward.Z = 0.f;
	Forward = Forward.GetSafeNormal();
	FVector Right = GetActorRightVector();
	Right.Z = 0.f;
	Right = Right.GetSafeNormal();
	AddActorWorldOffset(Forward * MouseY * DraggingSpeed * -1.f + Right * MouseX * DraggingSpeed * -1.f);
}

void APlayerCamera::RotatePanCamera()
{
	if(!IsRotating)
		return;
	float MouseX, MouseY;
	GetWorld()->GetFirstPlayerController()->GetInputMouseDelta(MouseX, MouseY);
	float Pitch = -60.f;
	Pitch = FMath::Clamp(SpringArm->GetComponentRotation().Pitch + (MouseY * PanningSpeed), -88.99f, -30.f);

	AddActorWorldRotation(FRotator(0.f, MouseX * RotationSpeed, 0.f));
	SpringArm->SetWorldRotation(FRotator(Pitch, SpringArm->GetComponentRotation().Yaw, 0.f));
}
