/*
 * An example morph::Visual scene, containing a Gridv, and using GridvVisual
 */

#include <iostream>
#include <vector>
#include <cmath>

#include <morph/Scale.h>
#include <morph/vec.h>
#include <morph/Visual.h>
#include <morph/VisualDataModel.h>
#include <morph/GridvVisual.h>
#include <morph/Gridv.h>

int main()
{
    // Contructor args are width, height, title, coordinate arrows offset, cooridnate
    // arrows lengths, coord arrow thickness, ?
    morph::Visual v(1600, 1000, "morph::GridvVisual", {-0.8,-0.8}, {.05,.05,.05}, 2.0f, 0.0f);
    v.lightingEffects();
    v.addLabel ("This is a\nmorph::GridvVisual\nobject", {0.26f, -0.16f, 0.0f});

    // Create a grid to show in the scene
    constexpr size_t Nside = 100;
    constexpr morph::vec<float, 2> grid_spacing = {0.01f, 0.01f};

    // The simplest declaration of Gridv is:
    //   morph::Gridv g(size_t n_x, size_t n_y);
    // grid_spacing, grid_zero, use of memory, wrapping and ordering are all possible arguments to
    // the constructor.
    morph::Gridv grid(Nside, Nside, grid_spacing);

    std::cout << "Number of pixels in grid:" << grid.n << std::endl;

    // Make some dummy data (a sine wave) to make an interesting surface
    std::vector<float> data(grid.n, 0.0);
    for (unsigned int ri=0; ri<grid.n; ++ri) {
        auto coord = grid[ri];
        data[ri] = 0.05f + 0.05f * std::sin(20.0f * coord[0]) * std::sin(10.0f*coord[1]) ; // Range 0->1
    }

    // Add a CartGridVisual to display the CartGrid within the morph::Visual scene
    morph::vec<float, 3> offset = { 0.0f, -0.05f, 0.0f };
    auto gv = std::make_unique<morph::GridvVisual<float>>(&grid, offset);
    v.bindmodel (gv);
    gv->gridVisMode = morph::GridvVisMode::RectInterp; // RectInterp or Triangles
    gv->setScalarData (&data);
    gv->cm.setType (morph::ColourMapType::Twilight);
    gv->finalize();
    v.addVisualModel (gv);

    v.keepOpen();

    return 0;
}
