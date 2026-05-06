import os
import torch

from torch.utils.data import Dataset


def read_off(file: str):
    with open(file, "r") as f:
        line = f.readline().strip()
        if "OFF" != line:
            # OFF and header is on one line
            header = line[3:].split()
            if not header:
                header = f.readline().split()
        else:
            header = f.readline().split()

        vertex_count, _, _ = [int(s) for s in header]
        vertices = [[float(s) for s in f.readline().split()] for _ in range(vertex_count)]

        return torch.tensor(vertices, dtype=torch.float32)


class ModelNet40(Dataset):
    def __init__(
        self,
        root_dir: str,
        categories: list[str] = [],
        mode: str = "train",
        num_points: int | None = None,
    ):
        self.root_dir = root_dir
        self.categories = categories
        self.mode = mode
        self.num_points = num_points

        if len(self.categories) == 0:
            # Use all categories
            self.categories = sorted([d for d in os.listdir(root_dir) if os.path.isdir(os.path.join(root_dir, d))])

        self.files = []
        for cat in self.categories:
            cat_folder = os.path.join(root_dir, cat, mode)
            for file in os.listdir(cat_folder):
                if file.endswith(".off"):
                    self.files.append({"path": os.path.join(cat_folder, file), "id": len(self.files)})

        self.num_instances = len(self.files)

    def __len__(self):
        return self.num_instances

    def __getitem__(self, index: int) -> tuple[torch.Tensor, int]:
        data = self.files[index]
        vertices = read_off(data["path"])

        # Sample or pad to reach fixed number of points
        if self.num_points is not None:
            if vertices.shape[0] >= self.num_points:
                indices = torch.randperm(vertices.shape[0])[: self.num_points]
                vertices = vertices[indices]
            else:
                pad_size = self.num_points - vertices.shape[0]
                indices = torch.randint(0, vertices.shape[0], (pad_size,))
                vertices = torch.cat([vertices, vertices[indices]], dim=0)

        # Center based on the bounding box center, not the average of points
        min = torch.min(vertices, dim=0)[0]
        max = torch.max(vertices, dim=0)[0]
        center = (min + max) / 2.0
        vertices -= center

        # Scale to unit sphere
        dist = torch.max(torch.sqrt(torch.sum(vertices**2, dim=1)))
        vertices /= dist + 1e-9

        return vertices, data["id"]
