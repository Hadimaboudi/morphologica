/*
 * Bootstrapping stats
 */

#pragma once

#include <vector>
#include <morph/vec.h>
#include <morph/vvec.h>

namespace morph {

    template <typename T>
    struct bootstrap {

        static constexpr bool debug_bstrap = false;

        // Resample B sets from data and place them in resamples
        static void resample_with_replacement (const morph::vvec<T>& data,
                                               std::vector<morph::vvec<T>>& resamples, const unsigned int B)
        {
            unsigned int data_n = data.size();
            resamples.resize (B);
            for (unsigned int i = 0; i < B; ++i) {
                morph::vvec<T> resample (data_n);
                morph::vvec<unsigned int> randindices (data_n);
                randindices.randomize (0, data_n);
                for (unsigned int j = 0; j < data_n; ++j) {
                    resample[j] = data[randindices[j]];
                }
                resamples[i] = resample;
            }
        }

        // Compute a bootstapped standard error of the mean of the data with B resamples
        static T error_of_mean (const morph::vvec<T>& data, const unsigned int B)
        {
            std::vector<morph::vvec<T>> resamples;
            morph::bootstrap<T>::resample_with_replacement (data, resamples, B);
            morph::vvec<T> r_mean (B, T{0});
            for (unsigned int i = 0; i < B; ++i) {
                r_mean[i] = resamples[i].mean();
            }
            // Standard error is the standard deviation of the resample means
            return r_mean.std();
        }
        // std::vector version of error_of_mean
        static T error_of_mean (const std::vector<T>& data, const unsigned int B)
        {
            morph::vvec<T> vdata;
            vdata.set_from (data);
            return bootstrap<T>::error_of_mean (vdata, B);
        }

        // Compute a bootstapped standard error of the SD of the data with B resamples
        static T error_of_std (const morph::vvec<T>& data, const unsigned int B)
        {
            std::vector<morph::vvec<T>> resamples;
            morph::bootstrap<T>::resample_with_replacement (data, resamples, B);
            morph::vvec<T> r_std (B, T{0});
            for (unsigned int i = 0; i < B; ++i) {
                r_std[i] = resamples[i].std();
            }
            // Standard error of the statistic is the standard deviation of the resampled statistic
            return r_std.std();
        }
        // std::vector version of error_of_std
        static T error_of_std (const std::vector<T>& data, const unsigned int B)
        {
            morph::vvec<T> vdata;
            vdata.set_from (data);
            return bootstrap<T>::error_of_std (vdata, B);
        }

