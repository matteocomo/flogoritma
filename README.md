# Flogoritma

Flogoritma is a Qt desktop editor for building, saving, and interpreting flowcharts as
`.flog` projects. Its runtime supports typed variables, input/output, assignments,
conditions, loops, and step-by-step execution.

## Requirements

- CMake 3.20 or newer
- A C++17 compiler
- Qt 6.2 or newer with Widgets, QML (QJSEngine), and SVG

## Build and run

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/flogoritma
```

Use the block palette to add nodes, select **Collega blocchi** to configure a successor
or conditional branches, and use **Modifica blocco** to edit its expression or label.
New blocks connect automatically to the selected block when it has an open connector;
without a selection, Flogoritma connects only when there is one unambiguous open connector.
Toggle **Modifica > Collega automaticamente i nuovi blocchi** to disable this behavior.
Manage the project name, author, and variables from **Progetto > Proprietà e variabili**.
Zoom with Ctrl+mouse wheel, or use **Visualizza** to zoom or fit the diagram to the view.
Press F5 to run, F6 to execute one node, and Shift+F5 to stop. The active node is
highlighted and output appears in the execution console. Edit an Input block with F2 to
set its prompt and destination variable. Expressions support declared variables,
arithmetic, comparisons, boolean operators, and quoted strings; plain Output text is
treated as a literal. Input values are checked against the declared variable type.

Projects can be opened and saved from the File menu. Existing `.flog` files without
saved coordinates are given an initial grid layout; old Input blocks are connected
automatically when the project declares exactly one variable. To open a project directly,
pass its path when launching the application: `./build/flogoritma esempio.flog`.

The standalone C++ exporter remains experimental and supports fewer node types than the
interpreter; it is not yet exposed in the editor.

To install the executable, application-menu entry, icon, and `.flog` file association
under your local user prefix:

```sh
cmake --install build --prefix "$HOME/.local"
```

If the desktop environment does not refresh its application/file-type database
automatically, run `update-desktop-database ~/.local/share/applications` and
`update-mime-database ~/.local/share/mime`. To make it the default handler for `.flog`
files, run `xdg-mime default flogoritma.desktop application/x-flogoritma`.