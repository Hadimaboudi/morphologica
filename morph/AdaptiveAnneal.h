/*
 * Simulated Annealing - An implementation of the Adaptive Annealing Algorithm described
 * in:
 *
 * Ingber, L. (1989). Very fast simulated re-annealing. Mathematical and Computer
 * Modelling 12, 967-973.
 *
 * Author: Seb James
 * Date: September 2021
 */
#pragma once

#include <utility>
#include <vector>
#include <iostream>
#include <morph/MathAlgo.h>
#include <morph/vVector.h>
#include <morph/Vector.h>
#include <morph/Random.h>
#include <morph/Anneal.h> // For morph::Anneal_State

namespace morph {

    /*!
     * A class implementing Lester Ingber's Adaptive Simlulated Annealing Algorithm. The
     * design is similar to that in NM_Simplex.h; the client code creates an
     * AdaptiveAnneal object, sets paramters and then runs a loop, checking
     * AdaptiveAnneal::state to tell it when to compute a new value of the objective
     * function from the parameters generated by the AdaptiveAnneal class and when the
     * algorithm has finished.
     *
     * \tparam T The type for the numbers in the algorithm. Expected to be floating
     * point, so float or double.
     */
    template <typename T>
    class AdaptiveAnneal
    {
    public: // Algorithm parameters to be adjusted by user

        // Set false to hide text output
        static constexpr bool debug = true;
        //! By default we *descend* to the *minimum* metric value of the user's
        //! objective function. Set this to false (before calling init()) to instead
        //! ascend to the maximum metric value.
        bool downhill = true;
        //! Lester's Temperature_Ratio_Scale (same name in the ASA C
        //! code). Related to m=-log(temperature_ratio_scale).
        T temperature_ratio_scale = T{1e-5};
        //! Lester's Temperature_Anneal_Scale in ASA C code. n=log(temperature_anneal_scale)
        T temperature_anneal_scale = T{100};
        //! Lester's Cost_Parameter_Scale_Ratio (used to compute temp_cost)
        T cost_parameter_scale_ratio = T{1};
        //! If accepted_vs_generated is less than this, reanneal
        T acc_gen_reanneal_ratio = T{0.7};
        //! Number of samples that should be used to estimate the partials dL/dx in ::reanneal()
        unsigned int partials_samples = 2;
        //! How many times to find the same best objective before considering that the algorithm has finished.
        unsigned int f_x_best_repeat_max = 10;
        //! If it has been this many steps since the last reanneal, reanneal again
        unsigned int reanneal_after_steps = 100;

    public: // Parameter vectors and objective fn results need to be client-accessible

        //! Candidate parameter values. In the Ingber papers, these are 'alphas'
        morph::vVector<T> x_cand;
        //! Value of obj fn for candidate parameters
        T f_x_cand = T{0};
        //! Currently accepted parameters
        morph::vVector<T> x;
        //! Value of obj fn for current parameters
        T f_x = T{0};
        //! Best parameters so far
        morph::vVector<T> x_best;
        //! Value of obj fn for best parameters
        T f_x_best = T{0};
        //! How many times has this best objective repeated? Reset on reanneal.
        unsigned int f_x_best_repeats = 0;
        //! A special set of parameters to ask the user to compute (when computing reanneal)
        morph::vVector<morph::vVector<T>> x_set;
        morph::vVector<T> f_x_set;

    public: // Statistical records and state. (May want AnnealStats class)

        //! Number of candidates that are improved (descents, if downhill is true)
        unsigned int num_improved = 0;
        //! Number of candidates that are worse (if downhill is true)
        unsigned int num_worse = 0;
        //! Record statistics on the number of acceptances of worse candidates
        unsigned int num_worse_accepted = 0;
        //! Number of accepted parameter sets
        unsigned int num_accepted = 0;
        //! Absolute count of number of calls to ::step()
        unsigned int steps = 0;
        //! All accepted parameters? All parameters computed?
        morph::vVector<morph::vVector<T>> param_hist;
        //! For each entry in param_hist, record also its objective function value
        morph::vVector<T> f_param_hist;

