/* Copyright (c) 2018 PaddlePaddle Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. */

/*
 * This file contains demo of mobilenet for tensorrt.
 */

#include <gflags/gflags.h>
#include <glog/logging.h>  // use glog instead of CHECK to avoid importing other paddle header files.
#include <fstream>
#include <iostream>

// #include "paddle/fluid/platform/enforce.h"
#include "paddle/fluid/inference/demo_ci/utils.h"

#ifdef PADDLE_WITH_CUDA
DECLARE_double(fraction_of_gpu_memory_to_use);
#endif
DEFINE_string(modeldir, "", "Directory of the inference model.");
DEFINE_string(refer, "", "path to reference result for comparison.");
DEFINE_string(
    data, "",
    "path of data; each line is a record, format is "
    "'<space splitted floats as data>\t<space splitted ints as shape'");

namespace paddle {
namespace demo {

/*
 * Use the native fluid engine to inference the demo.
 */
void Main() {
  std::unique_ptr<PaddlePredictor> predictor;
  paddle::contrib::MixedRTConfig config;
  config.param_file = FLAGS_modeldir + "/__params__";
  config.prog_file = FLAGS_modeldir + "/__model__";
  config.use_gpu = true;
  config.device = 0;
  config.max_batch_size = 1;
  config.fraction_of_gpu_memory = 0.1;  // set by yourself
  predictor = CreatePaddlePredictor<paddle::contrib::MixedRTConfig>(config);

  VLOG(3) << "begin to process data";
  // Just a single batch of data.
  std::string line;
  std::ifstream file(FLAGS_data);
  std::getline(file, line);
  auto record = ProcessALine(line);
  file.close();

  // Inference.
  PaddleTensor input;
  input.shape = record.shape;
  input.data =
      PaddleBuf(record.data.data(), record.data.size() * sizeof(float));
  input.dtype = PaddleDType::FLOAT32;

  VLOG(3) << "run executor";
  std::vector<PaddleTensor> output;
  predictor->Run({input}, &output, 1);

  VLOG(3) << "output.size " << output.size();
  auto& tensor = output.front();
  VLOG(3) << "output: " << SummaryTensor(tensor);

  // compare with reference result
  CheckOutput(FLAGS_refer, tensor);
}

}  // namespace demo
}  // namespace paddle

int main(int argc, char** argv) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  paddle::demo::Main();
  return 0;
}
