add_rules("mode.debug", "mode.release")

add_requires("nlohmann_json v3.11.3")
add_requires("dobby") 

option("path_type")
    set_default("default")
    set_values("default", "path-mod")

target("MinecraftBedrockArchive")
    set_kind("shared")
    add_files("src/**.cpp")
    add_includedirs("src")
    
    set_languages("c++20")
    set_strip("all")
    
    add_packages("nlohmann_json", "dobby")
    add_syslinks("log")

    if get_config("path_type") == "path-mod" then
        add_defines("USE_PATH_MOD")
    end
