#pragma once

/*!
 * \file Declares VectorVisual to visualize a vector.
 */

#ifndef USE_GLEW
#ifdef __OSX__
# include <OpenGL/gl3.h>
#else
# include <GL3/gl3.h>
#endif
#endif
#include <array>
#include <morph/vec.h>
#include <morph/colour.h>

namespace morph {

    //! How should the visualized vector go? Does it start at the origin? If so, it goes 'from'
    //! the origin; FromOrigin. Does it instead sit on top of the origin (OnOrigin)?
    //! Alternatively, it could go 'to' the origin; ToOrigin.
    enum class VectorGoes {
        FromOrigin,
        ToOrigin,
        OnOrigin
    };

    //! A class to visualize a single vector
    template <typename Flt, int ndim, int glver = morph::gl::version_4_1>
    class VectorVisual : public VisualModel<glver>
    {
        static_assert (ndim > 0 && ndim <= 3, "1, 2 or 3 dimensions please.");
    public:
        VectorVisual(const vec<float> _offset) {
            // Set up...
            this->mv_offset = _offset;
            this->viewmatrix.translate (this->mv_offset);
        }

        //! Do the computations to initialize the vertices that will represent the Quivers.
        void initializeVertices()
        {
            vec<float> start, end, origin = {0,0,0};

            // Convert thevec into a 3D vec
            vec<float> threevec = {0,0,0};
            if constexpr (ndim == 1) {
                threevec[0] = static_cast<float>(thevec[0]);
            } else if constexpr (ndim == 2) {
                threevec[0] = static_cast<float>(thevec[0]);
                threevec[1] = static_cast<float>(thevec[1]);
            } else {
                threevec = thevec.as_float();
            }

            // Shift threevec to sit on origin if necessary
            if (this->vgoes == VectorGoes::FromOrigin) {
                start = origin;
                end = threevec;
            } else if (this->vgoes == VectorGoes::ToOrigin) {
                end = origin;
                start = threevec;
            } else { // OnOrigin
                vec<float> halfvec = threevec * 0.5f;
                start = origin - halfvec;
                end = origin + halfvec ;
            }

            vec<float> colourvec = threevec;
            colourvec.renormalize();
            morph::ColourMap<float> cm (morph::ColourMapType::HSV);
            std::array<float, 3> clr = cm.convert (colourvec[0], colourvec[1]);

            // The right way to draw an arrow.
            vec<float> arrow_line = end - start;
            float len = arrow_line.length();
            vec<float> cone_start = arrow_line.shorten (len * arrowhead_prop);
            cone_start += start;

            this->computeTube (this->idx, start, cone_start, clr, clr, thickness, shapesides);
            float conelen = (arrow_line - cone_start).length();
            if (arrow_line.length() > conelen) {
                this->computeCone (this->idx, cone_start, end, 0.0f, clr, thickness * 2.0f, shapesides);
            }
            // Plus a sphere on the coordinate:
            //this->computeSphere (this->idx, origin, clr, thickness * 2.0f, shapesides / 2, shapesides);
        }

        // The vector to vis
        vec<Flt, ndim> thevec;

        //! An enumerated type to say whether we draw from, on or to the origin
        VectorGoes vgoes = VectorGoes::OnOrigin;

        // How many sides to an arrow/cone/sphere? Increase for smoother arrow
        // objects. Decrease to ease the load on your CPU and GPU. 12 is a reasonable
        // compromise. You can set this before calling finalize().
        int shapesides = 12;

        // Arrow thickness for the vector
        float thickness = 0.1f;

        // What proportion of the arrow length should the arrowhead length be?
        float arrowhead_prop = 0.25f;

        // User can choose a colour
        //std::array<float, 3> clr = morph::colour::crimson;
    };

} // namespace morph
