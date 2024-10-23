#pragma once

#include "CoreMinimal.h"
#include "ComponentVisualizer.h"
#include "Debug/DebugDrawComponent.h"

#include "GridGeneratorVis.generated.h"

UCLASS()
class WVC_BUILDING_API UGridGeneratorVis : public UDebugDrawComponent
{
	GENERATED_BODY()
public:
	UGridGeneratorVis();
	virtual FDebugRenderSceneProxy* CreateDebugSceneProxy() override;
	virtual FBoxSphereBounds CalcBounds(const FTransform& InTransform) const override;
	virtual bool ShouldRecreateProxyOnUpdateTransform() const override { return true; }
};
