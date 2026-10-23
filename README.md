# ColorBlockIdentify

- ImGui Assist branch

This branch combines `main` branch with `Dear ImGui` Gui library which is used to make panel.

Moreover, CCL functions are split in this branch with the aim of getting more clear structure.

### Structure:

- ImGuiAssist.sln --- Project Config File for `Visual Studio 2022`

- .gitignore --- Ignored File in repo folder for `git`

- ImGuiAssist (Folder) --- Project Content
    - CCL.cpp/CCL.h --- Spilt CCL Function
    - main.cpp --- combination result of CCL, OpenCV and ImGui
    - CCLMainColorUpdate.cpp --- Based on `main.cpp`, its color info got integrated into a `C++ style` structure
    - CCLMainVariableThreshold.cpp --- Based on `CCLMainColorUpdate.cpp`, this file modifies threshold to a variable, which allows us to debug more conveniently.
    - imgui.ini --- ImGui Runtime Config (Just Windows Position etc.)
    - Theme.h --- Just beautifying the panel.
    - ...... --- Remaining files are created by visual studio, i don't know what they are

#### Update Log:
- 2026.10.16
    - `CCLMainColorUpdate.cpp` Created, color structure updated.
    - Refresh Panel updated.
- 2026.10.21
    - `CCLMainCOlorUpdate.cpp` updated.
    - Known bugs fixed.
    - Create `Modify Panel`.
    - TODO: make HSV threshold variable.
- 2026.10.22
    - `Theme.h` Created for beautifying panel.
- 2026.10.23
    - `CCLMainVariableThreshold.cpp` Created, turning HSV Range variables, with the aim of making debug easier.