        //! The state tells client code what it needs to do next.
        Anneal_State state = Anneal_State::Unknown;

    protected: // Internal algorithm parameters

        //! The number of dimensions in the parameter search space. Set by constructor
        //! based on the number of dimensions in the initial parameters.
        unsigned int D = 0;
        //! k is the symbol Lester uses for the step count
        unsigned int k = 1;
        //! A count of the number of steps since last reanneal
        unsigned int k_r = 0;
        //! The expected number of steps that will be taken. Computed.
        unsigned int k_f = 1;
        //! The temperatures, Tik. Note that there is a temperature for each of D dimensions.
        morph::vVector<T> temp;
        //! Initial temperatures Ti0. Set to 1.
        morph::vVector<T> temp_0;
        //! Final temperatures Tif. Computed.
        morph::vVector<T> temp_f;
        //! Internal ASA parameter, m=-log(temperature_ratio_scale). Note that there is
        //! an mi for each of D dimensions, though these are ususally all set to the
        //! same value.
        morph::vVector<T> m;
        //! Internal ASA parameter, n=log(temperature_anneal_scale)
        morph::vVector<T> n;
        //! Internal control parameter, c = m * exp(-n/D)
        morph::vVector<T> c;
        morph::vVector<T> c_cost;
        morph::vVector<T> temp_cost_0;
        //! Temperature used in the acceptance function. k_cost is the number of
        //! accepted points, num_accepted
        morph::vVector<T> temp_cost;
        //! Parameter ranges - defining a part of R^n to search - [Ai, Bi]
        morph::vVector<T> range_min; // A
        morph::vVector<T> range_max; // B
        morph::vVector<T> rdelta;    // = range_max - range_min;
        morph::vVector<T> rmeans;    // = (range_max + range_min)/T{2};
        //! Reannealing sensitivies
        morph::vVector<T> s;
        morph::vVector<T> s_max;
        morph::vVector<T> partials;
        //! The random number generated used in the acceptance_check function
        morph::RandUniform<T> rng_u;

    public: // User-accessible methods

        //! General constructor for n dimensions with initial params
        AdaptiveAnneal (const morph::vVector<T>& initial_params,
                        const morph::vVector<morph::Vector<T,2>>& param_ranges)
        {
            this->D = initial_params.size();
            this->range_min.resize (this->D);
            this->range_max.resize (this->D);
            unsigned int i = 0;
            for (auto pr : param_ranges) {
                this->range_min[i] = pr[0];
                this->range_max[i] = pr[1];
                ++i;
            }
            this->rdelta = this->range_max - this->range_min;
            this->rmeans = (this->range_max + this->range_min)/T{2};

            this->x_cand = initial_params;
            this->x_best = initial_params;
            this->x = initial_params;

            // Before ::init() is called, user may need to manually change some
            // parameters, like temperature_ratio_scale.
            this->state = Anneal_State::NeedToInit;
        }

        //! After constructing, then setting parameters, the user must call init
        void init()
        {
            // Set up the parameter/cost value members
            this->f_x_best = (this->downhill == true) ? std::numeric_limits<T>::max() : std::numeric_limits<T>::lowest();
            this->f_x = f_x_best;
            this->f_x_cand = f_x_best;
            this->x.resize (this->D, T{0});
            this->x_cand.resize (this->D, T{0});
            this->x_best.resize (this->D, T{0});

            // Initial and current temperatures
            this->temp_0.resize (this->D, T{1});
            this->temp.resize (this->D, T{1});

            // Sensitivies containers
            this->s.resize (this->D, T{1});
            this->s_max.resize (this->D, T{1});
            this->partials.resize (this->D, T{1});

            // The m and n parameters
            this->m.resize (this->D);
            this->m.set_from (-std::log(this->temperature_ratio_scale));

            this->n.resize (this->D);
            this->n.set_from (std::log(this->temperature_anneal_scale));

            // Work out expected final temp
            this->temp_f = this->temp_0 * (-this->m).exp();

            this->k_f = static_cast<unsigned int>(std::exp (this->n.mean()));

            // Set the 'control parameter', c, from n and m
            this->c.resize (this->D, T{1});
            this->c = this->m * (-this->n/this->D).exp();

            this->c_cost = this->c * cost_parameter_scale_ratio;
            this->temp_cost_0 = this->c_cost;
            this->temp_cost = this->c_cost;

            this->state = Anneal_State::NeedToCompute; // or NeedToStep
        }

