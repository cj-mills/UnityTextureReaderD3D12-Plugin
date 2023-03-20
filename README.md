# UnityTextureReaderD3D12-Plugin
 This repository contains the source code for the UnityTextureReaderD3D12 plugin, a native plugin for Unity that enables efficient pixel data reading from Direct3D12 textures.



## Overview

The UnityTextureReaderD3D12 plugin allows you to efficiently read pixel data from Direct3D11 textures in Unity. This can be useful when you need to access texture data for various purposes, such as image processing or exporting textures.

The plugin is designed to work with the [UnityTextureReaderD3D](https://github.com/cj-mills/UnityTextureReaderD3D) package, which provides a higher-level C# interface for Unity projects.



## Prerequisites

- Visual Studio 2019 or newer.
- A Unity installation with DirectX 12 support.



## Building the plugin

1. Clone this repository to your local machine.
2. Open the `UnityTextureReaderD3D12.sln` solution file in Visual Studio.
3. In the Solution Explorer, right-click on the project and go to **Properties**.
4. In the **C/C++** tab, add the path to the `Editor\Data\PluginAPI` subfolder for your Unity Editor installation to the **Additional Include Directories**. The path should look like `C:\Program Files\Unity\Editor\Data\PluginAPI`.
5. Save the changes and close the Properties window.
6. Set the build configuration to **Release** and platform to **x64**.
7. Build the solution by pressing **Ctrl+Shift+B** or going to **Build > Build Solution**.

After building the solution, you will find the `UnityTextureReaderD3D12.dll` file in the `x64\Release` folder.



## Usage

To use the UnityTextureReaderD3D12 plugin in your Unity project, copy the `UnityTextureReaderD3D12.dll` file to your Unity project's `Assets/Plugins/x86_64` folder or follow the instructions in the [UnityTextureReaderD3D](https://github.com/cj-mills/UnityTextureReaderD3D) package repository.



## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.
