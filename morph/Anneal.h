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
#include <string>
#include <iostream>
#include <morph/MathAlgo.h>
#include <morph/vVector.h>
#include <morph/Vector.h>
#include <morph/Random.h>
#include <morph/HdfData.h>

namespace morph {

    //! What state is an instance of the Anneal class in?
    enum class Anneal_State
    {
        // The state is unknown
        Unknown,
        // Client code needs to call the init() function to setup parameters
        NeedToInit,
        // Client code should call step() to perform a step of the annealing algo
        NeedToStep,
        // Client code needs to compute the objective of the candidate
        NeedToCompute,
        // Client needs to compute the objectives of a set of parameter sets, x_set
        NeedToComputeSet,
        // The algorithm has finished
        ReadyToStop
    };

    //! Which stopping condition caused exit?
    enum class Anneal_StopCondition
    {
        Unknown,
        T_k_less_than_T_f,
        T_k_less_than_epsilon,
        T_cost_less_than_epsilon,
        f_x_best_repeated
    };

    /*!
     * A class implementing Lester Ingber's Adaptive Simlulated Annealing Algorithm. The
     * design is similar to that in NM_Simplex.h; the client code creates an Anneal
     * object, sets parameters and then runs a loop, checking Anneal::state to tell it
     * when to compute a new value of the objective function from the parameters
     * generated by the Anneal class. Anneal::state also tells the client code when the
     * algorithm has finished.
     *
     * \tparam T The type for the numbers in the algorithm. Expected to be floating
     * point, so float or double.
     */
    template <typename T>
    class Anneal
    {
        // Set false to hide text output
        static constexpr bool debug = false;
        static constexpr bool debug2 = false;
        // Use a short version of the numeric_limits epsilon
        static constexpr T eps = std::numeric_limits<T>::epsilon();
        // Used in reanneal_test(). A reanneal won't occur within 10 steps of the last reanneal.
        static constexpr unsigned int min_steps_to_reanneal = 10;

    public: // Algorithm parameters to be adjusted by user before calling Anneal::init()

        //! By default we *descend* to the *minimum* metric value of the user's
        //! objective function. Set this to false (before calling init()) to instead
        //! ascend to the maximum metric value.
        bool downhill = true;
        //! Lester's Temperature_Ratio_Scale (same name in the ASA C code). Related to
        //! m=-log(temperature_ratio_scale).
        T temperature_ratio_scale = T{1e-5};
        //! Lester's Temperature_Anneal_Scale in ASA C code. n=log(temperature_anneal_scale).
        T temperature_anneal_scale = T{100};
        //! Lester's Cost_Parameter_Scale_Ratio (used to compute T_cost).
        T cost_parameter_scale_ratio = T{1};
        //! If accepted_vs_generated is less than this, reanneal.
        T acc_gen_reanneal_ratio = T{1e-6};
        //! To compute tangents of cost fn near a point x, find cost at (1+/-delta_param)*x
        T delta_param = T{0.01};
        //! If the f_x_cand and within this precision of f_x_best, then f_x_best is deemed to have been repeated.
        T objective_repeat_precision = eps;
        //! How many times to find the same f_x_best objective before considering that
        //! the algorithm has finished.
        unsigned int f_x_best_repeat_max = 10;
        //! If false, don't reanneal
        bool enable_reanneal = true;
        //! If it has been this many steps since the last reanneal, reanneal again, even
        //! if the test of the accepted vs. generated ratio does not yet indicate a reanneal.
        unsigned int reanneal_after_steps = 100;
        //! Exit when T_i(k) reaches T_f
        bool exit_at_T_f = false;
        // Show a line of the current temperatures?
        bool display_temperatures = true;
        // Display info on reannealing?
        bool display_reanneal = true;

    public: // Parameter vectors and objective fn results need to be client-accessible.

        //! Allow user to set parameter names, so that these can be saved out
        std::vector<std::string> param_names;
        //! Candidate parameter values. In the Ingber papers, these are 'alphas'.
        morph::vVector<T> x_cand;
        //! Value of the objective function for the candidate parameters.
        T f_x_cand = T{0};
        //! The currently accepted parameters.
        morph::vVector<T> x;
        //! Value of the objective function for the current parameters.
        T f_x = T{0};
        //! The best parameters so far.
        morph::vVector<T> x_best;
        //! The value of the objective function for the best parameters.
        T f_x_best = T{0};
        //! How many times has this best objective repeated? Reset on reanneal.
        unsigned int f_x_best_repeats = 0;
        //! A special set of parameters to ask the user to compute (when computing reanneal).
        morph::vVector<T> x_plusdelta;
        //! The set of objective function values for x_set.
        T f_x_plusdelta = T{0};

