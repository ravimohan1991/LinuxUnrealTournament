Welcome to Unreal Tournament for Linux!
=======================================

Information: This repository is an attempt and not ready right now. Please don't download/clone this with the hope of building running UT. The structure is work in progress!!!

This is the project page for **Unreal Tournament** on GitHub abandoned by Epic (for now).  Unreal Tournament is being created in close collaboration between Epic and the Unreal Engine 4 developer community.  This repository is maintained by The_Cowboy to generate interest in native Linux based gaming and building.  You'll be able to see live commits from "liberal" developers along with integrated code submissions from the community!  You can also use this code as a starting point for experimentation (and we are exactly doing that!).

As described in the [License Agreement](LICENSE.pdf), the code and assets residing in this repository may only be used for Unreal Tournament submissions.

This project is the continuation of [Epic UT](https://github.com/EpicGames/UnrealTournament) with the last commit date stamp: Aug 11, 2017 (more than 555 days).

The aim of the project is threefold:
* Update the UT code as per latest Engine release in modular fashion
* Extensive focus on Linux platform
* Use Engine to push forward the AI realm


Check out the [Unreal Tournament wiki](https://wiki.unrealengine.com/Unreal_Tournament_Development) for documentation on the design and development process of the game.


How to get started (Linux)
---------------------------
* First we download the Engine and Game code from this repository. To do that type the following sequence of commands in the shell

        git clone https://github.com/ravimohan1991/LinuxUnrealTournament.git
        cd LinuxUnrealTournament/
        git clone https://github.com/EpicGames/UnrealEngine -b release

* Read [native build](https://github.com/EpicGames/UnrealEngine/blob/release/Engine/Build/BatchFiles/Linux/README.md) and complete the prerequisites mentioned there.

* Next we download binary content for the Engine.
    
        cd UnrealEngine
        ./Setup.sh
  
* Next we generate the projectfiles

        ./GenerateProjectFiles.sh 

* Build the Engine 
        
        make
  Unreal Editor is now functional. It can be accessed through the binary `./UE4Editor` located in `~/UnrealEngine/Engine/Binaries/Linux`.      

* Now we focus on the UT Game. Delete some directories/file(s) (not functional right now)
  * UnrealTournament/Plugins
  * UnrealTournament/Source/UnrealTournamentServer
  * UnrealTournament/Source/UnrealTournament.Server.Target.cs
  * UnrealTournament/Source/UnrealTournament/ThirdParty
* Now run the Editor and hit browse to select the project in directory `UnrealTournament`. The projectfiles will be generated and Editor will compile the project with new name.  NOTE: Due to some bugs in the Engine, the code will not compile without errors. You have to modify the code (I have introduced some pull requests and hopefully, they will be implemented in the new version). The request is [here](https://github.com/EpicGames/UnrealEngine/pull/5989). You can modify the Engine code appropriately after downloading form GitHub. Next comment out line 538 in SUTCreateGamePanel.cpp

                FPlatformMisc::RequestMinimize()

Now you should be good to go!

<!--
* Almost there! We generate the Game project files.
          
          mono ~/UnrealEngine/Engine/Binaries/DotNET/UnrealBuildTool.exe -Project="~/UnrealTournament/UnrealTournament.uproject" -projectfiles

* Finally we are ready to compile the game.

        mono ~/UnrealEngine/Engine/Binaries/DotNET/UnrealBuildTool.exe -Project="~/UnrealTournament/UnrealTournament.uproject" Development Linux −TargetType=Editor −Progress −NoHotReloadFromIDE
  This will compile UT which can be loaded in the Editor as a project.     
-->
**To get started using GitHub:**

- You'll want to create your own Unreal Tournament **fork** by clicking the __Fork button__ in the top right of this page.
- Next, [install a Git client](http://help.github.com/articles/set-up-git) on your computer.
- Use the GitHub program to **Sync** the project's files to a folder on your machine.
- You can now open up **UnrealTournament.uproject** in the editor!
- If you're a programmer, follow the platform specific steps below to set up the C++ project. 
- Using the GitHub program, you can easily **submit contributions** back up to your **fork**.  These files will be visible to all subscribers, and you can advertise your changes in the [Unreal Tournament forums](http://forums.unrealengine.com/forumdisplay.php?34-Unreal-Tournament).
- When you're ready to send the changes to the Unreal Tournament team for review, simply create a [Pull Request](https://help.github.com/articles/using-pull-requests).


