#include <stdio.h>
#include <stdlib.h>

struct graph {
  int n;
  int* off;
  int* adj;
};

int nedges(struct graph g)
{
  return g.off[g.n];
}

int deg(struct graph g, int u)
{
  return g.off[u + 1] - g.off[u];
}

struct graph read_graph(FILE* f)
{
  struct graph g;
  fscanf(f, "%d", &g.n);
  g.off = malloc(sizeof(int) * (g.n + 1));
  g.off[0] = 0;
  for (int i = 0; i < g.n; ++i)
    fscanf(f, "%d", g.off + i + 1);
  g.adj = malloc(sizeof(int) * g.off[g.n]);
  for (int i = 0; i < nedges(g); ++i)
    fscanf(f, "%d", g.adj + i);
  return g;
}

void write_graph(FILE* f, struct graph g)
{
  fprintf(f, "%d\n", g.n);
  for (int i = 0; i < g.n; ++i)
    fprintf(f, "%d\n", g.off[i + 1]);
  for (int i = 0; i < nedges(g); ++i)
    fprintf(f, "%d\n", g.adj[i]);
}

void free_graph(struct graph g)
{
  free(g.off);
  free(g.adj);
}

int la(struct graph g)
{
  int n = 0;
  for (int u = 0; u < g.n; ++u) {
    int a = g.off[u];
    int b = g.off[u + 1];
    for (int e = a; e < b; ++e)
      if (g.adj[e] < u)
        n += u - g.adj[e];
  }
  return n;
}

int edges_method(struct graph g)
{
  int n = g.n;
  int m = nedges(g) / 2;
  int i = 1;
  int f = 0;
  int lb = 0;
  while (m > 0) {
    int take = n - i;
    if (take > m)
      take = m;
    m = m - take;
    lb = lb + i * take;
    i = i + 1;
  }
  return lb;
}

int degree_method(struct graph g)
{
  int lb = 0;
  for (int u = 0; u < g.n; ++u) {
    int d = deg(g, u);
    if (d % 2 == 0)
      lb = lb + (d * d) / 4 + d / 2;
    else
      lb = lb + (d * d + 2 * d + 1) / 4;
  }
  return lb / 2;
}

void bfs(struct graph g, int start,
    int** p_order,
    int** p_layer)
{
  int* queue = malloc(sizeof(int) * g.n);
  int* layer = malloc(sizeof(int) * g.n);
  int nlayers = 0;
  int begin = 0;
  int end = 0;
  for (int i = 0; i < g.n; ++i)
    layer[i] = -1;
  /* assuming connected */
  queue[end++] = start;
  layer[start] = 0;
  while (end != begin) {
    int j = queue[begin++];
    int a = g.off[j];
    int b = g.off[j + 1];
    for (int e = a; e < b; ++e) {
      int k = g.adj[e];
      if (layer[k] == -1) {
        layer[k] = layer[j] + 1;
        queue[end++] = k;
      }
    }
  }
  *p_order = queue;
  *p_layer = layer;
}

struct graph reorder(struct graph g, int* new_to_old)
{
  struct graph g2;
  g2.n = g.n;
  g2.off = malloc(sizeof(int) * (g.n + 1));
  g2.adj = malloc(sizeof(int) * nedges(g));
  g2.off[0] = 0;
  for (int i = 0; i < g.n; ++i) {
    int j = new_to_old[i];
    int d = deg(g, j);
    int a = g2.off[i];
    int b = g2.off[i + 1] = g2.off[i] + d;
    for (int e = a; e < b; ++e)
      g2.adj[e] = g.adj[g.off[j] + (e - a)];
  }
  return g2;
}

int main()
{
  struct graph g = read_graph(stdin);
  printf("%d vertices, %d edges\n", g.n, nedges(g) / 2);
  printf("current total spread: %d\n", la(g));
  printf("lower bound by edges method: %d\n", edges_method(g));
  printf("lower bound by degree method: %d\n", degree_method(g));
  free_graph(g);
}
