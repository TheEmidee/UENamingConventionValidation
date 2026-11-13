# NamingConventionValidation

Standalone Unreal Engine plugin (MIT): an editor plugin that validates asset names against project rules (prefixes and suffixes per asset class, plus custom validators such as per-folder rules) and reports results through Data Validation and the message log. Other projects consume it as a drop-in folder or git submodule under `Plugins/NamingConventionValidation`, so it must make no assumptions about the host project.

## Layout

| Module | Type | LoadingPhase | Contents |
|---|---|---|---|
| `NamingConventionValidation` | Editor | `PreDefault` | flat `Public/` and `Private/` |

## Hard rules

- **Plugin dependencies are fixed.** The `.uplugin` depends on `DataValidation`. Host projects must enable all of them. Adding a plugin dependency, or a module dependency in `*.Build.cs`, is an API decision and should be called out in the PR.
- **No host-project assumptions.** No game module includes, project-specific gameplay tags, asset paths, config sections, or game-specific classes.
- **Editor-only plugin.** `NamingConventionValidation` never ships in a packaged game, so it may use `UnrealEd` and other editor modules. Never add a Runtime module that depends on it, and keep it working from commandlets (no UI assumptions in code that a commandlet runs).
- **Public API is a contract.** Anything in `Public/` or exposed to Blueprint is used by other projects. Renaming or removing a `UFUNCTION`, `UPROPERTY`, `UCLASS` or a Blueprint pin is a breaking change. It needs a major version bump, or a deprecation path (`UE_DEPRECATED`, `DeprecatedFunction` meta, `CoreRedirects`).
- The rules live in the host project's settings. Never ship project-specific prefixes or folders as defaults.
- Documentation is published at theemidee.github.io/UENamingConventionValidation. Update it when settings or behavior change.

## Versioning (CI-enforced)

Every PR must:
1. add a new entry at the top of `CHANGELOG.md` (`## [X.Y.Z] - YYYY-MM-DD`, Keep a Changelog sections: Added / Changed / Fixed / Removed), and
2. set `VersionName` in `NamingConventionValidation.uplugin` to the same version.

Use semver: breaking public or Blueprint API change → major, new feature → minor, fix → patch. If the README shows a version, keep it in sync.

## Code conventions

- Follow Epic's C++ coding standard and naming prefixes (U/A/F/E/I/T/b).
- Type prefix: `NamingConvention` or `NamingValidator` (`UNamingConventionValidationSettings`, `UEditorNamingValidator`). New types live in the matching `Public/` and `Private/` folders.
- Export with `NAMINGCONVENTIONVALIDATION_API`.
- Mark leaf classes `final`. Write overrides as `override` without a redundant `virtual`.
- Use `TObjectPtr<>` for UObject member pointers and `IsValid()` for UObject checks.
- Headers include what they use; don't rely on a PCH.
- Log through the plugin's own category: `LogNamingConventionValidation`.

## Formatting

C++ formatting comes from `.clang-format`, checked by CI with clang-format 20 on changed files. Don't hand-format or argue style that the tool decides.

## Building and testing

There is no standalone build: the plugin compiles inside a host UE project that also has its plugin dependencies. To verify a change, copy or link the plugin into a project's `Plugins/` folder and build its Editor target. You can also package the plugin by itself:

```
RunUAT.bat BuildPlugin -Plugin="<path>/NamingConventionValidation.uplugin" -Package="<out dir>" -TargetPlatforms=Win64
```

CI doesn't compile. Changes should stay buildable on all desktop platforms, with no `PLATFORM_*`-specific code unless it is guarded.
