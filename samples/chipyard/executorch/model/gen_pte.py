import torch
import torchvision.models as models

from torch.export import export, ExportedProgram
from torchvision.models.mobilenetv2 import MobileNet_V2_Weights
from torchvision.models.squeezenet import SqueezeNet1_0_Weights  # Import weights for SqueezeNet
from torchvision.models.mobilenetv3 import MobileNet_V3_Small_Weights  # Import weights for MobileNetV3-Small

from executorch.backends.xnnpack.partition.xnnpack_partitioner import XnnpackPartitioner
from executorch.exir import EdgeProgramManager, ExecutorchProgramManager, to_edge
from executorch.exir.backend.backend_api import to_backend

import argparse

parser = argparse.ArgumentParser()
parser.add_argument("--pte", type=str, default="model.pte", help="Path to output the PTE file.")
parser.add_argument("--model", type=str, choices=["mobilenet", "squeezenet", "lenet", "alexnet", "mobilenetv3small"],
                    default="mobilenet",
                    help="Choose the model to export: 'mobilenet' (default), 'squeezenet', 'lenet', 'alexnet', 'mobilenetv3small'.")
args = parser.parse_args()
pte_path = args.pte

print("Selected Model:", args.model)

if args.model == "mobilenet":
    model = models.mobilenetv2.mobilenet_v2(weights=MobileNet_V2_Weights.DEFAULT).eval()
    sample_inputs = (torch.randn(1, 3, 224, 224),)
elif args.model == "squeezenet":
    model = models.squeezenet1_0(weights=SqueezeNet1_0_Weights.DEFAULT).eval()
    sample_inputs = (torch.randn(1, 3, 224, 224),)
elif args.model == "alexnet":
    model = models.alexnet(weights=models.AlexNet_Weights.DEFAULT).eval()
    sample_inputs = (torch.randn(1, 3, 224, 224),)
elif args.model == "lenet":
    # Define a simple LeNet architecture.
    class LeNet(torch.nn.Module):
        def __init__(self):
            super(LeNet, self).__init__()
            self.conv1 = torch.nn.Conv2d(1, 6, kernel_size=5, stride=1)
            self.pool = torch.nn.MaxPool2d(2, 2)
            self.conv2 = torch.nn.Conv2d(6, 16, kernel_size=5, stride=1)
            self.fc1 = torch.nn.Linear(16 * 4 * 4, 120)
            self.fc2 = torch.nn.Linear(120, 84)
            self.fc3 = torch.nn.Linear(84, 10)
        def forward(self, x):
            x = torch.nn.functional.relu(self.conv1(x))
            x = self.pool(x)
            x = torch.nn.functional.relu(self.conv2(x))
            x = self.pool(x)
            x = x.view(x.size(0), -1)
            x = torch.nn.functional.relu(self.fc1(x))
            x = torch.nn.functional.relu(self.fc2(x))
            x = self.fc3(x)
            return x
    model = LeNet().eval()
    # LeNet expects a single-channel 28x28 image.
    sample_inputs = (torch.randn(1, 1, 28, 28),)
elif args.model == "mobilenetv3small":
    model = models.mobilenet_v3_small(weights=MobileNet_V3_Small_Weights.DEFAULT).eval()
    sample_inputs = (torch.randn(1, 3, 224, 224),)

exported_program: ExportedProgram = export(model, sample_inputs)
edge: EdgeProgramManager = to_edge(exported_program)

# Set up the partitioner (using XnnpackPartitioner as in the original code)
edge = edge.to_backend(XnnpackPartitioner())

exec_prog = edge.to_executorch()

with open(pte_path, "wb") as file:
    exec_prog.write_to_file(file)
