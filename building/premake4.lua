solution "Blockout"
	configurations {"Release-Dynamic", "Release-Static", "Debug-Dynamic", "Debug-Static"}

	project "Blockout"
		language "C"
		flags {"ExtraWarnings"}
		files {"../sources/**.c"}
		includedirs {"../API"}
		defines {"BLOCKOUT_USE_C_STANDARD_LIBRARY"}
		--buildoptions {"-std=c89 -pedantic -Wall -Weverything"}

		configuration "Release*"
			targetdir "lib/release"

		configuration "Debug*"
			flags {"Symbols"}
			targetdir "lib/debug"

		configuration "*Dynamic"
			kind "SharedLib"

		configuration "*Static"
			kind "StaticLib"
			defines {"BLOCKOUT_STATIC"}
