using Mutagen.Bethesda;
using Mutagen.Bethesda.Plugins;
using Mutagen.Bethesda.Plugins.Allocators;
using Mutagen.Bethesda.Skyrim;

namespace ShadowOpportunist.Generator;

internal static partial class Program
{
    private static readonly FormKey SneakActorValue = FormKey.Factory("000455:Skyrim.esm");
    private static readonly FormKey ShadowPerk = FormKey.Factory(
        "000800:ShadowOpportunist_Perk.esp"
    );

    private static void BuildPerk(string output, string allocatorPath)
    {
        var mod = CreateMod("ShadowOpportunist_Perk.esp", ["Skyrim.esm"], includeIntv: false);
        using var allocator = new TextFileFormKeyAllocator(mod, allocatorPath)
        {
            CommitOnDispose = false,
        };
        mod.SetAllocator(allocator);
        var perk = mod.Perks.AddNew("_GZ_ShadowOpportunistPerk");
        ConfigureShadowPerk(perk, "105F24:Skyrim.esm", 80);
        perk.NextPerk.SetTo(FormKey.Null);
        AddActorValueOverride(
            mod,
            VanillaNodes,
            "Sneaking is the art of moving unseen and unheard. Highly skilled sneaks can even hide in plain sight.",
            11.25F,
            0,
            []
        );
        Write(mod, output, "perk", "ShadowOpportunist_Perk.esp");
        allocator.Commit();
    }

    private static void BuildAdamant(string output)
    {
        var mod = CreateMod(
            "ShadowOpportunist_Perk - Adamant.esp",
            [
                "Skyrim.esm",
                "Update.esm",
                "Dawnguard.esm",
                "HearthFires.esm",
                "Dragonborn.esm",
                "ShadowOpportunist_Perk.esp",
                "MysticismMagic.esp",
                "Adamant.esp",
            ]
        );
        AddShadowPerkOverride(mod, "058214:Skyrim.esm", 90);
        AddActorValueOverride(
            mod,
            AdamantNodes,
            "Sneaking is the art of moving unseen and unheard, and striking from the shadows. Those who are skilled in Sneak can even hide in plain sight.",
            11.25F,
            12,
            []
        );
        AddLivingShadowOverride(mod);
        Write(mod, output, "adamant", "ShadowOpportunist_Perk - Adamant.esp");
    }

    private static void BuildOrdinator(string output)
    {
        var mod = CreateMod(
            "ShadowOpportunist_Perk - Ordinator.esp",
            [
                "Skyrim.esm",
                "Update.esm",
                "Dragonborn.esm",
                "ShadowOpportunist_Perk.esp",
                "Ordinator - Perks of Skyrim.esp",
            ]
        );
        AddShadowPerkOverride(mod, "05820C:Skyrim.esm", 80);
        AddActorValueOverride(
            mod,
            OrdinatorNodes,
            "Sneaking is the art of moving unseen and unheard. Highly skilled sneaks can even hide in plain sight.",
            9.5F,
            0,
            [23, 20, 1, 17]
        );
        Write(mod, output, "ordinator", "ShadowOpportunist_Perk - Ordinator.esp");
    }

    private static void BuildVokrii(string output)
    {
        var mod = CreateMod(
            "ShadowOpportunist_Perk - Vokrii.esp",
            [
                "Skyrim.esm",
                "Update.esm",
                "Dawnguard.esm",
                "Dragonborn.esm",
                "ShadowOpportunist_Perk.esp",
                "Vokrii - Minimalistic Perks of Skyrim.esp",
            ]
        );
        AddShadowPerkOverride(mod, "302DB1:Vokrii - Minimalistic Perks of Skyrim.esp", 80)
            .NextPerk.SetTo(FormKey.Null);
        AddActorValueOverride(
            mod,
            VokriiNodes,
            "Sneaking is the art of moving undetected. The Sneak skill makes you harder to be seen or heard while sneaking.",
            9.5F,
            12,
            [1, 23]
        );
        Write(mod, output, "vokrii", "ShadowOpportunist_Perk - Vokrii.esp");
    }

    private static Perk AddShadowPerkOverride(SkyrimMod mod, string prerequisite, float skill)
    {
        var perk = new Perk(ShadowPerk, SkyrimRelease.SkyrimSE);
        ConfigureShadowPerk(perk, prerequisite, skill);
        mod.Perks.Add(perk);
        return perk;
    }

