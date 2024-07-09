#ifndef RT_TORCH_RUNNER_H
#define RT_TORCH_RUNNER_H
void set_model_path(const char* model_path);
void set_model_type(int num);
typedef enum {
    normal = 1,
    neu_coordinate = 2,
    neu_sphere = 3
} render_type;
#ifdef __cplusplus
extern "C" {
#endif

	void run_torch(double* para, int* res);

#ifdef __cplusplus
}
#endif

#endif // !RT_TORCH_RUNNER_H