        // Compute a bootstrapped two sample t statistic as per algorithm 16.2
        // in Efron & Tibshirani.
        // zdata is treatment; ydata is control. B is the number of bootstrap samples to make
        //
        // This tests for equality of means, not whether the populations are identical and may be
        // used on distributions where the variances may not be equal. The null hypothesis is "the
        // means are the same".
        //
        // It returns a short array with 2 elements. Return element 0 is the 'achieved significance
        // level' (ASL) and element 1 is the minimum possible achieved significance level. The ASL
        // counts the number of transformed resamples that exceed the 'studentized difference of the
        // means of zdata and ydata'. The lower this value is, the less probably is the null
        // hypothesis that the means are the same. If this value is very low, then it is probable
        // that zdata and ydata were generated by different distributions and that the means are
        // different. If asl[0] is less than asl[1] then it should be 0. In this case, it's not
        // possible to determine the probability that the distributions have the same mean, only
        // that this probability is less than the min. ASL.
        //
        // For a tutorial description, see Appendix B of: B. Sen-Bhattacharya et al., "Building a
        // Spiking Neural Network Model of the Basal Ganglia on SpiNNaker," in IEEE Transactions on
        // Cognitive and Developmental Systems, vol. 10, no. 3, pp. 823-836, Sept. 2018, doi:
        // 10.1109/TCDS.2018.2797426.
        static morph::vec<T, 2> ttest_equalityofmeans (const morph::vvec<T>& _zdata, const morph::vvec<T>& _ydata, const unsigned int B)
        {
            // Ensure that the group which we name zdata is the larger one.
            morph::vvec<T> ydata = _ydata;
            morph::vvec<T> zdata = _zdata;
            if (_zdata.mean() <= _ydata.mean()) { std::swap (ydata, zdata); }

            unsigned int n = zdata.size();
            unsigned int m = ydata.size();

            T ymean = ydata.mean();
            T zmean = zdata.mean();
            if constexpr (debug_bstrap) {
                std::cout << "y mean: " << ymean << std::endl;
                std::cout << "z mean: " << zmean << std::endl;
            }

            // combine the data as if they were drawn from a single distribution
            morph::vvec<T> x = ydata;
            x.concat (zdata);
            T xmean = x.mean();
            if constexpr (debug_bstrap) { std::cout << "Combined mean: " << xmean << std::endl; }

            // Compute variances for the observed values:
            T obsvarz = (zdata-zmean).pow(2).sum() / (n-1);
            T obsvary = (ydata-ymean).pow(2).sum() / (m-1);

            // Compute the observed value of the studentised statistic (using separate
            // variances, rather than a pooled variance):
            T tobs = (zmean - ymean) / std::sqrt (obsvary/static_cast<T>(m) + obsvarz/static_cast<T>(n));
            if constexpr (debug_bstrap) { std::cout << "Observed value of studentized diff. of means: " << tobs << std::endl; }

            // Create shifted distributions; shifted by group mean and combined mean:
            morph::vvec<T> ztilda = zdata - zmean + xmean;
            morph::vvec<T> ytilda = ydata - ymean + xmean;
            if constexpr (debug_bstrap) {
                std::cout << "ztilda mean: " << ztilda.mean() << std::endl;
                std::cout << "ytilda mean: " << ytilda.mean() << std::endl;
            }

            // Resample from the shifted (tilda) distributions:
            std::vector<morph::vvec<T>> zstar;
            bootstrap<T>::resample_with_replacement (ztilda, zstar, B);
            if constexpr (debug_bstrap) {
                std::cout << "zstar size after resample: " << zstar.size() << std::endl;
            }
            std::vector<morph::vvec<T>> ystar;
            bootstrap<T>::resample_with_replacement (ytilda, ystar, B);

            // Create vectors of the means of these resamples:
            morph::vvec<T> zstarmeans (B, T{0});
            morph::vvec<T> ystarmeans (B, T{0});

            for (unsigned int i = 0; i < B; ++i) {
                zstarmeans[i] = zstar[i].mean();
                ystarmeans[i] = ystar[i].mean();
            }

            if constexpr (debug_bstrap) {
                std::cout << "zstarmeans of size " << zstarmeans.size() << " and content: " << zstarmeans << std::endl;
            }

            // Compute the variances.
            morph::vvec<T> zvariances (B, T{0});
            morph::vvec<T> yvariances (B, T{0});
            for (unsigned int i = 0; i < B; ++i) {
                if constexpr (debug_bstrap) {
                    //std::cout << "zstar[i] of size " << zstar[i].size() << " and content: " << zstar[i] << std::endl;
                    //std::cout << "zstar[i]-zstarmeans[i] = " << (zstar[i]-zstarmeans[i]) << std::endl;
                }
                zvariances[i] = (zstar[i]-zstarmeans[i]).pow(2).sum() / (n-1);
                yvariances[i] = (ystar[i]-ystarmeans[i]).pow(2).sum() / (m-1);
            }
            if constexpr (debug_bstrap) {
                std::cout << "zvariances: " << zvariances << std::endl;
            }

            morph::vvec<T> top = zstarmeans - ystarmeans;
            morph::vvec<T> bot = (yvariances/static_cast<T>(m) + zvariances/static_cast<T>(n)).sqrt();
            morph::vvec<T> txstar = top / bot;
            if constexpr (debug_bstrap) {
                std::cout << "txstar (compare with tobs=" << tobs << "): " << txstar << std::endl;
            }

            T numbeyond = (txstar.element_compare_gteq (tobs)).sum();
            if constexpr (debug_bstrap) {
                std::cout << "Number of txstar gt than or eq to tobs is " << numbeyond << std::endl;
            }

            auto asl = numbeyond / static_cast<T>(B);
            // The minimum achieved significance level for the number of resamples, B.
            T minasl = T{1} / static_cast<T>(B);

            return morph::vec<T, 2> ({asl, minasl});
        }
        // std::vector version of ttest_equalityofmeans()
        static morph::vec<T, 2> ttest_equalityofmeans (const std::vector<T>& _zdata, const std::vector<T>& _ydata, const unsigned int B)
        {
            morph::vvec<T> vzdata;
            vzdata.set_from (_zdata);
            morph::vvec<T> vydata;
            vydata.set_from (_ydata);
            return bootstrap<T>::ttest_equalityofmeans (vzdata, vydata, B);
        }
    };
}
