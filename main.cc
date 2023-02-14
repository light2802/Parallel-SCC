#include "scc.h"
typedef set<set<int>> scc_t;
enum msg_code {
    QUERY,
    TASK,
    TRIM_SOLUTION,
    SOLUTION,
    DISTRIBUTING_TO,
    DISTRIBUTING_FOR_BW,
    FREE_ME,
    KILL
};

void defineStruct(MPI_Datatype *tstype) {
    const int count = 3;
    int blocklens[count] = {1, 1, 1};
    MPI_Datatype types[count] = {MPI_INT, MPI_INT, MPI_INT};
    MPI_Aint disps[count] = {offsetof(edge, source),
                             offsetof(edge, destination),
                             offsetof(edge, weight)};

    MPI_Type_create_struct(count, blocklens, disps, types, tstype);
    MPI_Type_commit(tstype);
}
void free_me() {
    // free me
    int code = FREE_ME;
    MPI_Send(&code, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
    // printf("free\n");
}

void Trim(graph &g) {
    bool graph_changed;
    do {
        graph_changed = false;
        set<int> nodes = g.getNodes();
        for (auto node : nodes) {
            int in = g.in_degree(node), out = g.out_degree(node);
            // printf("node %d : in %d out %d\n", node, in, out);
            if (in == 0 || out == 0) {
                graph_changed = true;
                // mpi send trim solution i to proc0
                int msg_code = TRIM_SOLUTION;
                MPI_Send(&msg_code, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
                MPI_Send(&node, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
                // remove node i from g
                g.removeNode(node);
            }
        }
    } while (graph_changed);
}
void send_graph(int code, int procid, graph g) {
    MPI_Send(&code, 1, MPI_INT, procid, 0, MPI_COMM_WORLD);
    int num_edges = g.num_edges();
    // printf("edges %d\n", num_edges);
    // int *indexofNodes = g.getIndexofNodes();
    edge *edgelist = g.getEdgeList();
    // int *edgelen = g.getEdgeLen();
    MPI_Send(&num_edges, 1, MPI_INT, procid, 1, MPI_COMM_WORLD);
    MPI_Datatype structtype;
    defineStruct(&structtype);
    MPI_Send(edgelist, num_edges, structtype, procid, 2, MPI_COMM_WORLD);
    // printf("graph sent to procid %d\n", procid);
}

graph recv_graph(int procid) {
    MPI_Status status;
    int num_edges = 0;
    MPI_Recv(&num_edges, 1, MPI_INT, procid, 1, MPI_COMM_WORLD, &status);
    edge edgelist[num_edges];
    MPI_Datatype structtype;
    defineStruct(&structtype);
    MPI_Recv(edgelist, num_edges, structtype, procid, 2, MPI_COMM_WORLD,
             &status);
    // printf("recv edges (%d) ", num_edges);
    // for(int i=0;i<num_edges;i++)
    //     printf("%d %d,",edgelist[i].source, edgelist[i].destination);
    // printf("\n");
    graph g(num_edges, edgelist);
    // printf("graph recv from proc %d\n", procid);
    return g;
}
void Compute_SCC(graph g) {
    int my_rank, np;
    struct timeval start, end, start1, end1;
    long seconds, micros;

    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &np);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    // printf("%d : init %d procs\n", my_rank, np);
    MPI_Barrier(MPI_COMM_WORLD);
    // master node allocates tasks to salves
    if (my_rank == 0) {
        scc_t SCC;
        gettimeofday(&start1, NULL);
        g.parseGraph();
        gettimeofday(&end1, NULL);
        seconds = (end1.tv_sec - start1.tv_sec);
        micros = ((seconds * 1000000) + end1.tv_usec) - (start1.tv_usec);
        if (my_rank == 0) {
            printf("The graph loading time = %ld secs.\n", seconds);
        }
        printf("parse complete\n");
        vector<int> free_procs(np);
        fill(free_procs.begin(), free_procs.end(), 0);
        //  send graph to proc 1
        long num_dist = 1;
        gettimeofday(&start, NULL);
        send_graph(TASK, 1, g);
        free_procs[1] = 1;
        bool complete_flag = false;
        while (!complete_flag) {
            MPI_Status status;
            int src_procid, code;
            MPI_Recv(&code, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD,
                     &status);
            src_procid = status.MPI_SOURCE;
            switch (code) {
                case QUERY: {
                    // mpi send idle procid from free procs list
                    auto it = find(free_procs.begin() + 1, free_procs.end(), 0);

                    // If element was found
                    int index = -1;
                    if (it != free_procs.end()) {
                        // calculating the index
                        // of K
                        index = it - free_procs.begin();
                        free_procs[index]++;
                        num_dist++;
                    }
                    MPI_Send(&index, 1, MPI_INT, src_procid, 0, MPI_COMM_WORLD);
                    break;
                }
                case TRIM_SOLUTION: {
                    // mpi recv node index
                    int i;
                    MPI_Recv(&i, 1, MPI_INT, src_procid, 1, MPI_COMM_WORLD,
                             &status);
                    set<int> temp(&i, &i + 1);
                    scc_t trim = {temp};
                    // printf("Recv trim %d from proc %d\n", i, src_procid);
                    break;
                    set_union(SCC.begin(), SCC.end(), trim.begin(), trim.end(),
                              inserter(SCC, SCC.begin()));
                }
                case SOLUTION: {
                    // mpi recv soln
                    int count;
                    MPI_Recv(&count, 1, MPI_INT, src_procid, 1, MPI_COMM_WORLD,
                             &status);
                    int *recv_vec = new int[count];
                    printf("got count %d\n", count);
                    MPI_Recv(&recv_vec[0], count, MPI_INT, src_procid, 2,
                             MPI_COMM_WORLD, &status);
                    set<int> solution(recv_vec, recv_vec + count);
                    scc_t temp = {solution};
                    set_union(SCC.begin(), SCC.end(), temp.begin(), temp.end(),
                              inserter(SCC, SCC.begin()));
                    break;
                }
                case FREE_ME: {
                    free_procs[src_procid]--;
                    // printf("freed %d\n", src_procid);
                    if (!accumulate(free_procs.begin(), free_procs.end(), 0))
                        complete_flag = 1;
                    break;
                }
            }
        }
        gettimeofday(&end, NULL);
        seconds = (end.tv_sec - start.tv_sec);
        micros = ((seconds * 1000000) + end.tv_usec) - (start.tv_usec);

        for (int i = 1; i < np; i++) {
            // mpi send kill
            int code = KILL;
            MPI_Send(&code, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
        }
        cout << "SCC found : " << endl;
        for (auto it = SCC.begin(); it != SCC.end(); it++) {
            for (auto item = it->begin(); item != it->end(); item++) {
                cout << " " << *item;
            }
            cout << endl;
        }
        printf("The number of times graph dist taken %d \n", num_dist);
        printf("The iteration time = %ld micro secs.\n", micros);
        printf("The iteration time = %ld secs.\n", seconds);

    } else {
        queue<graph> pending_q;
        while (1) {
            graph g;
            if (pending_q.empty()) {
                int code = -1;
                MPI_Status status;
                int src_procid;
                MPI_Recv(&code, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD,
                         &status);
                src_procid = status.MPI_SOURCE;
                // printf("slave : recv %d %d\n", src_procid, my_rank);
                if (code == KILL && src_procid == 0) {
                    // printf("slave kill %d\n", my_rank);
                    break;
                }
                // recv graph g
                g = recv_graph(src_procid);
                // printf("graph recv from %d\n", src_procid);
            } else {
                printf("using self queue\n");
                g = pending_q.front();
                pending_q.pop();
            }
            if (!g.num_nodes()) {
                // send free me
                free_me();
                continue;
            }
            Trim(g);
            if (!g.num_nodes()) {
                // send free me
                free_me();
                continue;
            }
            set<int> FW = g.FW();
            set<int> BW = g.BW();
            vector<int> S;  // intersection FW, BW
            set_intersection(FW.begin(), FW.end(), BW.begin(), BW.end(),
                             inserter(S, S.begin()));
            // send solution S to proc0
            int code = SOLUTION;
            MPI_Send(&code, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
            int size = S.size();
            MPI_Send(&size, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
            MPI_Send(&S[0], size, MPI_INT, 0, 2, MPI_COMM_WORLD);

            int dst_procid;

            graph fw = g, bw = g, s = g;
            for (auto i : fw.getNodes()) {
                if (FW.find(i) == FW.end()) fw.removeNode(i);
            }
            for (auto i : bw.getNodes()) {
                if (BW.find(i) == BW.end()) bw.removeNode(i);
            }

            int temp = 0;
            // send fw-SCS
            graph FW_S = fw;
            for (auto i = S.begin(); i != S.end(); i++) {
                FW_S.removeNode(*i);
            }
            if (FW_S.num_nodes() > 0) {
                printf("FW - S : %d\n", FW_S.num_nodes());
                code = QUERY;
                MPI_Send(&code, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
                MPI_Status status;
                MPI_Recv(&dst_procid, 1, MPI_INT, 0, 0, MPI_COMM_WORLD,
                         &status);

                if (dst_procid > 0)
                    send_graph(TASK, dst_procid, FW_S);
                else
                    pending_q.push(FW_S);
            }

            // send bw-SCC
            graph BW_S = bw;
            for (auto i = S.begin(); i != S.end(); i++) {
                BW_S.removeNode(*i);
            }
            if (BW_S.num_nodes() > 0) {
                code = QUERY;
                MPI_Send(&code, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
                MPI_Status status;
                MPI_Recv(&dst_procid, 1, MPI_INT, 0, 0, MPI_COMM_WORLD,
                         &status);

                if (dst_procid > 0)
                    send_graph(TASK, dst_procid, BW_S);
                else
                    pending_q.push(BW_S);
            }

            // send g-(fw | bw)
            graph G_FW_BW = g;
            for (auto i = FW.begin(); i != FW.end(); i++) {
                G_FW_BW.removeNode(*i);
            }
            for (auto i = BW.begin(); i != BW.end(); i++) {
                G_FW_BW.removeNode(*i);
            }
            if (G_FW_BW.num_nodes() > 0) {
                code = QUERY;
                MPI_Send(&code, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
                MPI_Status status;
                MPI_Recv(&dst_procid, 1, MPI_INT, 0, 0, MPI_COMM_WORLD,
                         &status);

                if (dst_procid > 0)
                    send_graph(TASK, dst_procid, G_FW_BW);
                else
                    pending_q.push(G_FW_BW);
            }
            if (pending_q.empty()) free_me();
        }
    }
    MPI_Finalize();
}

int main(int argc, char *argv[]) {
    struct timeval start, end;
    if (argc < 3) {
        // printf("Execute ./a.out input_graph_file numberOfProcesses\n");
        exit(0);
    }
    int np = strtol(argv[2], NULL, 10);
    // //printf("The number of process entered : %d\n",np);
    graph G(argv[1], np);

    // mpi::environment env(argc, argv);
    Compute_SCC(G);
    return 0;
}
