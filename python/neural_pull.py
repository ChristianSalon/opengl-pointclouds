import numpy as np
import torch
import torch.nn as nn
import torch.nn.functional as F


def pull_loss(
    model: nn.Module, query_points: torch.Tensor, cloud_gt: torch.Tensor, latent_vector: torch.Tensor
) -> torch.Tensor:
    sdf = model(query_points, latent_vector)

    grad = torch.autograd.grad(
        outputs=sdf,
        inputs=query_points,
        grad_outputs=torch.ones_like(sdf),
        create_graph=True,
        retain_graph=True,
        only_inputs=True,
    )[0]
    grad_norm = F.normalize(grad, p=2, dim=-1)

    # Pulling equation
    # t = q - f(q) * grad / ||grad||
    pulled_points = query_points - sdf * grad_norm

    # L2 distance to ground truth
    loss = F.mse_loss(pulled_points, cloud_gt)
    return loss


class NeuralPull(nn.Module):
    def __init__(self, num_shapes: int, d_in: int = 3, d_hidden: int = 512):
        super(NeuralPull, self).__init__()

        self.latent_embed = nn.Embedding(num_shapes, 128)

        # Input (3) + Latent (128) -> 512
        self.first_layer = nn.Linear(d_in + 128, d_hidden)

        # 8 ResNet blocks
        self.blocks = nn.ModuleList([nn.Linear(d_hidden, d_hidden) for _ in range(8)])

        # SDF output
        self.out_layer = nn.Linear(d_hidden, 1)

        self._initialize_weights()

    def _initialize_weights(self) -> None:
        # Standard He Initialization for hidden layers
        for m in self.blocks:
            nn.init.normal_(m.weight, mean=0.0, std=np.sqrt(2) / np.sqrt(512))
            nn.init.constant_(m.bias, 0.0)

        # Geometric Initialization
        # Forces the network to start as a sphere of radius 0.5
        with torch.no_grad():
            nn.init.normal_(self.out_layer.weight, mean=2 * np.sqrt(np.pi) / np.sqrt(512), std=0.000001)
            nn.init.constant_(self.out_layer.bias, -0.5)

    def forward(self, x, latent_indices):
        # feat shape: (B, 128)
        feat = F.relu(self.latent_embed(latent_indices))
        # Expand feat to (B, N, 128) to match x (B, N, 3)
        feat = feat.unsqueeze(1).expand(-1, x.size(1), -1)

        x = torch.cat([x, feat], dim=-1)
        x = F.relu(self.first_layer(x))

        # ResNet blocks
        for block in self.blocks:
            identity = x
            x = F.relu(block(x))
            x = x + identity

        return self.out_layer(x)