        // Advance the simulated annealing algorithm by one step
        void step()
        {
            ++this->steps;
            if (this->state == Anneal_State::NeedToComputeSet) {
                // Continue reanneal
                this->reanneal_complete();
                this->state = Anneal_State::NeedToStep;
            }
            if (this->stop_check()) {
                this->state = Anneal_State::ReadyToStop;
                return;
            }
            this->cooling_schedule();
            this->acceptance_check();
            this->generate_next();
            ++this->k;
            ++this->k_r;
            if (this->reanneal_test()) {
                this->state = Anneal_State::NeedToComputeSet;
            } else {
                this->state = Anneal_State::NeedToCompute;
            }
        }

    protected: // Internal algorithm methods

        //! Generate a parameter set starting from _x_start
        morph::vVector<T> generate_parameter (const morph::vVector<T>& _x_start) const
        {
            morph::vVector<T> _x;
            bool generated = false;
            while (!generated) {
                morph::vVector<T> u(this->D);
                u.randomize();
                morph::vVector<T> u2 = ((u*T{2}) - T{1}).abs();
                morph::vVector<T> sigu = (u-T{0.5}).signum();
                morph::vVector<T> y = sigu * this->temp * ( ((T{1}/this->temp)+T{1}).pow(u2) - T{1} );
                _x = _x_start + y;
                if (_x <= this->range_max && _x >= this->range_min) {
                    generated = true;
                } // else we'll re-generate u and y
            }
            return _x;
        }

        //! A function to generate a new set of parameters
        void generate_next() { this->x_cand = this->generate_parameter (this->x); }

        //! The cooling schedule function
        void cooling_schedule()
        {
            this->temp = this->temp_0 * (-this->c * std::pow(this->k, T{1}/D)).exp();
            this->temp_cost = this->temp_cost_0 * (-this->c_cost * std::pow(this->num_accepted, T{1}/D)).exp();
            //std::cout << "(cooling_schedule) temp = " << temp << " temp_cost = " << temp_cost <<  std::endl;
        }

        //! The acceptance function
        bool acceptance_check()
        {
            bool candidate_is_better = false;
            if ((this->downhill == true && this->f_x_cand < this->f_x)
                || (this->downhill == false && this->f_x_cand > this->f_x)) {
                // In this case, the objective function of the candidate has improved
                candidate_is_better = true;
                ++this->num_improved;
            } else {
                ++this->num_worse;
            }

            T p = std::exp(-(this->f_x_cand - this->f_x)/(std::numeric_limits<T>::epsilon()+this->temp_cost.mean()));
            T u = this->rng_u.get();
            bool accepted = p > u ? true : false;

            if (candidate_is_better==false && accepted==true) { ++this->num_worse_accepted; }

            if (accepted) {
                this->x = this->x_cand;
                this->f_x = this->f_x_cand;
                this->param_hist.push_back (this->x);
                this->f_param_hist.push_back (this->f_x);
                this->f_x_best_repeats += this->f_x_cand == this->f_x_best ? 1 : 0;
                // Note reset of f_x_best_repeats if f_x_cand is better than f_x_best
                this->x_best = this->f_x_cand < this->f_x_best ? this->f_x_best_repeats=0, this->x_cand : this->x_best;
                this->f_x_best = this->f_x_cand < this->f_x_best ? this->f_x_cand : this->f_x_best;
                this->num_accepted++;
            }

            if constexpr (debug) {
                std::cout << "Candidate is " << (candidate_is_better ? "B  ": "W/S") <<  ", p = " << p
                          << ", accepted? " << (accepted ? "Y":"N") << " k_cost(num_accepted)=" << num_accepted << std::endl;
            }
            return accepted;
        }

