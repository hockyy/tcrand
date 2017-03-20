#pragma once
#include <algorithm>
#include <stdexcept>
#include <vector>
#include <set>
#include <string>
#include <cstdio>
#include <iostream>
#include "tcrand/graph.hpp"
#define DEBUG false
using namespace std;

namespace tcrand {

	vector<int> random_with_sum(int count, int sum){
		vector<int> res;
		for (int i=0;i < count-1;i++) res.push_back(rand_int(sum - count + 1));
		res.push_back(sum - count);
		sort(res.begin(), res.end());
		for (int i=count-1;i>=1;i--) res[i] = 1 + (res[i] - res[i-1]);
		res[0]++;
		//validate
		int tot = 0;
		for (int x:res)
			tot+=x;
		return res;

	}


class GraphRandomizer{
	int num_nodes;
	int num_edges;
	int num_components;
	int num_scc;
	int num_bridge;

	int params_components;
	int params_edges;
	int params_nodes;
	int params_scc;
	int params_bridge;

	bool is_scc_set;
	bool is_component_set;
	bool allow_loop;
	bool is_directed;
	bool is_bridge_set;

	enum GraphType { type_basic, type_dag, type_bipartite };
	GraphType graph_type;
	float bipartite_ratio;


	bool addEdge(int st, int ed, set<pair<int, int > >& pathSet){
		if (pathSet.count(make_pair(st, ed)))
			return false;

		if (!is_directed && pathSet.count(make_pair(ed, st)))
			return false;

		pathSet.insert(make_pair(st, ed));
		return true;
	}
	/*
	given a list of vertexes V
	we assume that V initially disconnected to each other
	create a single component of a directed graph by using at most E edges.
	update your edges in the set or adj. matrix
	returns the actual number of edges used
	*/
	int constructDirectedComponent(const vector<int>& V, const int& E, set<pair<int, int > >& pathSet){
		//if V is small, use temporary adj. matrix
		int used = 0;
		int N = V.size();
		int pos = 0;
		if (N == 1) //nothing to connect
			return 0;
		//connects everyone by making circles

		while (pos + 1 < N){
			int begin_pos = ++pos;
			int start_circle = V[ rand_int(pos) ];
			int end_circle = V[ rand_int(pos) ];
			//compute the probability of taking the next node to the circle.
			//also when you're running out of edges, there's no other option but join them all in single circle
			//cout<<"Mulai di "<<start_circle<<" stop di "<<end_circle<< endl;
			while (pos + 1 < N && (rand_int(N) > pos || E - used <= N - begin_pos + 1)) pos++;
			//join everyone
			for (int i=begin_pos + 1; i<=pos;i++, used++) addEdge(V[i-1], V[i], pathSet);
			addEdge(start_circle, V[begin_pos], pathSet);
			addEdge(V[pos], end_circle, pathSet);
			used += 2;
		}

		return used;
	}

		vector<vector<int> > mergeSCCintoComponents(const vector< vector<int> >& scc_members, set<pair<int, int > >& pathSet, int num_components ){
			vector<int> component_size = random_with_sum( num_components, scc_members.size() );

			//build the membership matrix
			vector<vector<int> > component_members(num_components);
			int pos = 0;
			for (int i=0;i<num_components;i++){
				//build tree of SCC
				for (int j=1;j< component_size[i] ;j++){
					int scc_from = j + pos;
					//magic (so that low numbered nodes do not have too big degree)
					int scc_to = rand_int(pos + j / 3, j - 1);
					int v1 = scc_members[ scc_from ][ rand_int( scc_members[scc_from].size() ) ];
					int v2 = scc_members[ scc_to ][ rand_int(scc_members[scc_to].size() ) ];
					addEdge(v1, v2, pathSet);
				
				}

				for (int j=0;j<component_size[i];j++, pos++){
					for (int v: scc_members[pos]){
						component_members[i].push_back(v);
					}
				}
			}
			return component_members;
		}

	void load_params(){
		num_nodes = params_nodes;
		num_scc = params_scc;
		num_components = params_components;
		num_edges = params_edges;
		if (num_edges < 0)
			num_edges = num_nodes * 11 / 10;

		if (graph_type == type_dag){
			num_scc = num_nodes;
			is_directed = true;
		}
	}

