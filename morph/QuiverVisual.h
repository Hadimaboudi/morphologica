#pragma once

#ifndef USE_GLEW
#ifdef __OSX__
# include <OpenGL/gl3.h>
#else
# include <GL3/gl3.h>
#endif
#endif
#include <morph/tools.h>
#include <morph/VisualDataModel.h>
#include <morph/MathAlgo.h>
#include <morph/Scale.h>
#include <morph/vec.h>
#include <morph/vvec.h>
#include <morph/colour.h>
#include <iostream>
#include <vector>
#include <array>
#include <algorithm>
#include <functional>

namespace morph {

    //! How does a quiver go? Does it start at the coordinate? If so, it goes 'from'
    //! the coordinate; FromCoord. Does it instead sit on top of the coord (OnCoord)?
    //! Alternatively, it could go 'to' the coordinate; ToCoord.
    enum class QuiverGoes {
        FromCoord,
        ToCoord,
        OnCoord
    };

    //! A class to make quiver plots
    template <typename Flt>
    class QuiverVisual : public VisualDataModel<Flt>
    {
    public:
        QuiverVisual(morph::gl::shaderprogs& _shaders,
                     std::vector<vec<float>>* _coords,
                     const vec<float> _offset,
                     const std::vector<vec<Flt,3>>* _quivers,
                     ColourMapType _cmt,
                     const float _hue = 0.0f) {
            // Set up...
            this->shaders = _shaders;
            this->mv_offset = _offset;
            this->viewmatrix.translate (this->mv_offset);

            this->dataCoords = _coords;
            this->vectorData = _quivers;

            this->cm.setHue (_hue);
            this->cm.setType (_cmt);

            this->length_scale.do_autoscale = true;
        }

        // Call before initializeVertices() to scale quiver lengths logarithmically
        void setlog() { this->length_scale.setlog(); }

        //! Do the computations to initialize the vertices that will represent the Quivers.
        void initializeVertices()
        {
            unsigned int ncoords = this->dataCoords->size();
            unsigned int nquiv = this->vectorData->size();

            if (ncoords != nquiv) {
                std::cout << "ncoords != nquiv, return." << std::endl;
                return;
            }

            vec<Flt> zero3 = { Flt{0}, Flt{0}, Flt{0} };
            vvec<Flt> dlengths;
            // Compute the lengths of each vector
            for (unsigned int i = 0; i < nquiv; ++i) {
                dlengths.push_back (MathAlgo::distance<Flt, 3> (zero3, (*this->vectorData)[i]));
            }

            // Linearly scale the dlengths to generate colours
            morph::Scale<Flt> lcscale;
            lcscale.do_autoscale = true;
            vvec<Flt> lengthcolours(dlengths);
            lcscale.transform (dlengths, lengthcolours);

            // Now scale the lengths for their size on screen. Do this with a linear or log scaling.

            // (if log) First replace zeros with NaNs so that log transform will work.
            if (this->length_scale.getType() == morph::ScaleFn::Logarithmic) {
                dlengths.search_replace (Flt{0}, std::numeric_limits<Flt>::quiet_NaN());
            }

            // Transform data lengths into "nrmlzedlengths"
            vvec<float> nrmlzedlengths (dlengths.size(), this->fixed_length);
            if (this->fixed_length == 0.0f) {
                this->length_scale.transform (dlengths, nrmlzedlengths);
            }

            // Find the scaling factor to scale real lengths into screen lengths, which are the
            // normalized lengths multiplied by a user-settable quiver_length_gain.
            vvec<float> lfactor = nrmlzedlengths/dlengths * this->quiver_length_gain;

            vec<Flt> half = { Flt{0.5}, Flt{0.5}, Flt{0.5} };
            vec<Flt> vectorData_i, halfquiv;
            vec<float> start, end, coords_i;
            std::array<float, 3> clr;
            for (unsigned int i = 0; i < ncoords; ++i) {

                coords_i = (*this->dataCoords)[i];

                float len = nrmlzedlengths[i] * this->quiver_length_gain;

                if ((std::isnan(dlengths[i]) || dlengths[i] == Flt{0}) && this->show_zero_vectors) {
                    // NaNs denote zero vectors when the lengths have been log scaled. Use the colour maps 'nan' colour to show the zero vector
                    this->computeSphere (this->idx, coords_i, zero_vector_colour, this->zero_vector_marker_size * quiver_thickness_gain);
                    continue;
                }

                vectorData_i = (*this->vectorData)[i];
                vectorData_i *= lfactor[i];

                clr = this->cm.convert (lengthcolours[i]);

                if (this->qgoes == QuiverGoes::FromCoord) {
                    start = coords_i;
                    std::transform (coords_i.begin(), coords_i.end(), vectorData_i.begin(), end.begin(), std::plus<Flt>());

                } else if (this->qgoes == QuiverGoes::ToCoord) {
                    std::transform (coords_i.begin(), coords_i.end(), vectorData_i.begin(), start.begin(), std::minus<Flt>());
                    end = coords_i;
                } else /* if (this->qgoes == QuiverGoes::OnCoord) */ {
                    std::transform (half.begin(), half.end(), vectorData_i.begin(), halfquiv.begin(), std::multiplies<Flt>());
                    std::transform (coords_i.begin(), coords_i.end(), halfquiv.begin(), start.begin(), std::minus<Flt>());
                    std::transform (coords_i.begin(), coords_i.end(), halfquiv.begin(), end.begin(), std::plus<Flt>());
                }
                // Will need a fixed scale for some visualizations
                this->computeTube (this->idx, start, end, clr, clr, len*quiver_thickness_gain);

                // Plus sphere or cone:
                this->computeSphere (this->idx, coords_i, clr, len*quiver_thickness_gain*2.0f);

                // Compute a tip for the cone.
                vec<Flt> frac = { Flt{0.4}, Flt{0.4}, Flt{0.4} };
                vec<float> tip;
                // Multiply vectorData_i by a fraction and that's the cone end. Note reuse of halfquiv variable
                std::transform (frac.begin(), frac.end(), vectorData_i.begin(), halfquiv.begin(), std::multiplies<Flt>());
                std::transform (end.begin(), end.end(), halfquiv.begin(), tip.begin(), std::plus<Flt>());
                this->computeCone (this->idx, end, tip, -0.1f, clr, len*quiver_thickness_gain*2.0f);
            }
        }

        //! An enumerated type to say whether we draw quivers with coord at mid point; start point or end point
        QuiverGoes qgoes = QuiverGoes::FromCoord;

        // Setting a fixed length can be useful to focus on the flow of the field.
        Flt fixed_length = 0.0f;

        // Allows user to linearly scale the size of the quivers that are plotted. Set before
        // calling finalize()
        float quiver_length_gain = 1.0f;

        // Allows user to scale the thickness of the quivers.
        float quiver_thickness_gain = 0.05f;

        // If true, show a marker indicating the location of zero vectors
        bool show_zero_vectors = false;

        // User can choose a colour
        std::array<float, 3> zero_vector_colour = morph::colour::crimson;

        // User can choose size of zero vector markers (which are spheres)
        float zero_vector_marker_size = 0.05f;

        // The input vectors are scaled in length to the range [0, 1], which is then modified by the
        // user using quiver_length_gain. This scaling can be made logarithmic by calling
        // QuiverVisual::setlog() before calling finalize().
        morph::Scale<Flt, float> length_scale;
    };

} // namespace morph