        //! Test for a reannealing. If reannealing is required, sample some parameters
        //! that will need to be computed by the client's objective function
        bool reanneal_test()
        {
            // Return if it's not yet time to reanneal
            if ((this->k_r < this->reanneal_after_steps)
                && (this->accepted_vs_generated() >= this->acc_gen_reanneal_ratio)) {
                return false;
            }
            // Get ready to compute partials. From current location in parameter space
            // (this->x), generate some new parameter sets and get ready to
            this->x_set.resize (this->partials_samples);
            this->f_x_set.resize (this->partials_samples);
            for (unsigned int ps = 0; ps < this->partials_samples; ++ps) {
                this->x_set[ps] = this->generate_parameter (this->x);
            }
            if constexpr (debug) { std::cout << "Reannealing...\n"; }
            return true;
        }

        //! Complete the reannealing process. Based on the objectives in f_x_set,
        //! compute partials and modify k and temp.
        void reanneal_complete()
        {
            // Compute the average dL/dx and place in partials
            this->partials.zero();
            for (unsigned int i = 0; i < this->partials_samples; ++i) {
                this->partials += (f_x_set[i] - f_x) / (x_set[i] - x);
            }
            this->partials /= this->partials_samples;
            // Check what we got.
            if (partials.has_nan_or_inf()) {
                throw std::runtime_error ("NaN or inf in partials");
            }
            if (partials.has_zero()) {
                // Objectives of the samples are all equal. Make no changes to k/T & return
                this->reset_stats();
                return;
            }

            this->s = -this->rdelta * this->partials; // (A-B) * dL/dx

            morph::vVector<T> temp_re = this->temp * (this->s.max()/this->s);
            if constexpr (debug) {
                std::cout << "Reanneal. Temp changes from " << temp << " to " << temp_re << std::endl;
            }
            // catch temp being 0
            if (temp_re > T{0}) {
                unsigned int k_re = k;
                k_re = static_cast<unsigned int>(((this->temp_0/temp_re).log() / this->c).pow(D).mean());
                if constexpr (debug) {
                    std::cout << "Reanneal. k changes from " << k << " to " << k_re << std::endl;
                }
                this->k = k_re;
                this->temp = temp_re;
            } else {
                if constexpr (debug) {
                    std::cout << "Can't update k based on new temp, as it is <=0\n";
                }
            }

            this->reset_stats();
        }

        //! The algorithm's stopping condition
        bool stop_check() const
        {
            if constexpr (debug) {
                std::cout << "f_x_best_repeats = " << f_x_best_repeats << std::endl;
            }
            return this->f_x_best_repeats >= this->f_x_best_repeat_max ? true : false;
        }

        //! Compute number accepted vs. number generated based on currently stored stats
        T accepted_vs_generated() const
        {
            T avg = static_cast<T>(this->num_accepted) / (this->num_improved+this->num_worse);
            if constexpr (debug) {
                std::cout << "k=" << k << "; accepted vs generated ratio = " << avg << std::endl;
            }
            return avg;
        }

        //! Reset the statistics on the number of objective functions accepted etc
        void reset_stats()
        {
            this->num_improved = 0;
            this->num_worse = 0;
            this->num_worse_accepted = 0;
            this->num_accepted = 0;
            this->k_r = 0;
        }
    };

} // namespace morph
