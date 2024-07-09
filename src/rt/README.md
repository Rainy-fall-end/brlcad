## Dependence
[libtorch](https://pytorch.org/cppdocs/installing.html)
used for loading model
do remember to change the absolute path in:src/rt/CMakeLists.txt Line 56-58
[nlohmann](https://github.com/nlohmann/json)
there is no need to install, it is just a "header-only" library
used for write json file
## Project structure
Add two projects:
### rt_trainneural
used for sampling and rendering
new files:train_neural.cpp(main) rt/neu_util.h rt/rt_trainer.h
neural.c rt_trainer.cpp

see train_neural.cpp for usage
### torch_runner
used for run torch model
