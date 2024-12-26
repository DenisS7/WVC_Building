#pragma once

class ABuildingPiece;
class AGridGenerator;

namespace UtilityLibrary
{
	WVC_BUILDING_API void GetGridAndShapeMouseIsHoveringOver(const UWorld* World, AGridGenerator*& Grid, int& ShapeIndex);
};
