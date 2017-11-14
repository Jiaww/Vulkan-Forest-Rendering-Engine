# Vulkan - Real-Time Forest Rendering Engine 
Jiawei Wang & Yi Guo
 				
## OverView
We are going to implement a real-time Forest Rendering Engine with Vulkan. Since we notice that many games and simulation systems involves the scenes with massive plants coverage, we want to try to implement such an engine to satisfy this need. Nowadays, most forest scenes are rendered using Direct3D and OpenGL. Compared with these APIs, Vulkan can work on different platforms and achieve good GPU performance. Therefore, we believe a Vulkan Forest rendering engine can be valuable.
 
The tree models are produced by speedTree. To accelerate the rendering process, we will use Multiple LOD technique and culling methods. These are implemented in the compute shader. Many other features, like shadow mapping and density multiplication are also included.

___
## Goals and Highlights
* High-performance Forest Rendering
* Wind zone system on Trees using vertex shader animation(Gem3)
* Multiple Level of Detail
* Chunks and Frustum Culling
* Density Multiplication(Fake trees)
* Smooth Transition between level morphing
* Shadow mapping

## Schedule
* Milestone 1:  Basic Vulkan rendering pipeline, produce terrain procedurally, render basic tree models.
* Milestone 2: Multiple Level of Details, Density Multiplication,Chunks and Frustum Culling Transition between level morphing.
* Milestone 3: Wind zone system and vertex shader animation
* Final: add shadow mapping and GUIs.

## Reference
* GPU Gem3 Chapter 16. Vegetation Procedural Animation and Shading in Crysis https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch16.html
* Fast Extraction of Viewing Frustum Planes from the WorldView-Projection Matrix http://www.cs.otago.ac.nz/postgrads/alexis/planeExtraction.pdf
* GPU Gem3 Chapter 8. Summed-Area Variance Shadow Maps https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch08.html
* NVIDIA GameWorks Vulkan and OpenGL Samples https://developer.nvidia.com/gameworks-vulkan-and-opengl-samples 
