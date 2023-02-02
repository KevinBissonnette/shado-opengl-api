
project "shado-editor"
    location "shado-editor"
    kind "ConsoleApp"
    language "C++"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

    files
    {
        "%{prj.name}/src/**.h",
        "%{prj.name}/src/**.cpp",
        "%{prj.name}/src/**.hpp"
    }

    includedirs
    {
        "%{IncludeDir.GLFW}", -- For some reason I need this and cherno doesn't
        "%{IncludeDir.GLEW}", -- For some reason I need this and cherno doesn't
        "%{IncludeDir.imgui}",
        "%{IncludeDir.glm}",
        "%{IncludeDir.spdlog}",
        "%{IncludeDir.entt}",
        "%{IncludeDir.box2d}",
        --"%{IncludeDir.mono}",	-- TODO: remove this
        "shado-opengl-api/src",
        "shado-opengl-api/vendor"
    }

    links
    {
        "shado-opengl-api",
        "cs-script"
    }

    filter "system:windows"
        cppdialect "C++17"
        staticruntime "Off"
        systemversion "latest"

    defines
    {
        "SHADO_PLATFORM_WINDOWS"
    }

    filter "configurations:Debug"
        defines "SHADO_DEBUG"
        symbols "On"

    filter "configurations:Release"
        defines "SHADO_RELEASE"
        optimize "On"

    filter "configurations:Dist"
        defines "SHADO_DIST"
        optimize "Full"