    private static void ConfigureShadowPerk(Perk perk, string prerequisite, float skill)
    {
        perk.EditorID = "_GZ_ShadowOpportunistPerk";
        perk.Name = "Shadow Opportunist";
        perk.Description = Description;
        perk.NumRanks = 2;
        perk.Playable = true;
        perk.Conditions.Add(HasPerk(FormKey.Factory(prerequisite)));
        perk.Conditions.Add(SkillAtLeast(skill));
    }

    private static void AddActorValueOverride(
        SkyrimMod mod,
        IEnumerable<(
            uint Index,
            string Perk,
            uint[] Connections,
            float X,
            float Y,
            uint GridX,
            uint GridY
        )> nodes,
        string description,
        float useMultiplier,
        ushort version2,
        uint[] zeroFnamNodes
    )
    {
        var actorValue = new ActorValueInformation(SneakActorValue, SkyrimRelease.SkyrimSE)
        {
            EditorID = "AVSneak",
            Name = "Sneak",
            Description = description,
            CNAM = new byte[] { 3, 0, 0, 0 },
            Skill = new ActorValueSkill
            {
                ImproveMult = 0.5F,
                ImproveOffset = 120,
                UseMult = useMultiplier,
            },
            Version2 = version2,
        };
        foreach (var spec in nodes)
        {
            var node = new ActorValuePerkNode
            {
                AssociatedSkill = { FormKey = SneakActorValue },
                FNAM = new byte[]
                {
                    spec.Index == 0 || zeroFnamNodes.Contains(spec.Index) ? (byte)0 : (byte)1,
                    0,
                    0,
                    0,
                },
                HorizontalPosition = spec.X,
                Index = spec.Index,
                PerkGridX = spec.GridX,
                PerkGridY = spec.GridY,
                VerticalPosition = spec.Y,
            };
            if (spec.Perk != "Null")
            {
                node.Perk.SetTo(FormKey.Factory(spec.Perk));
            }
            node.ConnectionLineToIndices.AddRange(spec.Connections);
            actorValue.PerkTree.Add(node);
        }
        mod.ActorValueInformation.Add(actorValue);
    }

    private static ConditionFloat HasPerk(FormKey perk) =>
        new()
        {
            CompareOperator = CompareOperator.EqualTo,
            ComparisonValue = 1,
            Data = new HasPerkConditionData { Perk = { Link = { FormKey = perk } } },
        };

    private static ConditionFloat SkillAtLeast(float skill) =>
        new()
        {
            CompareOperator = CompareOperator.GreaterThanOrEqualTo,
            ComparisonValue = skill,
            Data = new GetBaseActorValueConditionData { ActorValue = ActorValue.Sneak },
        };

    private static void AddLivingShadowOverride(SkyrimMod mod)
    {
        var perk = new Perk(FormKey.Factory("058213:Skyrim.esm"), SkyrimRelease.SkyrimSE)
        {
            EditorID = "MAG_LivingShadow",
            Name = "Living Shadow",
            Description =
                "Once every 10 seconds, sneaking in combat causes nearby enemies to stop searching for you.",
            NumRanks = 1,
            Playable = true,
        };
        perk.Conditions.Add(HasPerk(ShadowPerk));
        perk.Conditions.Add(SkillAtLeast(100));
        var entry = new PerkEntryPointSelectSpell
        {
            EntryPoint = APerkEntryPointEffect.EntryType.ApplySneakingSpell,
            PerkConditionTabCount = 1,
            Priority = 1,
            Spell = { FormKey = FormKey.Factory("F08666:Adamant.esp") },
        };
        var condition = new PerkCondition { RunOnTabIndex = 0 };
        condition.Conditions.Add(
            new ConditionFloat
            {
                CompareOperator = CompareOperator.EqualTo,
                ComparisonValue = 0,
                Data = new HasMagicEffectConditionData
                {
                    MagicEffect = { Link = { FormKey = FormKey.Factory("B7E24E:Adamant.esp") } },
                },
            }
        );
        condition.Conditions.Add(
            new ConditionFloat
            {
                CompareOperator = CompareOperator.EqualTo,
                ComparisonValue = 1,
                Data = new IsInCombatConditionData(),
            }
        );
        entry.Conditions.Add(condition);
        perk.Effects.Add(entry);
        mod.Perks.Add(perk);
    }

