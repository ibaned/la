#include <assert.h>
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
  assert(end == g.n);
  *p_order = queue;
  *p_layer = layer;
}

int* get_bfs_order(struct graph g, int start)
{
  int* order;
  int* layer;
  bfs(g, start, &order, &layer);
  free(layer);
  return order;
}

int* invert_ordering(int n, int* new_to_old)
{
  int* old_to_new = malloc(sizeof(int) * n);
  for (int i = 0; i < n; ++i)
    old_to_new[new_to_old[i]] = i;
  return old_to_new;
}

struct graph reorder(struct graph g, int* new_to_old)
{
  int* old_to_new = invert_ordering(g.n, new_to_old);
  struct graph g2;
  g2.n = g.n;
  g2.off = malloc(sizeof(int) * (g.n + 1));
  g2.adj = malloc(sizeof(int) * nedges(g));
  g2.off[0] = 0;
  for (int i = 0; i < g.n; ++i) {
    int oi = new_to_old[i];
    int d = deg(g, oi);
    int a = g2.off[i];
    g2.off[i + 1] = a + d;
    for (int j = 0; j < d; ++j) {
      int k = g.adj[g.off[oi] + j];
      g2.adj[a + j] = old_to_new[k];
    }
  }
  free(old_to_new);
  return g2;
}

void printla2(char const* name, int n)
{
  printf("%20s LA measure: %15d\n", name, n);
}

void printla(struct graph g, char const* name)
{
  printla2(name, la(g));
}

void info(struct graph g)
{
  printf("%d vertices, %d edges\n", g.n, nedges(g) / 2);
  printla2("edges LB", edges_method(g));
  printla2("degree LB", degree_method(g));
  printla(g, "normal layout");
}

void test_ordering(struct graph g, char const* name, int* order)
{
  struct graph g2 = reorder(g, order);
  free(order);
  printla(g2, name);
  free_graph(g2);
}

void test_bfs(struct graph g)
{
  test_ordering(g, "BFS", get_bfs_order(g, g.n - 1));
}

struct cm {
  int v;
  int l;
  int d;
};

int compare_cm(struct cm const* a, struct cm const* b)
{
  if (a->l != b->l)
    return a->l > b->l ? 1 : -1;
  if (a->d != b->d)
    return a->d > b->d ? 1 : -1;
  return 0;
}

typedef int (*comparator)(const void*, const void*);

int* get_cuthill_mckee_order(struct graph g, int start)
{
  int* bfs_order;
  int* bfs_layer;
  bfs(g, g.n - 1, &bfs_order, &bfs_layer);
  free(bfs_order);
  struct cm* cms = malloc(sizeof(struct cm) * g.n);
  for (int i = 0; i < g.n; ++i) {
    cms[i].v = i;
    cms[i].l = bfs_layer[i];
    cms[i].d = deg(g, i);
  }
  free(bfs_layer);
  qsort(cms, g.n, sizeof(struct cm), (comparator) compare_cm);
  int* new_to_old = malloc(sizeof(int) * g.n);
  for (int i = 0; i < g.n; ++i)
    new_to_old[i] = cms[i].v;
  free(cms);
  return new_to_old;
}

void test_cuthill_mckee(struct graph g)
{
  test_ordering(g, "Cuthill-McKee", get_cuthill_mckee_order(g, g.n - 1));
}

int main()
{
  struct graph g = read_graph(stdin);
  info(g);
  test_bfs(g);
  test_cuthill_mckee(g);
  free_graph(g);
}
