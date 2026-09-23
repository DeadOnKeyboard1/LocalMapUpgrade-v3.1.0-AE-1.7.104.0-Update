# Replacing the existing GitHub repository contents

The included `Upload-GitHub-Replace.cmd` targets:

`DeadOnKeyboard1/LocalMapUpgrade-v3.1.0-AE-1.7.104.0-Update`

## Usage

1. Extract this GitHub-ready source ZIP to a normal writable folder.
2. Run `Upload-GitHub-Replace.cmd`.
3. If GitHub CLI is not authenticated, complete the browser login when requested.
4. Confirm the replacement when prompted.

For unattended confirmation after authentication:

```bat
Upload-GitHub-Replace.cmd --yes
```

## What the script does

- verifies Git and GitHub CLI,
- authenticates GitHub CLI if needed,
- clones the current `main` branch into a temporary directory,
- creates a timestamped remote backup branch of the old `main`,
- mirrors the current source files into the temporary clone,
- excludes `_toolchain`, build directories, package/dist output, IDE state, logs and compiled binaries,
- commits all additions/updates/deletions, and
- pushes the replacement commit to `main`.

The repository history is preserved. The script does not use a force push.


## Upload safety

`Upload-GitHub-Replace.cmd` strips the trailing backslash from its source path, preserves the cloned `.git` directory, clears only the checked-out working tree, and then copies the prepared source tree. This makes paths containing spaces or parentheses safe and avoids mirroring over `.git`.
