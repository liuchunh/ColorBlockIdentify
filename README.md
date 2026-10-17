# ColorBlockIdentify

This is a project for `Identifying color block` which $\text{DO NOT}\color{RED}$ use `OpenCV` library.

### Structure
- FuncTest
    - DynamicArray.cpp/.h --- Abandoned.
    - Erode.cpp --- Abandoned.
    - ExtractRunsTest.cpp --- Initial Version for split runs from image info.
    - HandleRuns.cpp --- HSVImage array removed.
    - MergeRunsToComponent.cpp --- Initial Version for merge runs by Union-Set.
    - MakeFile --- For test.
- Project
    - Migrated CCL to Embedded system. $\text{HAVE NOT BEEN TESTED}\color{RED}$
    - All the code here are based on `Aurix Development Studio` and `Infineon TC264`
    - Just for backup
- VisionTest
    - Test CCL Code on Windows and visual studio 2022
        - VisionTest
            - CCL.cpp/CCLMain.cpp --- Initial Version of CCL Code
            - CCLMainHSVDistance.cpp --- Add an way of confirming color by three-dimension HSV distance. $\text{HAVE NOT BEEN TESTED}$
            - CCLMainOptimize.cpp, CCLMainOptimize4Struct.cpp --- maybe array `HSVImage, mask, RunsCount` removed. Logic Optimized.
            - Convert.c --- Initial test code for converting BGR to HSV. Created by AI.
            - Convert2Mask.cpp --- Initial test code for converting image info to mask array.
            - main.cpp/Moore.cpp --- Eight moore to find border. Abandoned. 
        - x64. --- Binary File. Ignored by Git.
        - CCLSln.md --- Initial CCL Document created by Chatgpt.
        - VisionTest.sln --- Project Configure File for `Visual Studio 2022`

#### Update Log
Allllllllllllll right i forgot to record it.

maybe Git Commit History gotta help me and rewind time.

<!--> 
Dear Git:
    Fk u for deleting my initial version of README.md
    Jesus would suck my dk.
                                                        UR dad _Oxygen_ 

Dear Darling Visual Studio Code:
    Thank you for recovering my README.md
    It is u that save my file and prevent me from rewriting it again.
    Thank you so much one more time.
                                                        Dear. _Oxygen_
<-->