#pragma once

#include <string_view>

namespace Plugin
{
	using namespace std::literals;

	inline constexpr REL::Version VERSION{
		UPSCALING_VERSION_MAJOR, UPSCALING_VERSION_MINOR, UPSCALING_VERSION_PATCH, 0u
	};
	inline constexpr auto         NAME = "Upscaling"sv;
}
