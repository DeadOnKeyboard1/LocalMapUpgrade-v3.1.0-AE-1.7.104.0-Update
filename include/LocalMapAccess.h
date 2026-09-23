#pragma once

// CommonLibSSE-NG exposes the four LocalMapMenu viewport floats but does not
// currently give the two pairs semantic names. Keep the tiny runtime-aware
// accessor here instead of maintaining a private copy of the complete engine
// class layout.
namespace LMU
{
	[[nodiscard]] inline RE::GPointF& GetLocalMapTopLeft(RE::LocalMapMenu& a_menu) noexcept
	{
		return REL::RelocateMember<RE::GPointF>(&a_menu, 0x30, 0x30);
	}

	[[nodiscard]] inline const RE::GPointF& GetLocalMapTopLeft(const RE::LocalMapMenu& a_menu) noexcept
	{
		return REL::RelocateMember<RE::GPointF>(&a_menu, 0x30, 0x30);
	}

	[[nodiscard]] inline RE::GPointF& GetLocalMapBottomRight(RE::LocalMapMenu& a_menu) noexcept
	{
		return REL::RelocateMember<RE::GPointF>(&a_menu, 0x38, 0x38);
	}

	[[nodiscard]] inline const RE::GPointF& GetLocalMapBottomRight(const RE::LocalMapMenu& a_menu) noexcept
	{
		return REL::RelocateMember<RE::GPointF>(&a_menu, 0x38, 0x38);
	}
}
