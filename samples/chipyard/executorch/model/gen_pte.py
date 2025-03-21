import torch
import torchvision.models as models

from torch.export import export, ExportedProgram
from torchvision.models.mobilenetv2 import MobileNet_V2_Weights

# from executorch.backends.xnnpack.partition.xnnpack_partitioner import XnnpackPartitioner
from executorch.exir import EdgeProgramManager, ExecutorchProgramManager, to_edge
from executorch.exir.backend.backend_api import to_backend

import argparse

parser = argparse.ArgumentParser()
parser.add_argument("--pte", type=str, default="model.pte")
args = parser.parse_args()
pte_path = args.pte

mobilenet_v2 = models.mobilenetv2.mobilenet_v2(
    weights=MobileNet_V2_Weights.DEFAULT
).eval()
sample_inputs = (torch.randn(1, 3, 224, 224),)

exported_program: ExportedProgram = export(mobilenet_v2, sample_inputs)
edge: EdgeProgramManager = to_edge(exported_program)

# edge = edge.to_backend(XnnpackPartitioner())

exec_prog = edge.to_executorch()

with open(pte_path, "wb") as file:
    exec_prog.write_to_file(file)
