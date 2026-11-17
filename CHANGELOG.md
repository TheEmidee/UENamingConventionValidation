# Changelog

All notable changes to this package will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/en/1.0.0/)
and this project adheres to [Semantic Versioning](http://semver.org/spec/v2.0.0.html).

## [1.4.2] - 2026-11-17

### Fixed

- When several class descriptions match an asset, only the rules of the most derived class are applied (e.g. a `UDataAsset` subclass with its own rule no longer gets validated against the `UDataAsset` rule), and errors from less derived classes are no longer reported
- When a class description is found and valid for an asset, the blueprint prefix is not tested against that asset name
- Improvements on the widget used to add/edit a class description
- Removed from the default class descriptions classes which are in plugins that are disabled by default in a new project

## [1.4.1] - 2026-11-13

### Changed

- CI now uses the shared workflows from `FishingCactus/ci-workflows-unreal` (clang-format, TODO to issue, version update) and runs the Claude PR review
- Added CLAUDE.md with the plugin's conventions for AI-assisted development

### Removed

- Vale documentation linting (`.vale.ini` and its workflow)

## [1.4.0] - 2026-11-13

### Changed

- Updated .clang-format and formatted the C++ sources
- Renamed locals and parameters to follow the naming convention (PascalCase, `b` prefix for bools)

## [1.3.1] - 2026-09-02

### Changed

- Added missing dependency to DataValidtion in uplugin

## [1.3.0] - 2026-04-20

### Changed

- Custom detail panel for the class descriptions in the project settings

## [1.2.0] - 2026-04-15

### Changed

- The naming convention validation is now checked through `UEditorValidatorBase`

## [1.0.0] - 2025-09-12

### Added

- First version of the package.