import argparse
import mcubes
import numpy as np
import torch
import trimesh

from neural_pull import NeuralPull


def reconstruct_neural_pull(
    model_path: str, num_shapes: int, shape_index: int, resolution: int = 128, threshold: float = 0.0
) -> trimesh.Trimesh:
    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")

    # Initialize Model with num_shapes (must match training)
    model = NeuralPull(num_shapes=num_shapes).to(device)

    # Load weights
    model.load_state_dict(torch.load(model_path, map_location=device, weights_only=True))
    model.eval()

    # Create latent index
    latent_index = torch.tensor([shape_index], device=device)

    # Create a 3D grid of coordinates
    # Use [-1.1, 1.1] to capture the full object
    x = np.linspace(-1.1, 1.1, resolution)
    y = np.linspace(-1.1, 1.1, resolution)
    z = np.linspace(-1.1, 1.1, resolution)
    grid_x, grid_y, grid_z = np.meshgrid(x, y, z, indexing="ij")

    # Flatten to (N, 3)
    grid_points = np.stack([grid_x.ravel(), grid_y.ravel(), grid_z.ravel()], axis=1)
    grid_points = torch.from_numpy(grid_points).float().to(device)

    # Query the model in chunks
    sdf_values = []
    chunk_size = 50000
    with torch.no_grad():
        for i in range(0, grid_points.shape[0], chunk_size):
            # (1, Chunk, 3) to satisfy batch requirement
            chunk = grid_points[i : i + chunk_size].unsqueeze(0)

            # Forward pass
            sdf = model(chunk, latent_index)
            sdf_values.append(sdf.cpu().numpy().flatten())

    sdf_values = np.concatenate(sdf_values).reshape(resolution, resolution, resolution)
    # Marching cubes to extract the iso-surface
    vertices, faces = mcubes.marching_cubes(sdf_values, threshold)

    # Rescale vertices back from voxel indices [0, res] to [-1.1, 1.1]
    step = 2.2 / (resolution - 1)
    vertices = vertices * step - 1.1

    return trimesh.Trimesh(vertices=vertices, faces=faces)


def main() -> None:
    # Parse arguments
    parser = argparse.ArgumentParser(
        prog="Inference surface reconstruction", description="Inference script for surface reconstruction using ML"
    )
    parser.add_argument("model", type=str, choices=["neural_pull"], help="Selected ML model")
    parser.add_argument("-mp", "--model-path", type=str, required=True, help="Path to .pth file")
    parser.add_argument("-ns", "--num-shapes", type=int, default=1, help="Total shapes in the trained checkpoint")
    parser.add_argument("-si", "--shape-index", type=int, default=0, help="Index of the shape to reconstruct")
    parser.add_argument("-i", "--input", type=str, required=False, help="Input point cloud")
    parser.add_argument("-o", "--output", type=str, required=True, help="Path to reconstructed mesh")

    # NeuralPull parameters
    parser.add_argument(
        "-r", "--resolution", type=int, default=128, help="Resolution of Voxel grid used by marching cubes"
    )

    args = parser.parse_args()

    print(f"Started inference")
    print(f"PyTorch version: {torch.__version__}")
    print(f"Is CUDA available: {torch.cuda.is_available()}")

    if args.model == "neural_pull":
        mesh = reconstruct_neural_pull(
            model_path=args.model_path,
            num_shapes=args.num_shapes,
            shape_index=args.shape_index,
            resolution=args.resolution,
        )
        mesh.export(args.output)
        mesh.show()
    else:
        print("Invalid model")

    print("Finished inference")


if __name__ == "__main__":
    main()
