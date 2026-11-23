# Gameplay regression checks

Run from the repository root in PowerShell with Visual Studio 2022 C++ tools installed:

```powershell
.\tests\RunGameplayTests.ps1
```

The script generates a separate test project under `build/tests`, compiles the actual
game sources, and creates a hidden GLFW/OpenGL context. It does not change the game
solution or use the player's save directory. Test records and rendered screenshots
are kept under `build/tests/run`.

Checks cover manual/pad boost and speed limits, handbrake, rotated vehicle bounds,
obstacle tunneling at maximum speed, coincident vehicles, checkpoint order/direction,
the first start-line crossing, timeout, respawn, circuit/car-specific ghost records,
and loading records in a second process. Three AI vehicle types must each complete
three laps on both tracks within the 300-second simulation limit.

The render checks exercise title, selection, countdown, both circuits, first person,
results and 800x600 selection. PPM captures can be opened with an image viewer that
supports PPM. These deterministic checks supplement manual driving and visual review;
they do not measure how enjoyable vehicle handling is.
