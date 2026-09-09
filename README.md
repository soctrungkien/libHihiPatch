# libMinecraftBedrockArchive

A native C++ library for Minecraft Bedrock Edition Android designed to force-close the new OreUI interface and disable disconnecting when minimizing Minecraft.

This repo has been **completely rewritten** to strip out all [libpreloader.so](https://github.com/LiteLDev/preloader-android) dependencies. It is now a fully standalone `.so` library optimized for raw memory injection via the **Snail Method**.

## Key Features

* **Snail Method Compatible:** Injects perfectly as a standalone library without triggering `UnsatisfiedLinkError` crashes.
* **No Levi Environment Required:** Completely removes `libGlossHook.a` and the LiteLDev preloader dependencies.
* **Safe Background Polling:** Utilizes a background thread to safely wait for `libminecraftpe.so` to unpack, preventing race conditions and segmentation faults during early injection.
* **Dobby Hook Integration:** Replaced static macro hooks with standard inline hooking via [Dobby](https://github.com/jmpews/Dobby).
* **Hardcoded JNI Bypass:** Bypasses JavaVM lookups to ensure the config file saves correctly regardless of when the mod is injected.

## Configuration

Once injected, the mod will automatically generate configuration files at the following path:
/storage/emulated/0/Android/data/PKG_NAME/files/mods/MinecraftBedrockArchive/

* **ForceCloseOreUI.json:** Controls OreUI screen toggles and the main module switch.
* **NoDisconnect.json:** Controls the NoDisconnect feature switch.

You can edit these JSON files to toggle specific options on or off.

## Building the Project

This project uses `xmake` and has a fully automated GitHub Actions CI/CD pipeline. 

### Automated Build (Recommended)
You do not need to install the Android NDK locally. 
1. Fork or push your code to GitHub.
2. The GitHub Actions workflow will automatically download the Dobby dependencies, compile the code for `arm64-v8a`, and upload the standalone `libMinecraftBedrockArchive.so` to the Actions tab.

### Manual Local Build
If you prefer to compile locally, ensure you have the Android NDK (r26b recommended) and `xmake` installed.

```bash
# Configure the project for Android (Accept the prompt to install Dobby)
xmake f -p android --ndk=/path/to/your/android-ndk -a arm64-v8a -c --yes

# Compile the project
xmake
```

## Credits

* @Pixelboy79: Standalone Snail Method Conversion, Levi Launcher/Preloader Dependency Removal, and Dobby Hook Integration
* @QYCottage, @yinghuajimew: Original concept and core logic
* @Stivusik: More signatures!
