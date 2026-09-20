set_xmakever("3.1.1")
set_project("ShadowOpportunist")
set_license("GPL-3.0")
set_policy("package.requires_lock", true)

local version = "2.0.0"

add_repositories("bmk https://github.com/gabriel-andreescu/BethesdaModKit.git")
add_addons("bmk 0.3.0")
includes("@addon/bmk/project")
includes("@addon/bmk/native")

-- Dependencies
add_requires("commonlibsse-ng 8.0.1", { system = false })
add_requires("clib-util 1.5.0", { system = false })
add_requires("bmk", "devbench-api 2026.09.13", { system = false })
add_requires("catch2 3.15.2", { system = false })

-- Build targets
target("Native", function()
    set_default(false)
    set_basename("ShadowOpportunist")
    set_version(version)
    add_rules("@commonlibsse-ng/plugin", {
        author = "GabonZ",
        description = "Slows time when detected while sneaking in combat.",
    })
    add_rules("@addon/bmk/skyrim.plugin")
    add_files("$(projectdir)/src/native/**.cpp")
    add_includedirs("$(projectdir)/src/native")
    set_pcxxheader("src/native/PCH.h")
    add_defines("WIN32_LEAN_AND_MEAN", "NOGDI")
    add_packages("commonlibsse-ng", "clib-util", "bmk")
    add_rules("@devbench-api/integration")
    add_packages("bmk", "devbench-api")
end)

target("SettingsTests", function()
    set_kind("binary")
    set_default(false)
    add_rules("platform.windows.subsystem")
    set_values("windows.subsystem", "console")
    add_rules("@addon/bmk/native.compiler")
    add_files("src/native/Settings.cpp", "tests/SettingsTests.cpp")
    add_includedirs("src/native")
    add_packages("commonlibsse-ng", "clib-util", "bmk")
    add_packages("catch2", { components = { "main", "lib" } })
    add_tests("settings")
end)

target("Mutagen", function()
    set_default(false)
    add_extrafiles("src/mutagen/ShadowOpportunist/FormIDs-main.txt", "src/mutagen/ShadowOpportunist/FormIDs-perk.txt")
    add_rules("@addon/bmk/dotnet", {
        project = "src/mutagen/ShadowOpportunist/ShadowOpportunist.csproj",
        arguments = {
            "$(outputdir)",
            path.absolute("src/mutagen/ShadowOpportunist/FormIDs-main.txt"),
            path.absolute("src/mutagen/ShadowOpportunist/FormIDs-perk.txt"),
        },
        outputs = {
            "main/ShadowOpportunist.esp",
            "perk/ShadowOpportunist_Perk.esp",
            "adamant/ShadowOpportunist_Perk - Adamant.esp",
            "ordinator/ShadowOpportunist_Perk - Ordinator.esp",
            "vokrii/ShadowOpportunist_Perk - Vokrii.esp",
        },
    })
end)

-- Packages
target("ShadowOpportunist", function()
    set_version(version)
    add_deps("Mutagen", { inherit = false })
    add_rules("@addon/bmk/skyrim.package", {
        targets = { "Native" },
        nexus = {
            mod_id = "7318624424986",
            file_id = "3238321",
            category = "main",
            primary = true,
            display_name = "Shadow Opportunist",
            description = "Updating from an older version? Follow the Updating to 2.0.0 instructions in the mod description before installing.",
        },
    })
    add_installfiles("$(projectdir)/assets/(**)|optional/**")
    add_installfiles("$(builddir)/artifacts/Mutagen/main/(ShadowOpportunist.esp)")
end)

target("ShadowOpportunistPerk", function()
    set_version("1.1.0")
    add_deps("Mutagen", { inherit = false })
    add_rules("@addon/bmk/skyrim.package", {
        package_name = "Shadow Opportunist - New Perk Addon",
        nexus = {
            mod_id = "7318624424986",
            file_id = "3238324",
            category = "optional",
            display_name = "Shadow Opportunist - New Perk Addon",
            description = "Requires Silence and 80 Sneak.",
        },
    })
    add_installfiles("$(builddir)/artifacts/Mutagen/perk/(ShadowOpportunist_Perk.esp)")
end)

target("ShadowOpportunistAdamantPatch", function()
    set_version("1.0.0")
    add_deps("Mutagen", { inherit = false })
    add_rules("@addon/bmk/skyrim.package", {
        package_name = "New Perk Addon - Adamant Patch",
        nexus = {
            mod_id = "7318624424986",
            file_id = "6144430",
            category = "miscellaneous",
            display_name = "New Perk Addon - Adamant Patch",
            description = "Requires New Perk Addon and Adamant.",
        },
    })
    add_installfiles("$(builddir)/artifacts/Mutagen/adamant/(ShadowOpportunist_Perk - Adamant.esp)")
end)

target("ShadowOpportunistOrdinatorPatch", function()
    set_version("1.0.0")
    add_deps("Mutagen", { inherit = false })
    add_rules("@addon/bmk/skyrim.package", {
        package_name = "New Perk Addon - Ordinator Patch",
        nexus = {
            mod_id = "7318624424986",
            file_id = "6144427",
            category = "miscellaneous",
            display_name = "New Perk Addon - Ordinator Patch",
            description = "Requires New Perk Addon and Ordinator.",
        },
    })
    add_installfiles("$(builddir)/artifacts/Mutagen/ordinator/(ShadowOpportunist_Perk - Ordinator.esp)")
end)

target("ShadowOpportunistVokriiPatch", function()
    set_version("1.0.1")
    add_deps("Mutagen", { inherit = false })
    add_rules("@addon/bmk/skyrim.package", {
        package_name = "New Perk Addon - Vokrii Patch",
        nexus = {
            mod_id = "7318624424986",
            file_id = "3238327",
            category = "miscellaneous",
            display_name = "New Perk Addon - Vokrii Patch",
            description = "Requires New Perk Addon and Vokrii.",
        },
    })
    add_installfiles("$(builddir)/artifacts/Mutagen/vokrii/(ShadowOpportunist_Perk - Vokrii.esp)")
end)
