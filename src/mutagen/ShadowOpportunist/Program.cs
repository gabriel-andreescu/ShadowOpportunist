using System.Drawing;
using Mutagen.Bethesda;
using Mutagen.Bethesda.Plugins;
using Mutagen.Bethesda.Plugins.Allocators;
using Mutagen.Bethesda.Plugins.Assets;
using Mutagen.Bethesda.Plugins.Binary.Parameters;
using Mutagen.Bethesda.Plugins.Records;
using Mutagen.Bethesda.Skyrim;
using Mutagen.Bethesda.Skyrim.Assets;

namespace ShadowOpportunist.Generator;

internal static partial class Program
{
    private const string Description = "Upon being detected in combat, time slows briefly.";

    private static void Main(string[] args)
    {
        var output = Path.GetFullPath(args[0]);
        Directory.CreateDirectory(output);

        BuildMain(output, args[1]);
        BuildPerk(output, args[2]);
        BuildAdamant(output);
        BuildOrdinator(output);
        BuildVokrii(output);
    }

    private static SkyrimMod CreateMod(
        string name,
        IEnumerable<string> masters,
        bool includeIntv = true
    )
    {
        var mod = new SkyrimMod(name, SkyrimRelease.SkyrimSE);
        mod.ModHeader.Author = includeIntv ? "DEFAULT" : string.Empty;
        mod.ModHeader.Flags = SkyrimModHeader.HeaderFlag.Small;
        mod.ModHeader.Stats.Version = 1.70F;
        if (includeIntv)
        {
            mod.ModHeader.INTV = 1;
        }
        foreach (var master in masters)
        {
            mod.ModHeader.MasterReferences.Add(
                new MasterReference { Master = ModKey.FromNameAndExtension(master), FileSize = 0 }
            );
        }
        return mod;
    }

