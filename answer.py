import sys
sys.setrecursionlimit(10**7)
input = sys.stdin.readline

# ----------------------------
# LCA and Tree Preprocessing
# ----------------------------
n, q = map(int, input().split())
adj = [[] for _ in range(n)]
for _edge_i in range(n-1):
    u, v = map(int, input().split())
    u -= 1
    v -= 1
    adj[u].append(v)
    adj[v].append(u)

LOG = max(1, (n-1).bit_length())
depth = [0]*n
parent = [[-1]*n for _ in range(LOG)]

def dfs_lca(u, p):
    for v in adj[u]:
        if v == p:
            continue
        depth[v] = depth[u]+1
        parent[0][v] = u
        dfs_lca(v, u)

depth[0] = 0
dfs_lca(0, -1)
for i in range(1, LOG):
    for v in range(n):
        if parent[i-1][v] != -1:
            parent[i][v] = parent[i-1][parent[i-1][v]]

def lca(u, v):
    if depth[u] < depth[v]:
        u, v = v, u
    diff = depth[u]-depth[v]
    for i in range(LOG):
        if diff&(1<<i):
            u = parent[i][u]
    if u == v:
        return u
    for i in reversed(range(LOG)):
        if parent[i][u] != parent[i][v]:
            u = parent[i][u]
            v = parent[i][v]
    return parent[0][u]

def dist(u, v):
    w = lca(u, v)
    return depth[u]+depth[v]-2*depth[w]

# ----------------------------
# Heavy Light Decomposition (HLD)
# ----------------------------
size = [0]*n
heavy = [-1]*n

def dfs_hld(u):
    sz = 1
    max_subtree = 0
    for v in adj[u]:
        if v == parent[0][u]:
            continue
        dfs_hld(v)
        if size[v]>max_subtree:
            max_subtree = size[v]
            heavy[u] = v
        sz += size[v]
    size[u]=sz

dfs_hld(0)

head = [0]*n
pos = [0]*n
cur_pos = 0

def decompose(u, h):
    global cur_pos
    head[u] = h
    pos[u] = cur_pos
    cur_pos += 1
    if heavy[u] != -1:
        decompose(heavy[u], h)
    for v in adj[u]:
        if v != parent[0][u] and v != heavy[u]:
            decompose(v, v)

decompose(0,0)

# We'll work on edges coverage. Actually we will store coverage on nodes except for root, 
# and for queries we consider edges as coverage between node and parent.
# However, we must be careful: We'll apply increments on the edges by updating on the path excluding the root.
# A common approach is to do updates on (node basis) except the root.

# Segment Tree with Lazy Propagation for Min and Add operations
INF = 10**9
class SegmentTree:
    def __init__(self, n):
        self.n = n
        self.size = 1
        while self.size < n: self.size <<= 1
        self.minv = [0]*(2*self.size)
        self.lazy = [0]*(2*self.size)
    def push_down(self, x):
        if self.lazy[x] != 0:
            self.minv[2*x] += self.lazy[x]
            self.minv[2*x+1] += self.lazy[x]
            self.lazy[2*x] += self.lazy[x]
            self.lazy[2*x+1] += self.lazy[x]
            self.lazy[x] = 0
    def update_range(self, l, r, val):
        self._update_range(1,0,self.size-1,l,r,val)
    def _update_range(self, x, lx, rx, l, r, val):
        if l>rx or r<lx:
            return
        if l<=lx and rx<=r:
            self.minv[x]+=val
            self.lazy[x]+=val
            return
        self.push_down(x)
        mid=(lx+rx)//2
        self._update_range(2*x,lx,mid,l,r,val)
        self._update_range(2*x+1,mid+1,rx,l,r,val)
        self.minv[x]=min(self.minv[2*x],self.minv[2*x+1])
    def query_range_min(self, l, r):
        return self._query_range_min(1,0,self.size-1,l,r)
    def _query_range_min(self, x, lx, rx, l, r):
        if l>rx or r<lx:
            return INF
        if l<=lx and rx<=r:
            return self.minv[x]
        self.push_down(x)
        mid=(lx+rx)//2
        left=self._query_range_min(2*x,lx,mid,l,r)
        right=self._query_range_min(2*x+1,mid+1,rx,l,r)
        return min(left,right)

seg = SegmentTree(n)

# Function to update along path u->v
def path_update(u,v,val):
    while head[u]!=head[v]:
        if depth[head[u]]<depth[head[v]]:
            u,v = v,u
        seg.update_range(pos[head[u]], pos[u], val)
        u=parent[0][head[u]]
    if depth[u]>depth[v]:
        u,v=v,u
    # Now u is ancestor of v
    # We must update edges on the path u->v, but if we consider coverage on nodes (excluding root)
    # we should start from u+1 if we are considering node-level increments. 
    # Actually, to represent an edge (p->c), we can store coverage at c (pos[c]).
    # So we do from pos[u]+1 to pos[v] if u!=v
    if u!=v:
        seg.update_range(pos[u]+1,pos[v],val)

def path_min(u,v):
    # To find the minimum coverage on u->v path
    res=INF
    while head[u]!=head[v]:
        if depth[head[u]]<depth[head[v]]:
            u,v=v,u
        res=min(res, seg.query_range_min(pos[head[u]],pos[u]))
        u=parent[0][head[u]]
    if depth[u]>depth[v]:
        u,v=v,u
    if u!=v:
        res=min(res, seg.query_range_min(pos[u]+1,pos[v]))
    return res

