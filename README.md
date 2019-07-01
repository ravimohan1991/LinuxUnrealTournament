Welcome to Unreal Tournament for Linux!
=======================================

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
        git submodule init
        git submodule update

* Read [native build](https://github.com/EpicGames/UnrealEngine/blob/release/Engine/Build/BatchFiles/Linux/README.md) and complete the prerequisites mentioned there.

* Next we download binary content for the Engine.
    
        cd UnrealEngine
        ./Setup.sh
  
  
  

**To get started using GitHub:**

- You'll want to create your own Unreal Tournament **fork** by clicking the __Fork button__ in the top right of this page.
- Next, [install a Git client](http://help.github.com/articles/set-up-git) on your computer.
- Use the GitHub program to **Sync** the project's files to a folder on your machine.
- You can now open up **UnrealTournament.uproject** in the editor!
- If you're a programmer, follow the platform specific steps below to set up the C++ project. 
- Using the GitHub program, you can easily **submit contributions** back up to your **fork**.  These files will be visible to all subscribers, and you can advertise your changes in the [Unreal Tournament forums](http://forums.unrealengine.com/forumdisplay.php?34-Unreal-Tournament).
- When you're ready to send the changes to the Unreal Tournament team for review, simply create a [Pull Request](https://help.github.com/articles/using-pull-requests).