    public: // Statistical records and state.

        //! Count of all generated parameter sets during the entire optimisation
        unsigned int num_generated = 0;
        //! Number of generated parameter sets at the last best set.
        unsigned int num_generated_best = 0;
        //! Count of recently generated; reset on reanneal or when new best is found.
        unsigned int num_generated_recently = 0;

        //! Number of candidates (x_cand) that are improved vs x (descents, if downhill is true).
        unsigned int num_improved = 0;
        //! Number of candidates that are worse during the entire optimisation
        unsigned int num_worse = 0;
        //! The number of acceptances of worse candidates during the entire optimisation
        unsigned int num_worse_accepted = 0;

        //! A count of ALL the accepted parameter sets; this one never gets reset. Same as in asa.c. Could instead use param_hist_accepted.size().
        unsigned int num_accepted = 0;
        //! Holds the value of num_accepted when the last best parameter set was found.
        unsigned int num_accepted_best = 0; // in asa.c: best_number_accepted_saved
        //! The number of accepted parameter sets 'recently' which is reset on reanneal or when a new best set is found.
        unsigned int num_accepted_recently = 0;

        //! Absolute count of number of calls to ::step().
        unsigned int steps = 0;
        //! A history of all accepted parameters evaluated
        morph::vVector<morph::vVector<T>> param_hist_accepted;
        //! For each entry in param_hist, record also its objective function value.
        morph::vVector<T> f_param_hist_accepted;
        //! History of rejected parameters. For num_rejected, use param_hist_rejected.size().
        morph::vVector<morph::vVector<T>> param_hist_rejected;
        //! Objective function values of rejected parameters.
        morph::vVector<T> f_param_hist_rejected;

        //! The state tells client code what it needs to do next.
        Anneal_State state = Anneal_State::Unknown;
        //! The stopping condition is recorded
        Anneal_StopCondition reason_for_exit = Anneal_StopCondition::Unknown;

    public: // Internal algorithm parameters, but public so it's easy to make graphs

        //! The number of dimensions in the parameter search space. Set by constructor
        //! to the number of dimensions in the initial parameters.
        unsigned int D = 0;
        //! k is the symbol Lester uses for the step count.
        unsigned int k = 1;
        //! The expected final 'step count'. Computed.
        unsigned int k_f = 0;
        //! A count of the number of steps since the last reanneal. Allows reannealing
        //! to be forced every reanneal_after_steps steps.
        unsigned int k_r = 0;
        //! The temperatures, T_i(k). Note that there is a temperature for each of D dimensions.
        morph::vVector<T> T_k;
        //! Initial temperatures T_i(0). Set to 1.
        morph::vVector<T> T_0;
        //! Expected final T_i(k_f). Computed.
        morph::vVector<T> T_f;
        //! Internal ASA parameter, m=-log(temperature_ratio_scale). Note that there is
        //! an mi for each of D dimensions, though these are usually all set to the same
        //! value.
        morph::vVector<T> m;
        //! Internal ASA parameter, n=log(temperature_anneal_scale).
        morph::vVector<T> n;
        //! Internal control parameter, c = m exp(-n/D).
        morph::vVector<T> c;
        morph::vVector<T> c_cost;
        //! Initial value for T_cost
        morph::vVector<T> T_cost_0;
        //! Temperature used in the acceptance function.
        morph::vVector<T> T_cost;
        //! Number of accepted parameter sets. index_cost_acceptances in asa.c
        unsigned int k_cost = 0;
        //! Parameter ranges defining the portion of parameter space to search - [Ai, Bi].
        morph::vVector<T> range_min; // A
        morph::vVector<T> range_max; // B
        morph::vVector<T> rdelta;    // = range_max - range_min;
        morph::vVector<T> rmeans;    // = (range_max + range_min)/T{2};
        //! Holds the estimated rates of change of the objective vs. parameter changes
        //! for the current location, x, in parameter space.
        morph::vVector<T> tangents;
        //! The random number generator used in the acceptance_check function.
        morph::RandUniform<T> rng_u;

    public: // User-accessible methods.

        //! The constructor requires initial parameters and parameter ranges.
        Anneal (const morph::vVector<T>& initial_params,
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

            // Before ::init() is called, user may wish to manually change some
            // parameters, like temperature_ratio_scale.
            this->state = Anneal_State::NeedToInit;
        }

