#pragma once

#include "RE/N/NiObject.h"

namespace RE
{
	class NiAlphaAccumulator : public NiObject
	{
	public:
		inline static constexpr auto RTTI = RTTI_NiAlphaAccumulator;
		inline static constexpr auto Ni_RTTI = NiRTTI_NiAlphaAccumulator;
		inline static constexpr auto VTABLE = VTABLE_NiAlphaAccumulator;

		~NiAlphaAccumulator() override = default;

		std::uint8_t pad10[0x58 - sizeof(NiObject)];
	};
	static_assert(sizeof(NiAlphaAccumulator) == 0x58);
}
