// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "PlayerCamera.generated.h"

class ABuildingPiece;
class AGridGenerator;
class UCameraComponent;
class USpringArmComponent;

UCLASS()
class WVC_BUILDING_API APlayerCamera : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	APlayerCamera();
	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
	UPROPERTY(BlueprintReadWrite)
	AGridGenerator* HoveredGrid = nullptr;

	UPROPERTY(BlueprintReadWrite)
	bool IsDragging = false;
	
	UPROPERTY(BlueprintReadWrite)
	bool IsRotating = false;
	
	UPROPERTY(EditAnywhere)
	float DraggingSpeed = 100.f;
	
	UPROPERTY(EditAnywhere)
	float PanningSpeed = 3.f;
	
	UPROPERTY(EditAnywhere)
	float RotationSpeed = 3.f;
	
	UPROPERTY(EditAnywhere)
	float ZoomFactor = 200.f;
	
	UPROPERTY(EditAnywhere)
	USpringArmComponent* SpringArm;
	
	UPROPERTY(EditAnywhere)
	UCameraComponent* Camera;

	FTimerHandle MouseHoverTimerHandle;
	FTimerDelegate MouseHoverDelegate;

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	void OnLeftMouseButtonPressed();
	void OnLeftMouseButtonReleased();
	void OnRightMouseButtonPressed();
	void OnRightMouseButtonReleased();
	void OnMiddleMouseButtonPressed();
	void OnMiddleMouseButtonReleased();
	void OnMouseWheelAxis(float AxisValue);
	void HoverOverShape();

	void DragCamera();
	void RotatePanCamera();
};