        //! After constructing, then setting parameters, the user must call init.
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
            this->T_0.resize (this->D, T{1});
            this->T_k.resize (this->D, T{1});

            // The m and n parameters
            this->m.resize (this->D);
            this->m.set_from (-std::log(this->temperature_ratio_scale));

            this->n.resize (this->D);
            this->n.set_from (std::log(this->temperature_anneal_scale));

            // Set the 'control parameter', c, from n and m
            this->c.resize (this->D, T{1});
            this->c = this->m * (-this->n/this->D).exp();

            this->T_f = this->T_0 * (-m).exp();
            this->k_f = static_cast<unsigned int>(this->n.exp().mean());

            this->tangents.resize (this->D, T{1});
            this->c_cost = this->c * cost_parameter_scale_ratio;
            this->T_cost_0 = this->c_cost;
            this->T_cost = this->c_cost;

            this->state = Anneal_State::NeedToCompute;
        }

        //! Advance the simulated annealing algorithm by one step.
        void step()
        {
            ++this->steps;

            if (this->stop_check()) {
                this->state = Anneal_State::ReadyToStop;
                return;
            }

            if (this->state == Anneal_State::NeedToComputeSet) {
                this->complete_reanneal();
                this->state = Anneal_State::NeedToStep;
            }

            this->cooling_schedule();
            this->acceptance_check();
            this->generate_next();
            ++this->k;
            ++this->k_r;

            if (this->enable_reanneal && this->reanneal_test()) {
                // If reanneal_test returns true, then we need to reanneal so set state
                // flag so that client code will compute a set of objective functions to
                // allow Anneal::complete_reanneal() to complete the reannealing.
                this->state = Anneal_State::NeedToComputeSet;
            } else {
                this->state = Anneal_State::NeedToCompute;
            }
        }

        //! Save optimization info/history into an HDF5 file.
        void save (const std::string& path) const
        {
            morph::HdfData data(path, morph::FileAccess::TruncateWrite);
            data.add_contained_vals ("/param_hist_accepted", this->param_hist_accepted);
            data.add_contained_vals ("/f_param_hist_accepted", this->f_param_hist_accepted);
            data.add_contained_vals ("/param_hist_rejected", this->param_hist_rejected);
            data.add_contained_vals ("/f_param_hist_rejected", this->f_param_hist_rejected);
            data.add_contained_vals ("/x_best", this->x_best);
            int i = 1;
            for (auto pn : this->param_names) {
                std::string s_name = "/param_name_" + std::to_string(i++);
                data.add_string (s_name.c_str(), pn);
            }
            data.add_val ("/f_x_best", this->f_x_best);
            data.add_val ("/num_generated", this->num_generated);
            data.add_val ("/num_worse", this->num_worse);
            data.add_val ("/num_worse_accepted", this->num_worse_accepted);
            data.add_val ("/num_improved", this->num_improved);
            data.add_val ("/num_generated_best", this->num_generated_best);
            data.add_val ("/num_accepted", this->num_accepted);
            data.add_val ("/num_accepted_best", this->num_accepted_best);
        }

    protected: // Internal algorithm methods.

        //! Generate delta parameter near to x_start, for cost tangent estimation
        morph::vVector<T> generate_delta_parameter (const morph::vVector<T>& x_start) const
        {
            // we do x_start*(1 + delta_param) or x_start*(1-delta_param). First try former.
            morph::vVector<T> plusminus (this->D, T{1});
            morph::vVector<T> x_new = x_start * (T{1} + plusminus * this->delta_param);
            // Check that each element of x_new is within the specified bounds.
            for (size_t i = 0; i < this->D; ++i) {
                if (x_new[i] > this->range_max[i] || x_new[i] < this->range_min[i]) {
                    plusminus[i] = T{-1};
                }
            }
            // Now re-compute x_new
            x_new = x_start * (T{1} + plusminus * this->delta_param);
            return x_new;
        }

        //! A function to generate a new set of parameters for x_cand.
        void generate_next()
        {
            morph::vVector<T> x_new;
            bool generated = false;
            while (!generated) {
                morph::vVector<T> u(this->D);
                u.randomize();
                morph::vVector<T> u2 = ((u*T{2}) - T{1}).abs();
                morph::vVector<T> sigu = (u-T{0.5}).signum();
                morph::vVector<T> y = sigu * this->T_k * ( ((T{1}/this->T_k)+T{1}).pow(u2) - T{1} );
                x_new = this->x + y;
                // Check that x_new is within the specified bounds
                if (x_new <= this->range_max && x_new >= this->range_min) { generated = true;  }
            }
            ++this->num_generated;
            ++this->num_generated_recently;
            this->x_cand = x_new;
        }

