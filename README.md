# NormalMaker

An application to create normal map textures easily

https://github.com/user-attachments/assets/ccfc4e12-b560-4faf-92a0-086c1e5c3216

## Build

First of all run this file if you don't have the VulkanSDK:

```bash
scripts/SetupVulkan.bat
```

### MSVS

Run this file to create the solution files:

```bash
scripts/Win-GenProjects.bat
```

### CMake

Run this file to create all project cmake files:

```bash
scripts/CMake-GenProjects.bat
```

Then, in the main folder run:

```bash
mkdir build
cd build
cmake ..
```

ATTENTION: Once the project has been built it may be necessary, within visual studio, to manually change the c++ version of the "NormalMaker" project, preferably to 20.

## Usage

### Project

Import or create a project via the "Project" tab in the tab menu.

<p align="center">
  <img src="Resources/Project.png" alt="Project">
</p>

### Image

Then import a base image as layer 0, you will calculate the normals using that layer.

<p align="center">
  <img src="Resources/Image.png" alt="Project">
</p>

### Layers

A layer will be added to the layer list whose position, z-order and alpha can be changed.

<p align="center">
  <img src="Resources/Layers.png" alt="Project">
</p>

With the New button you will add a normal layer on which you can calculate the normal vectors.
With the select button you select that layer to paint "Null Vectors" on it.

<p align="center">
  <img src="Resources/NormalLayer.png" alt="Project">
</p>

### Paint

By selecting "Use Normal Brush" you can paint the "Null Vectors" on the selected layer, otherwise you can paint specific colors by deselecting it.

<p align="center">
  <img src="Resources/Paint.png" alt="Project">
</p>

You can also erase pixels in that layer by selecting "Use Eraser".

<p align="center">
  <img src="Resources/Erase.png" alt="Project">
</p>

### Normal

By clicking on the viewport with the right mouse button and holding it down you can create vectors and orient them where you want.
Each vector can be selected, via the "Normal Arrows" window, and will be highlighted on the screen.
Furthermore, each vector has the possibility, again via the "Normal Arrows" window, to be correct by direction and you can point it in directions towards the z axis by changing the "Angle" value which by default is 0, which is equivalent to parallel to the image.
You can take it up to 90°, which is perpendicular to the image.

<p align="center">
  <img src="Resources/Arrows.png" alt="Project">
</p>

With a layer selected, if you press the "Calculate Normals" button, the corresponding normal vectors for each pixel, with the value "Null Vector", will be calculated and normal arrow.

<p align="center">
  <img src="Resources/Normal.png" alt="Project">
</p>
