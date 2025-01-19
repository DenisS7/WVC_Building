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

	MouseHoverDelegate = FTimerDelegate::CreateUObject(this, &APlayerCamera::HoverOverShape);
}

// Called when the game starts or when spawned
void APlayerCamera::BeginPlay()
{
	Super::BeginPlay();
	GetWorld()->GetTimerManager().SetTimer(MouseHoverTimerHandle, MouseHoverDelegate, 0.01f, true, 0.f);
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
	int HitBuildingIndex = -1;
	int HitBuildingElevation = -1;
	int AdjacentBuildingIndex = -1;
	int AdjacentBuildingElevation = -1;
	const bool Hit = UtilityLibrary::GetGridAndBuildingMouseIsHoveringOver(GetWorld(), Grid, HitBuildingIndex, HitBuildingElevation, AdjacentBuildingIndex, AdjacentBuildingElevation);

	if(!Hit || !Grid)
	{
		return;
	}

	int MarchingBit = -1;
	int MarchingBitElevation = -1;

	if(HitBuildingIndex != -1)
	{
		if(AdjacentBuildingIndex != -1)
		{
			MarchingBit = AdjacentBuildingIndex;
			MarchingBitElevation = AdjacentBuildingElevation;
		}
		else
		{
			MarchingBit = HitBuildingIndex;
			MarchingBitElevation = HitBuildingElevation;
		}
	}
		

	const FGridShape& GridShape = Grid->GetBuildingGridShapes()[MarchingBit];
	//Elevation will be changed
	Grid->UpdateMarchingBit(MarchingBitElevation, GridShape.CorrespondingBaseGridPoint, true);//MarchingBits[0][GridShape.CorrespondingGrid1Point] = true;
	
	//WVC Step to determine the pool
}

void APlayerCamera::OnLeftMouseButtonReleased()
{

}

void APlayerCamera::OnRightMouseButtonPressed()
{
	IsDragging = true;
	AGridGenerator* Grid = nullptr;
	int HitBuildingIndex = -1;
	int HitBuildingElevation = -1;
	int AdjacentBuildingIndex = -1;
	int AdjacentBuildingElevation = -1;
	const bool Hit = UtilityLibrary::GetGridAndBuildingMouseIsHoveringOver(GetWorld(), Grid, HitBuildingIndex, HitBuildingElevation, AdjacentBuildingIndex, AdjacentBuildingElevation);

	if(!Hit || !Grid || HitBuildingIndex < 0)
	{
		return;
	}
	
	const FGridShape& GridShape = Grid->GetBuildingGridShapes()[HitBuildingIndex];
	Grid->UpdateMarchingBit(HitBuildingElevation, GridShape.CorrespondingBaseGridPoint, false);
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
	SpringArm->TargetArmLength = FMath::Clamp(AxisValue * -1.f * ZoomFactor + SpringArm->TargetArmLength, 500.f, 5000.f);
}

void APlayerCamera::HoverOverShape()
{
	AGridGenerator* Grid = nullptr;
	int Shape = -1;
	int Elevation = -1;
	int AdjacentShape = -1;
	int AdjacentElevation = -1;
	const bool Hit = UtilityLibrary::GetGridAndBuildingMouseIsHoveringOver(GetWorld(), Grid, Shape, Elevation, AdjacentShape, AdjacentElevation);
	if(!Hit || !Grid || Shape < 0)
	{
		if(HoveredGrid)
			HoveredGrid->ResetShapeMesh();
		return;
	}
	HoveredGrid = Grid;
	if(AdjacentShape < 0)
		Grid->CreateGridShapeMesh(Shape);
	else
	{
		//FlushPersistentDebugLines(GetWorld());
		for(int i = 0; i < Grid->GetBuildingGridShapes()[Shape].ComposingQuads.Num(); i++)
		{
			const TArray<FVector>& Points = Grid->GetBuildingGridShapes()[Shape].ComposingQuads[i].Points;
			for(int j = 0; j < Points.Num(); j++)
			{
				//DrawDebugLine(GetWorld(), Points[j] + FVector(0.f, 0.f, 175.f), Points[(j + 1) % Points.Num()] + FVector(0.f, 0.f, 152.f), FColor::Green, true, 100.f, 0, 2.f);
			}
		}
		TArray<FVector> CommonPoints;
		const TArray<int>& ShapePoints = Grid->GetBuildingGridShapes()[Shape].Points;
		if(AdjacentElevation != Elevation)
		{
			const FVector ElevationDiff = AdjacentElevation < Elevation ? FVector(0.f) : FVector(0.f, 0.f, 200.f);
			for(int i = 0; i < ShapePoints.Num(); i++)
			{
				CommonPoints.Add(Grid->GetBuildingPointCoordinates(ShapePoints[i]) + ElevationDiff);
			}
		}
		else
		{
			const TArray<int>& AdjacentShapePoints = Grid->GetBuildingGridShapes()[AdjacentShape].Points;
			for(int i = 0; i < ShapePoints.Num(); i++)
				for(int j = 0; j < AdjacentShapePoints.Num(); j++)
				{
					if(ShapePoints[i] == AdjacentShapePoints[j])
						CommonPoints.Add(Grid->GetBuildingPointCoordinates(ShapePoints[i]));
				}
			CommonPoints.Insert(CommonPoints[0] * 0.5f + CommonPoints[1] * 0.5f, 1);
			for(int i = CommonPoints.Num() - 1; i >= 0; i--)
				CommonPoints.Add(CommonPoints[i] + FVector(0.f, 0.f, 200.f));
		}
		Grid->CreateAdjacentShapeMesh(CommonPoints);
		//UE_LOG(LogTemp, Warning, TEXT("Current: %d,   Adjacent: %d,    Elevation: %d"), Shape, AdjacentShape, Elevation);
	}
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