    private static readonly (uint, string, uint[], float, float, uint, uint)[] VanillaNodes =
    [
        (0, "Null", [3, 2], 0, 0, 0, 0),
        (3, "0BE126:Skyrim.esm", [10, 7], -0.43333334F, -0.04F, 3, 0),
        (10, "058210:Skyrim.esm", [2], -0.56F, -0.12F, 2, 1),
        (2, "1036F0:Skyrim.esm", [9], 0.16666667F, 1.16F, 1, 1),
        (9, "058211:Skyrim.esm", [], -0.13333334F, 0.44F, 2, 2),
        (7, "058213:Skyrim.esm", [6], 0.12F, -0.12F, 4, 1),
        (6, "05820C:Skyrim.esm", [4], -0.2F, 1.16F, 4, 1),
        (4, "105F23:Skyrim.esm", [5], 0.1F, 0.76F, 3, 2),
        (5, "105F24:Skyrim.esm", [8], -0.43333334F, 0.68F, 3, 3),
        (8, "000800:ShadowOpportunist_Perk.esp", [1], 0.06F, 1.32F, 1, 3),
        (1, "058214:Skyrim.esm", [], -0.28F, -0.04F, 0, 4),
    ];

    private static readonly (uint, string, uint[], float, float, uint, uint)[] AdamantNodes =
    [
        (0, "Null", [3, 2], 0, 0, 0, 0),
        (3, "0BE126:Skyrim.esm", [10, 7, 12, 11], 0.485714F, -0.1F, 2, 0),
        (10, "058210:Skyrim.esm", [2], -0.142857F, -0.385714F, 2, 1),
        (2, "1036F0:Skyrim.esm", [9], 0.357143F, 0.285714F, 1, 1),
        (9, "058211:Skyrim.esm", [], 0, 0.157143F, 2, 2),
        (7, "05820C:Skyrim.esm", [6], 0, -0.457143F, 4, 1),
        (6, "98DF4D:Adamant.esp", [5], 0.171429F, -0.328571F, 4, 2),
        (5, "058214:Skyrim.esm", [4], -0.057143F, 0.328571F, 3, 2),
        (8, "058213:Skyrim.esm", [], 0.042857F, -0.357143F, 2, 4),
        (12, "0581FD:Skyrim.esm", [1], 0.285714F, -0.257143F, 3, 1),
        (1, "098666:Adamant.esp", [], 0.357143F, -0.371429F, 3, 2),
        (11, "098664:Adamant.esp", [], -0.442857F, 0.242857F, 3, 1),
        (4, "000800:ShadowOpportunist_Perk.esp", [8], -0.32666665F, 0.04F, 3, 3),
    ];

