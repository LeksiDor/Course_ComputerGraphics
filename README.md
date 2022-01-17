## Programming Assignment for the course COMP.CE.430-2021-2022-1 Computer Graphics.

You can retrieve the code using git terminal:<br>
git clone --recursive https://github.com/LeksiDor/Course_ComputerGraphics.git

It was tested to run on Windows, but possibly can run on other platforms as well.<br>
You should have Vulkan SDK installed on your machine.


The project is mostly a refactored version of Vulkan tutorials: https://vulkan-tutorial.com

However, it features an automatic build system using CMake.
Also, a nicely composed Utilities library will help you to keep all boiler-plate code apart from the actual assignment.
It can even be used as a foundation for more serious Vulkan-based projects, after few extra steps of improvement.


## Programming Assignment 1

If the required features (Task 1a, 1b and 1c) work, grade is 1/5 (enough to pass the assignment)<br>
* Task 1a: Write a Vulkan program that displays (rasterizes) a coloured 2D triangle on the screen.
  - The triangle coordinates can be hardcoded into the program, you don't have to load them separately.
  - For simplicity, use per-vertex colouring in the vertex shader, so you give each vertex a unique colour value, which will then get interpolated for each pixel inside the triangle.
  - You can use https://vulkan-tutorial.com as a reference, but instead of just blindly following the tutorial, try to also understand what happens at each step. Relevant tutorial chapters: From the beginning until the end of the "Drawing a triangle" chapter.
* Task 1b: Make your triangle move in 2D (translation in the xy plane of the window), and add collision checking between the triangle and the edges of the window so that the triangle always bounces back from an edge instead of going off screen.
  - Assign your triangle a random initial direction and speed when the program starts, and move the object as a function of time. The initial direction must be something else than fully vertical or fully horizontal.
  - If you want to make the behaviour independent of the frame rate, you can use std::chrono::high_resolution_clock::now() to get the current time, as is done in https://vulkan-tutorial.com/Uniform_buffers/Descriptor_layout_and_buffer.
  - You are allowed to simplify the collision checking by assuming an axis-aligned minimum bounding box around the triangle, and colliding that rectangular bounding box against the window edge (but still drawing the triangle, not the bounding box)
  - For computing the new angle after collision, you can think of how a ray of light would specularly reflect from a flat surface, like in Lecture 3.
The collisions can be fully elastic (speed does not decrease when colliding), but they don't have to be.
  - Not everything is in the tutorial, you'll also have to apply your knowledge in completing this task.
* Task 1c: In your own words, write a short (about 200-300 words) high-level description of what the program does in order to render the triangle (in Task 1a). Try to describe the rendering flow in a way that would be helpful to you, if you were just about to start working on the assignment, instead of having completed it already. Good descriptions may be published anonymously after the deadline on the course's Moodle page for the other course participants.
* Please attach the high-level program description in the beginning of your main code file (main.cpp).
* Also describe your file structure in the beginning of the main file since your program will probably have multiple files.
* Task 2: Add a rectangle rotating in 3D. Relevant tutorial chapters: "Vertex buffers" and "Uniform buffers" (1p)
  - Colour it in the same way as the triangle in Task 1.
  - Keep the triangle of Task 1 on screen as well. Don't replace the triangle with the rectangle!
  - You don't have to include the rectangle in the collision checking.
* Task 3: Add a texture onto your triangle (and/or rectangle) instead of the per-vertex colouring. (1p)
  - Mipmap generation is not needed.
  - You can choose the texture yourself.
  - Remember to include the texture file in your submission.
  - Textures are discussed in Lecture 4.
* Task 4: Load and render a custom 3D model from a file. (1p)
  - You don't have to add lighting, you can use a model whose texture has "baked" (pre-computed) lighting in it, like in the tutorial.
  - Remember to include the model file(s) in your submission. Note that the submission can't be over 100 MB.
* Task 5: Add a non-trivial feature of your choice to the program. (1-2p depending on the difficulty)
  - Small modifications to the other tasks, such as changing the object's movement, colours or size are considered trivial.
  - You can for example add Phong shading, or make your program into a simple interactive game.
  - Describe your feature shortly in a comment at the beginning of the main code file.


## Programming Assignment 2

**Required functionalities (required for passing the assignment)**<br>
o   Perspective projection. The assignment skeleton is using orthographic projection. It must be changed into perspective projection.<br>
o   Phong shading. The scene must be shaded with Phong shading (or if you aim for extra points you can use more complicated shading).<br>
o   The camera must move around in the scene. Both movement and rotation are required to pass. The transformations must be controlled either by time or mouse coordinates.<br>
o   Sharp shadows.

**Extra functionalities (required for higher grades than pass)**<br>
o   Tone mapping (0.5p)<br>
o   PBR shading (1p)<br>
o   Soft shadows (1p)<br>
o   Sharp reflections (1p)<br>
o   Glossy reflections (2p)<br>
o   Refractions (1p)<br>
o   Caustics (2p)<br>
o   SDF ambient occlusions (1p)<br>
o   Textures (1p) Either procedurally generated textures or textures loaded from a file (requires own program). Must be more than just a plain checkerboard, stripes or gradient.

Your own program which shows the shader (1p) (Javascript that works in new Chrome/Firefox or C++ that builds in Visual Studio 2019 with less than 15 minutes of work. If you would like to use some other language or environment, you can ask the course staff at least one month before the deadline. If you submit your assignment with a random coding language without permission, we will ask you to remake the assignment with some allowed language. Remaking does not extend your deadline.)

o   Interactivity (1p) Must be more than just mouse coordinates. Simple games are very welcome.<br>
o   Progressive path tracing (2p)<br>
o   Basic post-processing like bloom (1p)<br>
o   Advanced post-processing like denoising (2p)<br>
o   Screen space reflections (2p)<br>
o   Screen space ambient occlusions (1p)

Custom distance field objects<br>
o   Simple (1p) Basic shapes or their combinations.<br>
o   Advanced (2p) Fractals or otherwise non-trivial shapes.<br>
o   Animated (1p) More than spinning, moving or scaling. Bending is fine.<br>

Faster ray marching (1p) (see e.g. https://erleuchtet.org/~cupe/permanent/enhanced_sphere_tracing.pdf)

Any other advanced rendering technique is implemented. (1p)
