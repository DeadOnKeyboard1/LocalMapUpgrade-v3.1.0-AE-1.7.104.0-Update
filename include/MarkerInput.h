#pragma once
namespace LMU
{
	constexpr bool ShouldPlaceMarker(bool readyBefore, bool readyAfter, bool markerAction,
		bool inBounds, float value, float heldSeconds)
	{
		return readyBefore && readyAfter && markerAction && inBounds && value > 0.0F && heldSeconds == 0.0F;
	}
}
