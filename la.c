#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#if USE_GSL
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_eigen.h>
#endif

#if USE_METIS
#include <metis.h>
#endif

#if USE_ZOLTAN
#include <zoltan.h>
#endif

struct graph {
  int n;
  int* off;
  int* adj;
  double* xyz;
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
  g.xyz = 0;
  return g;
}

void read_coords(FILE* f, struct graph* g)
{
  g->xyz = malloc(sizeof(double) * 3 * g->n);
  for (int i = 0; i < 3 * g->n; ++i)
    fscanf(f, "%lf", g->xyz + i);
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
  free(g.xyz);
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
  g2.xyz = 0;
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

void test_ordering(struct graph g, char const* name, int* order)
{
  struct graph g2 = reorder(g, order);
  free(order);
  printla(g2, name);
  free_graph(g2);
}

void test_bfs(struct graph g)
{
  test_ordering(g, "BFS (0)", get_bfs_order(g, 0));
  test_ordering(g, "BFS (n-1)", get_bfs_order(g, g.n - 1));
}

struct cm {
  int v;
  int l;
  int d;
  int o;
};

int compare_cm(struct cm const* a, struct cm const* b)
{
  if (a->l != b->l)
    return a->l > b->l ? 1 : -1;
  if (a->d != b->d)
    return a->d > b->d ? 1 : -1;
  return a->o > b->o ? 1 : -1;
}

typedef int (*comparator)(const void*, const void*);

int* get_cuthill_mckee_order(struct graph g, int start)
{
  int* bfs_order;
  int* bfs_layer;
  bfs(g, start, &bfs_order, &bfs_layer);
  int* old_to_new = invert_ordering(g.n, bfs_order);
  free(bfs_order);
  struct cm* cms = malloc(sizeof(struct cm) * g.n);
  for (int i = 0; i < g.n; ++i) {
    cms[i].v = i;
    cms[i].l = bfs_layer[i];
    cms[i].d = deg(g, i);
    cms[i].o = old_to_new[i];
  }
  free(bfs_layer);
  free(old_to_new);
  qsort(cms, g.n, sizeof(struct cm), (comparator) compare_cm);
  int* new_to_old = malloc(sizeof(int) * g.n);
  for (int i = 0; i < g.n; ++i)
    new_to_old[i] = cms[i].v;
  free(cms);
  return new_to_old;
}

void test_cuthill_mckee(struct graph g)
{
  test_ordering(g, "Cuthill-McKee (0)", get_cuthill_mckee_order(g, 0));
  test_ordering(g, "Cuthill-McKee (n-1)", get_cuthill_mckee_order(g, g.n - 1));
}

#if USE_GSL

gsl_matrix* get_laplacian(struct graph g)
{
  gsl_matrix* m = gsl_matrix_calloc(g.n, g.n);
  for (int i = 0; i < g.n; ++i) {
    gsl_matrix_set(m, i, i, deg(g, i));
    int a = g.off[i];
    int b = g.off[i + 1];
    for (int j = a; j < b; ++j)
      gsl_matrix_set(m, i, g.adj[j], -1);
  }
  return m;
}

gsl_vector* get_eigenvalues(gsl_matrix* m)
{
  gsl_vector* v = gsl_vector_alloc(m->size1);
  gsl_eigen_symm_workspace* ws = gsl_eigen_symm_alloc(m->size1);
  gsl_eigen_symm(m, v, ws);
  gsl_eigen_symm_free(ws);
  return v;
}

int compare_double_fabs(double const* a, double const* b)
{
  if (*a != *b)
    return fabs(*a) > fabs(*b) ? 1 : -1;
  return 0;
}

void sort_vector(gsl_vector* v)
{
  qsort(v->data, v->size, sizeof(double), (comparator) compare_double_fabs);
}

int juvan_mohar_method(struct graph g)
{
  if (g.n < 2)
    return 0;
  gsl_matrix* m = get_laplacian(g);
  gsl_vector* v = get_eigenvalues(m);
  gsl_matrix_free(m);
  sort_vector(v);
  double l2 = gsl_vector_get(v, 1);
  gsl_vector_free(v);
  return (int) (round(fabs(l2) * (g.n * g.n - 1) / 6.0));
}

#endif

#if USE_METIS
int* get_metis_order(struct graph g)
{
  idx_t nvtxs = g.n;
  idx_t* xadj = malloc(sizeof(idx_t) * (nvtxs + 1));
  idx_t* adjncy = malloc(sizeof(idx_t) * nedges(g));
  xadj[0] = 0;
  for (int i = 0; i < g.n; ++i) {
    xadj[i + 1] = g.off[i + 1];
    int a = g.off[i];
    int b = g.off[i + 1];
    for (int j = a; j < b; ++j)
      adjncy[j] = g.adj[j];
  }
  idx_t* vwgt = NULL;
  idx_t* options = NULL;
  idx_t* perm = malloc(sizeof(idx_t) * nvtxs);
  idx_t* iperm = malloc(sizeof(idx_t) * nvtxs);
  METIS_NodeND(&nvtxs, xadj, adjncy, vwgt, options, perm, iperm);
  free(xadj);
  free(adjncy);
  free(iperm);
  int* order = malloc(sizeof(int) * g.n);
  for (int i = 0; i < g.n; ++i)
    order[i] = perm[i];
  free(perm);
  return order;
}

void test_metis(struct graph g)
{
  test_ordering(g, "METIS", get_metis_order(g));
}
#endif

#if USE_ZOLTAN
/*ZOLTAN_OBJ_LIST_FN*/
void ztn_objs(void *data, int num_gid_entries, int num_lid_entries, 
    ZOLTAN_ID_PTR global_ids, ZOLTAN_ID_PTR local_ids, 
    int wgt_dim, float *obj_wgts, int *ierr)
{
  struct graph* g = (struct graph*) data;
  for(int i=0; i < g->n; i++)
    global_ids[i] = i;
  ierr = 0;
}

/* ZOLTAN_NUM_OBJ_FN */
static int ztn_num_obj(void *data, int *ierr)
{
  struct graph* g = (struct graph*) data;
  ierr = 0;
  return g->n;
}

/* ZOLTAN_NUM_GEOM_FN */
int ztn_num_dim(void* foo, int* bar)
{
  return 3;
}

/* ZOLTAN_GEOM_FN */
void ztn_geom(void *data, int num_gid_entries, int num_lid_entries,
    int num_obj, ZOLTAN_ID_PTR global_ids, ZOLTAN_ID_PTR local_ids,
    int num_dim, double *geom_vec, int *ierr)
{
  struct graph* g = (struct graph*) data;
  assert(num_obj == g->n);
  assert(num_dim == 3);
  for (int i = 0; i < num_dim * num_obj; ++i)
    geom_vec[i] = g->xyz[i];
  ierr = 0;
}

int* get_zoltan_hsfc_order(struct graph g)
{
  float ver;
  Zoltan_Initialize(0, 0, &ver);
  struct Zoltan_Struct* zz = Zoltan_Create(MPI_COMM_WORLD);
  Zoltan_Set_Param(zz, "num_lid_entries", "0");
  Zoltan_Set_Param(zz, "obj_weight_dim", "0");
  Zoltan_Set_Param(zz, "debug_level", "0");
  Zoltan_Set_Param(zz, "order_method", "LOCAL_HSFC");
  Zoltan_Set_Fn(zz, ZOLTAN_NUM_OBJ_FN_TYPE, (void (*)())ztn_num_obj, (void*) (&g));
  Zoltan_Set_Fn(zz, ZOLTAN_OBJ_LIST_FN_TYPE, (void (*)())ztn_objs, (void *) (&g));
  Zoltan_Set_Fn(zz, ZOLTAN_NUM_GEOM_FN_TYPE, (void (*)())ztn_num_dim, (void*) (&g));
  Zoltan_Set_Fn(zz, ZOLTAN_GEOM_MULTI_FN_TYPE, (void (*)())ztn_geom, (void*) (&g));
  int num_gid_entries = 1;
  ZOLTAN_ID_PTR ids = (ZOLTAN_ID_PTR) ZOLTAN_MALLOC(g.n*num_gid_entries*sizeof(ZOLTAN_ID_TYPE));
  ZOLTAN_ID_PTR perm = (ZOLTAN_ID_PTR) ZOLTAN_MALLOC(g.n*num_gid_entries*sizeof(ZOLTAN_ID_TYPE));
  assert(ids);
  assert(perm);
  for(int i=0; i<g.n; i++)
    ids[i] = i;
  Zoltan_Order(zz, num_gid_entries, g.n, ids, perm);
  int* order = malloc(g.n*sizeof(int));
  for(int i=0; i<g.n; i++)
    order[i] = (int) perm[i];
  ZOLTAN_FREE(&ids);
  ZOLTAN_FREE(&perm);
  Zoltan_Destroy(&zz);
  return order;
}
#endif

void info(struct graph g)
{
  printf("%d vertices, %d edges\n", g.n, nedges(g) / 2);
  printla2("edges LB", edges_method(g));
  printla2("degree LB", degree_method(g));
#if USE_GSL
  if (g.n < 2000)
    printla2("Juvan-Mohar LB", juvan_mohar_method(g));
#endif
  printla(g, "normal layout");
}

double sq(double x)
{
  return x * x;
}

double dist(double* a, double* b)
{
  return sqrt(sq(a[0] - b[0]) +
              sq(a[1] - b[1]) +
              sq(a[2] - b[2]));
}

int find_closest(struct graph g,
    double x, double y, double z)
{
  int closest = 0;
  double xyz[3] = {x,y,z};
  double cd = dist(xyz, g.xyz);
  for (int i = 1; i < g.n; ++i) {
    double d = dist(xyz, g.xyz + i * 3);
    if (d < cd) {
      closest = i;
      cd = d;
    }
  }
  return closest;
}

int main(int argc, char** argv)
{
  struct graph g = read_graph(stdin);
  int is_cube = (argc == 2 && !strcmp(argv[1], "-cube"));
  int has_coords = (argc == 2 && !strcmp(argv[1], "-coord")) || is_cube;
  if (has_coords)
    read_coords(stdin, &g);
  info(g);
  test_bfs(g);
  if (is_cube) {
    test_ordering(g, "BFS (center)", get_bfs_order(g, find_closest(g, .5,.5,.5)));
    test_ordering(g, "BFS (mid-face)", get_bfs_order(g, find_closest(g, .5,.5,0)));
    test_ordering(g, "BFS (corner)", get_bfs_order(g, find_closest(g, 0,0,0)));
  }
#if USE_ZOLTAN
  if (has_coords) {
    test_ordering(g, "HSFC", get_zoltan_hsfc_order(g));
  }
#endif
  test_cuthill_mckee(g);
#if USE_METIS
  test_metis(g);
#endif
  free_graph(g);
}