    private static void BuildMain(string output, string allocatorPath)
    {
        var mod = CreateMod("ShadowOpportunist.esp", ["Skyrim.esm", "Update.esm", "Dawnguard.esm"]);
        using var allocator = new TextFileFormKeyAllocator(mod, allocatorPath)
        {
            CommitOnDispose = false,
        };
        mod.SetAllocator(allocator);

        var spell = AddSpell(mod, "_GZ_ShadowOpportunistSpell", "Shadow Opportunist", 3, 0.2F);
        var feedbackSpell = AddSpell(
            mod,
            "_GZ_ShadowOpportunistFeedbackSpell",
            "Shadow Opportunist Feedback",
            3,
            0
        );
        var effect = AddMagicEffect(mod, "_GZ_ShadowOpportunistEffect", "Shadow Opportunist");
        effect.Archetype = new MagicEffectArchetype
        {
            Type = MagicEffectArchetype.TypeEnum.SlowTime,
        };
        effect.Flags =
            MagicEffect.Flag.NoHitEvent | MagicEffect.Flag.NoArea | MagicEffect.Flag.NoRecast;
        effect.MenuDisplayObject.SetTo(FormKey.Factory("0435A5:Skyrim.esm"));

        var feedbackEffect = AddMagicEffect(
            mod,
            "_GZ_ShadowOpportunistFeedbackEffect",
            "Shadow Opportunist Feedback"
        );
        feedbackEffect.Archetype = new MagicEffectArchetype
        {
            Type = MagicEffectArchetype.TypeEnum.Script,
        };
        feedbackEffect.CastingSoundLevel = SoundLevel.Silent;
        feedbackEffect.Flags =
            MagicEffect.Flag.NoHitEvent
            | MagicEffect.Flag.NoMagnitude
            | MagicEffect.Flag.NoArea
            | MagicEffect.Flag.HideInUI
            | MagicEffect.Flag.NoRecast;

        var shader = AddEffectShader(mod);
        var art = mod.ArtObjects.AddNew("_GZ_ShadowOpportunistHitEffect");
        art.Type = ArtObject.TypeEnum.MagicHitEffect;
        art.Model = new Model
        {
            File = new AssetLink<SkyrimModelAssetType>("magic\\_GZ_ShadowOpportunistHitEffect.nif"),
            Data = new byte[] { 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        };

        spell.Effects[0].BaseEffect.SetTo(effect.FormKey);
        feedbackSpell.Effects[0].BaseEffect.SetTo(feedbackEffect.FormKey);
        feedbackEffect.HitShader.SetTo(shader.FormKey);
        feedbackEffect.HitEffectArt.SetTo(art.FormKey);
        feedbackEffect.Sounds =
        [
            new MagicEffectSound
            {
                Type = MagicEffect.SoundType.OnHit,
                Sound = { FormKey = FormKey.Factory("010EC2:Dawnguard.esm") },
            },
        ];
        feedbackEffect.VirtualMachineAdapter = BuildFeedbackScript();

        Write(mod, output, "main", "ShadowOpportunist.esp");
        allocator.Commit();
    }

    private static Spell AddSpell(
        SkyrimMod mod,
        string editorId,
        string name,
        int duration,
        float magnitude
    )
    {
        var spell = mod.Spells.AddNew(editorId);
        spell.Name = name;
        spell.Description = string.Empty;
        spell.Type = SpellType.Spell;
        spell.CastType = CastType.FireAndForget;
        spell.TargetType = TargetType.Self;
        spell.Flags = SpellDataFlag.ManualCostCalc;
        spell.EquipmentType.SetTo(FormKey.Factory("013F44:Skyrim.esm"));
        spell.MenuDisplayObject.SetTo(FormKey.Factory("0A59AE:Skyrim.esm"));
        spell.Effects.Add(
            new Effect
            {
                Data = new EffectData { Duration = duration, Magnitude = magnitude },
            }
        );
        return spell;
    }

    private static MagicEffect AddMagicEffect(SkyrimMod mod, string editorId, string name)
    {
        var effect = mod.MagicEffects.AddNew(editorId);
        effect.Name = name;
        effect.Description = Description;
        effect.CastType = CastType.FireAndForget;
        effect.TargetType = TargetType.Self;
        effect.CastingSoundLevel = SoundLevel.Normal;
        effect.DualCastScale = 1;
        effect.Sounds = [];
        return effect;
    }

    private static EffectShader AddEffectShader(SkyrimMod mod)
    {
        var shader = mod.EffectShaders.AddNew("_GZ_ShadowOpportunistFXS");
        shader.ColorKey1 = Color.FromArgb(0, 130, 10, 30);
        shader.ColorKey2 = Color.FromArgb(0, 90, 10, 20);
        shader.ColorKey2Alpha = 0.9F;
        shader.ColorKey2Time = 0.8F;
        shader.ColorKey3 = Color.FromArgb(0, 40, 5, 10);
        shader.ColorKey3Time = 1;
        shader.AddonModelsFadeInTime = 1;
        shader.AddonModelsFadeOutTime = 1;
        shader.AddonModelsScaleEnd = 1;
        shader.AddonModelsScaleInTime = 1;
        shader.AddonModelsScaleOutTime = 1;
        shader.AddonModelsScaleStart = 1;
        shader.ColorScale = 1;
        shader.EdgeColor = Color.FromArgb(0, 255, 255, 255);
        shader.EdgeEffectAlphaFadeInTime = 0.01F;
        shader.EdgeEffectAlphaFadeOutTime = 1;
        shader.EdgeEffectAlphaPulseFrequency = 1;
        shader.EdgeEffectColor = Color.FromArgb(0, 160, 20, 50);
        shader.EdgeEffectFallOff = 5;
        shader.EdgeEffectFullAlphaRatio = 1;
        shader.EdgeEffectFullAlphaTime = 1;
        shader.EdgeEffectPersistentAlphaRatio = 1;
        shader.FillAlphaFadeInTime = 0.01F;
        shader.FillAlphaPulseFrequency = 1;
        shader.FillColorKey1 = Color.FromArgb(0, 0, 0, 0);
        shader.FillColorKey1Scale = 1;
        shader.FillColorKey2 = Color.FromArgb(0, 130, 10, 30);
        shader.FillColorKey2Scale = 1;
        shader.FillColorKey2Time = 0.75F;
        shader.FillColorKey3 = Color.FromArgb(0, 0, 0, 0);
        shader.FillColorKey3Scale = 1;
        shader.FillColorKey3Time = 1.5F;
        shader.FillFadeOutTime = 1;
        shader.FillFullAlphaRatio = 0.68F;
        shader.FillFullAlphaTime = 1;
        shader.FillPersistentAlphaRatio = 0.4F;
        shader.FillTexture = new AssetLink<SkyrimTextureAssetType>();
        shader.FillTextureAnimationSpeedU = 1;
        shader.FillTextureScaleU = 1;
        shader.FillTextureScaleV = 1;
        shader.HolesEndTime = 10;
        shader.HolesStartValue = 255;
        shader.HolesTexture = new AssetLink<SkyrimTextureAssetType>();
        shader.MembraneBlendOperation = EffectShader.BlendOperation.Add;
        shader.MembraneDestBlendMode = EffectShader.BlendMode.One;
        shader.MembranePaletteTexture = new AssetLink<SkyrimTextureAssetType>();
        shader.MembraneSourceBlendMode = EffectShader.BlendMode.SourceAlpha;
        shader.MembraneZTest = EffectShader.ZTest.EqualTo;
        shader.ParticleAcceleration3 = -40;
        shader.ParticleBirthRampDownTime = 1;
        shader.ParticleBirthRampUpTime = 0.1F;
        shader.ParticleBlendOperation = EffectShader.BlendOperation.Add;
        shader.ParticleDestBlendMode = EffectShader.BlendMode.One;
        shader.ParticleFullBirthRatio = 32;
        shader.ParticleFullBirthTime = 1;
        shader.ParticleInitialRotationDegreePlusMinus = 180;
        shader.ParticleInitialSpeedAlongNormal = 10;
        shader.ParticleInitialSpeedAlongNormalPlusMinus = -20;
        shader.ParticleInitialVelocity3 = -40;
        shader.ParticleLifetime = 0.25F;
        shader.ParticleLifetimePlusMinus = 0.05F;
        shader.ParticlePaletteTexture = new AssetLink<SkyrimTextureAssetType>();
        shader.ParticlePeristentCount = 16;
        shader.ParticleRotationSpeedDegreePerSecPlusMinus = 60;
        shader.ParticleScaleKey1 = 20;
        shader.ParticleScaleKey2Time = 1;
        shader.ParticleShaderTexture = new AssetLink<SkyrimTextureAssetType>(
            "Effects\\MagicCaustic01.dds"
        );
        shader.ParticleSourceBlendMode = EffectShader.BlendMode.SourceAlpha;
        shader.ParticleZTest = EffectShader.ZTest.Normal;
        shader.TextureCountU = 2;
        shader.TextureCountV = 2;
        return shader;
    }

    private static VirtualMachineAdapter BuildFeedbackScript()
    {
        var script = new ScriptEntry
        {
            Name = "shaderparticlegeometryscript",
            Flags = ScriptEntry.Flag.Local,
        };
        script.Properties.Add(
            new ScriptFloatProperty
            {
                Name = "FadeOutTime",
                Flags = ScriptProperty.Flag.Edited,
                Data = 0.1F,
            }
        );
        script.Properties.Add(
            new ScriptObjectProperty
            {
                Name = "PSGD",
                Flags = ScriptProperty.Flag.Edited,
                Object = { FormKey = FormKey.Factory("0486F3:Skyrim.esm") },
                Alias = -1,
            }
        );
        script.Properties.Add(
            new ScriptFloatProperty
            {
                Name = "FadeInTime",
                Flags = ScriptProperty.Flag.Edited,
                Data = 0.1F,
            }
        );
        var adapter = new VirtualMachineAdapter { ObjectFormat = 2, Version = 5 };
        adapter.Scripts.Add(script);
        return adapter;
    }

    private static void Write(SkyrimMod mod, string root, string folder, string name)
    {
        var output = Path.Combine(root, folder);
        Directory.CreateDirectory(output);
        mod.WriteToBinary(
            Path.Combine(output, name),
            new BinaryWriteParameters
            {
                MastersListContent = MastersListContentOption.NoCheck,
                MastersListOrdering = MastersListOrderingOption.NoCheck,
            }
        );
    }
}
