#include <torch/script.h> // One-stop header.
#include <vector>
#include "torch_runner.h"
#include <iostream>
std::string global_model_path;
typedef enum {
    normal = 1,
    neu_coordinate = 2,
    neu_sphere = 3
} render_type;
void set_model_path(const char* model_path)
{
    global_model_path = std::string(model_path);
}
static torch::jit::script::Module& get_model() {
    static torch::jit::script::Module module;
    static bool is_loaded = false;
    if (!is_loaded) {
        try {
            module = torch::jit::load(global_model_path);
            module.to(at::kCUDA);
            module.eval();
            is_loaded = true;
        }
        catch (const c10::Error& e) {
            std::cerr << "error loading the model\n";
            throw;
        }
    }
    return module;
}
void run_torch(double* para, int* res)
{
    torch::jit::script::Module& module = get_model();
    std::vector<float> input_vec;
    std::vector<torch::jit::IValue> inputs;
    torch::Tensor input_tensor;
    if (int(para[0]) == neu_coordinate)
    {
        para++;
        for (int i = 0; i < 6; i++)
        {
            input_vec.push_back(*para);
            para++;
        }
        inputs.push_back(torch::tensor({ {input_vec[0], input_vec[1], input_vec[2], input_vec[3],input_vec[4],input_vec[5]} }, torch::kFloat).to(at::kCUDA));

    }
    else if (int(para[0]) == neu_sphere)
    {
        para++;
        for (int i = 0; i < 5; i++)
        {
            input_vec.push_back(*para);
            para++;
        }
        inputs.push_back(torch::tensor({ {input_vec[0], input_vec[1]} }, torch::kFloat).to(at::kCUDA));
    }
    // inputs.push_back(input_tensor);
    at::Tensor output = module.forward(inputs).toTensor().to(torch::kCPU);
    auto tmp = output.data_ptr<float>();
    std::vector<double> output_vector(output.data_ptr<float>(), output.data_ptr<float>() + output.numel());
    res[0] = int(output_vector[0]);
    if (res[0] > 255)
        res[0] = 255;
    if (res[0] < 0)
        res[0] = 0;
    res[1] = int(output_vector[1]);
    if (res[1] > 255)
        res[1] = 255;
    if (res[1] < 0)
        res[1] = 0;
    res[2] = int(output_vector[2]);
    if (res[2] > 255)
        res[2] = 255;
    if (res[2] < 0)
        res[2] = 0;
}