        //! The cooling schedule function updates temperatures on each step.
        void cooling_schedule()
        {
            // T_k (T_i(k) in the papers) affects parameter generation and drops as k
            // increases. 'current_user_parameter_temp' in asa.c.
            this->T_k = this->T_0 * (-this->c * std::pow(this->k, T{1}/D)).exp();
            this->T_k.max_elementwise_inplace (eps);

            // T_cost (T(k_cost) or 'acceptance temperature' in the papers) is used in
            // the acceptance function. 'current_cost_temperature' in asa.c.
            this->T_cost = this->T_cost_0 * (-this->c_cost * std::pow(this->k_cost, T{1}/D)).exp();
            this->T_cost.max_elementwise_inplace (eps);

            if (display_temperatures == true) {
                std::cout << "T_i(k="<<k<<"["<<k_f<<"]) = " << this->T_k.mean()
                          << " [T_f="<<this->T_f.mean() << "]; T_cost(n_acc="
                          << this->k_cost<<") = " << this->T_cost.mean()
                          << ", f_x_best = " << this->f_x_best << std::endl;
            }
        }

        //! The acceptance function determines if x_cand is accepted, copies x_cand to x
        //! and x_best as necessary, and updates statistical variables.
        void acceptance_check()
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

            T p = std::exp(-(this->f_x_cand - this->f_x)/(eps+this->T_cost.mean()));
            p = std::min (T{1}, p);
            T u = this->rng_u.get();
            bool accepted = p >= u ? true : false;

            if (candidate_is_better==false && accepted==true) { ++this->num_worse_accepted; }

            if (accepted) {
                // Increment k_cost etc
                ++this->k_cost;
                ++this->num_accepted;
                ++this->num_accepted_recently;
                // Increment f_x_best_repeats if f_x_cand is within a short distance of f_x_best:
                this->f_x_best_repeats += (std::abs(this->f_x_cand - this->f_x_best) <= this->objective_repeat_precision) ? 1 : 0;
                // Update x_best, etc if x_cand is better than x; if the candidate is
                // more than objective_repeat_precision better than f_x_best
                if ((this->downhill == true && (this->f_x_cand - this->f_x_best + this->objective_repeat_precision) < T{0})
                    || (this->downhill == false && (this->f_x_cand - this->f_x_best - this->objective_repeat_precision) > T{0})) {
                    this->f_x_best_repeats = 0;
                    this->x_best = this->x_cand;
                    this->num_accepted_best = this->num_accepted;
                    this->num_generated_best = this->num_generated;
                    this->num_accepted_recently = 0;
                    this->num_generated_recently = 0;
                    this->f_x_best = this->f_x_cand;
                }
                // x_cand becomes x
                this->x = this->x_cand;
                this->f_x = this->f_x_cand;
                // Store x onto the history
                this->param_hist_accepted.push_back (this->x);
                this->f_param_hist_accepted.push_back (this->f_x);

            } else {
                this->param_hist_rejected.push_back (this->x);
                this->f_param_hist_rejected.push_back (this->f_x);
            }

