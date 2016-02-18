#include <stdio.h>
#include <stdlib.h>

struct graph {
  int n;
  int* off;
  int* adj;
};

struct graph read_graph(FILE* f)
{
  struct graph g;
  fscanf(f, "%d", &g.n);
  g.off = malloc(sizeof(int) * (g.n + 1));
  g.off[0] = 0;
  for (int i = 0; i < g.n; ++i)
    fscanf(f, "%d", g.off + i + 1);
  g.adj = malloc(sizeof(int) * g.off[g.n]);
  for (int i = 0; i < g.off[g.n]; ++i)
    fscanf(f, "%d", g.adj + i);
  return g;
}

void write_graph(FILE* f, struct graph g)
{
  fprintf(f, "%d\n", g.n);
  for (int i = 0; i < g.n; ++i)
    fprintf(f, "%d\n", g.off[i + 1]);
  for (int i = 0; i < g.off[g.n]; ++i)
    fprintf(f, "%d\n", g.adj[i]);
}

void free_graph(struct graph g)
{
  free(g.off);
  free(g.adj);
}

int main()
{
  struct graph g = read_graph(stdin);
  write_graph(stdout, g);
  free_graph(g);
}