    private static readonly (uint, string, uint[], float, float, uint, uint)[] OrdinatorNodes =
    [
        (0, "Null", [3], 0, 0, 0, 0),
        (3, "0BE126:Skyrim.esm", [10, 11, 6, 8, 18, 19, 13], -0.52F, -0.06F, 3, 0),
        (10, "058210:Skyrim.esm", [2, 7], -0.053333335F, 0, 1, 1),
        (2, "058211:Skyrim.esm", [9], -0.085714F, -0.1F, 0, 2),
        (9, "0363D5:Ordinator - Perks of Skyrim.esp", [], -0.042857F, -0.357143F, 0, 3),
        (7, "036939:Ordinator - Perks of Skyrim.esp", [21, 23], -0.457143F, -0.442857F, 1, 4),
        (21, "03FD0F:Ordinator - Perks of Skyrim.esp", [], -0.26666668F, 0.36F, 0, 4),
        (23, "0363BD:Ordinator - Perks of Skyrim.esp", [], -0.14F, 1.24F, 0, 4),
        (11, "105F23:Skyrim.esm", [14], 0.24F, 0.2F, 3, 1),
        (14, "035E3D:Ordinator - Perks of Skyrim.esp", [16], -0.08F, 1.12F, 3, 1),
        (16, "035E3F:Ordinator - Perks of Skyrim.esp", [5, 20, 22], 0.428571F, 0.071429F, 3, 3),
        (5, "03FD1F:Ordinator - Perks of Skyrim.esp", [], -0.428571F, -0.314286F, 5, 4),
        (20, "03742C:Ordinator - Perks of Skyrim.esp", [1], -0.457143F, 0.357143F, 3, 4),
        (1, "058214:Skyrim.esm", [], 0.385714F, 1.342857F, 2, 4),
        (22, "037423:Ordinator - Perks of Skyrim.esp", [], -0.026666667F, 0.742857F, 4, 4),
        (6, "0363B4:Ordinator - Perks of Skyrim.esp", [26], -0.25333333F, 0.14F, 4, 0),
        (26, "05820C:Skyrim.esm", [27], 0, 0.485714F, 5, 1),
        (8, "0379AA:Ordinator - Perks of Skyrim.esp", [12], 0, -0.12F, 4, 1),
        (12, "03FD17:Ordinator - Perks of Skyrim.esp", [5], -0.18F, -0.16F, 4, 2),
        (18, "03799E:Ordinator - Perks of Skyrim.esp", [], 0.12666667F, 0.26F, 1, 0),
        (19, "037F0F:Ordinator - Perks of Skyrim.esp", [17], -0.48F, 0, 3, 1),
        (17, "0842DD:Ordinator - Perks of Skyrim.esp", [20, 4, 15, 24], -0.013333334F, 0.34F, 2, 2),
        (4, "037419:Ordinator - Perks of Skyrim.esp", [25], -0.328571F, 0.342857F, 2, 3),
        (25, "058213:Skyrim.esm", [], -0.471429F, 0.542857F, 2, 4),
        (15, "036EA3:Ordinator - Perks of Skyrim.esp", [], -0.35333332F, -0.14F, 3, 3),
        (24, "03381C:Ordinator - Perks of Skyrim.esp", [], 0.11333334F, 0.06F, 1, 3),
        (13, "105F24:Skyrim.esm", [17], -0.26666668F, 0.42F, 2, 1),
        (27, "000800:ShadowOpportunist_Perk.esp", [], -0.16F, 1.64F, 6, 3),
    ];

    private static readonly (uint, string, uint[], float, float, uint, uint)[] VokriiNodes =
    [
        (0, "Null", [3], 0, 0, 0, 0),
        (3, "0BE126:Skyrim.esm", [10, 18, 6, 7], -0.5F, 0, 3, 0),
        (10, "058210:Skyrim.esm", [2, 4], -0.314286F, 0.342857F, 3, 1),
        (2, "058211:Skyrim.esm", [9], 0.185714F, -0.185714F, 2, 3),
        (9, "0363D5:Vokrii - Minimalistic Perks of Skyrim.esp", [12], 0.342857F, 0.014286F, 2, 4),
        (12, "03FD0F:Vokrii - Minimalistic Perks of Skyrim.esp", [1], -0.4F, 0.571429F, 2, 4),
        (1, "058214:Skyrim.esm", [], -0.214286F, 1.757143F, 2, 4),
        (4, "1036F0:Skyrim.esm", [], 0.228571F, 0.485714F, 3, 2),
        (18, "03799E:Vokrii - Minimalistic Perks of Skyrim.esp", [], -0.15333334F, 0.6F, 1, 0),
        (6, "058213:Skyrim.esm", [26, 8], -0.057143F, 0.271429F, 1, 1),
        (26, "05820C:Skyrim.esm", [], -0.214286F, -0.028571F, 0, 3),
        (8, "302DB3:Vokrii - Minimalistic Perks of Skyrim.esp", [23, 13], 0.5F, 0.385714F, 1, 3),
        (23, "302DC4:Vokrii - Minimalistic Perks of Skyrim.esp", [], 0.214286F, 1.485714F, 0, 4),
        (13, "302DBE:Vokrii - Minimalistic Perks of Skyrim.esp", [], 0.385714F, -0.171429F, 0, 4),
        (7, "105F23:Skyrim.esm", [16], 0.1F, -0.071429F, 4, 2),
        (16, "302DB1:Vokrii - Minimalistic Perks of Skyrim.esp", [5], -0.471429F, 0.528571F, 4, 4),
        (5, "000800:ShadowOpportunist_Perk.esp", [1], -0.11333334F, 0.92F, 3, 4),
    ];
}
