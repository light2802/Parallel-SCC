#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "omp.h"

typedef struct edge {
    int source;
    int destination;
    int weight;
} edge;

class graph {
private:
    int original_nodesTotal;
    int edgesTotal;

    char* filePath;
    std::set<int> nodes;
    std::map<int, std::vector<edge>> edges;

public:
    edge* edgeList;
    graph(char* file, int n) {
        filePath = file;
        original_nodesTotal = 0;
        edgesTotal = 0;
    }

    graph() {}
    graph(int num_edges, edge* edgelist) {
        edgesTotal = num_edges;
        edgeList = edgelist;

        for (int i = 0; i < num_edges; i++) {
            edges[edgeList[i].source].push_back(edgeList[i]);
            nodes.insert(edgeList[i].source);
            nodes.insert(edgeList[i].destination);
            // printf("adding nodes %d %d\n", edgeList[i].source,
            // edgeList[i].destination);
        }
        original_nodesTotal = nodes.size();
    }

    std::map<int, std::vector<edge>> getEdges() { return edges; }
    std::set<int> getNodes() { return nodes; }
    edge* getEdgeList() { return edgeList; }
    int num_nodes() { return original_nodesTotal; }
    int num_edges() { return edgesTotal; }

    void removeNode(int v) {
        if (!original_nodesTotal) return;
        edgesTotal -= edges[v].size();
        edges.erase(v);
        nodes.erase(v);
        for (auto itr = edges.begin(); itr != edges.end(); ++itr) {
            for (auto edge = itr->second.begin(); edge != itr->second.end();
                 edge++) {
                if (edge->destination == v) {
                    // printf("removed edge %d-%d\n", edge->source,
                    // edge->destination);
                    itr->second.erase(edge);
                    edgesTotal--;
                    break;
                }
            }
        }
        original_nodesTotal--;
        // printf("Removed node %d\n", v);

        edgeList = new edge[edgesTotal];

        int edge_no = 0;
        for (auto i : nodes) {
            std::vector<edge> edgeofVertex = edges[i];
            std::vector<edge>::iterator itr;
            for (itr = edgeofVertex.begin(); itr != edgeofVertex.end(); itr++) {
                edgeList[edge_no] = (*itr);
                // printf("dbg nodes %d %d\n", (*itr).source,
                // (*itr).destination);
                edge_no++;
            }
        }
    }

    std::set<int> FW() {
        int v = *nodes.begin();
        std::set<int> fw_set;
        std::map<int, int> visited;
        for (auto i : nodes) {
            visited[i] = 0;
        }
        std::queue<int> bfs_q;
        bfs_q.push(v);
        while (!bfs_q.empty()) {
            int curr_node = bfs_q.front();
            bfs_q.pop();
            if (visited[curr_node]) continue;
            std::vector<edge> edge_list = edges[curr_node];
            for (auto& element : edge_list) {
                if (!visited[element.destination]) {
                    bfs_q.push(element.destination);
                }
            }
            fw_set.insert(curr_node);
            visited[curr_node] = 1;
        }
        // std::cout <<"FW : ";
        // for(auto i:fw_set)
        //{
        //     std::cout << i << "  ";
        // }
        // std::cout << std::endl;
        return fw_set;
    }

    std::set<int> BW() {
        int v = *nodes.begin();
        std::set<int> bw_set;
        std::map<int, int> visited;
        for (auto i : nodes) {
            visited[i] = 0;
        }

        std::queue<int> bfs_q;
        bfs_q.push(v);
        while (!bfs_q.empty()) {
            int curr_node = bfs_q.front();
            bfs_q.pop();
            if (visited[curr_node]) continue;
            for (auto element : edges) {
                std::vector<edge> edge_list = element.second;
                for (auto& edge_element : edge_list) {
                    if (!visited[edge_element.source] &&
                        edge_element.destination == curr_node)
                        bfs_q.push(edge_element.source);
                }
            }
            bw_set.insert(curr_node);
            visited[curr_node] = 1;
        }
        // std::cout <<"BW : ";
        // for(auto i:bw_set)
        //{
        //     std::cout << i << "  ";
        // }
        // std::cout << std::endl;

        return bw_set;
    }

    int in_degree(int v) {
        int deg = 0;
        for (auto element : edges) {
            std::vector<edge> edge_list = element.second;
            for (auto& edge_element : edge_list) {
                if (edge_element.destination == v) deg++;
            }
        }
        return deg;
    }
    int out_degree(int v) { return edges[v].size(); }
    void parseGraph() {
        std::ifstream infile;
        infile.open(filePath);
        if (!infile) {
            std::cout << "Cannot open the file" << std::endl;
            exit(0);
        }
        std::string line;
        std::stringstream ss;
        edgesTotal = 0;
        while (getline(infile, line)) {
            if (line[0] < '0' || line[0] > '9') {
                continue;
            }

            ss.clear();
            ss << line;
            edgesTotal++;

            edge e;
            int source;
            int destination;
            if (ss >> source && ss >> destination) {
                if (source > original_nodesTotal) original_nodesTotal = source;
                if (destination > original_nodesTotal)
                    original_nodesTotal = destination;
                e.source = source;
                e.destination = destination;
                e.weight = 1;
                nodes.insert(source);
                nodes.insert(destination);
                edges[source].push_back(e);
            }
        }
        original_nodesTotal = nodes.size();

        edgeList = new edge[edgesTotal];

        int edge_no = 0;
        for (auto i : nodes) {
            std::vector<edge> edgeofVertex = edges[i];
            std::vector<edge>::iterator itr;
            for (itr = edgeofVertex.begin(); itr != edgeofVertex.end(); itr++) {
                edgeList[edge_no] = (*itr);
                edge_no++;
            }
        }
    }
};
