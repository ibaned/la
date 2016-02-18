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

int* bfs(struct graph g, int start)
{
  enum { BLACK, GREY, WHITE };
  int* queue = malloc(sizeof(int) * g.n);
  int* state = malloc(sizeof(int) * g.n);
  int begin = 0;
  int end = 0;
  for (int i = 0; i < g.n; ++i)
    state[i] = WHITE;
  /* assuming connected */
  queue[end++] = start;
  while (end != begin) {
    int j = queue[begin++];
    state[j] = BLACK;
    int a = g.off[j];
    int b = g.off[j + 1];
    for (int e = a; e < b; ++e) {
      int k = g.adj[e];
      state[k] = GREY;
      queue[end++] = k;
    }
  }
  free(state);
  return queue;
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
