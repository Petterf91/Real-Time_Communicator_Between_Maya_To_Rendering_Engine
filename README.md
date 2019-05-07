Description
===============
This lets you see a live view in a rendering engine of what you are doing in Maya.
Using MayaAPI to create callbacks and send messages to the rendering engine using a circular buffer.

Setup
===============
1: Open up loadPlugin.py and input your path to /MayaPlugin/x64/Debug/MayaAPI.mll on line 21.
2: Open Maya 2017 or 2018 and run "commandPort -n ":1234";
3: Open MayaAPI.sln and testRayLib.sln.
4: Build solution in MayaAPI.sln.
5: Run testRayLib.sln.
