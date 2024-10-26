// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerCamera.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"

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
}

// Called when the game starts or when spawned
void APlayerCamera::BeginPlay()
{
	Super::BeginPlay();
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
	
	PlayerInputComponent->BindAction("RightMouseButton", IE_Pressed, this, &APlayerCamera::OnRightMouseButtonPressed);
	PlayerInputComponent->BindAction("RightMouseButton", IE_Released, this, &APlayerCamera::OnRightMouseButtonReleased);

	PlayerInputComponent->BindAction("MiddleMouseButton", IE_Pressed, this, &APlayerCamera::OnMiddleMouseButtonPressed);
	PlayerInputComponent->BindAction("MiddleMouseButton", IE_Released, this, &APlayerCamera::OnMiddleMouseButtonReleased);

	PlayerInputComponent->BindAxis("MouseWheelAxis", this, &APlayerCamera::OnMouseWheelAxis);
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
