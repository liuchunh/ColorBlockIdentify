# ColorBlockIdentify

- ImGui Assist branch

This branch combines `main` branch with `Dear ImGui` Gui library which is used to make panel.

Moreover, CCL functions are split in this branch with the aim of getting more clear structure.

### Structure:

- ImGuiAssist.sln --- Project Config File for `Visual Studio 2022`

- .gitignore --- Ignored File in repo folder

- ImGuiAssist (Folder) --- Project Content
    - CCL.cpp/CCL.h --- Spilt CCL Function
    - main.cpp --- combination result of CCL, OpenCV and ImGui
    - CCLMainColorUpdate.cpp --- Based on `main.cpp`, its color info got integrated into a `C++ style` structure
    - imgui.ini --- ImGui Runtime Config (Just Windows Position etc.)
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
