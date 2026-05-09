import numpy as np
import torch
import trimesh

from torch.utils.data import Dataset


class SingleObjectDataset(Dataset):
    def __init__(self, file_path, num_points: int | None = None):
        self.num_points = num_points

        geometry = trimesh.load(file_path)
        # If geometry is a mesh, get vertices, if it is a point cloud, get points
        if hasattr(geometry, "vertices"):
            points = np.asarray(geometry.vertices).astype(np.float32)
        else:
            points = np.asarray(geometry.points).astype(np.float32)

        self.points = torch.from_numpy(points)

        # Center based on the bounding box center, not the average of points
        min = torch.min(self.points, dim=0)[0]
        max = torch.max(self.points, dim=0)[0]
        center = (min + max) / 2.0
        self.points -= center

        # Scale to unit sphere
        dist = torch.max(torch.sqrt(torch.sum(self.points**2, dim=1)))
        self.points /= dist + 1e-9

    def __len__(self):
        return 512

    def __getitem__(self, index: int) -> tuple[torch.Tensor, int]:
        vertices = self.points

        # Sample or pad to reach fixed number of points
        if self.num_points is not None:
            if vertices.shape[0] >= self.num_points:
                indices = torch.randperm(vertices.shape[0])[: self.num_points]
                vertices = vertices[indices]
            else:
                pad_size = self.num_points - vertices.shape[0]
                indices = torch.randint(0, vertices.shape[0], (pad_size,))
                vertices = torch.cat([vertices, vertices[indices]], dim=0)

        return vertices, 0
