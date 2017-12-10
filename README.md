# Vulkan-Forest-Rendering-Engine
* Jiawei Wang, Yi Guo
* University of Pennsylvania, CGGT
* 2017.11.25 - 2017.12.11
___
## Overview
* The project is a high-performance **Real-time** forest rendering engine developed using Vulkan. We optimized the performance both for CPU part and GPU part. 
* It contains **multiple LOD strategy** rendering(realized by compute shader in Vulkan), **nature Wind-zone** of forest(realized by vertex animation for each tree), **Density Multiplication Effect**('fake trees'), **Day&Night Cycle**, etc. 
* The users can also use the GUI to modify the parameters to get the disired result effect they want in the demo.

|**Final Rendering**|
|---|
|<img src="./results/final.jpg" width="1200" height="450">|

___
## Features
### WindZone
* We generate different Wind Force for different trees in the forest according to their horizontal positions, which can simulate a realistic wind effect for a large scale zone.
* Besides, for each tree, we simulate their waving animation using the algorithm presented in [Gem3 Chapter 16]
* Example Gif

### Multiple LOD Strategy
* In computer graphics, accounting for Level of detail involves decreasing the complexity of a 3D model representation as it moves away from the viewer or according to other metrics such as object importance, viewpoint-relative speed or position. Level of detail techniques increase the efficiency of rendering by decreasing the workload on graphics pipeline stages, usually vertex transformations. The reduced visual quality of the model is often unnoticed because of the small effect on object appearance when distant or moving fast. [Wiki]
* Here we are using only 2 levels of detail just for example:
    * LOD 0: Full Model
    * LOD 1: Billboard
  When the object is close, we will render its full model, otherwise, billboard.
* The visual effect can be realized by using `discard` in fragment shader according to the object position and viewer position.
* Example Gifs

### LOD & Frustum Culling using Compute Shader
* As we presented before, we can realize the visual effect using only fragment shader, but for those models/billboards which will not show on the screen, we still need to run their vertex shader.
* So, what we disired is to only render what could be on the screen, and we can realize it by culling them before we start render.
* Example Gifs

### Morphing
* We know that we will choose what to render according to the distance, but in the border of different LOD, even though they are similar, you can still easily find out that there is a strange shift like following example.
* To fix that, we implement a smooth morphing process to make this shifting more natrual.(Realized using a noise map and an overlapping area for different LODs)
* Example Gifs
* There are many methods to do the Frustum Culling, for example, using 6 planes of the camera and do the "Whether intersect" test for the objects' bounding boxes. Here I use another method, instead of computing the planes info on GPU each frame, I choose to compute 6 NDC coordinates for each object in the shader, because of the symmetric structure for trees(6 points: Up, Bottom, Left, Right, Front, Back), and then check if they are inside of NDC space.

### Density Multiplication
* In real game, we always want players to have a feeling like "Wow, that moutain is full of trees", but we also want that when players are actually in the forest, they will have enough space to walk and interact, whic means "Several Trees closely, Lots of Trees distantly".
* This effect can be generated using "Fake Trees", which are also billboards but not the billboards for the real model, they will disappear when the players are close enough, and appear again when players are distant enough. Also, because they are far from the players, who are focus on the nearby things, most of them couldn't even be realized. 
* For more specific algorithm to generate them, you can look into the code in 'Scene.cpp'
* Example Gifs

|**Density Multiplication**|
|---|
|<img src="./results/density.gif" width="1200" height="450">|

|**Close**|**Distant**|
|---|---|
|<img src="./results/density01.JPG" width="500" height="300">|<img src="./results/density02.JPG" width="500" height="300">|

### Day & Night Cycle
* We blend the skybox and change the light Color and intensity according to the time.
* Example Gifs

|**Day & Night (10s)**|
|---|
|<img src="./results/day_night_cycle.gif" width="1000" height="600">|

