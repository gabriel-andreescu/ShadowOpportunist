# Changelog

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

## [2.0.0] - 2026-09-20

### Added

- Support Skyrim 1.7.104

### Changed

- **Breaking change:** Move user settings to
  `MCM/Settings/ShadowOpportunist.ini`
- **Breaking change:** Renumber the main plugin's records for older SE and VR
  runtimes
- Require the Shadow Opportunist perk automatically when the New Perk Addon is
  enabled
- **Breaking change:** Change required-perk settings to `0xFORMID~Plugin.esp`

### Fixed

- Remove the obsolete INI from New Perk Addon 1.1.1
- Keep the compiled defaults when settings files cannot be read instead of
  disabling activation
- Resolve the slow-time hook on Skyrim VR
- Discard pending detection work when loading a save
- Validate duration and required-perk settings