            if constexpr (debug) {
                std::cout << "Candidate is " << (candidate_is_better ? "B  ": "W/S") <<  ", p = " << p
                          << ", this->f_x_cand - this->f_x = " << (this->f_x_cand - this->f_x)
                          << ", accepted? " << (accepted ? "Y":"N") << " k_cost(k_cost)=" << k_cost << std::endl;
            }
        }

        //! Test for a reannealing. If reannealing is required, sample some parameters
        //! that will need to be computed by the client's objective function.
        bool reanneal_test()
        {
            // Don't reanneal too soon since the last reanneal
            if (this->k_r < min_steps_to_reanneal) { return false; }
            // Don't reanneal if the accepted:generated ratio is >= the threshold
            if ((this->k_r < this->reanneal_after_steps)
                && (this->accepted_vs_generated() >= this->acc_gen_reanneal_ratio)) {
                return false;
            }

            if (this->accepted_vs_generated() < this->acc_gen_reanneal_ratio) {
                this->num_accepted_recently = 0;
                this->num_generated_recently = 0;
            }

            // Set x back to x_best when reannealing.
            this->x = this->x_best;
            this->f_x = this->f_x_best;

            // add a delta to the current parameters and then ask client to compute f_x and f_x_plusdelta (instead of f_x_cand)
            this->x_plusdelta = this->generate_delta_parameter (this->x);

            if (display_reanneal) { std::cout << "Reannealing... "; }
            return true;
        }

        //! Complete the reannealing process. Based on the objectives in f_x and f_x_plusdelta,
        //! compute tangents and modify k and temp.
        void complete_reanneal()
        {
            // Compute dCost/dx and place in tangents
            this->tangents = (f_x_plusdelta - f_x) / (x_plusdelta - x + eps);

            if (tangents.has_nan_or_inf()) { throw std::runtime_error ("NaN or inf in tangents"); }

            if (tangents.has_zero()) {
                // The delta_param factor wasn't sufficient to bring about any change in
                // the objective function, so double it and return.
                if (display_reanneal) {
                    std::cout << "Tangents had a zero, so double delta_param from "
                              << this->delta_param << " to " << (this->delta_param * T{2}) << std::endl;
                }
                this->delta_param *= T{2};
                return;
            }

            morph::vVector<T> abs_tangents = tangents.abs();
            T max_tangent = abs_tangents.max();
            for (auto& t : abs_tangents) {
                if (t < eps) { t = max_tangent; } // Will ensure T_re won't update for this one
            }

            // Update parameter temperature and k
            morph::vVector<T> T_re = (this->T_k * (max_tangent / tangents)).abs();

            if (T_re > T{0}) {
                unsigned int k_re = static_cast<unsigned int>(((this->T_0/T_re).log() / this->c).pow(D).mean());
                if (display_reanneal) {
                    std::cout.precision(5);
                    std::cout << "Done. T_i(k): " << T_k.mean() << " --> " << T_re.mean()
                              << " and k: " << k << " --> " << k_re << std::endl;
                }
                this->k = k_re;
                this->T_k = T_re;
            } else { // temp should not be <=0
                throw std::runtime_error ("Can't update k based on new temp, as it is <=0\n");
            }

            // Also update the cost temperature, T_cost and k_cost.

            // Reset initial cost temperature from f_x, f_x_best, their delta and the epsilon
            morph::vVector<T> T_cost_0_candidates = { f_x, f_x_best, (f_x_best - f_x), eps };
            T_cost_0_candidates.abs_inplace();
            this->T_cost_0.min_elementwise_inplace (T_cost_0_candidates.max());

            morph::vVector<T> T_cost_candidates = {std::abs(f_x_best - f_x), T_cost.max(), eps };
            morph::vVector<T> tmp_dbl1 = T_cost_0;
            tmp_dbl1.min_elementwise_inplace (T_cost_candidates.max());

            morph::vVector<T> tvdb3 = ((T_cost_0+eps)/tmp_dbl1).log().abs();
            this->k_cost = static_cast<unsigned int>(eps + (tvdb3/this->c_cost).pow(this->D).mean());
            // Note: asa.c code has opion to reduce this down if its too high.

            // From cooling_schedule:
            this->T_cost = this->T_cost_0 * (-this->c_cost * std::pow(this->k_cost, T{1}/D)).exp();
            this->T_cost.max_elementwise_inplace (eps);

            this->k_r = 0; // k_r is 'steps since reanneal'
        }

        //! The algorithm's stopping conditions.
        bool stop_check()
        {
            if (this->exit_at_T_f == true && this->T_k < this->T_f) {
                this->reason_for_exit = Anneal_StopCondition::T_k_less_than_T_f;
                std::cout << "T_k < T_f; stopping.\n";
                return true;
            }
            if (this->T_k[0] <= eps) {
                this->reason_for_exit = Anneal_StopCondition::T_k_less_than_epsilon;
                std::cout << "T_k < eps; stopping.\n";
                return true;
            }
            if (this->T_cost[0] <= eps) {
                this->reason_for_exit = Anneal_StopCondition::T_cost_less_than_epsilon;
                std::cout << "T_cost < eps; stopping.\n";
                return true;
            }
            if (this->f_x_best_repeats >= this->f_x_best_repeat_max) {
                this->reason_for_exit = Anneal_StopCondition::f_x_best_repeated;
                if (this->display_temperatures == true) {
                    std::cout << "f_x_best repeated " << this->f_x_best_repeat_max << " times; stopping.\n";
                }
                return true;
            }
            // Also: optional number_accepted limit and number_generated limit
            return false;
        }

        //! Compute & return number accepted vs. number generated based on currently stored stats.
        T accepted_vs_generated() const
        {
            return static_cast<T>(this->num_accepted_recently) + T{1} / (this->num_generated_recently + T{1});
        }
    };

} // namespace morph