# ----------------------------
# Handling queries
# ----------------------------
# We must keep track of which train lines are active. They are given by pairs (u,v).
# For type 1 (add line): increment coverage on path(u,v)
# For type 2 (remove line): decrement coverage on path(u,v)
#
# For type 3 queries:
# - Gather all (u_i,v_i) and their LCA
# - Form minimal subtree and check if it's a simple path
# - If yes, query the min coverage on that path

# We'll store a dictionary (or map) for active lines to know when we remove them
active_lines = set()

# To check if minimal subtree is a path:
# Steps:
# 1) Collect all nodes: 
#    For each pair (u_i,v_i), add u_i,v_i,lca(u_i,v_i) to a set.
# 2) Extract induced subgraph on these nodes.
# 3) Count degrees in this induced subgraph (each edge from connecting these nodes through LCAs).
#    Actually, we must connect them by edges on the tree. The induced subgraph edges are exactly 
#    the edges on paths u_i->v_i (including the LCAs).
# A simpler approach:
# - Put all nodes in a list W.
# - Sort W by pos[] (HLD preorder), which gives us a topological order along the tree.
# - For consecutive nodes in W, also add their LCA to W (if not present).
# - Then build edges between consecutive nodes in this sorted order by connecting them through LCA paths.
# After this, count degrees.
#
# If it's a simple path: 
# Exactly two nodes have degree 1 (the endpoints), and all others have degree 2 (if more than one node).
# If k=1, a single pair (u,v) is trivially a path.

def build_minimal_subtree(nodes):
    # nodes: list of distinct nodes
    # We will sort by pos (HLD order) so that we can reconnect them easily
    nodes = list(set(nodes))
    nodes.sort(key=lambda x: pos[x])
    # Add LCAs of consecutive nodes
    extra = []
    for i in range(len(nodes)-1):
        extra.append(lca(nodes[i], nodes[i+1]))
    for x in extra:
        nodes.append(x)
    nodes = list(set(nodes))
    nodes.sort(key=lambda x: pos[x])

    # Now we connect consecutive nodes by their LCA chain:
    # Actually, to form the minimal subtree edges, consider consecutive nodes in 'nodes' and 
    # connect them with the path between them. The edges on minimal subtree are exactly the edges 
    # on these small paths. Counting degrees can be done by linking each node to its parent on these paths.
    deg = {x:0 for x in nodes}

    # A known technique: 
    # When sorted by pos[], if we connect each consecutive pair by the unique path, 
    # we can just link them through their LCA again:
    # Actually, minimal subtree edges = edges along paths connecting consecutive nodes in sorted order.
    # We can just do a "stack" approach:
    stack = []
    for x in nodes:
        if not stack:
            stack.append(x)
            continue
        w = lca(stack[-1], x)
        # Pop until we find a node that is an ancestor of x or equal to w
        prev = stack[-1]
        while stack and depth[stack[-1]]>depth[w]:
            top = stack.pop()
            if stack:
                # top and stack[-1] are connected
                deg[top]+=1
                deg[stack[-1]]+=1
                prev = stack[-1]
        if w!=prev and w in deg:  # connect w with prev if not same
            stack.append(w)
        if x!=stack[-1]:
            stack.append(x)
    # Now connect remaining stack
    while len(stack)>1:
        top = stack.pop()
        deg[top]+=1
        deg[stack[-1]]+=1

    # Check degrees
    leaf_count=0
    endpoints=[]
    for v in deg:
        d = deg[v]
        if d==1:
            leaf_count+=1
            endpoints.append(v)
        elif d!=2 and len(deg)>1:
            # If subtree has more than 1 node and found a node with degree !=1 or 2, not a path
            return None
    # If there's more than 2 leaves, not a simple path
    if leaf_count>2:
        return None
    if not endpoints:
        # Single node or no edges case: treat as endpoint = same node twice
        endpoints = [nodes[0], nodes[0]]
    elif len(endpoints)==1:
        # This would be odd, but if it happens, let's assume a single edge
        endpoints.append(endpoints[0])
    return endpoints[0], endpoints[-1]


for _ in range(q):
    line = input().split()
    t = int(line[0])
    if t==1:
        u,v = int(line[1])-1,int(line[2])-1
        # add line u->v
        # increment coverage on path(u,v)
        path_update(u,v,1)
        active_lines.add((u,v))
    elif t==2:
        u,v = int(line[1])-1,int(line[2])-1
        # remove line u->v
        path_update(u,v,-1)
        active_lines.remove((u,v))
    else:
        k = int(line[1])
        persons = []
        nodes_set = set()
        for _p in range(k):
            uu,vv= map(int,input().split())
            uu-=1
            vv-=1
            persons.append((uu,vv))
            nodes_set.add(uu)
            nodes_set.add(vv)
            nodes_set.add(lca(uu,vv))
        # Check if minimal subtree is a single path
        res = build_minimal_subtree(nodes_set)
        if res is None:
            # Not a simple path
            print(0)
            continue
        R,S=res
        # Query min coverage on path(R,S)
        ans = path_min(R,S)
        if ans==INF:
            ans=0
        print(ans)
