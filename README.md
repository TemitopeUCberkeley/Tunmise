# CS184 Final Project: Lip Gloss Renderer

This project extends a CS184 path tracer to render a procedural human lip mesh with different lip finish/material variants.

The lip scene is generated procedurally in `src/pathtracer/generate_lip.cpp` and saved as a Collada file at:

```bash
dae/final-project/lips1.dae
```

The generated scene contains three lip variants:

- `default`: standard glossy lip
- `glossy`: wetter, shinier lip gloss material
- `matte`: non-glossy/matte lip material

The renderer supports choosing which variant to render with the `-v` flag.

## Build

From the repository root:

```bash
cmake -S . -B build
cmake --build build --target generate_lip pathtracer -j 8
```

This builds:

- `build/generate_lip`: procedural Collada scene generator
- `build/pathtracer`: renderer

## Regenerate the Lip Scene

Run this after changing `src/pathtracer/generate_lip.cpp`:

```bash
./build/generate_lip dae/final-project/lips1.dae
```

This rewrites `dae/final-project/lips1.dae` with the latest procedural mesh and material definitions.

## Render a Lip Variant

Use `-v` to choose which lip to render:

```bash
./build/pathtracer -v default -s 32 -l 8 -m 5 -t 8 -r 700 500 -f default.png dae/final-project/lips1.dae
./build/pathtracer -v glossy  -s 32 -l 8 -m 5 -t 8 -r 700 500 -f glossy.png  dae/final-project/lips1.dae
./build/pathtracer -v matte   -s 32 -l 8 -m 5 -t 8 -r 700 500 -f matte.png   dae/final-project/lips1.dae
```

Available variants:

```bash
default
glossy
matte
all
```

If `-v` is omitted, the renderer uses `default`.

## Render All Variants Together

```bash
./build/pathtracer -v all -s 32 -l 8 -m 5 -t 8 -r 1300 700 -f all_lips.png dae/final-project/lips1.dae
```

## Interactive Viewer

To open the viewer instead of writing directly to a PNG:

```bash
./build/pathtracer -v default dae/final-project/lips1.dae
```

You can replace `default` with `glossy`, `matte`, or `all`.

## Main Code Locations

- `src/pathtracer/generate_lip.cpp`: procedural lip geometry and material generation
- `src/application/main.cpp`: command-line parsing and `-v` variant filtering
- `src/pathtracer/bsdf.cpp`: layered, glossy, and Disney-style lip BSDF logic
- `src/scene/collada/collada.cpp`: Collada parsing and material construction
- `dae/final-project/lips1.dae`: generated lip scene used by the renderer

## Notes

This project does not use image texture maps for the lip surface. The lip shape, crease, subtle folds, and material variants are generated procedurally through geometry and BSDF/material parameters.

Generated render outputs such as `matte.png`, `default.png`, `glossy.png`, and `*_rate.png` are preview artifacts and do not need to be committed unless they are being used in the final report.
