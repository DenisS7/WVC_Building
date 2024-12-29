#pragma once

enum class EHitPosition : uint8;
class ABuildingPiece;
class AGridGenerator;

namespace UtilityLibrary
{
	WVC_BUILDING_API void GetGridAndShapeMouseIsHoveringOver(const UWorld* World, AGridGenerator*& Grid, int& ShapeIndex);
	WVC_BUILDING_API bool GetGridAndBuildingMouseIsHoveringOver(const UWorld* World, AGridGenerator*& Grid, int& HitBuildingIndex, int& AdjacentHitBuildingIndex);
};
