#ifndef RT_TORCH_RUNNER_H
#define RT_TORCH_RUNNER_H
void set_model_path(const char* model_path);
#ifdef __cplusplus
extern "C" {
#endif

	void run_torch(double* para, int* res);

#ifdef __cplusplus
}
#endif
typedef enum {
    normal = 1,
    neu_coordinate = 2,
    neu_sphere = 3
} render_type;
#endif // !RT_TORCH_RUNNER_H