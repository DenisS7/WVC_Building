#include "GridDebugSceneProxy.h"

#include "DebugStrings.h"

FGridDebugSceneProxy::FGridDebugSceneProxy(const UPrimitiveComponent* InComponent)
	: FDebugRenderSceneProxy(InComponent)
{
	DrawType = EDrawType::WireMesh;
	DrawAlpha = 1;

	ViewFlagName = TEXT("GridGen");
	ViewFlagIndex = static_cast<uint32>(FEngineShowFlags::FindIndexByName(*ViewFlagName));
}

FPrimitiveViewRelevance FGridDebugSceneProxy::GetViewRelevance(const FSceneView* View) const
{
	FPrimitiveViewRelevance ViewRelevance;
	
	ViewRelevance.bDrawRelevance = ViewFlagIndex != INDEX_NONE && View->Family->EngineShowFlags.GetSingleFlag(ViewFlagIndex);
	ViewRelevance.bSeparateTranslucency = ViewRelevance.bNormalTranslucency = true;
	ViewRelevance.bDynamicRelevance = true;
	ViewRelevance.bShadowRelevance = false;
	ViewRelevance.bEditorPrimitiveRelevance = UseEditorCompositing(View);
	ViewRelevance.bDisableDepthTest = true;
	
	return ViewRelevance;
}

void FGridDebugSceneProxy::GetDynamicMeshElementsForView(const FSceneView* View, const int32 ViewIndex,
	const FSceneViewFamily& ViewFamily, const uint32 VisibilityMap, FMeshElementCollector& Collector,
	FMaterialCache& DefaultMaterialCache, FMaterialCache& SolidMeshMaterialCache) const
{
	FDebugRenderSceneProxy::GetDynamicMeshElementsForView(View, ViewIndex, ViewFamily, VisibilityMap, Collector,
	                                                      DefaultMaterialCache, SolidMeshMaterialCache);
}
