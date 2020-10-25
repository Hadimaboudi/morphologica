#include <morph/nn/ElmanNet.h>
#include <iostream>
#include <morph/Random.h>

// Prepare XOR sequence (xs) and prediction sequence (ps). The XOR sequence is computed
// as in Elman, 1990. Create two bits at random, then xor the bits to make the third
// bit. Create another two bits at random and then insert the XOR of these as the sixth
// bit. Repeat xs_trips times. The sequence ps[i] is simply the value of xs[i+1].
void generateInput (size_t xs_trips, morph::vVector<float>& xs, morph::vVector<float>& ps)
{
    xs.resize (xs_trips*3, 0.0f);
    ps.resize (xs_trips*3, 0.0f);
    morph::RandUniform<unsigned long long int> rng;
    size_t j = 0;
    for (size_t i = 0; i < xs_trips;) {
        // Can get 64 bits of random at a time:
        unsigned long long int left = rng.get();
        unsigned long long int right = rng.get();
        unsigned long long int xor_ = left ^ right;
        // Get the relevant bits from the random number generator and insert into xs
        for (size_t ii = 0; i<xs_trips && ii<64; ii++,i++) {
            unsigned long long int l = left >> ii & 0x1;
            unsigned long long int r = right >> ii & 0x1;
            unsigned long long int x = xor_ >> ii & 0x1;
            if (j) { ps[j-1] = l > 0 ? 1.0f : 0.0f; }
            xs[j++] = l > 0 ? 1.0f : 0.0f;

            ps[j-1] = r > 0 ? 1.0f : 0.0f;
            xs[j++] = r > 0 ? 1.0f : 0.0f;

            ps[j-1] = x > 0 ? 1.0f : 0.0f;
            xs[j++] = x > 0 ? 1.0f : 0.0f;
        }
    }
}

int main()
{
    //
    // Create an Elman style feed-forward network with context layers
    //
    std::vector<unsigned int> layer_spec = {1,2,1};
    morph::nn::ElmanNet<float> el1(layer_spec);

    //
    // Prepare the input
    //
    size_t xs_trips = 1000;   // Number of 'triplets' in the XOR sequence (triplet comprises 'in', 'in' and 'correct out' bits.
    morph::vVector<float> xs; // The XOR sequence container
    morph::vVector<float> ps; // The prediction sequence container
    generateInput (xs_trips, xs, ps);

    // Accumulate the dC/dw and dC/db values. for each pair, the first is
    // nabla_w the second is nabla_b. There are as many pairs as there are connections
    // in ff1. Here, we declare and initialize gradients
    std::vector<std::pair<std::vector<morph::vVector<float>>, morph::vVector<float>>> gradients;
    for (auto& c : el1.connections) {
        gradients.push_back (std::make_pair(c.nabla_ws, c.nabla_b));
    }

    //
    // Train the network
    //
    // A learning rate
    float eta = 0.1f;
    // How many times to run through the data stream?
    size_t epochs = 600;
    // Containers to pass as input and desired output. Values from xs and ps are copied into these, shorter vectors.
    morph::vVector<float> input = {1};
    morph::vVector<float> des_output = {1};
    // For each epoch run through the length of xs/ps
    for (size_t ep = 0; ep<epochs; ++ep) {
        for (size_t i = 0; i < xs.size(); ++i) {

            // Zero the gradient containers
            size_t gi = 0;
            for (gi = 0; gi < el1.connections.size(); ++gi) {
                for (size_t j = 0; j < gradients[gi].first.size(); ++j) {
                    gradients[gi].first[j].zero();
                }
                gradients[gi].second.zero();
            }

            // Set the new input and desired output
            input[0] = xs[i];
            des_output[0] = ps[i];
            el1.setInput (input, des_output);

            // Compute the network fowards
            el1.feedforward();
            el1.computeCost();

            // Back propagate the error to find the gradients of the weights and biases
            el1.backprop();

            // Copy gradients from the network into the container (this would be more relevant if we were doing batches)
            gi = 0;
            for (auto& c : el1.connections) {
                for (size_t j = 0; j < gradients[gi].first.size(); ++j) {
                    gradients[gi].first[j] += c.nabla_ws[j];
                }
                gradients[gi].second += c.nabla_b;
                ++gi;
            }

            // Examine a single epoch:
            //std::cout << ep << "," << i << "," << cost << "," << input[0] << "," << des_output[0] << std::endl;

            // perform the gradient update. v -> v' = v - eta * gradC
            gi = 0;
            for (auto& c : el1.connections) {
                // for each in weights:
                for (size_t j = 0; j < gradients[gi].first.size(); ++j) {
                    c.ws[j] -= (gradients[gi].first[j] * eta);
                }

                c.b -= (gradients[gi].second * eta);
                ++gi;
            }
        }
        //std::cout << ep << "," << xs.size() << "," << cost  << "," << input[0] << "," << des_output[0] << std::endl;
    }

    //
    // Evaluate the network
    //
    morph::vVector<float> costs(12);
    for (size_t i = 0; i < 1200; ++i) {
        // Set the new input and desired output
        input[0] = xs[i];
        des_output[0] = ps[i];
        el1.setInput (input, des_output);
        // Compute the network fowards
        el1.feedforward();
        float cost = el1.computeCost(); // Gets the squared error
        //std::cout << "cost[i%12(" << i%12 << ")] += " << cost << std::endl;
        costs[i%12] += cost; // sums the squared error
    }

    std::cout << "Dividing cost by " << (1200/12) << std::endl;
    costs /= (1200/12); // gets the mean of the squared error
    // square root of costs?
    costs.sqrt_inplace();
    std::cout << "costs: " << costs.str_mat() << std::endl;

    return 0;
}
