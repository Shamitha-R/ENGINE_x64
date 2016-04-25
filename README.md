# ENGINE_x64
Correlation Mapping Optical Coherence Tomography processing and rendering pipeline with OpenCL/OpenGL

Pre-requisites 
Visual Studio Community 2015
OpenCL Code Builder
SDL 2.0.3
AntTweakBar 1.16
GLEW
glm 0.9.7.1

Testing the application:

1. Sample Data for the application can be found at 
https://www.dropbox.com/s/f708oebhlnmt85n/Engine_Test_Data.zip?dl=0

2. Place the extracted files in the main project folder

3. The Load OCT Data command button opens a windows directory window allowing users to locate and select each of the OCT data files individually. The terminal window displays the outcome of the loading function indicating if any files have failed the importing process. 
4.  By clicking the Execute Processing command button, the system progress through each component within the processing and rendering pipeline, while simultaneously printing the outcome of the operations in the terminal window. If the operation is successful, the final three-dimensional render is displayed in the viewport area. 

5. Parameters such as the correlation window can now be changed and the render can be updated to reflect these changes using the Update Parameters command button. 
