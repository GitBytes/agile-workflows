import torch
from torch import Tensor
from torch.nn import ReLU, Dropout, LogSoftmax
from torch_geometric.nn import Sequential, GCNConv

class GCN(torch.nn.Module):
    def __init__(self, in_channels, hidden_channels, out_channels, dropout):
        super(GCN, self).__init__()
        self.model = Sequential('x, edge_index', [
            (GCNConv(
                in_channels, hidden_channels,
                cached=True,
                add_self_loops=False,
                normalize=False,
            ).jittable(), 'x, edge_index -> x'),
            ReLU(inplace=True),
            (GCNConv(
                hidden_channels, out_channels,
                cached=True,
                add_self_loops=False,
                normalize=False,
            ).jittable(), 'x, edge_index -> x'),
            Dropout(dropout, inplace=True),
            LogSoftmax(dim=1)
        ])

    def forward(self, x: Tensor, edge_index: Tensor) -> Tensor:
        return self.model(x, edge_index)


if __name__ == '__main__':
    model = GCN(30, 16, 5, 0.5)
    sm = torch.jit.script(model)
    sm.save('GNN-agile.pt')