	Graph load_graph(const set<pair<int, int > >& pathSet){
		Graph g;
		vector<int> from;
		vector<int> to;
		for (auto path: pathSet){
			from.push_back(path.first);
			to.push_back(path.second);
		}
		g.setPath(from, to);
		g.setNode(num_nodes);
		return g;
	}

public:
	GraphRandomizer(){
		params_components = 1;
		params_nodes = 8;
		params_edges = -1;
		allow_loop = false;
		is_component_set = false;
		is_scc_set = false;
		is_directed = true;
		is_bridge_set = false;
		graph_type = type_basic;
	}
	//special graphs
	GraphRandomizer& dag(){
		graph_type = type_dag;
		return *this;
	}

	GraphRandomizer& bipartite(float ratio = 0.5){
		graph_type = type_bipartite;
		bipartite_ratio = ratio;
		return *this;
	}

	// parameters
	GraphRandomizer& component(int n){
		params_components = n;
		is_component_set = true;
		return *this;
	}
	
	GraphRandomizer& scc(int n){
		params_scc = n;
		is_scc_set = true;
		return *this;
	}
	
	GraphRandomizer& node(int n){
		params_nodes = n;
		return *this;
	}
	
	GraphRandomizer& edge(int n){
		params_edges = n;
		return *this;
	}
	
	GraphRandomizer& directed(bool n){
		is_directed = n;
		return *this;
	}

	GraphRandomizer& loop(bool n){
		allow_loop = n;
		return *this;
	}

	Graph next(){
		load_params();
		if (!is_scc_set)
			num_scc = num_nodes;

		//if the graph is undirected, skip scc part completely.
		if (!is_directed && !is_bridge_set)
			num_scc = num_nodes;

		set<pair<int,int> > paths;
		
		//split into scc. 
		vector< vector<int> > scc_members(num_scc);
		vector<int> splitter = random_with_sum(num_scc, num_nodes);
		int node_count = 0;
		for (int i=0;i< num_scc;i++){
			for (int j=0;j<splitter[i];j++) scc_members[i].push_back(node_count++);
		}
		//for each components, generate your graphs
		int used = 0;
		for (int i=0;i< num_scc;i++){
			double edge_rate = (double)num_edges / num_nodes;
			if (edge_rate > 2.)
				edge_rate = 2.;
			used += constructDirectedComponent(scc_members[i] , (int) ((double)scc_members[i].size() * edge_rate) , paths);
		}

		//merge those scc into component
		vector< vector<int> > component_members = mergeSCCintoComponents( scc_members, paths, num_components );
		used += (num_scc - num_components);
		//build component index table
		vector<int> component_id(num_nodes);
		for (int i=0;i<num_components;i++){
			for (int v: component_members[i]){
				component_id[v] = i;
			}
		}

		//finishing the leftover
		//if the nodes is small, generate all pairs instead
		vector<pair<int, int> > options;
		if (num_nodes <= 1000)
			for (int i=0;i<num_nodes;i++)
				for (int j=0;j<num_nodes;j++)
					options.push_back(make_pair(i,j));
		random_shuffle(options.begin(), options.end());
		int idx = 0;
		int tries = 0;
		int max_attempt = 10 * num_edges;
		while(used < num_edges && tries++ < max_attempt){
			int v1 = rand_int(num_nodes);
			int cid = component_id[v1];
			int v2 = component_members[cid][rand_int(component_members[cid].size())];
			if (num_nodes <= 1000){
				if (idx >= options.size()){
					throw runtime_error("insufficient edges");
				}
				v1 = options[idx].first;
				v2 = options[idx].second;
				idx++;
			}
			if (!allow_loop && v1 == v2)
				continue;
			//don't make a new cycle
			if (is_scc_set && v1 < v2) 
				swap(v1, v2);
			//don't use same edge
			if (paths.count(make_pair(v1, v2))) 
				continue;
			//don't connect from different component
			if (component_id[v1] != component_id[v2])
				continue;

			if (!addEdge(v1, v2, paths))
				continue;
			used++;
		}
		if (used < num_edges)
			throw runtime_error("insufficient edges");

		return load_graph(paths);
	}
};

}

