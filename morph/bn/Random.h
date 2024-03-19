/*
 * A singleton class to hold random number generators for use in Boolean gene networks.
 */

#pragma once

#include <array>
#include <cuchar>
#include <morph/Random.h>
#include <morph/bn/Genosect.h>

namespace morph {
    namespace  bn {

        //! Access random number generators for boolean gene networks
        template<std::size_t N=5, std::size_t K=5>
        class Random
        {
            using genosect_t = typename Genosect<K>::type;

            //! Private constructor/destructor
            Random() {};
            ~Random() {};
            //! A pointer returned to the single instance of this class
            static Random<N,K>* pInstance;

        public:
            //! The instance public function. Uses the very short name 'i' to keep code tidy.
            static Random<N,K>* i()
            {
                // <N,K> template args seem to be optional here, at least on g++/icpc
                if (Random<N,K>::pInstance == 0) {
                    Random<N,K>::pInstance = new morph::bn::Random<N,K>;
                    //Random::i()->init(); // If required
                }
                return Random<N,K>::pInstance;
            }

            // To enable tests to be fully valgrind memcheckable, call this at program end
            static void i_deconstruct()
            {
                if (Random<N,K>::pInstance != 0) {
                    delete Random<N,K>::pInstance;
                }
            }

            //! Hold an array to place random numbers in of 'genosect_t width' - gw
            static constexpr std::size_t gw = N*(1<<K);
            //! An array to hold a sequence of random floats generated by frng
            std::array<float, gw> rnums;

            //! Width of a GradGenome with N genes
            static constexpr std::size_t grad_gw = 2 * N * N;
            //! An array to hold a sequence of random floats generated by frng
            std::array<float, grad_gw> grad_rnums;

            //! Populate rnums with gw new random numbers.
            //! See https://www.aerialmantis.co.uk/blog/2017/03/17/template-keywords/ to
            //! explain the odd looking .template get<> syntax.
            void fill_rnums() { this->frng.template get<gw> (rnums); }

            //! Like fill_rnums, but for GradGenome<> not Genome<>
            void fill_grad_rnums() { this->frng.template get<grad_gw> (grad_rnums); }

            //! A random number generator of width genosect_t.
            morph::RandUniform<genosect_t, std::mt19937> genosect_rng;

            //! A floating point random number generator
            morph::RandUniform<float, std::mt19937> frng;
        };
    }
}
