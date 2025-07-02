###note ： The UI of this project is designed in Chinese. If you need English, please enter the source code translation and replace it yourself. The automatic language switching function will be developed in the future. This is a completely free and unrestricted open source project, and due to its small scale, it does not currently adopt a license.

#Desktop pet project construction instructions
##Source code compilation environment requirements: Windows 10 or later operating system, default installation of Visual Studio 2022 (or 2019), the following compilation methods shall prevail, please complete other compilation methods by yourself.
##Construction method: 1. Open the command line, enter the build folder in the project home directory, enter cmake.. - G "Visual Studio 17 2022" - A x64 (VS2019 is cmake.. - G "Visual Studio 16 2019" - A x64)
After success, you will see the VS solution DesktopPet.sln in the build folder. After opening it, select "configure release" or "debug" from the menu bar above, then click "generate" and choose "generate solution"
After successful generation, return to the project main directory where you can see a batch file copy_desources.bat. Double click to run it, and if there are no errors, it will automatically close, indicating that the run has been successful
4. Enter the bin folder in the project's main directory, and according to your configuration, enter the debug or release folder. Check if there are any folder frames, dependencies json cpp.dll, json cpp.lib, main_icon.ico. If they exist, the build is successful. Double click on DesktopPet.exe to run it

#Desktop Pet Project Document

##1、 Project Overview
Lightweight Windows desktop pet program that enables basic animation playback and simple interaction functions. Pets have two states of standby/walking, supporting basic operations such as mouse drag and click pause, as well as advanced functions such as intelligent direction recognition, dynamic size adjustment, and frame rate control. At the same time, it provides pixel style memo function, supports recording and managing user notes, and displays system resource monitoring function when hovering the mouse.

##2、 Functional requirements
### 1. Animation System
-Supports two animation states:
*IDLE (standby): 5 frames per second loop playback, default mode
*WALK: 6-frame loop playback with lateral movement
-Frame rate control: default 15 frames per second, adjustable through the right-click menu (6-30 frames per second)
-Auto Flip: Automatically flip the image when the pet changes direction, ensuring that the walking direction is consistent with the image
-Random state switching: Automatically switch between standby and walking, making behavior more natural

### 2. Motion Logic
-Horizontal reciprocating motion: continuous movement between the left and right boundaries of the screen
-Mobile speed: dynamically adjusted with display scale
-Collision detection: Automatically turn direction and flip image when reaching the edge of the screen


### 3. interactional
-Left mouse click: Pause/Resume animation
-Left click double-click: Open memo
-Drag and drop operation: Hold down any position on the form and drag the pet
-Right click menu:
*Open the memo
*Switch to standby/walking mode
*Adjust the display size up/down
*Speed up/slow down animation speed
*Exit the program
-Mouse hover: After hovering over the pet for 2 seconds, the system monitoring window will be displayed

### 4. Display
-Support dynamic adjustment of display scale: default magnification of 4x
-Proportion range: 1-8 times adjustable
-Automatically reload resources when adjusting the size

### 5. Memo function
-Pixel style UI interface
-Support adding, viewing, and deleting memos
-Automatically save notes to local files for persistent storage
-Support dragging windows and adjusting positions
-Record creation timestamp
-The newly added memo is displayed at the top
-Use white text instead of green to improve readability

### 6. System monitoring function
-Pixel style monitoring interface
-Hover the mouse over the pet for 2 seconds and it will automatically display
-Monitoring indicators:
*CPU usage: Real time display of percentage and progress bar
*Memory usage: Real time display of percentage and progress bar

-Support dragging windows and adjusting positions
-Automatically close when the mouse leaves the pet area




