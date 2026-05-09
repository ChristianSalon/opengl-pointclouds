from scipy.spatial import cKDTree
import argparse
import numpy as np
import os
import torch

from model_net_40 import ModelNet40
from neural_pull import NeuralPull, pull_loss
from single_object_dataset import SingleObjectDataset


def get_adaptive_query_points(cloud_points: torch.Tensor, k: int = 50) -> torch.Tensor:
    # cloud_points shape: (B, N, 3)
    batch_size, num_pts, _ = cloud_points.shape
    cloud_np = cloud_points.detach().cpu().numpy()
    all_sigmas = []

    # Compute sigmas per object in the batch
    for b in range(batch_size):
        tree = cKDTree(cloud_np[b])
        dists, _ = tree.query(cloud_np[b], k=k)
        # Get distance to the k-th neighbor
        all_sigmas.append(dists[:, -1:])

    # Reshape sigmas to (B, N, 1)
    sigmas = torch.from_numpy(np.stack(all_sigmas)).to(cloud_points.device).float()

    # Scale based on point density
    scale = 0.25 * np.sqrt(num_pts / 20000)

    # Apply noise: (B, N, 3) * (B, N, 1)
    noise = torch.randn_like(cloud_points) * sigmas * scale
    return cloud_points + noise


def train_neural_pull(
    dataloader: torch.utils.data.DataLoader,
    num_shapes: int,
    epochs: int,
    save_path: str | None,
    resume_path: str | None = None,
) -> None:
    if save_path:
        os.makedirs(save_path, exist_ok=True)

    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")

    # ML model
    neural_pull = NeuralPull(num_shapes=num_shapes).to(device)

    if resume_path:
        print(f"Loading previous weights from: {resume_path}")
        neural_pull.load_state_dict(torch.load(resume_path, map_location=device, weights_only=True))

    # Optimization algorithm
    optimizer = torch.optim.Adam(neural_pull.parameters(), lr=1e-4)

    # Training
    for epoch in range(epochs):
        neural_pull.train()
        epoch_loss = 0.0

        for cloud_points, indices in dataloader:
            cloud_points = cloud_points.to(device)
            indices = indices.to(device)

            query_points = get_adaptive_query_points(cloud_points, k=50)
            query_points = query_points.to(device)
            query_points.requires_grad_(True)

            optimizer.zero_grad()
            loss = pull_loss(model=neural_pull, query_points=query_points, cloud_gt=cloud_points, latent_vector=indices)
            loss.backward()
            optimizer.step()

            epoch_loss += loss.item()

        avg_epoch_loss = epoch_loss / len(dataloader)
        print(f"Epoch {epoch}: average_loss={avg_epoch_loss:.9f}")

        torch.save(neural_pull.state_dict(), f"{f'{save_path}/' if save_path else ''}neural_pull_e{epoch}.pth")


def main() -> None:
    torch.backends.cudnn.benchmark = True

    # Parse arguments
    parser = argparse.ArgumentParser(
        prog="Training surface reconstruction", description="Training script for surface reconstruction using ML"
    )
    parser.add_argument("model", type=str, choices=["neural_pull"], help="Selected ML model")
    parser.add_argument("dataset", type=str, choices=["mn40", "single"], help="Selected dataset")
    parser.add_argument("-dp", "--dataset-path", type=str, help="Path to dataset")
    parser.add_argument("-c", "--categories", nargs="*", help="Modelnet 40 categories")
    parser.add_argument("-np", "--num-points", type=int, default=1028, help="Number of query points per model")
    parser.add_argument("-ns", "--num-shapes", type=int, default=1, help="Number of different model shapes")
    parser.add_argument("-b", "--batch-size", type=int, default=64, help="Batch size")
    parser.add_argument("-e", "--epochs", type=int, default=100, help="Number of epochs")
    parser.add_argument("-sp", "--save-path", type=str, help="Directory where to save trained NN")
    parser.add_argument("-r", "--resume", type=str, default=None, help="Path to .pth file to resume training")

    args = parser.parse_args()

    print(f"Started training")
    print(f"PyTorch version: {torch.__version__}")
    print(f"Is CUDA available: {torch.cuda.is_available()}")

    # Init dataset
    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    if args.dataset == "single":
        train_dataset = SingleObjectDataset(file_path=args.dataset_path, num_points=args.num_points)
    else:
        train_dataset = ModelNet40(root_dir=args.dataset_path, categories=args.categories, num_points=args.num_points)
    train_loader = torch.utils.data.DataLoader(
        train_dataset, batch_size=args.batch_size, shuffle=True, num_workers=2, pin_memory=(device.type == "cuda")
    )

    # Train selected model
    if args.model == "neural_pull":
        train_neural_pull(
            dataloader=train_loader,
            num_shapes=args.num_shapes,
            epochs=args.epochs,
            save_path=args.save_path,
            resume_path=args.resume,
        )
    else:
        print("Invalid model")

    print("Finished training")


if __name__ == "__main__":
    main()
