Description
===============
This lets you see a live view in a rendering engine of what you are doing in Maya.
Using MayaAPI to create callbacks and send messages to the rendering engine using a circular buffer.

Setup
===============
1: Open up loadPlugin.py and input your path to /MayaPlugin/x64/Debug/MayaAPI.mll on line 21.<br />
2: Open Maya 2017 or 2018 and run commandPort -n ":1234";<br />
3: Open MayaAPI.sln and testRayLib.sln.<br />
4: Build solution in MayaAPI.sln.<br />
5: Run testRayLib.sln.<br />

Note: If you want to change the texture on the mesh, make sure the texture is a .png file.

Made by Petter Flood and Emil Hallin.
